{ pkgs ? import <nixpkgs> { overlays = [ (import ./nix/overlays.nix) ]; } }:

(pkgs.mkShell.override { stdenv = pkgs.gcc11.stdenv; }) {
  nativeBuildInputs = with pkgs; [
    gcc11
    re2c
    clang-tools
    lemon
  ];
}
