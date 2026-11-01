# libahci

Support library for AHCI controller, ATA command and SATA FIS definitions. The
AHCI driver uses these structures to interpret mapped controller registers and
prepare DMA commands. Applications read disks through `libblockdevice`.

Given a mapped controller base address, inspect the first port's device state:

```cpp
#include <libahci/ahci.hpp>

auto port = reinterpret_cast<volatile g_ahci_hba_port*>(
    controllerBase + G_AHCI_HBA_PORT_OFFSET);
bool present = (port->ssts & 0x0f) == G_AHCI_HBA_PORT_SSTS_DET_READY;
```
