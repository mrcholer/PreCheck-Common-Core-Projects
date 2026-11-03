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
def makefile_checks(proj, results, initial=None):
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
    if initial is not None:
        left = sorted(initial_files(proj) - initial)
        results.append(PR("`make fclean` leaves no build files behind", "FAIL" if left else "PASS", "the tree as you submitted it",
                          "left behind: " + ", ".join(left[:8])) if left else PR("`make fclean` leaves no build files behind", "PASS"))
    rc, out = run(["make", "re"], cwd=proj, timeout=240)
    results.append(PR("`make re` rebuilds", "PASS" if rc == 0 and lib.is_file() else "FAIL", "libft.a rebuilt", tail(out, 5)))
    rc, out = run(["make", "all"], cwd=proj)
    results.append(PR("`make all` works", "PASS" if rc == 0 and lib.is_file() else "FAIL", "exit 0", tail(out, 5)))
    # --- deeper behaviour
    srcs = sorted(proj.glob("ft_*.c"))
    if srcs:
        tgt = srcs[0]
        obj = proj / (tgt.stem + ".o")
        if obj.is_file():
            import time
            before = obj.stat().st_mtime_ns
            time.sleep(0.05)
            os.utime(tgt, None)
            run(["make"], cwd=proj)
            ok = obj.stat().st_mtime_ns != before if obj.is_file() else True
            results.append(PR("editing a .c file triggers a rebuild", "PASS" if ok else "FAIL", "%s.o rebuilt" % tgt.stem, "stale object kept", "the .o rule must depend on its .c file"))
            snap = {o: o.stat().st_mtime_ns for o in proj.glob("*.o")}
            time.sleep(0.05)
            os.utime(proj / "libft.h", None)
            run(["make"], cwd=proj)
            changed = any(o.is_file() and o.stat().st_mtime_ns != t for o, t in snap.items())
            results.append(PR("editing libft.h triggers a rebuild", "PASS" if changed else "WARN", "objects rebuilt", "nothing rebuilt",
                              "add libft.h as a dependency of the .o files (otherwise header changes are silently ignored)"))
    rc, out = run(["make", "fclean"], cwd=proj)
    rc, out = run(["make", "-j8"], cwd=proj, timeout=240)
    results.append(PR("`make -j8` (parallel) works", "PASS" if rc == 0 and lib.is_file() else "FAIL", "libft.a built in parallel", tail(out, 6),
                      "a race between rules (e.g. fclean/all in one command, or the archive rule listing no prerequisites)"))
    run(["make", "fclean"], cwd=proj)
    rc, out = run(["make", "-C", str(proj), "all"], cwd=tempfile.gettempdir(), timeout=240)
    results.append(PR("`make -C <dir>` works from another directory", "PASS" if rc == 0 and lib.is_file() else "FAIL", "libft.a built", tail(out, 6)))
    rc, out = run(["make", "re"], cwd=proj, timeout=240)
    return lib.is_file(), log


# ---------------------------------------------------------------- nm analysis
_NM = re.compile(r"^(?P<ar>[^:]+):(?P<mem>[^:]+):\s*(?P<addr>[0-9a-fA-F]*)\s+(?P<t>[A-Za-z?])\s+(?P<sym>\S+)\s*$")


def nm_analysis(proj):
    rc, out = run(["nm", "-A", "libft.a"], cwd=proj)
    defined, undef, data = {}, {}, {}
    local = {}
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
        elif t in "bd":
            local.setdefault(mem, set()).add(sym)
    nm_analysis.local = local
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
    loc = sorted("%s: %s" % (m, s) for m, syms in getattr(nm_analysis, "local", {}).items() for s in syms)
    if loc:
        results.append(PR("no static (hidden-state) variables", "WARN", "no mutable static data", "; ".join(loc[:5]),
                          "a `static` variable inside a function keeps state between calls (breaks re-entrancy / repeated calls)"))
    else:
        results.append(PR("no static (hidden-state) variables", "PASS"))
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
        norm_lite(proj, results)
        return
    rc, out = run([exe], cwd=proj, timeout=120)
    errs = [l for l in out.splitlines() if "Error" in l and "Error!" not in l]
    results.append(PR("Norm (norminette)", "PASS" if rc == 0 else "FAIL", "OK", "%d error(s)" % len(errs), "\n".join(errs[:5])))


# ---------------------------------------------------------------- repo hygiene / source scan
ALLOWED_ROOT = {"Makefile", "libft.h", "README.md", ".gitignore", ".gitattributes", "LICENSE", ".git"}
LIBC_CALLS = set("""strlen strcpy strncpy strcat strncat strcmp strncmp strchr strrchr strstr strnstr strdup strlcpy strlcat memcpy memmove memset
memcmp memchr bzero calloc realloc atoi atol strtol strtoul printf fprintf sprintf snprintf dprintf puts putchar putc fputs exit abort
toupper tolower isalpha isdigit isalnum isascii isprint isspace getenv system fopen fclose read open close perror strerror abs labs
qsort rand srand time sleep usleep""".split())


def _strip(text):
    text = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), text, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    text = re.sub(r'"(\\.|[^"\\\n])*"', '""', text)
    text = re.sub(r"'(\\.|[^'\\\n])+'", "'x'", text)
    return text


def initial_files(proj):
    return {str(p.relative_to(proj)) for p in proj.rglob("*") if p.is_file()}


def hygiene_checks(proj, results):
    names = sorted(p.name for p in proj.iterdir())
    extra = [n for n in names if n not in ALLOWED_ROOT and not re.fullmatch(r"ft_\w+\.c", n)]
    results.append(PR("no unused files at the repo root", "WARN" if extra else "PASS", "only Makefile, libft.h, README.md, ft_*.c",
                      "also present: " + ", ".join(extra[:8]), "the subject forbids submitting unused files (tests, main.c, binaries, folders)") if extra else PR("no unused files at the repo root", "PASS"))
    nested = [str(p.relative_to(proj)) for p in proj.rglob("ft_*.c") if p.parent != proj]
    results.append(PR("every ft_*.c is at the repo root", "FAIL", "all files at the root", ", ".join(nested[:5]), "move them to the root of the repository") if nested else PR("every ft_*.c is at the repo root", "PASS"))
    mk = (proj / "Makefile").read_text(errors="replace")
    live = "\n".join(l for l in mk.splitlines() if not l.lstrip().startswith("#"))
    if re.search(r"\$\(wildcard|\*\.c\b", live):
        results.append(PR("Makefile lists source files explicitly", "WARN", "explicit file list", "wildcard used", "wildcards are not allowed by the 42 rules"))
    else:
        results.append(PR("Makefile lists source files explicitly", "PASS"))
    srcs = sorted(proj.glob("ft_*.c"))
    unlisted = [s.name for s in srcs if s.name not in mk and s.stem not in mk]
    if unlisted:
        results.append(PR("every ft_*.c is listed in the Makefile", "WARN", "all sources listed", "not listed: " + ", ".join(unlisted[:6])))
    rd = proj / "README.md"
    if not rd.is_file():
        results.append(PR("README.md present with the required content", "FAIL", "README.md at the repo root", "missing", "see Chapter V of the subject"))
    else:
        t = rd.read_text(errors="replace")
        first = t.strip().splitlines()[0] if t.strip() else ""
        miss = []
        if not re.fullmatch(r"[*_]This project has been created as part of the 42 curriculum by .+[*_]\.?", first.strip()):
            miss.append("first line must be the italic sentence \"This project has been created as part of the 42 curriculum by <login>\"")
        for sec in ("Description", "Instructions", "Resources"):
            if not re.search(r"^#+\s*%s\b" % sec, t, flags=re.I | re.M):
                miss.append('missing "%s" section' % sec)
        if "AI" not in t and "ai " not in t.lower():
            miss.append("Resources section should say how AI was used (or that it was not)")
        results.append(PR("README.md present with the required content", "WARN" if miss else "PASS", "Chapter V of the subject", "; ".join(miss)) if miss else PR("README.md present with the required content", "PASS"))
    hits = []
    for s in srcs:
        code = _strip(s.read_text(errors="replace"))
        for i, line in enumerate(code.splitlines(), 1):
            for m in re.finditer(r"\b([A-Za-z_]\w*)\s*\(", line):
                if m.group(1) in LIBC_CALLS:
                    hits.append("%s:%d %s()" % (s.name, i, m.group(1)))
    results.append(PR("source does not call libc functions", "WARN" if hits else "PASS", "only malloc/free/write where allowed",
                      "; ".join(hits[:6]), "inside the library use your own ft_* versions") if hits else PR("source does not call libc functions", "PASS"))


def norm_lite(proj, results):
    """Approximate Norm checks used only when norminette is not installed."""
    bad = []
    for f in sorted(list(proj.glob("ft_*.c")) + [proj / "libft.h"]):
        if not f.is_file():
            continue
        raw = f.read_text(errors="replace").splitlines()
        code = _strip(f.read_text(errors="replace")).splitlines()
        nfunc = 0
        for i, line in enumerate(raw, 1):
            if len(line.expandtabs(4)) > 80:
                bad.append("%s:%d line longer than 80 columns" % (f.name, i))
        for i, line in enumerate(code, 1):
            for kw in ("for", "switch", "case", "goto", "do"):
                if re.search(r"\b%s\b" % kw, line):
                    bad.append("%s:%d forbidden keyword '%s'" % (f.name, i, kw))
            if "?" in line:
                bad.append("%s:%d ternary operator" % (f.name, i))
        i = 0
        while i < len(code):
            if code[i] == "{" and f.suffix == ".c":
                nfunc += 1
                j = i + 1
                while j < len(code) and code[j] != "}":
                    j += 1
                if j - i - 1 > 25:
                    bad.append("%s:%d function longer than 25 lines (%d)" % (f.name, i + 1, j - i - 1))
                hdr = code[i - 1] if i else ""
                if hdr.count(",") > 3:
                    bad.append("%s:%d more than 4 parameters" % (f.name, i))
                i = j
            i += 1
        if nfunc > 5:
            bad.append("%s: more than 5 functions in one file (%d)" % (f.name, nfunc))
    results.append(PR("Norm-lite (approximate; norminette not installed)", "WARN" if bad else "PASS", "no violations found",
                      "\n".join(bad[:6]) + ("\n... %d more" % (len(bad) - 6) if len(bad) > 6 else ""),
                      "install norminette for the real check") if bad else PR("Norm-lite (approximate; norminette not installed)", "PASS"))
