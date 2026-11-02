# prtest design notes (condensed)

Python orchestrator + C harness. Python: detection, sandbox, Makefile/nm/header checks, reporting, CLI.
C: one binary, `fork()` per test, results as tab-separated lines on stdout (children's own stdout/stderr are captured).

## Spec as data
`prtest_core/spec.py` holds the 43 functions of subject v19.3 (prototype, category, allowed externals) and
generates both the harness's private `proto.h` (weak declarations + presence table) and the header-conformance check.
Adding another subject version = another table.

## Verdict model
`T_SPEC` → FAIL, `T_CONV` → WARN, `T_UB` → INFO (only with `--probe-ub`). Statuses: PASS FAIL WARN CRASH TIMEOUT LEAK MEMORY INFO SKIP MISSING.

## Allocator modes
guard-page (default, first 8000 blocks per test) → canary blocks (fallback, avoids exhausting `vm.max_map_count`) →
plain tracked (ASan / valgrind mode). Callbacks wrap their own mallocs in `pt_cb_enter/leave` so injection only hits the code under test.

## Extending to another project
Add `prtest_core/projects/<name>` with its own spec + harness tests; the fork runner, allocator and guard buffers (`harness/*.c`)
are project-independent. Deliberately not generalised yet: there is only one project.
