# prtest — a local tester for 42 Libft (subject v19.3)

```
prtest libft .                      # test the Libft in the current directory
prtest libft . --function ft_strlen # or: -f strlen,split
prtest libft . --category memory    # ctype|string|memory|convert|alloc|output|list|project
prtest libft . --verbose            # list every test
prtest libft . --failed-only        # hide passing functions
```

Default timeout is 10 s per test (`--timeout`). Requirements: Linux, `cc`, `make`, `nm`, `ar`, Python ≥ 3.8 (standard library only).
Optional: `valgrind`, `norminette`. Install: `./install.sh` (symlink in `~/.local/bin`, no sudo).

**It never touches your files.** Your repo is copied to `/tmp/prtest-XXXX`, *your* Makefile is run there,
and the resulting `libft.a` is what gets tested. Nothing is fixed or rewritten for you.

## How big is the test suite?

About **440 named tests**; most are loops, so the reference library goes through **~52 million individual checks** in ~20 s
(`--runs 3` ≈ 156 million with different seeds and heap-fill patterns). Four layers per function:

1. **Behaviour tests**: the cases from the subject / man page (normal, edge, boundary).
2. **Exhaustive tests** over small input spaces: every string over `{a,b}` up to length 9 x every search value (`strchr`), every `(src, dst, n)`
   overlap (`memmove`), all 9 331 short strings over `{' ','+','-','0','5','a'}` (`atoi`), every integer in -100000..100000 (`itoa`), every
   string over `{',','a','b'}` up to length 8 (`split`), every `(nmemb, size)` in 0..24 (`calloc`), all trim/set combinations (`strtrim`), ...
3. **Randomized differential fuzzing** against libc or a reference model (fixed seed, `--seed` / `--runs`): up to 100 000 inputs per function,
   strings up to 5000 bytes, all alignments, bytes 0x80..0xFF, page-boundary sweeps.
4. **Deep tests** for the bugs that normal tests miss:
   * **Guard page BEFORE the data**: every test that uses guarded buffers is run a second time with the buffer at the *start* of its page
     (`[front guard]` variants). Catches reading/writing before the start (`s1[-1]` in `ft_strtrim("")`, backwards scans in `ft_strrchr`, ...).
   * **Read-only memory**: `const` inputs (and whole `t_list` nodes for `ft_lstsize/last/iter/map`) live in `PROT_READ` pages; any write crashes.
   * **Hidden state / re-entrancy**: 100-400 results alive at once must stay correct and distinct (no static buffers), results must not alias
     the input (input overwritten afterwards), callbacks that call the same function again (`strmapi`-in-`strmapi`, `lstclear`-in-`del`, ...),
     and `nm` flags mutable `static` variables.
   * **Huge inputs with a 1 MiB stack**: 8 MiB strings, 32 MiB blocks, 100 000-node lists. Recursive implementations crash,
     quadratic ones time out; indices on 300 000-char strings catch `unsigned short` truncation.
   * **Aliasing**: `ft_strjoin(s, s)`, `ft_strtrim(s, s)`, `ft_strncmp(s, s, n)`, `ft_memmove(p, p, n)`, ...
   * **Cascading allocation failure**: every allocating function is run with malloc #k failing, and also with #k *and every later malloc*
     failing (an error path that itself allocates crashes here).
   * **Write errors**: `put*_fd` on a closed pipe must return instead of retrying forever.
   * **Over-allocation** (`ft_substr(s, 1, 1000000)` must not malloc a megabyte), exact byte-difference results of `strncmp/memcmp` (man page).
5. **Project checks**: Makefile (relink, `-j8`, `make -C`, stale `.o` after editing a `.c` or `libft.h`, `fclean` leftovers, wildcards),
   unused files / folders, README format, libc calls in the source, hidden statics, globals, forbidden externals, header vs subject prototypes,
   norminette (or an approximate "Norm-lite" when it is not installed).

Not tested on purpose: anything that is undefined behaviour (see tiers below).

## How it works

```
prtest (Python)  →  copy repo to sandbox  →  run YOUR Makefile  →  nm / header / Makefile checks
                 →  compile C harness against libft.a  →  fork one process per test  →  report
```

* **The harness does not use your `libft.h`.** It has its own subject-derived prototypes (so a wrong
  prototype in your header can't silently make the tests pass). A separate check compiles your header
  against the subject's prototypes and reports conflicts (e.g. `ft_lstsize` must return `unsigned int` in v19.3).
* **Every test runs in its own forked process** with a timeout (`--timeout`, default 3 s), memory limit,
  and its own stdout/stderr (leftover debug prints are reported as warnings). Crashes become `CRASH`, hangs `TIMEOUT`.
* **Instrumented allocator** (linker `--wrap` of `malloc/free/calloc`, only for *your* code):
  every block sits against a `PROT_NONE` guard page (overflow = immediate fault), fresh memory is poisoned `0xA5`
  (a "calloc" that forgot to zero is caught), freed blocks become inaccessible (use-after-free faults),
  double / invalid frees are reported, and leaks are listed per test.
* **Allocation-failure injection** (`[inject]` tests): for each allocating function the test counts the mallocs,
  then re-runs with malloc #1, #2, … #N returning `NULL`. Expected: `NULL` returned, nothing leaked, no crash.
  This is where `ft_split` / `ft_lstmap` usually break.
* **Guard-page input buffers**: inputs end exactly at an unmapped page, so reading past a NUL / past `n` crashes.
  Output buffers are surrounded by canary bytes.
* **Libc isn't blindly trusted.** `ft_is*` must return exactly `1`/`0`; `strlcpy/strlcat/strnstr` use BSD semantics
  (not in glibc); comparison functions compare the *sign*; `calloc(0, x)` must return a unique freeable pointer.

### Verdict tiers

| Tier | Meaning | Result |
|---|---|---|
| spec | stated by the subject / man page | `FAIL` |
| convention | not stated, but evaluators usually expect it (e.g. `ft_lstsize(NULL) == 0`, `ft_substr` with `start > len` → `""`) | `WARN` (`--strict` → fail) |
| undefined | UB in libc too (`ft_strlen(NULL)`, overlapping `memcpy`) | never run; `--probe-ub` runs them as `INFO` |

Exit codes: `0` ok, `1` failures, `2` usage / build / detection error.

## Other options

`--runs N` repeats the suite with different seeds and heap-poison bytes (0xA5, 0xFF, 0x01, ...); `--opt 2|3` also tests your sources
recompiled at -O2/-O3 (finds optimizer-dependent bugs); `--no-front` skips the guard-before-data pass;
`--sanitize` recompiles your `ft_*.c` with ASan+UBSan and tests those (finds UB in *your* code on valid inputs);
`--valgrind` runs the harness under valgrind; `--stress` adds heavy tests (200 000-node lists);
`--seed N` reproduces a random run; `--json`; `--no-color`; `--no-norm`; `--list`; `--keep-sandbox`.

## Validating prtest itself

```
prtest --selftest
```
Runs a known-good reference Libft (`selftest/golden/`, must pass, also under ASan/UBSan) and **70 mutants**
(`selftest/mutants.py`: off-by-one, missing NUL, leak-on-failure, unsigned/signed mix-ups, and bugs that only trigger for one value, for long inputs, or for a 0xFF delimiter). Every mutant must be caught.
`examples/broken_libft/` is a project with several deliberate bugs; try `prtest libft examples/broken_libft`.

> **42 note:** `selftest/golden/` is a complete reference implementation used *only* to validate the tester.
> The AI chapter of your subject asks you to build Libft yourself and be able to explain it — don't copy from it,
> and feel free to delete `selftest/golden` (and `--selftest`) from your own copy.

## Known limitations

* Linux only (`--wrap` is GNU ld). macOS needs a different interposition mechanism.
* `--valgrind` and the `norminette` integration are implemented but were **not exercised** in the environment this
  was built in (neither tool was installed there). ASan/UBSan mode and everything else were run.
* Over-read detection is probabilistic by nature: aligned word-sized reads that stay inside one page can't be caught.
* Tier-"convention" cases are judgement calls; check the hint before assuming a warning is a bug.
* `--only functional|leaks|…` from the design doc is not implemented; use `--category` / `--function`.
* Your Makefile runs for real inside the sandbox; a Makefile with an unsafe `rm -rf $(UNSET)/` could still do damage.
* No `restrict`/C99 check on `.c` files beyond the header; no bonus profile (v19.3 has no bonus part).

## Layout

```
prtest                  launcher            prtest_core/   Python (cli, project checks, harness driver, report, spec)
harness/                C harness: run.c (fork runner), alloc.c (allocator/guards/injection), libft/t_*.c (tests)
selftest/               golden reference + mutants + runner   examples/   broken sample project   docs/DESIGN.md
```
