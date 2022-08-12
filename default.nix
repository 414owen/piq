{ pkgs ? import <nixpkgs> { overlays = [ (import ./nix/overlays.nix) ]; } }:

let
  sources = import ./nix/sources.nix;
  hedley = pkgs.stdenv.mkDerivation {
    name = "hedley";
    version = "v15";
    src = sources.hedley;
    dontBuild = true;
    installPhase = ''
      mkdir -p $out/include
      mv hedley.h $out/include
    '';
  };
in

# pkgs.mkShell.override { stdenv = pkgs.pkgsCross.musl64.stdenv; } {
pkgs.stdenv.mkDerivation rec {
  name = "lang-c";

  src = pkgs.nix-gitignore.gitignoreSource [] ./.;

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
    hedley
  ];

  CFLAGS = "-O2 -s";
  CPATH = pkgs.lib.makeSearchPathOutput "dev" "include" buildInputs;

  installPhase = ''
    mkdir -p $out/bin
    mv repl $out/bin/lang
  '';
}
