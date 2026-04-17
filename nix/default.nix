{
  lib,
  stdenv,
  cmake,
  ninja,
  jdk17,
  stripJavaArchivesHook,
}:

stdenv.mkDerivation {
  pname = "javalauncher";
  version = "1.0.0";

  src = ../.;

  nativeBuildInputs = [
    cmake
    ninja
    jdk17
    stripJavaArchivesHook
  ];

  meta = {
    description = "Java launcher for MeshMC";
    license = lib.licenses.gpl3Plus;
    platforms = lib.platforms.all;
  };
}
