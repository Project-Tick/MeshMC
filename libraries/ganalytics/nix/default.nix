{
  lib,
  stdenv,
  cmake,
  ninja,
  kdePackages,
  systeminfo,
}:

stdenv.mkDerivation {
  pname = "ganalytics";
  version = "1.0.0";

  src = ../.;

  nativeBuildInputs = [
    cmake
    ninja
  ];

  buildInputs = [
    kdePackages.qtbase
    systeminfo
  ];

  dontWrapQtApps = true;

  meta = {
    description = "Google Analytics wrapper library for Qt";
    license = lib.licenses.lgpl21Plus;
    platforms = lib.platforms.all;
  };
}
