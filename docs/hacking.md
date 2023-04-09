# Hacking

First, install [nix](https://nixos.org/). Then:

```sh
$ nix develop # enter the development environment
$ tup init    # first time only
$ tup build   # build the compiler
```

Optional extras:

```sh
$ git config blame.ignoreRevsFile .blameignore # ignore some useless commits
$ tup compiledb                                # get clangd working properly
```

Alternatively, you can mess around with the debian-based build instructions in
[guide.md](docs/guide.md). Tup isn't up-to-date enough on debian to generate a
compiledb, so clangd will probably malfunction.
