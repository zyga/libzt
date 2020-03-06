Name:           libzt
Version:        0.1
Release:        0%{?dist}
Summary:        A simple and robust unit testing library for C
License:        LGPLv3
URL:            https://github.com/zyga/libzt/
Source0:        https://github.com/zyga/libzt/releases/download/%{version}/%{name}_%{version}.tar.gz

BuildRequires:  gcc
BuildRequires:  make
BuildRequires:  ShellCheck

%description
libzt is a simple and robust unit test library for C.

The library provides functions for common checks and assertions,
which produce readable diagnostic messages that integrate well with
"make check" and programming editors, such as vi.

 - Robust, allowing you to focus on your code.
 - Simple and small, making it quick to learn and use.
 - Doesn't use dynamic memory allocation, reducing error handling.
 - Equipped with useful helpers for writing test cases.
 - Portable and supported on Linux, MacOS and Windows.
 - Documented and fully coverage and integration tested.

%package devel
Summary:       Development files for %{name}
BuildArch:     noarch

%package doc
Summary:       Manual pages for %{name}
BuildArch:     noarch

%package static
Summary:       Static library for static linking with %{name}

%description devel
This package provides the development headers for %{name}.

%description doc
This package provides manual pages for %{name}.

%description static
This package contains the %{name} static library for -static linking.

%package unit-test-devel
Summary:         Unit tests for %{name} package
Requires:        %{name}-unit-test-devel = %{version}-%{release}

%description unit-test-devel
This package contains unit tests for libzt.

%prep
%setup -q
%configure

%build
%make_build

%install
%make_install

%check
make check

%files
%license LICENSE
%{_libdir}/libzt.so.1

%files static
%attr(644, root, root) %{_libdir}/libzt.a

%files devel
%license LICENSE
%{_includedir}/zt.h
%{_libdir}/libzt.so

%files doc
%license LICENSE
%{_mandir}/man3/zt_*.3.gz
%{_mandir}/man3/ZT_*.3.gz

%files unit-test-devel
%license LICENSE
%{_bindir}/libzt-test
%{_mandir}/man1/libzt-test.1.gz

%changelog
* Fri Jan 3 2020 Zygmunt Krynicki <me@zygoon.pl> - 0.1
- Initial package for Fedora
