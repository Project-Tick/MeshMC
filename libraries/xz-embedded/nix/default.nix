{
  lib,
  stdenv,
  cmake,
  ninja,
}:

stdenv.mkDerivation {
  pname = "xz-embedded";
  version = "1.0.0";

  src = ../.;

  nativeBuildInputs = [
    cmake
    ninja
  ];

  cmakeFlags = [
    "-DXZ_BUILD_BCJ=OFF"
    "-DXZ_BUILD_CRC64=ON"
    "-DXZ_BUILD_MINIDEC=OFF"
  ];

  meta = {
    description = "Lightweight XZ decompressor for embedded systems";
    license = lib.licenses.publicDomain;
    platforms = lib.platforms.all;
  };
}
