# libps2driver

Client library used to connect to the PS/2 driver and obtain keyboard and mouse
input pipes. Keyboard data contains scancodes; mouse data uses
`g_ps2_mouse_packet`. `libinput` can decode both streams.

```cpp
#include <libps2driver/ps2driver.hpp>

g_fd keyboard = G_FD_NONE;
g_fd mouse = G_FD_NONE;
bool connected = ps2DriverInitialize(&keyboard, &mouse);
```

Optional partner-task arguments let the driver yield to the tasks consuming new
input. ƒenster uses this connection to receive system input.
