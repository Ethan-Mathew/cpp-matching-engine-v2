#!/usr/bin/env bash

set -euo pipefail

RAW_DIR="data/raw"
FULL_DIR="data/full"

FILE="03272019.NASDAQ_ITCH50.gz"
BASE_URL="https://emi.nasdaq.com/ITCH/Nasdaq%20ITCH"

mkdir -p "${RAW_DIR}"
mkdir -p "${FULL_DIR}"

echo "Downloading ITCH sample..."
curl -L --fail --retry 3 \
    -o "${RAW_DIR}/${FILE}" \
    "${BASE_URL}/${FILE}"

echo "Verifying gzip archive..."
gzip -t "${RAW_DIR}/${FILE}"

echo "Extracting full binary file..."
gzip -cd "${RAW_DIR}/${FILE}" \
    > "${FULL_DIR}/03272019.NASDAQ_ITCH50.bin"

echo "Done."
echo "Extracted file:"
echo "  ${FULL_DIR}/03272019.NASDAQ_ITCH50.bin"