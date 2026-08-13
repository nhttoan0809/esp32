#!/bin/sh
set -eu

REPO_ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

usage() {
  cat <<'EOF'
Usage: ./scripts/poc.sh <1|2|3|4> <build|artifacts|lint|verify|simulate|serial> [options]

Examples:
  ./scripts/poc.sh 1 verify
  ./scripts/poc.sh 3 simulate --expect-text HTTP_SERVER_STARTED
  ./scripts/poc.sh 3 serial --lines 10

Environment:
  PIO_BIN             Override PlatformIO executable
  WOKWI_CLI_BIN       Override Wokwi CLI executable
  WOKWI_TIMEOUT_MS    Simulation timeout, default 30000
  WOKWI_CLI_TOKEN     Required by Wokwi CLI for simulate
EOF
}

resolve_executable() {
  override=$1
  command_name=$2
  fallback=$3

  if [ -n "${override}" ]; then
    printf '%s\n' "${override}"
  elif command -v "${command_name}" >/dev/null 2>&1; then
    command -v "${command_name}"
  elif [ -x "${fallback}" ]; then
    printf '%s\n' "${fallback}"
  else
    return 1
  fi
}

if [ "$#" -lt 2 ]; then
  usage
  exit 2
fi

POC_NUMBER=$1
ACTION=$2
shift 2

case "${POC_NUMBER}" in
  1)
    PROJECT="${REPO_ROOT}/pocs/poc1-wifi-http"
    RFC2217_PORT=4001
    ;;
  2)
    PROJECT="${REPO_ROOT}/pocs/poc2-bluetooth"
    RFC2217_PORT=""
    ;;
  3)
    PROJECT="${REPO_ROOT}/pocs/poc3-web-led"
    RFC2217_PORT=4000
    ;;
  4)
    PROJECT="${REPO_ROOT}/pocs/poc4-softap-provisioning"
    RFC2217_PORT=4004
    ;;
  *)
    echo "Unknown POC: ${POC_NUMBER}" >&2
    usage
    exit 2
    ;;
esac

if [ ! -d "${PROJECT}" ]; then
  echo "Project is not present: ${PROJECT}" >&2
  exit 1
fi

PIO=$(resolve_executable "${PIO_BIN:-}" pio "${HOME}/.platformio/penv/bin/pio") || {
  echo "PlatformIO CLI not found. Run ./scripts/setup-tools.sh first." >&2
  exit 1
}

build() {
  "${PIO}" run -d "${PROJECT}" -e esp32dev
}

artifacts() {
  test -f "${PROJECT}/.pio/build/esp32dev/firmware.elf"
  test -f "${PROJECT}/.pio/build/esp32dev/firmware.bin"
  echo "ELF and BIN artifacts exist for POC ${POC_NUMBER}."
}

lint_diagram() {
  if [ ! -f "${PROJECT}/diagram.json" ]; then
    echo "POC ${POC_NUMBER}: lint N/A because this POC has no Wokwi diagram."
    return 0
  fi

  WOKWI=$(resolve_executable "${WOKWI_CLI_BIN:-}" wokwi-cli "${HOME}/bin/wokwi-cli") || {
    echo "Wokwi CLI not found. Run ./scripts/setup-tools.sh first." >&2
    exit 1
  }
  "${WOKWI}" lint "${PROJECT}"
}

case "${ACTION}" in
  build)
    build
    ;;
  artifacts)
    artifacts
    ;;
  lint)
    lint_diagram
    ;;
  verify)
    if [ -f "${PROJECT}/diagram.json" ]; then
      node -e 'JSON.parse(require("fs").readFileSync(process.argv[1], "utf8"))' \
        "${PROJECT}/diagram.json"
      echo "diagram.json is valid JSON."
    fi
    build
    artifacts
    lint_diagram
    ;;
  simulate)
    if [ ! -f "${PROJECT}/wokwi.toml" ]; then
      echo "POC ${POC_NUMBER} cannot be simulated: Wokwi does not support its BLE runtime." >&2
      exit 3
    fi
    if [ -z "${WOKWI_CLI_TOKEN:-}" ]; then
      echo "WOKWI_CLI_TOKEN is not set." >&2
      echo "Create one at https://wokwi.com/dashboard/ci and export it before running simulate." >&2
      exit 4
    fi
    WOKWI=$(resolve_executable "${WOKWI_CLI_BIN:-}" wokwi-cli "${HOME}/bin/wokwi-cli") || {
      echo "Wokwi CLI not found. Run ./scripts/setup-tools.sh first." >&2
      exit 1
    }
    build
    "${WOKWI}" --timeout "${WOKWI_TIMEOUT_MS:-30000}" "$@" "${PROJECT}"
    ;;
  serial)
    if [ -z "${RFC2217_PORT}" ]; then
      echo "POC ${POC_NUMBER} has no Wokwi RFC2217 port." >&2
      exit 3
    fi
    if [ ! -x "${REPO_ROOT}/.venv/bin/python" ]; then
      echo "Python environment not found. Run ./scripts/setup-tools.sh first." >&2
      exit 1
    fi
    "${REPO_ROOT}/.venv/bin/python" "${REPO_ROOT}/scripts/read-rfc2217.py" \
      --port "${RFC2217_PORT}" "$@"
    ;;
  *)
    echo "Unknown action: ${ACTION}" >&2
    usage
    exit 2
    ;;
esac
