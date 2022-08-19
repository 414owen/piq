{...}:

let
  sources = import ./sources.nix {};
in

import ../default.nix {
  pkgs = import sources."nixpkgs-22.05" {
    overlays = [ (import ./overlays.nix) ];
  };
}
