#!/bin/sh

# Exit on any error and treat unset variables as errors
set -euo

UID="$1"
GID="$2"
shift 2

GROUP_NAME="hostgroup"
USER_NAME="hostuser"

if ! getent group "$GID" >/dev/null; then
    groupadd -g "$GID" "$GROUP_NAME"
else
    GROUP_NAME="$(getent group "$GID" | cut -d: -f1)"
fi

if ! id "$UID" >/dev/null 2>&1; then
    useradd -m -u "$UID" -g "$GID" "$USER_NAME"
else
    USER_NAME="$(getent passwd "$UID" | cut -d: -f1)"
fi

exec su "$USER_NAME" -c "$*"
