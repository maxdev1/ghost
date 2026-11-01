# libstorage

Client API for the `storagemanager` service, covering disk and partition
administration, volume discovery, filesystem mounting and boot-code installation.
Include headers from `libstorage/` and link with `-lstorage`.
Volume IDs identify storage regions for the current boot; disk IDs come from
device manager.

`storageManager*` functions handle service requests, jobs and formatter registration.
`volume*` functions operate on volumes and mounts; `disk*` functions operate on
disks, partitions and boot code.

`volumeNext` enumerates volumes, optionally filtered by disk ID.
`volumeGet` reports their disk, raw driver, sector range, filesystem,
serving task, mount path and state. `volumeControl` requests mount or
unmount from the selected filesystem worker; `volumeFlush` flushes it. `volumeMount` accepts an optional
public name under `/mount`; `volumeUnmount` can require an expected claim
token so resolving a name cannot accidentally unmount a replacement mount.

Filesystem workers probe an announced region without writing, then call
`volumeClaim` before registering a VFS delegate. The first accepted claim
wins. Conflicts return the current owner. An accepted claim is `MOUNTING` until
`volumeCommit` confirms the VFS mount ID. After confirmed detachment or a
failed mount, the same worker calls `volumeRelease`.

Claims require the serving thread and returned token. Repeated claim, commit and
release requests are idempotent. IPC timeout does not cancel an accepted request
or expire its claim. Query the manager to reconcile uncertain results.

`diskClaim` excludes all volume claims while a formatter changes a
disk layout. Rescan while holding that claim, then release it. A failed rescan
must be resolved before releasing the disk for new mounts.

`admin.hpp` adds disk/partition enumeration and asynchronous administrative jobs.
`diskGet`, `diskNext` and `diskPartitionNext` report geometry, layout,
free-range bounds, table entries and corresponding volume IDs. `storageManagerAdminStart`
accepts a mutation with the observed disk generation; `storageManagerJobGet` reports progress
and final status. A stable nonzero operation ID permits an idempotent retry from the
same client thread after a lost acknowledgement. Never assume timeout canceled I/O.
`activeJob` identifies a disk's outstanding reservation. Job history is boot-local.

`storageManagerFormatterRegister` registers a filesystem formatter. Only the manager sends
bounded `G_VOLUME_FORMAT_WORK` requests; formatter completion acknowledges closed
DMA windows. `G_VOLUME_PROBE` asks workers to recognize changed media without mounting;
`volumeOffer` records a handler for a later explicit mount. Administrative
formatting/wiping and table edits remain excluded until every volume on the disk is
unmounted. Direct raw block clients must cooperate with these reservations.

`volumeStat` queries a mounted filesystem's type, allocation unit, total and
free space, and writable state through its driver. Protocol version 3 permits
intermediate STAT replies with nonzero `total` and advancing `completed` allocation
units. These renew the 30-second inactivity deadline; stalled or unauthenticated
progress does not. The optional callback reports scan progress. Final replies have
both counters zero.

`diskClaim` can allow one selected volume to be mounted through requests from the reservation's
owner; competing controls and administrative jobs remain excluded. The owner can
rebind that volume while the disk is unmounted, keeping one reservation throughout
an installation. An administrative job from the same owner may run under its disk
reservation. `G_DISK_TABLE_REPLACE` explicitly replaces MBR/GPT metadata before MBR
initialization; ordinary table creation retains its non-destructive checks.

`diskBootGet` validates a primary MBR volume and its requested boot area under
the caller's disk claim. It returns the disk identity, partition range, claim and
generation to retain for subsequent writes. `diskBootWrite` submits up to 1024
bytes of whole sectors outside sector zero. `diskBootCommit` submits 440 bytes
of MBR boot code, preserving partition entries and optionally activating the
selected partition. Both return jobs that flush, verify and reject stale metadata;
they require every volume on the disk to be unmounted. These operations use
administrative protocol version 2 and accept bootloader-independent payloads.
