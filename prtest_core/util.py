import os
import subprocess


def run(cmd, cwd=None, timeout=120, env=None):
    """Run a command, return (returncode, combined output). Never raises on timeout."""
    e = dict(os.environ, LC_ALL="C")
    if env:
        e.update(env)
    try:
        p = subprocess.run(cmd, cwd=cwd, env=e, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           timeout=timeout, universal_newlines=True, errors="replace")
        return p.returncode, p.stdout
    except subprocess.TimeoutExpired as ex:
        out = ex.stdout if isinstance(ex.stdout, str) else ""
        return 124, out + "\n[prtest] command timed out after %ss" % timeout
    except FileNotFoundError:
        return 127, "command not found: %s" % cmd[0]


def tail(text, n=12):
    lines = text.strip().splitlines()
    return "\n".join(lines[-n:])
