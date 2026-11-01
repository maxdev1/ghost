#!/bin/bash

set -euo pipefail

SOURCE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT="${1:-"$SOURCE_DIR/out"}"
DOCINFO="$SOURCE_DIR/common"
MERMAID_VERSION="11.12.2"
MERMAID_FILE="mermaid-$MERMAID_VERSION.min.js"
MERMAID_CACHE="$SOURCE_DIR/.cache/$MERMAID_FILE"

if [ ! -f "$DOCINFO/docinfo-footer.html" ]; then
    echo "Missing documentation renderer: $DOCINFO/docinfo-footer.html" >&2
    exit 1
fi

mkdir -p "$SOURCE_DIR/.cache" "$OUTPUT/assets"
if [ ! -s "$MERMAID_CACHE" ]; then
    echo "Downloading Mermaid $MERMAID_VERSION"
    trap 'rm -f "$MERMAID_CACHE.download"' EXIT
    curl --fail --location --silent --show-error \
        "https://cdn.jsdelivr.net/npm/mermaid@$MERMAID_VERSION/dist/mermaid.min.js" \
        -o "$MERMAID_CACHE.download"
    mv "$MERMAID_CACHE.download" "$MERMAID_CACHE"
fi
cp "$MERMAID_CACHE" "$OUTPUT/assets/$MERMAID_FILE"

echo "Generating documentation in '$OUTPUT'"
asciidoctor -a docinfo=shared-footer -a "docinfodir=$DOCINFO" -D "$OUTPUT" \
    "$SOURCE_DIR"/*.adoc "$SOURCE_DIR"/*/*.adoc
