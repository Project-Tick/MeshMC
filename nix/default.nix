{
  lib,
  stdenv,
  cmake,
  ninja,
  kdePackages,
}:

stdenv.mkDerivation {
  pname = "katabasis";
  version = "1.0.0";

  src = ../.;

  nativeBuildInputs = [
    cmake
    ninja
  ];

  buildInputs = [
    kdePackages.qtbase
  ];

  dontWrapQtApps = true;

  meta = {
    description = "OAuth2 Device Authorization Grant flow implementation";
    license = lib.licenses.bsd2;
    platforms = lib.platforms.all;
  };
}
