import argparse
import json
import os
import re
import shutil
import sys
import tempfile
import time
from pathlib import Path

from . import __version__, harness, project, report, spec
from .results import Result, project_result
from .util import run, tail


POISONS = [0xA5, 0xFF, 0x01, 0x7F, 0x5A, 0xC3]
RANK = {"PASS": 0, "INFO": 1, "WARN": 2, "FAIL": 3, "LEAK": 4, "MEMORY": 4, "TIMEOUT": 5, "CRASH": 5}


def parse_args(argv):
    p = argparse.ArgumentParser(prog="prtest", description="Local tester for 42 projects (currently: libft). Never modifies your sources.")
    p.add_argument("target", nargs="*", help="[libft] [path]   (default: libft in .)")
    p.add_argument("-f", "--function", action="append", default=[], help="only this function (ft_strlen or strlen); repeatable / comma separated")
    p.add_argument("-c", "--category", action="append", default=[], help="ctype|string|memory|convert|alloc|output|list|project")
    p.add_argument("-v", "--verbose", action="store_true", help="show every test name")
    p.add_argument("--failed-only", action="store_true", help="hide passing functions")
    p.add_argument("--strict", action="store_true", help="convention warnings count as failures")
    p.add_argument("--probe-ub", action="store_true", help="also run undefined-behaviour probes (informational)")
    p.add_argument("--stress", action="store_true", help="run heavy tests (huge lists)")
    p.add_argument("--sanitize", action="store_true", help="recompile your ft_*.c with ASan+UBSan and test those")
    p.add_argument("--valgrind", action="store_true", help="run the harness under valgrind (if installed)")
    p.add_argument("--runs", type=int, default=1, help="repeat the whole suite N times with different seeds and heap-fill patterns (deeper fuzzing)")
    p.add_argument("--opt", choices=["1", "2", "3", "s"], help="also test your sources recompiled with -O<level> (exposes optimizer-dependent / UB bugs)")
    p.add_argument("--no-front", action="store_true", help="skip the second pass that puts guard pages BEFORE buffers")
    p.add_argument("--seed", type=lambda s: int(s, 0), default=0x2F9C41AA)
    p.add_argument("--timeout", type=int, default=10, help="seconds per test (default 10)")
    p.add_argument("--json", action="store_true")
    p.add_argument("--no-color", action="store_true")
    p.add_argument("--no-norm", action="store_true", help="do not run norminette even if installed")
    p.add_argument("--list", action="store_true", help="list tests and exit")
    p.add_argument("--keep-sandbox", action="store_true")
    p.add_argument("--selftest", action="store_true", help="validate prtest itself (golden libft + mutants)")
    p.add_argument("--version", action="version", version="prtest " + __version__)
    return p.parse_args(argv)


def die(msg, code=2):
    sys.stderr.write("prtest: %s\n" % msg)
    return code


def norm_funcs(items):
    out = []
    for it in items:
        for n in it.split(","):
            n = n.strip()
            if not n:
                continue
            n = n if n.startswith("ft_") else "ft_" + n
            if n not in spec.BY_NAME:
                raise ValueError("unknown function '%s' (see: prtest --list)" % n)
            out.append(n)
    return out


def main(argv=None):
    o = parse_args(argv if argv is not None else sys.argv[1:])
    t0 = time.time()
    if o.selftest:
        from selftest import run as selftest_run
        return selftest_run.main(o)
    try:
        funcs = norm_funcs(o.function)
    except ValueError as e:
        return die(str(e))
    if o.opt and o.sanitize:
        return die("--opt and --sanitize cannot be combined")
    cats = [c for c in sum([x.split(",") for x in o.category], []) if c]
    for c in cats:
        if c not in spec.CATEGORIES + ["project"]:
            return die("unknown category '%s' (choose from: %s, project)" % (c, ", ".join(spec.CATEGORIES)))
    if shutil.which("cc") is None:
        return die("no C compiler `cc` found")

    if o.list:
        tmp = Path(tempfile.mkdtemp(prefix="prtest-list-"))
        try:
            binary, text = harness.build(tmp, with_lib=False)
            if not binary:
                return die("cannot build harness:\n" + tail(text, 20))
            res, err = harness.run_harness(binary, tmp, funcs or None, [c for c in cats if c != "project"] or None, o.probe_ub, True, list_only=True)
            for r in res:
                print("%-16s %-8s %s%s" % (r.func, r.cat, r.name, "" if r.tier == 1 else "  [tier %d]" % r.tier))
            print("\n%d tests" % len(res))
        finally:
            shutil.rmtree(tmp, ignore_errors=True)
        return 0

    names = o.target[:]
    if names and names[0] in ("libft",):
        names = names[1:]
    elif names and names[0] not in ("libft",) and not os.path.exists(names[0]):
        return die("unknown project '%s' (supported: libft)" % names[0])
    path = names[0] if names else "."
    if len(names) > 1:
        return die("too many arguments")
    info, note = project.locate(path)
    if not info:
        return die("'%s' does not look like a Libft project (need a Makefile + libft.h / ft_*.c). Run from your repo root: prtest libft ." % path)

    root, proj = project.make_sandbox(info["path"])
    results = []
    try:
        want_project = not cats or "project" in cats
        want_tests = not cats or any(c != "project" for c in cats)
        if not funcs and want_project:
            results.append(project_result("detected as Libft", "PASS", "", ", ".join(info["why"])))
        init = project.initial_files(proj)
        if not funcs and want_project:
            project.hygiene_checks(proj, results)
        ok, log = project.makefile_checks(proj, results, init)
        if not ok:
            for r in results:
                pass
            text, code = report.render(results, o, _meta(root, info, o), time.time() - t0)
            print(text.rsplit("\nSummary", 1)[0])
            return die("build failed: cannot run tests without a working `make` producing libft.a")
        present = project.static_checks(proj, results)
        project.header_checks(proj, results, present)
        if not o.no_norm:
            project.norm_check(proj, results)
        if not (want_project and not funcs):
            results = [r for r in results if r.status in ("FAIL",)]   # filters: only surface project failures
        tests = []
        if want_tests:
            binary, text = harness.build(root, proj, present, sanitize=o.sanitize, strip_main=project.has_main[0], opt=o.opt)
            if not binary:
                text2, _ = report.render(results, o, _meta(root, info, o), time.time() - t0)
                print(text2.rsplit("\nSummary", 1)[0])
                return die("cannot build the test harness (a leftover main()? duplicate symbols?):\n" + tail(text, 20))
            sel = funcs or [f["name"] for f in spec.FUNCS if not cats or f["cat"] in cats]
            runnable = [f for f in sel if f in present]
            for f in sel:
                if f not in present:
                    tests.append(Result(f, spec.BY_NAME[f]["cat"], "implemented", "MISSING", 1, "", "ft_ function in libft.a", "symbol not found"))
            if o.valgrind and not shutil.which("valgrind"):
                results.append(project_result("valgrind", "SKIP", "", "valgrind not installed"))
                o.valgrind = False
            if runnable:
                merged = {}
                for i in range(max(1, o.runs)):
                    seed = (o.seed + i * 0x9E3779B9) & 0xFFFFFFFF
                    poison = POISONS[i % len(POISONS)] if o.runs > 1 else None
                    res, err = harness.run_harness(binary, root, runnable, None, o.probe_ub, o.stress, seed, o.timeout, o.sanitize,
                                                   o.valgrind, poison=poison, front=not o.no_front)
                    if err and not res:
                        return die(err)
                    for r in res:
                        k = (r.func, r.name)
                        old = merged.get(k)
                        if o.runs > 1 and r.status not in ("PASS", "INFO"):
                            r.hint = (r.hint + "  " if r.hint else "") + "[run %d: seed 0x%x, heap poison 0x%02x]" % (i + 1, seed, poison)
                        if old is None:
                            merged[k] = r
                        else:
                            old.checks += r.checks
                            if RANK.get(r.status, 0) > RANK.get(old.status, 0):
                                r.checks = old.checks
                                merged[k] = r
                tests += list(merged.values())
        allres = results + tests
        meta = _meta(root, info, o)
        if o.json:
            print(json.dumps({"meta": {k: str(v) for k, v in meta.items()}, "results": [r.to_dict() for r in allres],
                              "summary": report.counts(allres)}, indent=1))
            return 1 if report.counts(allres)["failed"] else 0
        if note:
            print("note: " + note)
        text, code = report.render(allres, o, meta, time.time() - t0)
        print(text)
        return code
    finally:
        if o.keep_sandbox:
            sys.stderr.write("sandbox kept: %s\n" % root)
        else:
            shutil.rmtree(root, ignore_errors=True)


def _meta(root, info, o):
    rc, out = run(["cc", "--version"])
    return dict(version=__version__, path=info["path"], sandbox=root, cc=out.splitlines()[0] if out else "cc",
                seed=o.seed, sanitize=o.sanitize)
