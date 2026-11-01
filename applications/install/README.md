# Ghost installer

Installs Ghost and the Limine BIOS bootloader from the live system. The desktop
application lists whole disks and their volumes, with a confirmation specific to
the chosen target.

```sh
install list
install disk 2
install volume 4
```

A **disk** target replaces every partition with a new MBR and one FAT32 partition.
The app warns that all existing disk data will be lost. This initializes filesystem
metadata rather than zero-filling the entire drive.

A **volume** target reuses an existing writable FAT32 primary MBR partition,
including partition 2 or 3. It preserves the filesystem, unrelated files and other
partitions. Matching installation paths are replaced; file/directory conflicts and
insufficient free space are rejected before copying. Free-space scans report progress
and may run longer than 30 seconds while advancing. The UI shows separate storage,
space-check, copy and verification phases. The bootloader is installed in
the reserved gap before the first partition. Partition boundaries, types, boot
flags and an existing disk identity are preserved.
The generated Limine configuration is installed at `/limine.conf`.
GPT/whole-disk filesystem targets and layouts without a sufficient bootloader gap
are not supported by this BIOS installation path.

Limine scans the boot disk in partition order and uses the first configuration
it finds. An earlier partition's configuration can shadow the installed one.
Newly formatted disks have one active partition for BIOS compatibility.

Type `INSTALL` to confirm. `--yes` before `disk` or `volume` skips confirmation.
The old numeric disk argument and `--list` remain accepted.

The installer uses `libstorage` for discovery, disk preparation, reservations,
filesystem space/type queries, guarded boot writes and mount/flush/unmount coordination.
The manager validates partition metadata and boot space and preserves disk identity.
The installer supplies Limine assets and generates its configuration. The filesystem
driver performs FAT32 formatting and FAT inspection. Copying and verification use
ordinary file APIs. Both target modes share one installation sequence and commit
bootloader code only after files are verified and the target is unmounted.
Busy disks and the running system disk are protected. The live installer, its private assets and its desktop/start-menu launchers are
not copied.
