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
BuildRequires:  pkgconfig(callmgr_client)
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
BuildRequires:  pkgconfig(notification)
BuildRequires:  pkgconfig(capi-system-runtime-info)
BuildRequires:  pkgconfig(capi-system-info)
BuildRequires:  pkgconfig(capi-system-sensor)
BuildRequires:  pkgconfig(capi-ui-efl-util)
BuildRequires:  pkgconfig(libtzplatform-config)

%description
Call UI application.

%prep
%setup -q

%build
export CFLAGS="${CFLAGS} -fvisibility=hidden"
export CXXFLAGS="${CXXFLAGS} -fvisibility-inlines-hidden -fvisibility=hidden"
export FFLAGS="${FFLAGS} -fvisibility-inlines-hidden -fvisibility=hidden"

%define APPDIR      %{TZ_SYS_RO_APP}/%{name}
%define BINDIR      %{APPDIR}/bin
%define RESDIR      %{APPDIR}/res
%define LICENSEDIR	%{TZ_SYS_SHARE}/license
%define PACKAGESDIR	%{TZ_SYS_RO_PACKAGES}

%define _tmp_buld_dir TEMP_BUILD_DIR/%{_project}-%{_arch}

mkdir -p %{_tmp_buld_dir}
cd %{_tmp_buld_dir}

cmake ../../ -DCMAKE_PKG_NAME=%{name} \
        -DCMAKE_APP_DIR=%{APPDIR} \
        -DCMAKE_BIN_DIR=%{BINDIR} \
        -DCMAKE_RES_DIR=%{RESDIR} \
        -DCMAKE_LICENSE_DIR=%{LICENSEDIR} \
        -DCMAKE_SHARE_PACKAGES_DIR=%{PACKAGESDIR}
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
cd %{_tmp_buld_dir}

%make_install

%clean
rm -f debugfiles.list debuglinks.list debugsources.list

%files
%defattr(-,root,root,-)
%manifest %{name}.manifest

%{BINDIR}/*
%{RESDIR}/*
%{PACKAGESDIR}/%{name}.xml
%{LICENSEDIR}/%{name}
