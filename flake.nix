{
  description = "Lang - A programming language";

  inputs = {
    nixpkgs.url = "github:414owen/nixpkgs/os/lang-c-env-22.11";
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

        app = {files ? ["piq"]}: stdenv.mkDerivation rec {
          name = packageName;

          inherit src;

          nativeBuildInputs = with pkgs; [
            re2c
            lemon
            pkg-config
            editline
            tup
          ];

          buildInputs = with pkgs; [
            llvmPackages_14.libllvm
            hedley
            predef
            xxHash
          ];

          CFLAGS = "-s -DTIME_ALL";

          buildPhase = ''
            . ./scripts/environment.sh
            tup generate build.sh
            ./build.sh
          '';

          installPhase = ''
            mkdir -p $out/bin
            ${str.concatMapStrings (f: "mv ${f} $out/bin/${f}\n") files}
          '';
        };

        noopInstall = ''
          mkdir $out
        '';
      in

      rec {
        packages = {
          ${packageName} = app {};
          "${packageName}-tests" = app {files = ["test"];};
          "${packageName}-all" = app {files = ["piq" "test"];};
          "${packageName}-metrics" = pkgs.stdenvNoCC.mkDerivation {
            name = "metrics";
            unpackPhase = "true";
            buildInputs = [pkgs.jq];
            buildPhase = ''
              builddir=$PWD
              ${packages."${packageName}-all"}/bin/test bench -j
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
              ${packages."${packageName}-tests"}/bin/test --lite
            '';
            installPhase = noopInstall;
          };

          "valgrind" = pkgs.stdenvNoCC.mkDerivation {
            name = "formatting";
            buildInputs = with pkgs; [
              valgrind
            ];
            unpackPhase = "true";
            buildPhase = ''
              valgrind --leak-check=full --error-exitcode=1 \
                ${packages."${packageName}-tests"}/bin/test --lite
            '';
            installPhase = noopInstall;
          };
        };

        defaultPackage = packages.${packageName};

        devShell = (pkgs.mkShell.override { stdenv = stdenv; }) {
          buildInputs = with pkgs; lib.concatLists [
            (if stdenv.isLinux then [gdb cgdb] else [])
            (if system != "armv6l-linux" then [
              clang-tools
              clang-tools.clang
              bear
              lldb
              lcov
              valgrind
              watchexec
              commitizen
              flamegraph
            ] else [])
          ];

          inputsFrom = builtins.attrValues packages;

          CFLAGS = "-std=c99 -pedantic -O0 -ggdb -Wall -Wextra -DTIME_ALL";
          CPATH = pkgs.lib.makeSearchPathOutput "dev" "include" packages.${packageName}.buildInputs;

          shellHook = ''
            source ${./scripts/environment.sh}
            tup compiledb
          '';
        };
      });
}
