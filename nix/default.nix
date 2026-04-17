{
  lib,
  stdenv,
  cmake,
  ninja,
  neozip,
}:

stdenv.mkDerivation {
  pname = "libnbtplusplus";
  version = "3.1";

  src = ../.;

  nativeBuildInputs = [
    cmake
    ninja
  ];

  buildInputs = [
    neozip
  ];

  cmakeFlags = [
    "-DNBT_BUILD_SHARED=ON"
    "-DNBT_USE_ZLIB=ON"
    "-DNBT_BUILD_TESTS=OFF"
  ];

  meta = {
    description = "C++ library for reading and writing NBT files";
    license = lib.licenses.asl20;
    platforms = lib.platforms.all;
  };
}
