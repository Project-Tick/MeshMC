{
  lib,
  stdenv,
  cmake,
  ninja,
  kdePackages,
}:

stdenv.mkDerivation {
  pname = "LocalPeer";
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
    description = "Single-instance application support via IPC/locks";
    license = lib.licenses.lgpl21Plus;
    platforms = lib.platforms.all;
  };
}
