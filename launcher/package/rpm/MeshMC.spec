Name:           MeshMC
Version:        1.4
Release:        3%{?dist}
Summary:        A local install wrapper for MeshMC

License:        ASL 2.0
URL:            https://projecttick.org
BuildArch:      x86_64

Requires:       zenity qt5-qtbase wget xrandr
Provides:       meshmc MeshMC

%description
A local install wrapper for MeshMC

%prep


%build

%install
mkdir -p %{buildroot}/opt/meshmc
install -m 0644 ../ubuntu/meshmc/opt/meshmc/icon.svg %{buildroot}/opt/meshmc/icon.svg
install -m 0755 ../ubuntu/meshmc/opt/meshmc/run.sh %{buildroot}/opt/meshmc/run.sh
mkdir -p %{buildroot}/%{_datadir}/applications
install -m 0644 ../ubuntu/meshmc/usr/share/applications/meshmc.desktop %{buildroot}/%{_datadir}/applications/meshmc.desktop
mkdir -p %{buildroot}/%{_datadir}/metainfo
install -m 0644 ../ubuntu/meshmc/usr/share/metainfo/meshmc.metainfo.xml %{buildroot}/%{_datadir}/metainfo/meshmc.metainfo.xml
mkdir -p %{buildroot}/%{_mandir}/man1
install -m 0644 ../ubuntu/meshmc/usr/share/man/man1/meshmc.1 %{buildroot}/%{_mandir}/man1/meshmc.1

%files
%dir /opt/meshmc
/opt/meshmc/icon.svg
/opt/meshmc/run.sh
%{_datadir}/applications/meshmc.desktop
%{_datadir}/metainfo/meshmc.metainfo.xml
%dir /usr/share/man/man1
%{_mandir}/man1/meshmc.1.gz

%changelog
* Sun Oct 03 2021 imperatorstorm <30777770+ImperatorStorm@users.noreply.github.com>
- added manpage

* Tue Jun 01 2021 kb1000 <fedora@kb1000.de> - 1.4-2
- Add xrandr to the dependencies

* Tue Dec 08 00:34:35 CET 2020 joshua-stone <joshua.gage.stone@gmail.com>
- Add metainfo.xml for improving package metadata

* Wed Nov 25 22:53:59 CET 2020 kb1000 <fedora@kb1000.de>
- Initial version of the RPM package, based on the Ubuntu package
