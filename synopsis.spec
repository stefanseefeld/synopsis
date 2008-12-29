%define name synopsis
%define version 0.11
%define release 1
%define py_sitedir %(%{__python} -c "from distutils.sysconfig  import get_python_lib; print get_python_lib()")
%define py_sitearch %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib(1)")
%define url http://synopsis.fresco.org

Summary: Source-code Introspection Tool
Name: %{name}
Version: %{version}
Release: %{release}%{?dist}
Source0: %{url}/download/%{name}-%{version}.tar.gz
License: LGPLv2+
Group: Development/Tools
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
Url: %{url}
BuildRequires: python-devel
BuildRequires: pkgconfig
BuildRequires: libgc-devel
BuildRequires: flex
BuildRequires: bison
Requires: python-docutils
Requires: graphviz

%description
Synopsis is a multi-language source code introspection tool that
provides a variety of representations for the parsed code, to
enable further processing such as documentation extraction,
reverse engineering, and source-to-source translation.

%package devel
Summary: The Synopsis development environment
Group: Development/Libraries
Requires: synopsis = %{version}-%{release}
Requires: pkgconfig

%description devel
Headers and libraries for developing software that uses Synopsis APIs.

%package doc
Summary: The Synopsis documentation
Group: Documentation
Requires: synopsis = %{version}-%{release}

%description doc
Synopsis documentation

%package idl
Summary: The Synopsis IDL Parser
License: GPLv2+
Group: Development/Tools
Requires: synopsis = %{version}-%{release}

%description idl
Synopsis IDL Parser module to parse CORBA IDL.


%prep

%setup -q
env CFLAGS="$RPM_OPT_FLAGS" CXXFLAGS="$RPM_OPT_FLAGS" \
python setup.py config --with-gc-prefix=%{_prefix} --libdir=%{_libdir}

%build
env CFLAGS="$RPM_OPT_FLAGS" CXXFLAGS="$RPM_OPT_FLAGS" python setup.py build

%install
rm -rf $RPM_BUILD_ROOT
python setup.py install --root=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig 

%files
%defattr(-, root, root, -)
%{_bindir}/*
%{_libdir}/*.so.*
%{py_sitearch}/*.egg-info
%dir %{py_sitearch}/Synopsis/
%{py_sitearch}/Synopsis/*.py
%{py_sitearch}/Synopsis/*.pyc
%{py_sitearch}/Synopsis/*.pyo
%dir %{py_sitearch}/Synopsis/Parsers/
%{py_sitearch}/Synopsis/Parsers/*.py
%{py_sitearch}/Synopsis/Parsers/*.pyc
%{py_sitearch}/Synopsis/Parsers/*.pyo
%{py_sitearch}/Synopsis/Parsers/Cpp
%{py_sitearch}/Synopsis/Parsers/C
%{py_sitearch}/Synopsis/Parsers/Cxx
%{py_sitearch}/Synopsis/Parsers/Python
%{py_sitearch}/Synopsis/Processors
%{py_sitearch}/Synopsis/Formatters
%{_datadir}/synopsis-%{version}
%dir %{_docdir}/synopsis-%{version}
%{_docdir}/synopsis-%{version}/README 
%{_docdir}/synopsis-%{version}/COPYING
%{_docdir}/synopsis-%{version}/NEWS
%{_mandir}/man1/*

%files devel
%defattr(-, root, root, -)
%{_includedir}/Synopsis
%{_libdir}/*.so
%{_libdir}/pkgconfig/synopsis.pc
%{_docdir}/synopsis-%{version}/README 
%{_docdir}/synopsis-%{version}/COPYING
%{_docdir}/synopsis-%{version}/NEWS

%files doc
%defattr(-, root, root, -)
%{_docdir}/synopsis-%{version}/README 
%{_docdir}/synopsis-%{version}/COPYING
%{_docdir}/synopsis-%{version}/NEWS
%{_docdir}/synopsis-%{version}/html
%{_docdir}/synopsis-%{version}/print
%{_docdir}/synopsis-%{version}/examples

%files idl
%defattr(-, root, root, -)
%{py_sitearch}/Synopsis/Parsers/IDL
%{_docdir}/synopsis-%{version}/README 
%{_docdir}/synopsis-%{version}/COPYING
%{_docdir}/synopsis-%{version}/NEWS

%changelog
* Fri Dec 26 2008 Stefan Seefeld <stefan@fresco.org> 0.11-1
- initial package.
