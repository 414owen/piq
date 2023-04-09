# Hacking

First, install [nix](https://nixos.org/).

```sh
$ nix develop   # enter the development environment
$ tup init      # first time only
$ tup compiledb # get clangd working
$ tup build     # build the compiler
```

Alternatively, you can mess around with the debian-based build instructions in
[guide.md](docs/guide.md). Tup isn't up-to-date enough on debian to generate a
compiledb, so clangd will probably malfunction.
