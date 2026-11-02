"""Libft project detection, sandboxing, Makefile checks and static (nm / header) analysis."""
import os
import re
import shutil
import tempfile
from pathlib import Path

from . import spec
from .results import project_result as PR
from .util import run, tail


def detect(path):
    p = Path(path)
    if not p.is_dir():
        return None
    mk, hd = p / "Makefile", p / "libft.h"
    srcs = sorted(p.glob("ft_*.c"))
    why = []
    if mk.is_file() and re.search(r"libft\.a", mk.read_text(errors="replace")):
        why.append("Makefile builds libft.a")
    if hd.is_file():
        why.append("libft.h")
    if srcs:
        why.append("%d ft_*.c" % len(srcs))
    if mk.is_file() and len(why) >= 2:
        return {"path": p.resolve(), "why": why, "nsrc": len(srcs)}
    return None


def locate(path):
    """Accept the repo root, or a directory containing a libft/ folder."""
    d = detect(path)
    if d:
        return d, None
    sub = Path(path) / "libft"
    d = detect(sub)
    if d:
        return d, "using %s (found a libft/ folder)" % sub
    return None, None


def make_sandbox(src, keep=False):
    root = Path(tempfile.mkdtemp(prefix="prtest-"))
    dst = root / "proj"
    shutil.copytree(src, dst, ignore=shutil.ignore_patterns(".git", "*.o", "*.a", "*.d", "__pycache__", ".DS_Store"))
    return root, dst


# ---------------------------------------------------------------- Makefile
def makefile_checks(proj, results):
    """Runs the student's Makefile inside the sandbox. Returns (ok_to_continue, log)."""
    mk_text = (proj / "Makefile").read_text(errors="replace")
    lib = proj / "libft.a"
    log = ""

    rc, out = run(["make", "fclean"], cwd=proj)
    if rc != 0:
        results.append(PR("make fclean works", "FAIL", "exit status 0", "exit status %d" % rc, tail(out, 6)))
    rc, dry = run(["make", "-n"], cwd=proj)
    comp = [l for l in dry.splitlines() if re.search(r"\s-c\s", l) or re.search(r"\.c\b", l) and re.match(r"\s*(cc|gcc|clang)\b", l)]
    if comp:
        bad_cc = [l for l in comp if not re.match(r"\s*cc\s", l)]
        miss = [f for f in ("-Wall", "-Wextra", "-Werror") if any(f not in l.split() for l in comp) and f]
        if bad_cc:
            results.append(PR("compiler is cc", "WARN", "cc", bad_cc[0].split()[0], "the subject asks for cc"))
        else:
            results.append(PR("compiler is cc", "PASS"))
        if miss:
            results.append(PR("compiles with -Wall -Wextra -Werror", "FAIL", "-Wall -Wextra -Werror", "missing: " + " ".join(miss), comp[0][:160]))
        else:
            results.append(PR("compiles with -Wall -Wextra -Werror", "PASS"))
    else:
        results.append(PR("compiles with -Wall -Wextra -Werror", "SKIP", "", "could not see compile commands (make -n printed none)"))
    if re.search(r"\blibtool\b", mk_text) or re.search(r"\blibtool\b", dry):
        results.append(PR("library built with ar (not libtool)", "FAIL", "ar", "libtool", "libtool is strictly forbidden"))
    elif re.search(r"(^|\s)ar\s", dry) or re.search(r"(^|\s)ar\s", mk_text):
        results.append(PR("library built with ar (not libtool)", "PASS"))
    else:
        results.append(PR("library built with ar (not libtool)", "WARN", "an `ar` command", "none found"))

    rc, out = run(["make"], cwd=proj, timeout=240)
    log = out
    if rc != 0 or not lib.is_file():
        results.append(PR("make builds libft.a at the repo root", "FAIL", "libft.a created, exit 0",
                          "exit %d%s" % (rc, "" if lib.is_file() else ", libft.a missing"), tail(out, 15)))
        return False, out
    results.append(PR("make builds libft.a at the repo root", "PASS"))
    if re.search(r"warning:", out):
        results.append(PR("no compiler warnings", "WARN", "no warnings", "warnings printed", tail(out, 6)))

    m1 = lib.stat().st_mtime_ns
    objs = {p: p.stat().st_mtime_ns for p in proj.glob("*.o")}
    rc, out2 = run(["make"], cwd=proj)
    objs2 = {p: p.stat().st_mtime_ns for p in proj.glob("*.o")}
    relinked = lib.stat().st_mtime_ns != m1 or objs != objs2 or re.search(r"(^|\n)\s*(cc|gcc|clang|ar)\s", out2)
    results.append(PR("second `make` does not relink", "FAIL" if relinked else "PASS",
                      "nothing to do", "rebuilt something", tail(out2, 5) if relinked else ""))

    rc, _ = run(["make", "clean"], cwd=proj)
    left = list(proj.glob("*.o"))
    ok = rc == 0 and not left and lib.is_file()
    results.append(PR("`make clean` removes objects, keeps libft.a", "PASS" if ok else "FAIL",
                      "no .o files, libft.a kept", "%d .o left, libft.a %s" % (len(left), "kept" if lib.is_file() else "removed")))
    rc, _ = run(["make", "fclean"], cwd=proj)
    results.append(PR("`make fclean` removes libft.a", "PASS" if rc == 0 and not lib.exists() else "FAIL",
                      "libft.a removed", "libft.a still present" if lib.exists() else "exit %d" % rc))
    rc, out = run(["make", "re"], cwd=proj, timeout=240)
    results.append(PR("`make re` rebuilds", "PASS" if rc == 0 and lib.is_file() else "FAIL", "libft.a rebuilt", tail(out, 5)))
    rc, out = run(["make", "all"], cwd=proj)
    results.append(PR("`make all` works", "PASS" if rc == 0 and lib.is_file() else "FAIL", "exit 0", tail(out, 5)))
    return lib.is_file(), log


# ---------------------------------------------------------------- nm analysis
_NM = re.compile(r"^(?P<ar>[^:]+):(?P<mem>[^:]+):\s*(?P<addr>[0-9a-fA-F]*)\s+(?P<t>[A-Za-z?])\s+(?P<sym>\S+)\s*$")


def nm_analysis(proj):
    rc, out = run(["nm", "-A", "libft.a"], cwd=proj)
    defined, undef, data = {}, {}, {}
    for line in out.splitlines():
        m = _NM.match(line)
        if not m:
            continue
        mem, t, sym = m["mem"], m["t"], m["sym"]
        if t in "TtW" and t != "t":
            defined.setdefault(mem, set()).add(sym)
        elif t == "U":
            undef.setdefault(mem, set()).add(sym)
        elif t in "DBRCGSV":
            data.setdefault(mem, set()).add(sym)
    return defined, undef, data, rc


has_main = [False]


def static_checks(proj, results):
    """Returns the set of implemented spec function names."""
    has_main[0] = False
    defined, undef, data, rc = nm_analysis(proj)
    allsyms = set().union(*defined.values()) if defined else set()
    present = {f["name"] for f in spec.FUNCS if f["name"] in allsyms}
    results.append(PR("libft.a defines the required functions",
                      "PASS" if len(present) == len(spec.FUNCS) else "WARN",
                      "%d functions" % len(spec.FUNCS), "%d found" % len(present),
                      "missing: " + ", ".join(sorted(set(f["name"] for f in spec.FUNCS) - present)) if len(present) != len(spec.FUNCS) else ""))
    if "main" in allsyms:
        results.append(PR("no main() inside libft.a", "FAIL", "no main", "main found", "remove leftover test files from the library (prtest renames it so the other tests can still run)"))
        has_main[0] = True
    # extra non-static symbols
    helpers = sorted(s for s in allsyms if s not in spec.BY_NAME and s != "main")
    results.append(PR("helper functions are static", "WARN" if helpers else "PASS", "only the subject's functions exported",
                      "exported: " + ", ".join(helpers[:8]), "make helper functions `static` (subject, IV.1)") if helpers else PR("helper functions are static", "PASS"))
    gl = sorted(s for syms in data.values() for s in syms)
    results.append(PR("no global variables", "FAIL" if gl else "PASS", "none", "global data: " + ", ".join(gl[:8]), "global variables are forbidden") if gl else PR("no global variables", "PASS"))
    # forbidden externals
    viol = []
    for mem, syms in undef.items():
        owned = [spec.BY_NAME[s] for s in defined.get(mem, ()) if s in spec.BY_NAME]
        allowed = set().union(*[f["ext"] for f in owned]) if owned else set()
        for s in syms:
            if s.startswith("ft_") or s in spec.ALLOWED_RUNTIME or s in allowed or s.startswith(("__ubsan", "__asan", "_ITM_")):
                continue
            viol.append((mem, s))
    if viol:
        lines = ["%s uses %s" % (m, s) for m, s in sorted(viol)[:6]]
        hint = "; ".join(sorted({spec.LIBC_HINTS.get(s, "not an allowed external function") for _, s in viol})[:2])
        results.append(PR("only allowed external functions are used", "FAIL", "malloc/free/write only where listed", "\n".join(lines), hint))
    else:
        results.append(PR("only allowed external functions are used", "PASS"))
    return present


def header_checks(proj, results, present):
    hdr = proj / "libft.h"
    if not hdr.is_file():
        results.append(PR("libft.h exists at the repo root", "FAIL", "libft.h", "missing"))
        return
    text = hdr.read_text(errors="replace")
    results.append(PR("libft.h exists at the repo root", "PASS"))
    if re.search(r"\brestrict\b|__restrict", text):
        results.append(PR("no `restrict` in prototypes", "FAIL", "no restrict", "found restrict", "C99-only keyword is forbidden by the subject"))
    else:
        results.append(PR("no `restrict` in prototypes", "PASS"))
    guard = re.search(r"#\s*ifndef\s+\w+", text) or "#pragma once" in text
    results.append(PR("include guard present", "PASS" if guard else "FAIL", "#ifndef ... / #pragma once", "none"))
    undeclared = [n for n in sorted(present) if not re.search(r"\b%s\s*\(" % n, text)]
    results.append(PR("every implemented function is declared in libft.h", "FAIL" if undeclared else "PASS", "all declared",
                      "not declared: " + ", ".join(undeclared[:8]), "") if undeclared else PR("every implemented function is declared in libft.h", "PASS"))
    tmp = Path(tempfile.mkdtemp(prefix="prtest-hdr-"))
    flags = ["-Wall", "-Wextra", "-Werror", "-fsyntax-only", "-I", str(proj)]
    try:
        (tmp / "a.c").write_text('#include "libft.h"\n#include "libft.h"\nint main(void){ t_list l; l.content = 0; l.next = (t_list *)0; (void)l; return 0; }\n')
        rc, out = run(["cc"] + flags + [str(tmp / "a.c")])
        results.append(PR("libft.h compiles alone, twice, and defines t_list{content,next}", "PASS" if rc == 0 else "FAIL",
                          "compiles with -Wall -Wextra -Werror", tail(out, 5)))
        (tmp / "b.c").write_text(spec.gen_conformance_c())
        rc, out = run(["cc"] + flags + [str(tmp / "b.c")])
        hint = ""
        if rc != 0:
            hint = "your header's prototypes must match the subject exactly"
            if "ft_lstsize" in out:
                hint += " (v%s: ft_lstsize returns unsigned int)" % spec.SPEC_VERSION
        results.append(PR("prototypes in libft.h match the subject (v%s)" % spec.SPEC_VERSION, "PASS" if rc == 0 else "FAIL",
                          "no conflicting declarations", tail(out, 6), hint))
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def norm_check(proj, results):
    exe = shutil.which("norminette")
    if not exe:
        results.append(PR("norminette", "SKIP", "", "norminette not installed"))
        return
    rc, out = run([exe], cwd=proj, timeout=120)
    errs = [l for l in out.splitlines() if "Error" in l and "Error!" not in l]
    results.append(PR("Norm (norminette)", "PASS" if rc == 0 else "FAIL", "OK", "%d error(s)" % len(errs), "\n".join(errs[:5])))
