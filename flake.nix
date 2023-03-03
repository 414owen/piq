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
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; overlays = [ (import ./nix/overlays.nix) ]; };
        lib = pkgs.lib;
        stdenv = pkgs.stdenv;
        packageName = "lang-c";

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

        app = {test ? false}: stdenv.mkDerivation rec {
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

          CFLAGS = "-Ofast -std=c99 -s -flto -DTIME_ALL";

          buildPhase = ''
            . ./scripts/environment.sh
            tup generate build.sh
            ./build.sh
          '';

          installPhase = ''
            mkdir -p $out/bin
            ${if test
              then "mv test $out/bin/test"
              else "mv main $out/bin/lang"}
          '';
        };

        noopInstall = ''
          mkdir $out
        '';
      in

      rec {
        packages = {
          ${packageName} = app {};
          "${packageName}-tests" = app {test = true;};
          "${packageName}-benchmark" = pkgs.stdenvNoCC.mkDerivation {
            name = "benchmark";
            unpackPhase = "true";
            buildPhase = ''
              ${packages."${packageName}-tests"}/bin/test bench -j
            '';
            installPhase = ''
              mkdir -p $out
              mv bench.json $out/bench.json
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

        devShell = pkgs.mkShell {
          buildInputs = with pkgs; lib.concatLists [
            (if stdenv.isLinux then [gdb cgdb] else [])
            [
              clang-tools
              clang-tools.clang
              bear
              lldb
              lcov
              valgrind
              watchexec
              commitizen
            ]
          ];

          inputsFrom = builtins.attrValues packages;

          CFLAGS = "-std=c99 -pedantic -O0 -ggdb -Wall -Wextra -DDEBUG -DTIME_ALL";
          CPATH = pkgs.lib.makeSearchPathOutput "dev" "include" packages.${packageName}.buildInputs;

          shellHook = ''
            source ${./scripts/environment.sh}
            tup compiledb
          '';
        };
      });
}
