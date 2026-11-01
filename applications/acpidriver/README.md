# ACPI driver

Runs uACPI in a driver process to initialize ACPI tables, evaluate firmware methods
and handle ACPI events. It prepares firmware for shutdown and reboot; the kernel
coordinates those transitions and performs the final hardware operations.

Shutdown and reboot preparation failures are written to `klog` with the failing
stage and uACPI status. The terminal `shutdown` command reports the same overall
power status as the desktop.

Applications query firmware capabilities through `libacpi`. This checks whether
the firmware describes the S5 power-off state:

```cpp
#include <libacpi/acpi.hpp>

g_acpi_sleep_state state{};
auto status = acpiGetSleepState(5, &state);
```
