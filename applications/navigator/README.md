# Navigator

Navigator is the resident desktop and file manager. It owns desktop icons, the
taskbar, start menu and independent folder windows. ƒenster starts it at boot;
closing the last folder window leaves the desktop running.

Open a new folder window from the start menu or run:

```sh
/applications/navigator.bin /user
```

Later invocations send the folder path to the resident process over IPC and exit.
Without a path they open Computer (`/`). Relative paths use the caller's working
directory. The first invocation starts the desktop; an explicit path also opens
that folder. No startup flag is needed.

Folder windows offer icon/list views, sorting, hidden-file visibility, a name
filter, location shortcuts and independent Back/Forward history. Ctrl+N opens a
new window, Ctrl+W closes it, Ctrl+L selects the path, Ctrl+F selects the filter,
and Alt+Up opens the parent folder. Files open in Editor and executables launch
as separate applications.

Add a `.launcher` file under `/user/desktop/` or `/user/start-menu/`, then restart
Navigator to load it:

```ini
name=Terminal
application=/applications/terminal.bin
icon=/system/graphics/app-icons/terminal.png
arguments=
```

`name` and an absolute `application` path are required. Icon and arguments are
optional. Start-menu entries are ordered by filename. Navigator launchers and
common-folder actions open windows within the resident process. The taskbar
switches between windows and restores minimized ones; the start menu also offers
shutdown and restart.

The `navigator` task endpoint accepts the versioned open-folder protocol in
`src/interface.hpp`. Paths are absolute UTF-8 byte strings under 4096 bytes;
1024-byte fragments share a transaction and are acknowledged individually.
`Accepted` means the request was queued; filesystem failures appear in the window.
