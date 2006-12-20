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
Summary: The Synopsis development environment.
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

%package idl
Summary: The Synopsis IDL Parser
License: GPL
Group: System Environment/Libraries
Requires: synopsis = %{version}-%{release}

%description idl
Synopsis IDL Parser module to parse CORBA IDL.


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
%{py_sitedir}/Synopsis/*.py
%{py_sitedir}/Synopsis/*.pyc
%{py_sitedir}/Synopsis/*.pyo
%{py_sitedir}/Synopsis/Parsers/*.py
%{py_sitedir}/Synopsis/Parsers/*.pyc
%{py_sitedir}/Synopsis/Parsers/*.pyo
%{py_sitedir}/Synopsis/Parsers/Cpp
%{py_sitedir}/Synopsis/Parsers/C
%{py_sitedir}/Synopsis/Parsers/Cxx
%{py_sitedir}/Synopsis/Parsers/Python
%{py_sitedir}/Synopsis/Processors
%{py_sitedir}/Synopsis/Formatters
%{_datadir}/Synopsis 
%doc README COPYING NEWS
%doc %_mandir/man1/*

%files devel
%defattr(-, root, root)
%{_includedir}/Synopsis
%{_libdir}/pkgconfig/Synopsis.pc

%files doc
%defattr(-, root, root)
%{_docdir}/Synopsis

%files idl
%defattr(-, root, root)
%{py_sitedir}/Synopsis/Parsers/IDL

%changelog
* Wed Dec 20 2006 Stefan Seefeld <stefan@fresco.org> 0.9-1
- initial package.
