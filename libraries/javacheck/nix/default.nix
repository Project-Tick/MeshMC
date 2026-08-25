{
  lib,
  stdenv,
  cmake,
  ninja,
  jdk17,
  stripJavaArchivesHook,
}:

stdenv.mkDerivation {
  pname = "javacheck";
  version = "1.0.0";

  src = ../.;

  nativeBuildInputs = [
    cmake
    ninja
    jdk17
    stripJavaArchivesHook
  ];

  meta = {
    description = "Java version detection utility";
    license = lib.licenses.gpl3Plus;
    platforms = lib.platforms.all;
  };
}
