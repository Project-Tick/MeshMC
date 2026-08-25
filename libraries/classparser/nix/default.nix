{
  lib,
  stdenv,
  cmake,
  ninja,
  kdePackages,
  libarchive,
}:

stdenv.mkDerivation {
  pname = "classparser";
  version = "1.0.0";

  src = ../.;

  nativeBuildInputs = [
    cmake
    ninja
  ];

  buildInputs = [
    kdePackages.qtbase
    libarchive
  ];

  dontWrapQtApps = true;

  meta = {
    description = "Java class file parser library";
    license = lib.licenses.lgpl21Plus;
    platforms = lib.platforms.all;
  };
}
