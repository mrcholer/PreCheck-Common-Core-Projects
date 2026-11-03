"""Builds and runs the C test harness against the student's libft.a (or sources, in --sanitize mode)."""
import glob
import os
import re
import shutil
import subprocess
from pathlib import Path

from . import spec
from .results import Result
from .util import run, tail

HARNESS_DIR = Path(__file__).resolve().parent.parent / "harness"


def build(workdir, proj=None, present=(), sanitize=False, with_lib=True, strip_main=False, opt=None):
    """Compile the harness. Returns (binary_path or None, compiler_output)."""
    workdir = Path(workdir)
    gen = workdir / "gen"
    gen.mkdir(parents=True, exist_ok=True)
    (gen / "proto.h").write_text(spec.gen_proto_h())
    srcs = sorted(glob.glob(str(HARNESS_DIR / "*.c"))) + sorted(glob.glob(str(HARNESS_DIR / "libft" / "*.c")))
    out = workdir / "prtest_harness"
    cmd = ["cc", "-O0", "-g", "-D_GNU_SOURCE", "-Wall", "-Wno-unused-function", "-I", str(HARNESS_DIR), "-I", str(gen)]
    if sanitize:
        cmd += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer"]
    cmd += srcs
    cmd += ["-o", str(out), "-Wl,--wrap=malloc", "-Wl,--wrap=free", "-Wl,--wrap=calloc"]
    if with_lib and proj is not None:
        if sanitize:
            ft = sorted(glob.glob(str(Path(proj) / "ft_*.c")))
            cmd = cmd[:1] + ["-I", str(proj)] + cmd[1:-3] + ft + cmd[-3:]
        else:
            for n in sorted(present):
                cmd += ["-Wl,-u," + n]
            lib = Path(proj) / "libft.a"
            if opt:
                od = workdir / "opt"
                od.mkdir(exist_ok=True)
                objs = []
                for f in sorted(Path(proj).glob("ft_*.c")):
                    o = od / (f.stem + ".o")
                    rc, t = run(["cc", "-O%s" % opt, "-fno-builtin", "-fno-tree-loop-distribute-patterns", "-w", "-I", str(proj), "-c", str(f), "-o", str(o)])
                    if rc != 0:
                        return None, "cannot compile %s at -O%s:\n%s" % (f.name, opt, t)
                    objs.append(str(o))
                lib = workdir / "libft_opt.a"
                if lib.exists():
                    lib.unlink()
                run(["ar", "rcs", str(lib)] + objs)
            if strip_main:
                alt = workdir / "libft_nomain.a"
                shutil.copy(lib, alt)
                run(["objcopy", "--redefine-sym", "main=_prtest_leftover_main", str(alt)])
                lib = alt
            cmd += [str(lib)]
    rc, text = run(cmd, cwd=workdir, timeout=240)
    if rc != 0:
        return None, text
    return out, text


def _env(workdir, sanitize, valgrind):
    log = str(Path(workdir) / "sanlog")
    env = {"PRTEST_LOG": log}
    if sanitize:
        env["ASAN_OPTIONS"] = "log_path=%s:exitcode=77:detect_leaks=0:abort_on_error=0" % log
        env["UBSAN_OPTIONS"] = "log_path=%s:exitcode=77:print_stacktrace=0" % log
    return env


def _condense(act):
    """Shrink ASan/UBSan/valgrind dumps to: the error + the top stack frames."""
    if not act.startswith("exit code 77"):
        return act
    t = " ".join(act.split())
    m = re.search(r"runtime error: .*?(?= #\d|$)", t) or re.search(r"ERROR: \w+Sanitizer: \S+", t) \
        or re.search(r"==\d+== [A-Z][^=]*", t)
    frames = re.findall(r"#\d+ 0x[0-9a-f]+ in (\S+) (\S+)", t)
    frames = [(f, os.path.basename(loc.split(":")[0]) + ":" + ":".join(loc.split(":")[1:2])) for f, loc in frames
              if not f.startswith(("pt_", "main"))][:3]
    out = (m.group(0) if m else "sanitizer/valgrind reported an error").strip()
    if frames:
        out += "  at " + " <- ".join("%s (%s)" % fl for fl in frames)
    return out


def run_harness(binary, workdir, functions=None, categories=None, probe=False, stress=False, seed=None,
                timeout=3, sanitize=False, valgrind=False, list_only=False, poison=None, front=True):
    args = [str(binary)]
    if list_only:
        args.append("--list")
    if functions:
        args += ["--function", ",".join(functions)]
    if categories:
        args += ["--category", ",".join(categories)]
    if probe:
        args.append("--probe-ub")
    if stress:
        args.append("--stress")
    if seed is not None:
        args += ["--seed", str(seed)]
    args += ["--timeout", str(timeout)]
    if sanitize or valgrind:
        args.append("--passthrough")
    if poison is not None:
        args += ["--poison", "0x%02x" % poison]
    if not front:
        args.append("--no-front")
    cmd = args
    if valgrind:
        vg = Path(workdir) / "vglog"
        cmd = ["valgrind", "-q", "--error-exitcode=77", "--leak-check=full", "--errors-for-leak-kinds=definite",
               "--trace-children=no", "--log-file=%s.%%p" % vg] + args
    env = dict(os.environ, **_env(workdir, sanitize, valgrind))
    if valgrind:
        env["PRTEST_LOG"] = str(Path(workdir) / "vglog")
    try:
        p = subprocess.run(cmd, cwd=workdir, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                           universal_newlines=True, errors="replace", timeout=1800)
    except subprocess.TimeoutExpired:
        return [], "harness timed out"
    res = []
    for line in p.stdout.splitlines():
        parts = line.split("\t")
        if parts[0] == "T" and len(parts) >= 10:
            _, func, cat, name, tier, status, call, exp, act, hint = parts[:10]
            checks = int(parts[10]) if len(parts) > 10 and parts[10].isdigit() else 0
            act = _condense(act)
            if status == "INFO":
                exp, hint = "any outcome (undefined behaviour in libc too)", ""
            res.append(Result(func, cat, name, status, int(tier), call, exp, act, hint, checks))
        elif parts[0] == "L" and len(parts) >= 5:
            res.append(Result(parts[1], parts[2], parts[3], "LIST", int(parts[4])))
    err = p.stderr.strip()
    if p.returncode != 0 and not res:
        err = "harness exited with %d\n%s" % (p.returncode, err)
    return res, err
