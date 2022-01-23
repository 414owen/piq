{ pkgs ? import <nixpkgs> { overlays = [ (import ./nix/overlays.nix) ]; } }:

pkgs.mkShell {
  nativeBuildInputs = with pkgs; [
    re2c
    clang-tools
    lemon
  ];
}
