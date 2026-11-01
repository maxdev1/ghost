# Core tools

Small command-line programs for files, processes, devices and text streams. They can run
individually or be connected with pipes in Ghost's shell.

| Command | Purpose |
| --- | --- |
| `echo [-n] [TEXT...]` | Prints arguments separated by spaces; `-n` omits the final newline. With no arguments, copies standard input. |
| `ls [DIRECTORY]` | Lists a directory, or the working directory when no path is given. |
| `reverse` | Reads standard input and reverses the characters in each line. |
| `lines` | Reports zero for empty standard input, otherwise one plus the number of newline characters. |
| `read [FILE...]` | Prints files in order. With no files, or a `-` argument, reads standard input. |
| `write [-a] FILE` | Writes standard input to a file, creating or replacing it; `-a` appends. |
| `copy SOURCE DESTINATION` | Copies a regular file. A directory destination keeps the source filename. |
| `process list [--threads]` | Lists processes with CPU usage and resident user memory; optionally adds individual thread rows. |
| `process ID kill` | Requests termination of the process and all its threads, subject to kernel privilege checks. |
| `topic [-f] TOPIC` | Prints retained IPC topic messages; `-f` continues waiting for new messages. |
| `device list` | Queries device manager for all registered devices, showing ID, type and handler task ID. |
| `blockdevice list` | Lists registered block devices with their ID, model and capacity in MiB. |
| `blockdevice ID get` | Queries the device's driver for its model, serial, geometry and write/flush support. |
| `disk list` | Lists disks, partition-table types, partition counts and active jobs. |
| `disk ID get` | Shows disk geometry, identity, table state and active job. |
| `disk ID partition list` | Lists partitions, corresponding volume IDs and free sector ranges. |
| `disk ID partition NUMBER get` | Shows the selected partition. |
| `disk ID partition NUMBER delete` | Removes a primary MBR entry, preserving its data. |
| `disk ID partition create --size SIZE` | Allocates a primary MBR partition in an aligned free region. |
| `disk ID table create mbr [--replace]` | Creates an empty MBR; `--replace` discards existing partitions and filesystems. |
| `disk ID wipe --zero` | Zero-fills the complete disk, including all tables and filesystems. |
| `volume ID get` | Shows the volume's location, filesystem and mount state. |
| `volume ID format fat32 [--label LABEL]` | Formats the selected volume and leaves it unmounted. |
| `volume ID wipe --zero` | Zero-fills the volume while preserving its partition entry. |
| `disk job ID get`, `volume job ID get` | Reports progress and outcome of an administrative job. |
| `volume list` | Lists every volume with its disk, filesystem, state and current mount path. |
| `volume ID mount [NAME]` | Mounts a volume at `/mount/NAME`, or uses the filesystem driver's default name. |
| `volume TARGET unmount` | Unmounts by volume ID, mount name or full mount path. |

Examples in a Ghost terminal:

```sh
ls /user
echo hello | reverse
echo -n hello | lines
echo hello | write /user/example.txt
echo again | write -a /user/example.txt
copy /user/example.txt /user/example-copy.txt
read /user/example-copy.txt
process list
process list --threads
process 5 kill
topic -f device
device list
blockdevice list
blockdevice 2 get
volume list
volume fat2 unmount
volume 4 mount archive
volume /mount/archive unmount
```

`topic` writes message payloads unchanged, so binary topics produce binary output.
`copy` handles individual files rather than directory trees.
`process` samples CPU usage over 250 ms, using actual elapsed time. CPU% uses one
CPU as 100%, so a multithreaded process can exceed 100%. CPU-MS is cumulative
scheduled time, including exited threads and kernel work on their behalf.
RSS-KiB counts resident user mappings, including shared RAM, excluding MMIO,
uncommitted pages and kernel overhead. Threads share that memory, so it appears
only on the process row (TID `-`). New tasks show `-` for CPU% until a baseline
exists. Without HPET, CPU accounting has timer-tick precision.
`blockdevice` discovers devices through device-manager announcements and queries
their drivers through `libblockdevice`. IDs are decimal numbers from 1 to 65535.

`disk` and `volume` communicate with `storagemanager` through `libstorage`.
For `volume`, numeric targets are volume IDs; use
`/mount/4` to select a mount named `4`. Mount names are single components of up to
31 letters, digits, dots, underscores or hyphens; `.` and `..` and the internal
`fat-root` name are reserved. An existing mount is not moved by requesting another
name. Busy or protected mounts remain mounted, and a timeout leaves the operation's
outcome uncertain; use `volume list` to inspect it.

Partition creation accepts `--start OFFSET`, `--align SIZE`, `--slot 1..4`, and
`--type fat32|0xNN` (default `fat32`, an MBR type code, not formatting).
`--size remaining` uses the first suitable free region. Sizes use sectors
(bare numbers or `s`), `KiB`, `MiB`, `GiB` or `TiB`; alignment defaults to 1 MiB.
Explicit starts must satisfy alignment. Sector values are shown without rounding.
GPT tables can be listed; creating or editing GPT and editing extended MBR layouts
are unsupported. Without `--replace`, table initialization checks for nonzero data
in the first and last 1 MiB and refuses existing partition layouts or whole-disk
filesystems. `disk ID table create mbr --replace` clears those metadata areas and
creates an empty MBR, discarding the existing partitions and filesystems. It does
not scan or erase the disk's interior. Use `wipe --zero` to zero-fill all contents.
Partition deletion removes only the entry, leaving its data.

Administrative writes reserve the entire disk, so all its volumes must be unmounted.
FAT32 formatting supports 512-byte sectors, up to 32-bit sector counts and starting
LBAs, labels up to 11 printable ASCII characters excluding FAT punctuation, and
volumes large enough for at least 65,525 clusters (roughly 33 MiB).
Formatting initializes metadata, whereas `wipe --zero` clears every selected sector;
zero-fill does not guarantee erasure of remapped or hidden hardware sectors.

Jobs continue if a CLI exits or its IPC request times out. Query `disk ID get` for
its active job and `disk job JOB get` for its result. Also inspect the current
partition/volume layout before retrying a request whose acknowledgement was lost. Job IDs and
history last for the manager's boot lifetime. If I/O completion is uncertain, the
disk remains reserved until reboot. Administrative rescans leave volumes unmounted;
filesystem workers advertise recognition for a subsequent explicit mount.
