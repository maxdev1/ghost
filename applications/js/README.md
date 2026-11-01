# JavaScript console

Runs JavaScript with the embedded Duktape engine. Without arguments it opens an
interactive console; a file argument evaluates that script. `console.log` writes
to the terminal.

Run an existing script from a Ghost terminal:

```sh
/applications/js.bin /user/example.js
```

For interactive use, run `js` and enter statements such as:

```js
console.log(1 + 2);
```

A line ending in `;` evaluates immediately. Otherwise the console accepts more
lines until an empty line is entered.
