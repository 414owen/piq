{ lib, stdenv, fetchurl }:

let

  srcs = {
    lemon = fetchurl {
      sha256 = "sha256-aWu2pys14re0Yby6uFfKm0cDhKEzf0i6rlBnvHy8wvw=";
      url = "https://www.sqlite.org/src/raw/1c5a14f6044193e42864c36de48359026fa2cdcf205a43cc1a31116101e27258?at=lemon.c";
      name = "lemon.c";
    };
    lempar = fetchurl {
      sha256 = "sha256-TP5CW1E4ld0Rr3083pY5IKLa639XMfcG5pc7/HnyB+0=";
      url = "https://www.sqlite.org/src/raw/57478ea48420da05faa873c6d1616321caa5464644588c97fbe8e0ea04450748?at=lempar.c";
      name = "lempar.c";
    };
  };

in stdenv.mkDerivation {
  pname = "lemon";
  version = "3.37.2"; # sqlite version

  dontUnpack = true;

  buildPhase = ''
    sh -xc "$CC ${srcs.lemon} -o lemon"
  '';

  installPhase = ''
    install -Dvm755 lemon $out/bin/lemon
    install -Dvm644 ${srcs.lempar} $out/bin/lempar.c
  '';

  meta = with lib; {
    description = "An LALR(1) parser generator";
    longDescription = ''
      The Lemon program is an LALR(1) parser generator that takes a
      context-free grammar and converts it into a subroutine that will parse a
      file using that grammar. Lemon is similar to the much more famous
      programs "yacc" and "bison", but is not compatible with either.
    '';
    homepage = "http://www.hwaci.com/sw/lemon/";
    license = licenses.publicDomain;
    platforms = platforms.unix;
  };
}
