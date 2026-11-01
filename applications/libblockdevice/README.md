# libblockdevice

Client library for querying, reading, writing and flushing block devices. It
provides ordinary-buffer operations and reusable shared DMA buffers for repeated
I/O. The driver task and disk ID come from a block-device registration.

```cpp
#include <libblockdevice/driver.hpp>

g_block_device_info info{};
auto status = blockDeviceGetInfo(event.driver, event.id, &info);
```

The returned information includes sector size, capacity and write/flush support.
Use `blockDeviceRead` and `blockDeviceWrite` for transfers, then
`blockDeviceFlush` when writes must reach durable storage.
