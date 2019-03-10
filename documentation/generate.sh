#!/bin/bash

OUTPUT="$1"
if [ -z "$OUTPUT" ]
then
    OUTPUT="out"
fi

echo "Removing output folder '$OUTPUT'"
rm -rf out
echo "Generating documentation"
asciidoctor *.adoc -D "$OUTPUT"
asciidoctor **/*.adoc -D "$OUTPUT"
