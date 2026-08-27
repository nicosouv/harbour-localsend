Name:       harbour-localsend
Summary:    Send files over your local network
Version:    0.1.0
Release:    1
Group:      Applications/Internet
License:    MIT
URL:        https://github.com/nicosouv/harbour-localsend
Source0:    %{name}-%{version}.tar.bz2

Requires:   sailfishsilica-qt5 >= 0.10.9
Requires:   nemo-qml-plugin-notifications-qt5

BuildRequires:  pkgconfig(sailfishapp) >= 1.0.2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5Network)
# Qt cannot generate an X.509 certificate, and each device is its own
# authority, so the TLS identity is built against OpenSSL directly.
BuildRequires:  pkgconfig(openssl)
BuildRequires:  desktop-file-utils

%description
An unofficial LocalSend client for Sailfish OS: send and receive files with
any phone or computer on the same network, with no account, no cloud and no
Internet connection involved.

Speaks the LocalSend v2 protocol, so it works with the official apps on
Android, iOS, Windows, macOS and Linux.

Features:
- Finds nearby devices over multicast, with a manual sweep for networks
  that block it
- Send anything: photos, documents, archives, several files at once
- Encrypted by default, with peers authenticated by certificate pinning
- Incoming transfers are shown before anything is written to disk
- Optional PIN, optional quick save, per-sender folders
- Native Sailfish interface with Silica components

%prep
%setup -q -n %{name}-%{version}

%build
%qmake5 "VERSION=%{version}"

make %{?_smp_mflags}

%install
rm -rf %{buildroot}
%qmake5_install

desktop-file-install --delete-original       \
  --dir %{buildroot}%{_datadir}/applications             \
   %{buildroot}%{_datadir}/applications/*.desktop

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
