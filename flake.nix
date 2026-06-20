{
  description = " Project Tick is a project dedicated to providing developers with ease of use and users with long-lasting software.";

  nixConfig = {
    extra-substituters = [ "https://meshmc.cachix.org" ];
    extra-trusted-public-keys = [
      "meshmc.cachix.org-1:6ZNLcfqjVDKmN9/XNWGV3kcjBTL51v1v2V+cvanMkZA="
    ];
  };

  inputs = {
    nixpkgs.url = "https://channels.nixos.org/nixos-unstable/nixexprs.tar.xz";

    classparser = {
      url = "git+https://gitlab.com/project-tick/projects/classparser";
      flake = false;
    };
    project-tick-cmark = {
      url = "git+https://gitlab.com/project-tick/projects/cmark";
      flake = false;
    };
    ganalytics = {
      url = "git+https://gitlab.com/project-tick/projects/ganalytics";
      flake = false;
    };
    genqrcode = {
      url = "git+https://gitlab.com/project-tick/projects/genqrcode";
      flake = false;
    };
    iconfix = {
      url = "git+https://gitlab.com/project-tick/projects/iconfix";
      flake = false;
    };
    javacheck = {
      url = "git+https://gitlab.com/project-tick/projects/javacheck";
      flake = false;
    };
    javalauncher = {
      url = "git+https://gitlab.com/project-tick/projects/javalauncher";
      flake = false;
    };
    katabasis = {
      url = "git+https://gitlab.com/project-tick/projects/katabasis";
      flake = false;
    };
    libnbtplusplus = {
      url = "git+https://gitlab.com/project-tick/projects/libnbtplusplus";
      flake = false;
    };
    localpeer = {
      url = "git+https://gitlab.com/project-tick/projects/localpeer";
      flake = false;
    };
    neozip = {
      url = "git+https://gitlab.com/project-tick/projects/neozip";
      flake = false;
    };
    optional-bare = {
      url = "git+https://gitlab.com/project-tick/projects/optional-bare";
      flake = false;
    };
    rainbow = {
      url = "git+https://gitlab.com/project-tick/projects/rainbow";
      flake = false;
    };
    systeminfo = {
      url = "git+https://gitlab.com/project-tick/projects/systeminfo";
      flake = false;
    };
    project-tick-tomlplusplus = {
      url = "git+https://gitlab.com/project-tick/projects/tomlplusplus";
      flake = false;
    };
    xz-embedded = {
      url = "git+https://gitlab.com/project-tick/projects/xz-embedded";
      flake = false;
    };
  };

  outputs =
    {
      self,
      nixpkgs,
      classparser,
      project-tick-cmark,
      ganalytics,
      genqrcode,
      iconfix,
      javacheck,
      javalauncher,
      katabasis,
      libnbtplusplus,
      localpeer,
      neozip,
      optional-bare,
      rainbow,
      systeminfo,
      project-tick-tomlplusplus,
      xz-embedded
    }:

    let
      inherit (nixpkgs) lib;

      # While we only officially support aarch and x86_64 on Linux and MacOS,
      # we expose a reasonable amount of other systems for users who want to
      # build for most exotic platforms
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "aarch64-darwin"
      ];

      forAllSystems = lib.genAttrs systems;
      nixpkgsFor = forAllSystems (system: nixpkgs.legacyPackages.${system});
    in

    {
      overlays.default =
        final: prev:

        let
          llvm = final.llvmPackages_22 or prev.llvmPackages_22;
          mkTickPkg = name: src: final.callPackage "${src}/nix/default.nix" { 
            inherit (llvm) stdenv; 
          };
        in

        {

          classparser = mkTickPkg "classparser" classparser;
          project-tick-cmark = mkTickPkg "cmark" project-tick-cmark;
          ganalytics = mkTickPkg "ganalytics" ganalytics;
          genqrcode = mkTickPkg "genqrcode" genqrcode;
          iconfix = mkTickPkg "iconfix" iconfix;
          javacheck = mkTickPkg "javacheck" javacheck;
          javalauncher = mkTickPkg "javalauncher" javalauncher;
          katabasis = mkTickPkg "katabasis" katabasis;
          libnbtplusplus = mkTickPkg "libnbtplusplus" libnbtplusplus;
          localpeer = mkTickPkg "localpeer" localpeer;
          neozip = mkTickPkg "neozip" neozip;
          optional-bare = mkTickPkg "optional-bare" optional-bare;
          rainbow = mkTickPkg "rainbow" rainbow;
          systeminfo = mkTickPkg "systeminfo" systeminfo;
          project-tick-tomlplusplus = mkTickPkg "tomlplusplus" project-tick-tomlplusplus;
          xz-embedded = mkTickPkg "xz-embedded" xz-embedded;

          # ── MeshMC ────────────────────────────────────────────────────
          meshmc-unwrapped = prev.callPackage ./nix/unwrapped.nix {
            inherit (llvm) stdenv;
            inherit self;
            inherit (final)
              project-tick-cmark
              project-tick-tomlplusplus
              neozip
              libnbtplusplus
              systeminfo
              ganalytics
              rainbow
              iconfix
              localpeer
              classparser
              optional-bare
              xz-embedded
              katabasis
              javacheck
              javalauncher
              ;
          };

          meshmc = final.callPackage ./nix/wrapper.nix { };
        };

      packages = forAllSystems (
        system:

        let
          pkgs = nixpkgsFor.${system};

          # Build a scope from our overlay
          tickPackages = lib.makeScope pkgs.newScope (final: self.overlays.default final pkgs);

          packages = {
            # Internal libraries
            inherit (tickPackages)
              project-tick-tomlplusplus
              optional-bare
              xz-embedded
              project-tick-cmark
              neozip
              genqrcode
              rainbow
              iconfix
              localpeer
              katabasis
              systeminfo
              classparser
              libnbtplusplus
              ganalytics
              javacheck
              javalauncher
              ;

            # MeshMC
            inherit (tickPackages) meshmc-unwrapped meshmc;
            default = tickPackages.meshmc;
          };
        in

        # Only output them if they're available on the current system
        lib.filterAttrs (_: lib.meta.availableOn pkgs.stdenv.hostPlatform) packages
      );

      # We put these under legacyPackages as they are meant for CI, not end user consumption
      legacyPackages = forAllSystems (
        system:

        let
          packages' = self.packages.${system};
        in

        rec {
          meshmc-unwrapped-debug = packages'.meshmc-unwrapped.overrideAttrs {
            cmakeBuildType = "Debug";
            dontStrip = true;
          };

          meshmc-debug = packages'.meshmc.override {
            meshmc-unwrapped = meshmc-unwrapped-debug;
          };
        }
      );

      devShells = forAllSystems (
        system:

        let
          pkgs = nixpkgsFor.${system};
          llvm = pkgs.llvmPackages_22;
          python = pkgs.python3;
          mkShell = pkgs.mkShell.override { inherit (llvm) stdenv; };

          packages' = self.packages.${system};

          welcomeMessage = ''
            Welcome to Project Tick!
          '';
        in

        {
          default = mkShell {
            name = "project-tick";

            packages = [

              (pkgs.stdenvNoCC.mkDerivation {
                pname = "clang-tidy-diff";
                inherit (llvm.clang) version;

                nativeBuildInputs = [
                  pkgs.installShellFiles
                  python.pkgs.wrapPython
                ];

                dontUnpack = true;
                dontConfigure = true;
                dontBuild = true;

                postInstall = "installBin ${llvm.libclang.python}/share/clang/clang-tidy-diff.py";
                postFixup = "wrapPythonPrograms";
              })
            ];

            shellHook = ''
              git submodule update --init --force

              echo ${lib.escapeShellArg welcomeMessage}
            '';
          };
        }
      );

      checks = forAllSystems (
        system:

        let
          pkgs = nixpkgsFor.${system};
          llvm = pkgs.llvmPackages_22;
        in

        {
          clang-format = pkgs.runCommand "meshmc-clang-format-check"
            {
              nativeBuildInputs = [ llvm.clang-tools pkgs.bash pkgs.ripgrep ];
              src = self;
            }
            ''
              cd "$src"
              bash ./scripts/check-clang-format.sh
              touch "$out"
            '';
        }
      );

      formatter = forAllSystems (system: nixpkgsFor.${system}.nixfmt-rfc-style);
    };
}
