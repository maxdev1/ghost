# libinput

Input library used to turn keyboard scancodes and mouse packets into application
input events. Keyboard layouts provide key names and character mappings, while
mouse events contain relative movement, button states and scrolling.

Given a keyboard pipe obtained through `libps2driver`:

```cpp
#include <ghost.h>
#include <libinput/keyboard/keyboard.hpp>

if(g_keyboard::loadLayout("de-DE"))
{
    auto key = g_keyboard::readKey(keyboard);
    char character = g_keyboard::charForKey(key);
}
```

Use `g_mouse::readMouse` with the mouse pipe to receive decoded mouse events.
