{
  description = "Lang - A programming language";

  inputs = {
    # nixpkgs.url = "github:NixOS/nixos-22.11";
    flake-utils.url = "github:numtide/flake-utils";
    hedley-src = {
      url = "github:nemequ/hedley/v15";
      flake = false;
    };
    predef-src = {
      url = "github:natefoo/predef";
      flake = false;
    };
  };

  outputs =
    { self
    , nixpkgs
    , hedley-src
    , predef-src
    , flake-utils
    }:

    let
      systems = flake-utils.lib.defaultSystems ++ [flake-utils.lib.system.armv6l-linux];
    in

    flake-utils.lib.eachSystem systems (system:
      let
        pkgs = import nixpkgs { inherit system; overlays = [ (import ./nix/overlays.nix) ]; };
        lib = pkgs.lib;
        str = lib.strings;
        # stdenv = pkgs.clangStdenv;
        stdenv = pkgs.stdenv;
        packageName = "piq";
        isArm6 = system == "armv6l-linux";
        valgrindSupported =
          let meta = pkgs.valgrind.meta;
          in builtins.elem system meta.platforms
          && !meta.broken;

        hedley = stdenv.mkDerivation {
          name = "hedley";
          version = "v15";
          src = hedley-src;
          dontBuild = true;
          installPhase = ''
            mkdir -p $out/include
            mv hedley.h $out/include
          '';
        };

        predef = stdenv.mkDerivation {
          name = "predef";
          version = "1.0.0";
          src = predef-src;
          dontBuild = true;
          installPhase = ''
            mkdir -p $out/include
            mv predef $out/include
          '';
        };

        src = pkgs.nix-gitignore.gitignoreSource [] ./.;

        app = {installTests ? false}: stdenv.mkDerivation rec {
          name = packageName;

          inherit src;

          nativeBuildInputs = with pkgs; [
            # breakpointHook
            # gnused
            cmake
            re2c
            lemon
            readline
          ];

          cmakeFlags = if installTests then [
            "-DINSTALL_TEST=true"
          ] else [];

          buildInputs = with pkgs; [
            llvmPackages_14.libllvm
            hedley
            predef
          ];
        };

        noopInstall = ''
          mkdir $out
        '';
      in

      rec {
        packages = {
          ${packageName} = app {};
          "${packageName}-all" = app {installTests = true;};
          "${packageName}-metrics" = pkgs.stdenvNoCC.mkDerivation {
            name = "metrics";
            unpackPhase = "true";
            buildInputs = [pkgs.jq];
            buildPhase = ''
              builddir=$PWD
              ${packages."${packageName}-all"}/bin/test-exe bench -j
              pushd ${packages."${packageName}-all"}/bin
              ${./scripts/collect-size-metrics.sh} > $builddir/sizes.json
              popd
              ${./scripts/merge-metrics.sh} sizes.json bench.json > metrics.json
            '';
            installPhase = ''
              mkdir -p $out
              mv metrics.json $out/metrics.json
            '';
          };
        };

        checks = {
          "formatting" = pkgs.stdenvNoCC.mkDerivation {
            name = "formatting";
            inherit src;
            buildInputs = with pkgs; [
              clang-tools
            ];
            buildPhase = ''
              ${pkgs.bash}/bin/bash ${packages.${packageName}.src}/scripts/checks/formatting.sh
            '';
            installPhase = noopInstall;
          };

          "header-pragmas" = pkgs.stdenvNoCC.mkDerivation {
            name = "formatting";
            inherit src;
            buildPhase = ''
              ${pkgs.bash}/bin/bash ${packages.${packageName}.src}/scripts/checks/headers-have-pragmas.sh
            '';
            installPhase = noopInstall;
          };

          "licences" = pkgs.stdenvNoCC.mkDerivation {
            name = "licenses";
            inherit src;
            buildPhase = ''
              ${pkgs.bash}/bin/bash ${packages.${packageName}.src}/scripts/checks/licences.sh
            '';
            installPhase = noopInstall;
          };

          "tests" = pkgs.stdenvNoCC.mkDerivation {
            name = "formatting";
            unpackPhase = "true";
            buildPhase = ''
              ${packages."${packageName}-all"}/bin/test-exe --lite
            '';
            installPhase = noopInstall;
          };
        } // (if valgrindSupported then {
          "valgrind" = pkgs.stdenvNoCC.mkDerivation {
            name = "formatting";
            buildInputs = with pkgs; [
              valgrind
            ];
            unpackPhase = "true";
            buildPhase = ''
              valgrind --leak-check=full --error-exitcode=1 \
                ${packages."${packageName}-all"}/bin/test-exe --lite
            '';
            installPhase = noopInstall;
          };
        } else {});

        defaultPackage = packages.${packageName};

        devShell = (pkgs.mkShell.override { stdenv = stdenv; }) {
          buildInputs = with pkgs; lib.concatLists [
            (if stdenv.isLinux then [gdb cgdb linuxPackages.perf] else [])
            (if valgrindSupported then [ valgrind ] else [])
            (if isArm6 then [ ] else [ commitizen ])
            [
              clang-tools
              clang-tools.clang
              ninja
              watchexec
              flamegraph
            ]
          ];

          inputsFrom = builtins.attrValues packages;
          CFLAGS = "-DTIME_ALL -Werror";
        };
      });
}
