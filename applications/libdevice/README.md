# libdevice

Library used to register and list devices with the device manager. It also
defines the device types and registration events shared with consumers.
Registration returns a system-wide ID and announces the serving task on the
`device` topic.

```cpp
#include <ghost.h>
#include <libdevice/manager.hpp>

g_device_id id{};
bool registered = deviceManagerRegisterDevice(G_DEVICE_TYPE_VIDEO, g_get_tid(), &id);
```

The registered task must handle the protocol for its device type, such as video
mode requests for a video device.

`deviceManagerListDevices(afterId, &response)` queries a page of registered
devices in ascending ID order. Start with `afterId = 0`, then use the last ID
returned until `response.count < G_DEVICE_MANAGER_LIST_MAX_DEVICES`. Each entry
contains its ID, type and handler task. The call returns false if the manager is
unavailable or the exchange fails. `device list` uses this interface.
