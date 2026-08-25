{
  lib,
  stdenv,
  cmake,
  ninja,
}:

stdenv.mkDerivation {
  pname = "optional-bare";
  version = "1.0.0";

  src = ../.;

  nativeBuildInputs = [
    cmake
    ninja
  ];

  meta = {
    description = "A single-header C++ optional type polyfill";
    license = lib.licenses.boost;
    platforms = lib.platforms.all;
  };
}
