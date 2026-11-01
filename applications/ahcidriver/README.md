# AHCI driver

Exposes SATA disks on AHCI controllers as block devices with 512-byte sectors.
The device manager starts it automatically, and each disk is announced on the
`device` topic. Applications access disks through `libblockdevice`.

Given a block-device registration event, read the disk's first sector:

```cpp
#include <libblockdevice/driver.hpp>

uint8_t sector[512];
auto status = blockDeviceRead(event.driver, event.id, 0, 1, sector);
```

Writes and flushes use the same driver task and device ID. Filesystem access is
provided separately by `fatdriver`.
