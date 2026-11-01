# PCI driver

Enumerates PCI devices and serves configuration-space and BAR requests for other
drivers. The kernel starts it before ACPI and device discovery; clients use
`libpci` to access the service.

Given a device address from `pciDriverListDevices`, read its vendor ID:

```cpp
#include <libpci/driver.hpp>

uint32_t vendorId = 0;
bool read = pciDriverReadConfig(device, PCI_CONFIG_OFF_VENDOR_ID, 2, &vendorId);
```

The service also provides BAR sizes and enables or disables resource access for
a device.
