# libpci

Library for communicating with the PCI driver. It lists discovered devices and
provides configuration-space, BAR and resource-access operations used by hardware
drivers.

PCI constants and service messages are defined in libapi's C-compatible
`ghost/system/pci.h`. The library headers include that shared interface.

```cpp
#include <libpci/driver.hpp>
#include <cstdio>

int count = 0;
g_pci_device_data* devices = nullptr;
if(pciDriverListDevices(&count, &devices))
{
    printf("PCI devices: %d\n", count);
    pciDriverFreeDeviceList(devices);
}
```

Each entry includes a device address and class codes. Pass that address to
functions such as `pciDriverReadConfig` or `pciDriverReadBAR`.
