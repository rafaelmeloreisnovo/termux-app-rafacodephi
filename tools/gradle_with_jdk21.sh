#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if [[ -z "${JAVA_HOME:-}" ]]; then
  for candidate in \
    "/usr/lib/jvm/java-21-openjdk-amd64" \
    "/usr/lib/jvm/java-21-openjdk" \
    "/usr/lib/jvm/temurin-21-jdk-amd64" \
    "/opt/java/openjdk"; do
    if [[ -x "$candidate/bin/java" ]]; then
      export JAVA_HOME="$candidate"
      break
    fi
  done
fi

if [[ -n "${JAVA_HOME:-}" && -x "$JAVA_HOME/bin/java" ]]; then
  export PATH="$JAVA_HOME/bin:$PATH"
fi

exec "$ROOT_DIR/gradlew" "$@"
