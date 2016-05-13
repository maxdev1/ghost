echo Removing output folder
rm -rf out
echo Generating documentation
asciidoctor *.adoc -D out
asciidoctor **/*.adoc -D out
