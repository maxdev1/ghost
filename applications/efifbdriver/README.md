# Boot framebuffer driver

Exposes the framebuffer supplied at boot as a video device. The device manager
starts it when no VMSVGA controller is selected. It shares the framebuffer with
clients and retains the existing firmware video mode.

Use `libvideo` with the driver and ID from a video-device registration:

```cpp
#include <libvideo/videodriver.hpp>

g_video_mode_info mode{};
bool ready = videoDriverSetMode(event.driver, event.id, 1280, 800, 32, mode);
```

The returned `mode` contains the actual dimensions, byte stride and shared
framebuffer address; this driver does not change them to the requested values.
