{ pkgs ? import <nixpkgs> { overlays = [ (import ./nix/overlays.nix) ]; } }:

# pkgs.mkShell.override { stdenv = pkgs.pkgsCross.musl64.stdenv; } {
pkgs.stdenv.mkDerivation {
  name = "lang-c";
  nativeBuildInputs = with pkgs; [
    cgdb
    re2c
    clang-tools
    clang-tools.clang
    lcov
    lemon
    valgrind
    tup
    pkg-config
  ];

  buildInputs = with pkgs; [
    llvmPackages_13.libllvm
    readline81
  ];
}
