# libps2

Library used by the PS/2 driver to initialize the keyboard/mouse controller and
decode incoming data. It handles controller commands, keyboard scancodes, mouse
movement, buttons and wheel packets, delivering them through callbacks.

Synaptics PS/2 touchpads are detected and explicitly placed in relative mode
with firmware tapping enabled. Initialization clears retained sleep settings.
Native absolute input, multi-finger scrolling and palm rejection are not supported.

A driver supplies the callbacks and initializes the controller:

```cpp
#include <libps2/ps2.hpp>

void onMouse(int16_t x, int16_t y, uint8_t buttons, int8_t scroll);
void onKey(uint8_t scancode);
auto status = ps2Initialize(onMouse, onKey);
```

This interface requires driver privilege. Applications normally receive input
through ƒenster or connect to `ps2driver` using `libps2driver`.
