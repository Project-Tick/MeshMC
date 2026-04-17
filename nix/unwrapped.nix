{
  lib,
  stdenv,
  cmake,
  extra-cmake-modules,
  gamemode,
  jdk17,
  kdePackages,
  ninja,
  qrencode,
  self,
  stripJavaArchivesHook,
  msaClientID ? null,
  libarchive,
  project-tick-cmark,
  project-tick-tomlplusplus,
  neozip,
  libnbtplusplus,
  systeminfo,
  ganalytics,
  rainbow,
  iconfix,
  LocalPeer,
  classparser,
  optional-bare,
  xz-embedded,
  katabasis,
  javacheck,
  javalauncher,
}:

let
  date =
    let
      # YYYYMMDD
      date' = lib.substring 0 8 self.lastModifiedDate;
      year = lib.substring 0 4 date';
      month = lib.substring 4 2 date';
      date = lib.substring 6 2 date';
    in
    if (self ? "lastModifiedDate") then
      lib.concatStringsSep "-" [
        year
        month
        date
      ]
    else
      "unknown";
in

stdenv.mkDerivation {
  pname = "meshmc-unwrapped";
  version = "7.3-unstable-${date}";

  src = lib.fileset.toSource {
    root = ../.;
    fileset = lib.fileset.unions [
      ../branding
      ../buildconfig
      ../cmake
      ../crashreporter
      ../launcher
      ../nix
      ../plugins
      ../scripts
      ../updater
      ../.clang-format
      ../.clang-tidy
      ../.markdownlint.yaml
      ../.markdownlintignore
      ../CMakeLists.txt
      ../CMakePresets.json
      ../Containerfile
      ../COPYING.md
      ../README.md
      ../REUSE.toml
    ];
  };

  nativeBuildInputs = [
    cmake
    ninja
    extra-cmake-modules
    jdk17
    stripJavaArchivesHook
  ];

  buildInputs = [
    project-tick-cmark
    kdePackages.qtbase
    kdePackages.qtnetworkauth
    qrencode
    libarchive
    project-tick-tomlplusplus
    neozip
    libnbtplusplus
    systeminfo
    ganalytics
    rainbow
    iconfix
    LocalPeer
    classparser
    optional-bare
    xz-embedded
    katabasis
    javacheck
    javalauncher
  ]
  ++ lib.optional stdenv.hostPlatform.isLinux gamemode;

  cmakeFlags = [
    # downstream branding
    (lib.cmakeFeature "MeshMC_BUILD_PLATFORM" "nixpkgs")
    (lib.cmakeBool "MeshMC_PLUGINS" true)
    (lib.cmakeBool "MeshMC_STAGING_PLUGINS" true)
  ]
  ++ lib.optionals (msaClientID != null) [
    (lib.cmakeFeature "MeshMC_MSA_CLIENT_ID" (toString msaClientID))
  ]
  ++ lib.optionals stdenv.hostPlatform.isDarwin [
    # we wrap our binary manually
    (lib.cmakeFeature "INSTALL_BUNDLE" "nodeps")
    # disable built-in updater
    (lib.cmakeFeature "MACOSX_SPARKLE_UPDATE_FEED_URL" "''")
    (lib.cmakeFeature "CMAKE_INSTALL_PREFIX" "${placeholder "out"}/Applications/")
  ];

  doCheck = true;

  dontWrapQtApps = true;

  meta = {
    description = "Free, open source launcher for Minecraft";
    longDescription = ''
      Allows you to have multiple, separate instances of Minecraft (each with
      their own mods, texture packs, saves, etc) and helps you manage them and
      their associated options with a simple interface.
    '';
    homepage = "https://projecttick.org/";
    license = lib.licenses.gpl3Plus;
    maintainers = with lib.maintainers; [
      Scrumplex
      getchoo
    ];
    mainProgram = "meshmc";
    platforms = lib.platforms.linux ++ lib.platforms.darwin;
  };
}
