# Usage Guide

## [`nix`](https://nixos.org/)

```sh
$ nix build
```

### Running

```sh
$ ./result/piq compile -i examples/snd.scm -o a.out
```

## debian/ubuntu

This is what's done in CI. It involves putting some headers in /usr/include.

For this reason, I recommend just installing nix, and using the instructions
above instead.

```sh
$ sudo apt install -y tup re2c lemon libxxhash-dev llvm-dev valgrind gcc g++ pkg-config libreadline-dev subversion
$ sed -i 's/\^j\^//g' Tupfile # downgrade tupfile syntax
$ pushd /usr/include

# install hedley globally
$ [ -f hedley.h ] || sudo wget https://raw.githubusercontent.com/nemequ/hedley/master/hedley.h -O ./hedley.h

# install predef globally
$ [ -d predef ] || sudo svn checkout https://github.com/natefoo/predef/trunk/predef

$ cmake -B build -DCMAKE_BUILD_TYPE=Release # configure phase
$ cmake --build build/                      # build phase
```

### Running

```sh
$ ./build/piq compile -i examples/snd.scm -o a.out
```
