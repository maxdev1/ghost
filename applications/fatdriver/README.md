# FAT filesystem driver

Provides read/write access to FAT12, FAT16 and FAT32 volumes through Ghost's
filesystem API. It listens to the storage manager's `volume` topic, probes
announced regions and claims supported
volumes before mounting. The first partition (or whole-disk volume) appears under
`/mount/fat<DISK_ID>`; other partitions use `/mount/fat<DISK_ID>p<PARTITION>`.
Installed systems can use a FAT volume as `/`.

Use normal file operations in a Ghost terminal, replacing `fat2` with the mounted
volume's name:

```sh
echo hello | write /mount/fat2/example.txt
read /mount/fat2/example.txt
```

The driver supports long filenames, subdirectories, file creation/removal,
append, truncation and flushing. Writable mounts require a disk that supports
writes and cache flushing.

Use `fsync()` or `volumeFlush` when file data must be durable. Ordinary
writes can remain in the disk cache; allocation and directory updates retain
their ordering barriers.

`libstorage` provides mount, flush and unmount control through the storage manager.
Unmount rejects open files, active transactions and the boot root. An idle mount
is detached even if flushing fails; the I/O error is still returned.
If block-buffer closure fails, the detached mount retains its claim. Repeating
unmount completes buffer cleanup before releasing ownership.

The registered `fat32` formatter accepts storage-manager jobs with explicit sector
bounds. It initializes FAT32 inside that region, preserving the partition table and
neighboring partitions. The primary boot sector is published after durable FAT and
backup metadata. Labels are uppercased and limited to 11 FAT-compatible ASCII bytes.
Administrative reprobes advertise an unmounted handler through `volumeOffer`.

`volumeStat` obtains FAT type, allocation unit and exact free space through
the shared FAT reader, with bounded read-ahead and periodic scan progress. The count
is cached and maintained across allocations and frees, then invalidated on remount
or cache reset. On-disk FSInfo counts are not trusted. Boot-root arguments can include
an explicit partition LBA with the disk identity, preventing another FAT partition
on that disk from taking the root mount.
