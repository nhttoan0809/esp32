#!/bin/sh
set -eu

WOKWI_CLI_VERSION="${WOKWI_CLI_VERSION:-0.26.1}"
REPO_ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
USER_BIN="${HOME}/bin"
PLATFORMIO_BIN="${HOME}/.platformio/penv/bin"

if [ -x "${USER_BIN}/wokwi-cli" ] &&
  "${USER_BIN}/wokwi-cli" --short-version | grep -q "^${WOKWI_CLI_VERSION} "; then
  echo "Wokwi CLI ${WOKWI_CLI_VERSION} is already installed."
else
  echo "Installing Wokwi CLI ${WOKWI_CLI_VERSION} from the official installer..."
  curl -L https://wokwi.com/ci/install.sh | sh -s -- "${WOKWI_CLI_VERSION}"
fi

mkdir -p "${USER_BIN}"
if [ -x "${PLATFORMIO_BIN}/pio" ]; then
  ln -sfn "${PLATFORMIO_BIN}/pio" "${USER_BIN}/pio"
  ln -sfn "${PLATFORMIO_BIN}/platformio" "${USER_BIN}/platformio"
else
  echo "PlatformIO IDE CLI was not found at ${PLATFORMIO_BIN}." >&2
  echo "Install PlatformIO IDE first, then rerun this script." >&2
  exit 1
fi

python3 -m venv "${REPO_ROOT}/.venv"
"${REPO_ROOT}/.venv/bin/python" -m pip install \
  --disable-pip-version-check \
  -r "${REPO_ROOT}/requirements-dev.txt"

echo
echo "Installed:"
"${USER_BIN}/wokwi-cli" --short-version
"${USER_BIN}/pio" --version
"${REPO_ROOT}/.venv/bin/python" -c \
  'import serial; print("pyserial", serial.VERSION)'
echo
echo "Open a new shell so ${USER_BIN} is on PATH."
echo "A WOKWI_CLI_TOKEN is still required to run cloud simulations."
