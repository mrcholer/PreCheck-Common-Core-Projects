"""prtest --selftest : golden libft must pass; every mutant must be caught."""
import json
import os
import shutil
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent


def _prtest(path, *extra):
    p = subprocess.run([sys.executable, str(ROOT / "prtest"), "libft", str(path), "--json", "--no-norm"] + list(extra),
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
    try:
        return json.loads(p.stdout), p.returncode
    except ValueError:
        return {"results": [], "summary": {}, "error": (p.stderr or p.stdout)[-400:]}, p.returncode


def _mutant(m):
    mid, fname, old, new, func = m
    d = Path(tempfile.mkdtemp(prefix="prtest-mut-"))
    try:
        shutil.copytree(HERE / "golden", d / "libft", ignore=shutil.ignore_patterns("*.o", "*.a"))
        f = d / "libft" / fname
        txt = f.read_text()
        if old not in txt:
            return mid, func, "BROKEN-MUTANT", "pattern not found in " + fname
        f.write_text(txt.replace(old, new, 1))
        data, rc = _prtest(d / "libft", "-f", func)
        if "error" in data:
            return mid, func, "ERROR", data["error"]
        bad = [r for r in data["results"] if r["func"] == func and r["status"] in ("FAIL", "CRASH", "TIMEOUT", "LEAK", "MEMORY")]
        if bad:
            return mid, func, "KILLED", "%s: %s" % (bad[0]["status"], bad[0]["name"])
        proj_bad = [r for r in data["results"] if r["cat"] == "project" and r["status"] == "FAIL"]
        if proj_bad:
            return mid, func, "BUILD/STATIC", proj_bad[0]["name"]
        return mid, func, "SURVIVED", "prtest did not notice!"
    finally:
        shutil.rmtree(d, ignore_errors=True)


def main(opts=None):
    from .mutants import M
    ok = True
    print("prtest self-test: golden libft + %d mutants\n" % len(M))
    g, rc = _prtest(HERE / "golden")
    s = g.get("summary", {})
    good = rc == 0 and s.get("failed") == 0 and s.get("warnings") == 0
    print("golden libft              %s  (passed %s, failed %s, warnings %s)" % ("OK " if good else "BAD", s.get("passed"), s.get("failed"), s.get("warnings")))
    ok &= good
    if shutil.which("cc"):
        gs, rc = _prtest(HERE / "golden", "--sanitize")
        s2 = gs.get("summary", {})
        good2 = s2.get("failed") == 0 and "error" not in gs
        print("golden libft + ASan/UBSan %s  (passed %s, failed %s)%s" % ("OK " if good2 else "BAD", s2.get("passed"), s2.get("failed"), "" if good2 else " " + gs.get("error", "")))
        ok &= good2
    print()
    with ThreadPoolExecutor(max_workers=4) as ex:
        rows = list(ex.map(_mutant, M))
    killed = 0
    for mid, func, verdict, detail in rows:
        mark = "OK " if verdict in ("KILLED", "BUILD/STATIC") else "BAD"
        killed += mark == "OK "
        print("%s %-34s %-14s %-13s %s" % (mark, mid, func, verdict, detail))
    print("\nmutants caught: %d/%d" % (killed, len(M)))
    ok &= killed == len(M)
    print("SELF-TEST " + ("PASSED" if ok else "FAILED"))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.path.insert(0, str(ROOT))
    sys.exit(main())
