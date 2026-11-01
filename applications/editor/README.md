# Editor

A text editor with file dialogs, undo/redo, search/replace, selection and word
wrap. It reads UTF-8 and BOM-marked UTF-16 files and preserves their encoding and
line-ending format when saving.

Open it from the desktop/start menu or a Ghost terminal. An optional path opens
an existing file:

```sh
/applications/editor.bin
/applications/editor.bin /user/example.txt
```

Use the File menu to open or save documents. Ctrl+S saves, Ctrl+Z undoes, and
Ctrl+F opens Find. Unsaved changes are checked before replacing a document or
closing the window.

Save waits for the filesystem and disk cache to flush before marking the
document saved. A failed flush keeps the changes marked unsaved.
