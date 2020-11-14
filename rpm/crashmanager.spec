Name:           crashmanager
Version:        1.0
Release:        1%{?dist}
Summary:        Linux process crash manager

License:        GPL2+
URL:            https://gitlab.com/anpopa/crashmanager
Source:         %{url}/-/archive/master/crashmanager-master.zip

BuildRequires:  meson
BuildRequires:  gcc
BuildRequires:  libarchive-devel >= 3.4
BuildRequires:  glib2-devel >= 2.60
BuildRequires:  systemd-devel >= 243
BuildRequires:  libssh2-devel >= 1.9
BuildRequires:  sqlite-devel >= 3.30

Requires: sqlite >= 3.30
Requires: libssh2 >= 1.9
Requires: systemd >= 243
Requires: glib2 >= 2.60
Requires: libarchive >= 3.4


%description
%{summary}

%package devel
Summary:        Development tools for %{name}
Requires:       %{name}%{?_isa} = %{?epoch:%{epoch}:}%{version}-%{release}

%description devel
%{summary}.

%global _vpath_srcdir %{name}-master

%prep
%autosetup -c

%build
%meson
%meson_build

%install
%meson_install

%check
%meson_test

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%license LICENSE
%{_bindir}/crashhandler
%{_bindir}/crashmanager
%{_bindir}/crashinfo
%{_sysconfdir}/crashmanager.conf
%{_sysconfdir}/sysctl.d/51-crashhandler.conf
%{_sysconfdir}/systemd/system/crashmanager.service
%{_sysconfdir}/dbus-1/system.d/crashmanager-dbus.conf

%files devel
%{_bindir}/crashtest
%{_includedir}/crashmanager/cdh-epilog.h
%{_prefix}/%{_lib}/libCdhEpilog.a
%{_prefix}/%{_lib}/pkgconfig/libcdhepilog.pc
