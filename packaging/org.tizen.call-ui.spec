Name:       org.tizen.call-ui
Summary:    Call UI Application
Version:    0.0.1
Release:    1
License:    Apache-2.0
Group:      Applications/Telephony
Source0:    %{name}-%{version}.tar.gz

%if "%{?tizen_profile_name}" == "wearable" || "%{?tizen_profile_name}" == "tv"
ExcludeArch: %{arm} %ix86 x86_64
%endif

BuildRequires:  pkgconfig(appcore-efl)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(capi-appfw-application)
BuildRequires:  pkgconfig(appsvc)
BuildRequires:	pkgconfig(callmgr_client)
BuildRequires:  pkgconfig(msg-service)
BuildRequires:  pkgconfig(efl-extension)
BuildRequires:  cmake
BuildRequires:  gettext-tools
BuildRequires:  edje-tools
BuildRequires:  pkgconfig(capi-network-bluetooth)
BuildRequires:  pkgconfig(capi-network-connection)
BuildRequires:  pkgconfig(capi-system-system-settings)
BuildRequires:  pkgconfig(minicontrol-provider)
BuildRequires:  pkgconfig(contacts-service2)
BuildRequires:  pkgconfig(capi-system-device)
BuildRequires: 	pkgconfig(notification)
BuildRequires: 	pkgconfig(capi-system-runtime-info)
BuildRequires:  pkgconfig(capi-system-info)
BuildRequires: 	pkgconfig(capi-system-sensor)
BuildRequires: 	pkgconfig(capi-ui-efl-util)

%description
Call UI application.

%prep
%setup -q

%build
export CFLAGS="${CFLAGS} -fvisibility=hidden"
export CXXFLAGS="${CXXFLAGS} -fvisibility-inlines-hidden -fvisibility=hidden"
export FFLAGS="${FFLAGS} -fvisibility-inlines-hidden -fvisibility=hidden"
%define PREFIX	/usr
%define APPDIR	%{PREFIX}/apps
%define	PKGDIR	%{APPDIR}/%{name}
%define BINDIR	%{PKGDIR}/bin
%define RESDIR	%{PKGDIR}/res

cmake . -DCMAKE_INSTALL_PREFIX=%{PREFIX}
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

#install license file
mkdir -p %{buildroot}/usr/share/license
cp LICENSE %{buildroot}/usr/share/license/%{name}

%post

%files
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{BINDIR}/*
%{RESDIR}/*
/usr/share/packages/*
/usr/share/license/%{name}
#/etc/smack/accesses2.d/%{name}.rule
/etc/smack/accesses.d/%{name}.efl
