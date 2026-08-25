{
  lib,
  stdenv,
  cmake,
  ninja,
  kdePackages,
}:

stdenv.mkDerivation {
  pname = "systeminfo";
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
    description = "Platform system information library";
    license = lib.licenses.lgpl21Plus;
    platforms = lib.platforms.all;
  };
}
