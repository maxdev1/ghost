# Threads & processes
A thread follows a single stream of execution. Threads always run in
user space, there are no threads running in kernel space.

Each thread is assigned to a process. A process has a main thread. Once
the main thread dies, all threads of the process exit, too.

Each thread has a unique id that is obtainable from within the thread
with the `g_get_tid()` system call. The *process id* is equal to the
*thread id* of the process main thread.

## Security level
A process has a security level. All threads of the process are having
the same security privileges. This security levels are:

`KERNEL`: is only assigned to the initial system processes like the
spawner binary, as they need special rights

`DRIVER`: may do low-level system I/O and use a variety of driver-
related system calls

`APPLICATION`: has normal process privileges