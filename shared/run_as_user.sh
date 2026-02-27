#!/bin/sh

# Exit on any error and treat unset variables as errors
set -euo

TARGET_UID="$1"
TARGET_GID="$2"
shift 2

GROUP_NAME="hostgroup"
USER_NAME="hostuser"

if ! getent group "$TARGET_GID" >/dev/null; then
    groupadd -g "$TARGET_GID" "$GROUP_NAME"
else
    GROUP_NAME="$(getent group "$TARGET_GID" | cut -d: -f1)"
fi

if ! id "$TARGET_UID" >/dev/null 2>&1; then
    useradd -m -u "$TARGET_UID" -g "$TARGET_GID" "$USER_NAME"
else
    USER_NAME="$(getent passwd "$TARGET_UID" | cut -d: -f1)"
fi

exec su "$USER_NAME" -c "$*"
