import os
import sys

from . import spec
from .results import FAILING

ICON = {"PASS": "✓", "FAIL": "✗", "CRASH": "✗", "TIMEOUT": "✗", "LEAK": "✗", "MEMORY": "✗", "WARN": "!", "INFO": "i", "SKIP": "-", "MISSING": "-"}
ASCII = {"✓": "ok", "✗": "XX", "!": "!!", "i": "ii", "-": "--", "›": ">"}
TIERNAME = {1: "spec", 2: "convention", 3: "undefined"}


class Style:
    def __init__(self, color):
        self.color = color

    def c(self, code, s):
        return "\033[%sm%s\033[0m" % (code, s) if self.color else s

    def status(self, st, s):
        code = {"PASS": "32", "WARN": "33", "INFO": "36", "SKIP": "90", "MISSING": "90"}.get(st, "31")
        return self.c(code, s)


def use_color(opts):
    if opts.no_color or os.environ.get("NO_COLOR") or not sys.stdout.isatty():
        return False
    return True


def counts(results):
    c = dict(passed=0, failed=0, warnings=0, info=0, skipped=0, missing=0)
    for r in results:
        if r.status == "PASS":
            c["passed"] += 1
        elif r.status in FAILING:
            c["failed"] += 1
        elif r.status == "WARN":
            c["warnings"] += 1
        elif r.status == "INFO":
            c["info"] += 1
        elif r.status == "SKIP":
            c["skipped"] += 1
        elif r.status == "MISSING":
            c["missing"] += 1
    return c


def _detail(r, st, indent="     "):
    out = []
    for label, val in (("call", r.call), ("expected", r.expected), ("actual", r.actual), ("hint", r.hint)):
        if val:
            lines = val.split("\n")
            out.append("%s%-9s %s" % (indent, label, lines[0]))
            out += ["%s%-9s %s" % (indent, "", l) for l in lines[1:]]
    return out


def render(results, opts, meta, elapsed):
    st = Style(use_color(opts))
    uni = sys.stdout.encoding and "utf" in sys.stdout.encoding.lower()
    out = []

    def ic(s):
        return s if uni else ASCII.get(s, s)

    out.append(st.c("1", "prtest %s · libft (subject %s) · %s" % (meta["version"], spec.SPEC_VERSION, meta["path"])))
    out.append(st.c("90", "sandbox %s · %s · seed 0x%x%s" % (meta["sandbox"], meta["cc"], meta["seed"], " · sanitizers" if meta.get("sanitize") else "")))
    out.append("")
    proj = [r for r in results if r.cat == "project"]
    if proj:
        out.append(st.c("1", "Project"))
        for r in proj:
            if opts.failed_only and not (r.failing or r.status == "WARN"):
                continue
            out.append("  %s %s" % (st.status(r.status, ic(ICON[r.status])), r.name))
            if r.status != "PASS":
                out += _detail(r, st, "       ")
        out.append("")
    by = {}
    for r in results:
        if r.cat != "project":
            by.setdefault(r.func, []).append(r)
    shown_any = False
    for cat in spec.CATEGORIES:
        funcs = [f["name"] for f in spec.FUNCS if f["cat"] == cat and f["name"] in by]
        lines = []
        for fn in funcs:
            rs = by[fn]
            tests = [r for r in rs if r.status != "MISSING"]
            if rs and rs[0].status == "MISSING":
                sym, line = "MISSING", "  %s %-18s not found in libft.a" % (st.status("MISSING", ic("-")), fn)
            else:
                ok = sum(1 for r in tests if r.status == "PASS")
                bad = any(r.failing for r in tests)
                warn = any(r.status == "WARN" for r in tests)
                sym = "FAIL" if bad else "WARN" if warn else "PASS"
                total = sum(1 for r in tests if r.status != "INFO")
                line = "  %s %-18s %d/%d%s" % (st.status(sym, ic(ICON[sym])), fn, ok, total,
                                               ("   %d warning(s)" % sum(1 for r in tests if r.status == "WARN")) if warn else "")
            if opts.failed_only and sym in ("PASS", "MISSING"):
                continue
            lines.append(line)
            if opts.verbose:
                for r in tests:
                    lines.append("       %s %s" % (st.status(r.status, ic(ICON[r.status])), r.name))
        if lines:
            out.append(st.c("1", " " + cat))
            out += lines
            shown_any = True
    if shown_any:
        out.append("")
    fails = [r for r in results if r.failing]
    warns = [r for r in results if r.status == "WARN" and r.cat != "project"]
    infos = [r for r in results if r.status == "INFO"]
    if fails:
        out.append(st.c("1;31", "Failures"))
        for r in fails:
            out.append(" %s %s %s %s%s" % (st.status("FAIL", ic("✗")), r.func if r.cat != "project" else "project", ic("›"), r.name,
                                         st.c("90", "   [%s%s]" % (TIERNAME.get(r.tier, ""), "" if r.status == "FAIL" else ", " + r.status.lower()))))
            out += _detail(r, st)
            out.append("")
    if warns:
        out.append(st.c("1;33", "Warnings"))
        for r in warns:
            out.append(" %s %s %s %s%s" % (st.status("WARN", ic("!")), r.func, ic("›"), r.name, st.c("90", "   [%s]" % TIERNAME.get(r.tier, "convention"))))
            out += _detail(r, st)
            out.append("")
    if infos:
        out.append(st.c("1;36", "Undefined-behaviour probes (informational, never failures)"))
        for r in infos:
            out.append(" %s %s %s %s" % (st.status("INFO", ic("i")), r.func, ic("›"), r.name))
            out += _detail(r, st)
        out.append("")
    c = counts(results)
    nfun = len({r.func for r in results if r.cat != "project"})
    out.append("Summary  passed %d · failed %d · warnings %d · skipped %d · missing %d%s   (%d functions, %.1f s)" % (
        c["passed"], c["failed"], c["warnings"], c["skipped"], c["missing"],
        (" · probes %d" % c["info"]) if c["info"] else "", nfun, elapsed))
    bad = c["failed"] > 0 or (opts.strict and c["warnings"] > 0) or (c["missing"] > 0 and False)
    out.append("Result   " + (st.c("1;31", "FAIL") if bad else st.c("1;32", "PASS")))
    return "\n".join(out), (1 if bad else 0)
