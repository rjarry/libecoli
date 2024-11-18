# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2024 Robin Jarry

%global forgeurl https://github.com/rjarry/libecoli

Name: libecoli
Version: 0.3.0
Summary: Extensible COmmand LIne library
License: BSD-3-Clause AND LicenseRef-Fedora-Public-Domain

%forgemeta

URL: %{forgeurl}
Release: %{autorelease}
Source: %{forgesource}

BuildRequires: gcc
BuildRequires: meson
BuildRequires: ninja-build
BuildRequires: libedit-devel
BuildRequires: pkgconf
BuildRequires: doxygen
BuildRequires: fdupes

%description
libecoli stands for Extensible COmmand LIne library.

This library provides helpers to build interactive command line interfaces.

What can it be used for?

* Complex interactive command line interfaces in C (e.g.: a router CLI).
* Application arguments parsing, native support for bash completion.
* Generic text parsing.

Main Features

* Dynamic completion.
* Contextual help.
* Integrated with libedit, but can use any readline-like library.
* Modular: the CLI behavior is defined through an assembly of basic nodes.
* Extensible: the user can write its own nodes to provide specific features.
* C API.

%package devel
Summary: Development files for %{name}
Requires: %{name}%{?_isa} = %{version}-%{release}

%description devel
This package contains development files for %{name}.

%package doc
BuildArch: noarch
Summary: Documentation for %{name}

%description doc
This package contains the HTML documentation for %{name}.

%prep
%forgesetup

%build
%meson -Dexamples=disabled -Dyaml=disabled
%meson_build

%check
%meson_test

%install
%meson_install

# Doxygen creates man links which are text files containing a reference to
# another man page. Upstream tried to convert them to symlinks but meson
# converts them back to regular files on install. Handle the conversion from
# man "link" to symbolic link to avoid duplicated files in the rpm.
for man in "%{buildroot}%{_mandir}"/*/*; do
	read -r so link < "$man"
	if [ "$so" = ".so" ]; then
		rm -f "$man"
		if [ -f "%{buildroot}%{_mandir}/$link" ]; then
			ln -sr "%{buildroot}%{_mandir}/$link" "$man"
		fi
	fi
done

# Replace duplicate files with hardlinks.
%fdupes "%{buildroot}%{_datadir}/doc/libecoli"

%files
%doc README.md
%license LICENSE
%{_libdir}/%{name}.so.0*

%files devel
%{_mandir}/man3/*
%{_includedir}/ecoli*.h
%{_libdir}/%{name}.so
%{_libdir}/pkgconfig/libecoli.pc

%files doc
%license LICENSE
%{_datadir}/doc/libecoli

%autochangelog
