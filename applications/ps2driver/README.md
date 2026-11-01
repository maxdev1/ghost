# PS/2 driver

Runs the PS/2 controller handling from `libps2` and exposes keyboard and mouse
input through pipes. The kernel starts it during boot, and ƒenster connects to
receive scancodes, relative mouse motion, buttons and scrolling.

Clients obtain input pipes through `libps2driver`:

```cpp
#include <libps2driver/ps2driver.hpp>

g_fd keyboard = G_FD_NONE;
g_fd mouse = G_FD_NONE;
bool connected = ps2DriverInitialize(&keyboard, &mouse);
```

Use `libinput` to interpret these streams, or read the raw keyboard bytes and
`g_ps2_mouse_packet` records directly.
