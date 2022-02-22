{ pkgs ? import <nixpkgs> { overlays = [ (import ./nix/overlays.nix) ]; } }:

let 
  unstable = import <unstable> {};
in

# pkgs.mkShell.override { stdenv = pkgs.pkgsCross.musl64.stdenv; } {
pkgs.mkShell {
  nativeBuildInputs = with pkgs; [
    gcc11
    unstable.cgdb
    re2c
    clang-tools
    lcov
    lemon
    valgrind
  ];
  buildInputs = with pkgs; [
    llvmPackages_13.libllvm
  ];
}
