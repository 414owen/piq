{ pkgs ? import <nixpkgs> { overlays = [ (import ./nix/overlays.nix) ]; } }:

# pkgs.mkShell.override { stdenv = pkgs.pkgsCross.musl64.stdenv; } {
pkgs.mkShell {
  nativeBuildInputs = with pkgs; [
    gcc11
    cgdb
    re2c
    clang-tools
    lcov
    lemon
    valgrind
  ];
}
