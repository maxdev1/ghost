# Ghost shell

The interactive shell used by Terminal. It launches programs, connects pipelines,
and provides line editing and filename completion. Built-in commands include
`cd`, `clear`/`cls`, `which`, `bg`, `shutdown`, `reboot` and `exit`.

Enter commands at the terminal prompt:

```sh
cd /user
echo hello | write example.txt
read example.txt | reverse
which editor
```

Programs can be named directly or by path. `bg` starts a program without waiting
for it; `exit` leaves the shell. File output uses tools such as `write`.
