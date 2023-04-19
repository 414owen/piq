# Hacking

First, install [nix](https://nixos.org/). Then:

```sh
$ nix develop                             # enter the development environment
$ cmake -B build -DCMAKE_BUILD_TYPE=Debug # configure
$ cmake --build build                     # build
$ ./build/piq                             # run
```

Optional extras:

```sh
$ git config blame.ignoreRevsFile .blameignore # ignore some useless commits
```

Alternatively, you can mess around with the debian-based build instructions in
[guide.md](docs/guide.md)
