# Device manager

Starts the display, storage and filesystem drivers and the storage manager needed by Ghost. It selects
VMSVGA when available and otherwise uses the boot framebuffer. Drivers register
the devices they provide, and the manager assigns IDs and publishes registrations
on the replayable `device` topic.

A driver registers its serving task through `libdevice`:

```cpp
#include <ghost.h>
#include <libdevice/manager.hpp>

g_device_id id{};
bool registered = deviceManagerRegisterDevice(G_DEVICE_TYPE_BLOCK, g_get_tid(), &id);
```

Consumers use the registration's device type, ID and driver task to choose the
appropriate client library, such as `libvideo` or `libblockdevice`.

Run `device list` to query the manager for all registered devices. The listing
shows each device's ID, type and handler task ID, ordered by device ID.
