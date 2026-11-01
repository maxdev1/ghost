# Kernel log viewer

A small diagnostic application installed as `tester.bin`. It opens a ƒenster
window that displays recent output from the kernel log pipe, keeping the latest
1,000 bytes and showing the retained byte count.

Launch it in Ghost while diagnosing kernel or driver activity:

```sh
/applications/tester.bin
```

The window updates as log messages arrive and closes through its normal window
close button.
