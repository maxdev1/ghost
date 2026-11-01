# Storage manager

`storagemanager` manages disks, partitions and volumes. It discovers storage
regions, announces available volumes and coordinates their use by filesystem
drivers. It handles mount, unmount, flush and filesystem-information requests
through `libstorage`.

It validates partition layouts and boot space, manages partition changes and
wiping, and coordinates formatting and boot-code installation. Disk reservations
keep these operations from conflicting with mounted filesystems or other storage
work.

The `disk` and `volume` tools use it for storage administration. The Ghost
installer uses it to prepare its destination and install boot code. Filesystem
drivers handle formatting and file access.
