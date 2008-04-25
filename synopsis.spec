%define name synopsis
%define version 0.11
%define release 1
%define py_sitedir %(%{__python} -c "from distutils.sysconfig  import get_python_lib; print get_python_lib()")
%define python_sitearch %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib(1)")
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

%build
env CFLAGS="$RPM_OPT_FLAGS" CXXFLAGS="$RPM_OPT_FLAGS" python setup.py build

%install
rm -rf $RPM_BUILD_ROOT
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
%{_libdir}/*.so.*
%dir %{py_sitedir}/Synopsis/
%{py_sitedir}/Synopsis/*.py
%{py_sitedir}/Synopsis/*.pyc
%{py_sitedir}/Synopsis/*.pyo
%dir %{py_sitedir}/Synopsis/Parsers/
%{py_sitedir}/Synopsis/Parsers/*.py
%{py_sitedir}/Synopsis/Parsers/*.pyc
%{py_sitedir}/Synopsis/Parsers/*.pyo
%{py_sitedir}/Synopsis/Parsers/Cpp
%{py_sitedir}/Synopsis/Parsers/C
%{py_sitedir}/Synopsis/Parsers/Cxx
%{py_sitedir}/Synopsis/Parsers/Python
%{py_sitedir}/Synopsis/Processors
%{py_sitedir}/Synopsis/Formatters
%{_datadir}/synopsis-%{version}
%doc %{_docdir}/synopsis-%{version}/README 
%doc %{_docdir}/synopsis-%{version}/COPYING
%doc %{_docdir}/synopsis-%{version}/NEWS
%doc %_mandir/man1/*

%files devel
%defattr(-, root, root)
%{_includedir}/Synopsis
%{_libdir}/*.so
%{_libdir}/pkgconfig/synopsis.pc
%doc %{_docdir}/synopsis-%{version}/README 
%doc %{_docdir}/synopsis-%{version}/COPYING
%doc %{_docdir}/synopsis-%{version}/NEWS

%files doc
%defattr(-, root, root)
%{_docdir}/synopsis-%{version}/html
%{_docdir}/synopsis-%{version}/print
%{_docdir}/synopsis-%{version}/examples

%files idl
%defattr(-, root, root)
%{py_sitedir}/Synopsis/Parsers/IDL
%doc %{_docdir}/synopsis-%{version}/README 
%doc %{_docdir}/synopsis-%{version}/COPYING
%doc %{_docdir}/synopsis-%{version}/NEWS

%changelog
* Thu Apr 24 2008 Stefan Seefeld <stefan@fresco.org> 0.11-1
* Thu Mar 20 2008 Stefan Seefeld <stefan@fresco.org> 0.10-1
* Wed Dec 20 2006 Stefan Seefeld <stefan@fresco.org> 0.9-1
- initial package.
