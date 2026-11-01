# VMSVGA driver

Drives the VMware SVGA graphics controller. The device manager selects it when
that controller is present. Clients receive a shared framebuffer and notify the
driver about changed regions through `libvideo`.

Given a video-device registration, request a mode:

```cpp
#include <libvideo/videodriver.hpp>

g_video_mode_info mode{};
bool ready = videoDriverSetMode(event.driver, event.id, 1280, 800, 32, mode);
```

Call `videoDriverUpdate` after drawing when the returned mode requires explicit
updates. Much of the implementation derives from the
[VMware SVGA example driver](https://sourceforge.net/projects/vmware-svga/).
