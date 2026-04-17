{
  lib,
  stdenv,
  cmake,
  ninja,
  kdePackages,
}:

stdenv.mkDerivation {
  pname = "iconfix";
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
    description = "XDG icon theme support and icon loader for Qt";
    license = lib.licenses.lgpl21Plus;
    platforms = lib.platforms.all;
  };
}
