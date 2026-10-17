<p align="center">
  <img src="https://github.com/Project-Tick/MeshMC/raw/develop/launcher/packaging/linux/org.projecttick.MeshMC_256.png" alt="MeshMC logo"/>
</p>

MeshMC
======

MeshMC is a custom launcher for Minecraft that focuses on predictability, long term stability and simplicity.

## Contributing

Contributions are welcome from all. This repository is managed in [GitHub](https://github.com/Project-Tick/MeshMC).

* Want to contribute code? Fork this project and send PR.
* Reporting a bug? Please submit it on the [GitHub Issue](https://github.com/Project-Tick/MeshMC/issues).
* Is there a part of MeshMC that's not translated? See the [Crowdin](https://crowdin.com/project/projtlauncher) page.
* If you get stuck or need help with anything at all, please use [GitHub Issue](https://github.com/Project-Tick/MeshMC/issues) system.
* Do you want to write a plugin? The location is `launcher/plugin/plugins`. In-tree plugins are located here.

### Building

If you want to build the launcher yourself, check [this wiki page](https://projecttick.org/wiki/development/build-instructions) for build instructions.

#### Fedora

Fedora (44 or newer) packages everything the build needs, KDE Frameworks 6 included, except Java 17:
Fedora's own JDK is too new for the Java helpers, so it comes from Adoptium's repository.

```sh
sudo dnf install gcc-c++ cmake ninja-build git extra-cmake-modules \
  qt6-qtbase-devel qt6-qtbase-private-devel qt6-qtdeclarative-devel qt6-qtsvg-devel \
  qt6-qtnetworkauth-devel qt6-qtquick3d-devel qt6-qtshadertools-devel \
  qt6-qtimageformats qt6-qttools-devel \
  kf6-ki18n-devel kf6-kcoreaddons-devel kf6-kguiaddons-devel kf6-kirigami-devel \
  kf6-kiconthemes-devel kf6-breeze-icons-devel kf6-kcolorscheme-devel \
  kf6-qqc2-desktop-style kf6-kirigami-addons-devel plasma-breeze-qt6 \
  cmark-devel libarchive-devel zlib-ng-compat-devel gpgmepp-devel \
  scdoc gamemode-devel tomlplusplus-devel qrencode-devel
sudo dnf install adoptium-temurin-java-repository
sudo dnf config-manager setopt adoptium-temurin-java-repository.enabled=1
sudo dnf install temurin-17-jdk

export JAVA_HOME=/usr/lib/jvm/temurin-17-jdk
cmake --preset linux
cmake --build --preset linux --config Release
MESHMC_QML_UI=1 ./build/Release/meshmc   # the KDE interface
```

The build does not fail without `kf6-qqc2-desktop-style`, `kf6-kirigami-addons-devel` and `plasma-breeze-qt6`,
but the KDE interface needs them at run time (its controls style, its context menus and About page, and Breeze
off Plasma); a Plasma desktop already has them.

### Code formatting
Just follow the existing formatting.

In general, in order of importance:
* Make sure your IDE is not messing up line endings or whitespace and avoid using linters.
* Prefer readability over dogma.
* Keep to the existing formatting.
* Indent with 4 space unless it's in a submodule.
* Keep lists (of arguments, parameters, initializers...) as lists, not paragraphs. It should either read from top to bottom, or left to right. Not both.

## License

Copyright &copy; 2012-2022 MultiMC Contributors<br>
Copyright &copy; 2026 Project Tick

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this program except in compliance with the License. You may obtain a copy of the License at [http://www.apache.org/licenses/LICENSE-2.0](http://www.apache.org/licenses/LICENSE-2.0).

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.

## Forking/Redistribution

We keep __MeshMC__ open source because we believe it's important to be able to see the source code of a project like this, and we do this using the Apache license.

One reason we use the Apache license is that we don't want people using the name __MeshMC__ when they fork the project. This means people should examine the source code and remove all references to __MeshMC__, including the project icon and window titles (the title should not contain the phrase __MeshMC-fork__). The Apache license covers reasonable use of the name; mentioning the project's origins in the About dialog and license is acceptable. However, it must be explicitly stated that the project is a fork, which does not mean you have our approval.

However, we give you the freedom to distribute this project as you wish, in any non-exclusive way, without changing its functionality, on a voluntary basis to package managers, without expecting any financial gain. Take the project and distribute it wherever people can reach it. But abide by our restrictions.
