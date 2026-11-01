# libvideo

Library used to request video modes and access a video driver's shared
framebuffer. It defines the common protocol used by the boot framebuffer and
VMSVGA drivers.

Given a video-device registration:

```cpp
#include <libvideo/videodriver.hpp>

g_video_mode_info mode{};
bool ready = videoDriverSetMode(event.driver, event.id, 1280, 800, 32, mode);
```

On success, `mode` contains the actual dimensions, pixel depth, byte stride and
framebuffer address. If `explicit_update` is set, call `videoDriverUpdate` after
drawing to notify the driver which region changed.
