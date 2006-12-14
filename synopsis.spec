%define name synopsis
%define version 0.9
%define release 1
%define py_sitedir %(echo `%{__python} -c "import distutils.sysconfig; print distutils.sysconfig.get_python_lib()"`)

Summary: Source-code Introspection Tool
Name: %{name}
Version: %{version}
Release: %{release}
Source0: %{name}-%{version}.tar.gz
License: LGPL
Group: Development/Libraries
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot
Prefix: %{_prefix}
Vendor: Stefan Seefeld <stefan@fresco.org>
Url: http://synopsis.fresco.org

%description
Synopsis is a multi-language source code introspection tool that
provides a variety of representations for the parsed code, to
enable further processing such as documentation extraction,
reverse engineering, and source-to-source translation.

%package devel
Summary: The Synopsis header files
Group: System Environment/Libraries
Requires: synopsis = %{version}-%{release}

%description devel
Headers and libraries for developing software that uses Synopsis APIs.

%package doc
Summary: The Synopsis documentation
Group: System Environment/Libraries
Requires: synopsis = %{version}-%{release}

%description doc
Synopsis documentation


%prep

%setup

%build
env CFLAGS="$RPM_OPT_FLAGS" python setup.py build

%install
python setup.py install --root=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%post
/sbin/ldconfig

%postun
/sbin/ldconfig 

%files
%defattr(-, root, root)
%{_bindir}/*
%{_libdir}/*.so
%{py_sitedir}/Synopsis
%{_datadir}/Synopsis 
%doc README COPYING NEWS

%files devel
%defattr(-, root, root)
%{_includedir}/Synopsis
%{_libdir}/pkgconfig/Synopsis.pc

%files doc
%defattr(-, root, root)
%{_docdir}/Synopsis

%changelog
* Wed Dec 13 2006 Stefan Seefeld <stefan@fresco.org> 0.9-1
- initial package.
