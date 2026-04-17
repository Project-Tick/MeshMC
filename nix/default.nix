{
  lib,
  stdenv,
  cmake,
  ninja,
  kdePackages,
}:

stdenv.mkDerivation {
  pname = "rainbow";
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
    description = "Color/formatting utilities for Qt applications";
    license = lib.licenses.lgpl21Plus;
    platforms = lib.platforms.all;
  };
}
