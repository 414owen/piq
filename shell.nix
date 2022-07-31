{ pkgs ? import <nixpkgs> { overlays = [ (import ./nix/overlays.nix) ]; } }:

# pkgs.mkShell.override { stdenv = pkgs.pkgsCross.musl64.stdenv; } {
pkgs.mkShell {
  nativeBuildInputs = with pkgs; [
    cgdb
    re2c
    clang-tools
    clang-tools.clang
    lcov
    lemon
    valgrind
  ];

  buildInputs = with pkgs; [
    llvmPackages_13.libllvm
    # readline
  ];

}
