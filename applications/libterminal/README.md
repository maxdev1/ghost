# libterminal

Library for applications that interact with Ghost's terminal protocol. It
provides cursor movement, screen clearing, terminal dimensions, input modes and
extended key codes. Ordinary text output continues to use standard output.

```cpp
#include <libterminal/terminal.hpp>
#include <cstdio>

g_terminal::clear();
g_terminal::setCursor(g_term_cursor_position(0, 0));
printf("Hello from Ghost\n");
```

Interactive programs can use `setMode`, `setEcho` and `getChar` to control input
and receive keys such as arrows separately from ordinary characters.
