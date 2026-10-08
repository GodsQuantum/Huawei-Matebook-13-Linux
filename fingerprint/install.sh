#!/bin/sh
set -eu

ROOT=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)

if command -v bash >/dev/null 2>&1; then
  exec bash "$ROOT/install-linux.sh" "$@"
fi

if command -v apk >/dev/null 2>&1; then
  echo "==> Bash is not installed; bootstrapping it for Alpine."
  if [ "$(id -u)" -eq 0 ]; then
    apk add --no-cache bash
  elif command -v sudo >/dev/null 2>&1; then
    sudo apk add --no-cache bash
  else
    echo "ERROR: Alpine needs bash; install it as root or provide sudo." >&2
    exit 3
  fi
  exec bash "$ROOT/install-linux.sh" "$@"
fi

echo "ERROR: this installer requires Bash. Install bash, then rerun:" >&2
echo "       $ROOT/install.sh $*" >&2
exit 3
