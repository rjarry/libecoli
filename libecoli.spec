# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2024 Robin Jarry

%global forgeurl https://github.com/rjarry/libecoli

Name: libecoli
Version: 0.2.0
Summary: Extensible COmmand LIne library
License: BSD-3-Clause

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

%description
libecoli stands for Extensible COmmand LIne library.

This library provides helpers to build interactive command line interfaces.

What can it be used for?

* Complex interactive command line interfaces in C (e.g.: a router CLI).
* Application arguments parsing, natively supporting bash completion.
* Generic parsers.

Main Features

* Dynamic completion.
* Contextual help.
* Integrated with libedit, but can use any readline-like library.
* Modular: the cli behavior is defined through an assembly of basic nodes.
* Extensible: the user can write its own nodes to provide specific features.
* C API.

%package devel
Summary: Development files for %{name}

%description devel
This package contains development files for %{name}.

%prep
%forgesetup

%build
%meson -Dbuild_doc=true -Dbuild_tests=true -Dwith_editline=true
%meson_build

%check
%meson_test

%install
%meson_install

%files
%doc README.md
%license LICENSE
%{_libdir}/%{name}.so*

%files devel
%{_mandir}/man3/*
%{_includedir}/ecoli*.h
%{_libdir}/pkgconfig/libecoli.pc
%{_datadir}/doc/libecoli

%changelog
* Fri Oct 25 2024 Robin Jarry <robin@jarry.cc> - 0.2.0
- First packaged version
