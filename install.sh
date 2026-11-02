#!/bin/sh
# Symlinks prtest into ~/.local/bin (no sudo). Re-run safely.
set -e
here="$(cd "$(dirname "$0")" && pwd)"
mkdir -p "$HOME/.local/bin"
ln -sf "$here/prtest" "$HOME/.local/bin/prtest"
echo "installed: $HOME/.local/bin/prtest -> $here/prtest"
case ":$PATH:" in *":$HOME/.local/bin:"*) ;; *) echo "note: add \$HOME/.local/bin to your PATH";; esac
