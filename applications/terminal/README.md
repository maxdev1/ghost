# Terminal

Provides a graphical terminal window and starts the Ghost shell, `gsh`. It
connects keyboard input to the running program and displays standard output and
error output, including terminal color and cursor-control sequences.

Open it from the desktop/start menu or launch another window in Ghost:

```sh
/applications/terminal.bin
```

The window supports resizing and scrollback. Applications use ordinary standard
streams for text and `libterminal` for interactive terminal controls.
