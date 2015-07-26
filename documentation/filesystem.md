# Filesystem
The virtual filesystem of Ghost consists of *nodes*. A node is a structure
that contains information about the file. Each node has a `g_fs_node_type`.

* a **mountpoint** is the root entry of an actual filesystem. In the kernel
  each mountpoint must have a *delegate*. A delegate handles the different
  actions that a filesystem must be able to handle (or fail appropriately,
  setting the correct status codes)

## Custom drivers
This section describes how to create a custom driver for a filesystem
device.

To tell the kernel that the currently running thread should be used as the
delegate for a specific device, use `g_fs_register_as_delegate`.

Calling this function attempts to create a mountpoint with the given name
and appends a delegate that redirects requests to this thread as the
mountpoints driver delegate.

After registering itself, the driver must listen for messages addressed to
the registered thread using `g_recv_msg`. The `type` field of these messages
is set to one of the `g_fs_tasked_delegate_request_type` codes.

See the `applications/examplefsdriver/` for an example implementation.

## Transactions
As there are no kernel threads, all filesystem actions are implemented with
transactions. Once an action is requested, a new transaction id is assigned.
The filesystem delegate then does the requested work and sets the status of
the transaction accordingly. The *requesting* task wakes up once the
transaction status is set to finished.
