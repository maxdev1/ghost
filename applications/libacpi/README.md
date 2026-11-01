# libacpi

Library used to query the ACPI driver's firmware status and supported sleep
states. Queries return cached capabilities, including whether the firmware
namespace initialized successfully.

Service messages and shared types are defined in libapi's C-compatible
`ghost/system/acpi.h`. The library headers include that shared interface.

```cpp
#include <libacpi/acpi.hpp>

g_acpi_info info{};
auto status = acpiGetInfo(&info);
```

`acpiGetSleepState` queries one state from S0 through S5. These capabilities do
not imply OS suspend support; request orderly shutdown or reboot through
`g_system_power`.
