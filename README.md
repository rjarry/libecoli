# libecoli

![logo.svg](/doc/logo.svg)

libecoli stands for **E**xtensible **CO**mmand **LI**ne library.

This library provides helpers to build interactive command line interfaces.

## What can it be used for?

* Complex interactive command line interfaces in C (e.g.: a router CLI).
* Application arguments parsing, natively supporting bash completion.
* Generic parsers.

## Main Features

* Dynamic completion.
* Contextual help.
* Integrated with libedit, but can use any readline-like library.
* Modular: the cli behavior is defined through an assembly of basic nodes.
* Extensible: the user can write its own nodes to provide specific features.
* C API.

## Architecture

See [architecture.md](/doc/architecture.md).

## License

libecoli uses the Open Source BSD-3-Clause license. The full license text can
be found in LICENSE.

libecoli makes use of Unique License Identifiers, as defined by the SPDX
project (https://spdx.org/):

- it avoids including large license headers in all files
- it ensures licence consistency between all files
- it improves automated detection of licences

The SPDX tag should be placed in the first line of the file when possible, or
on the second line (e.g.: shell scripts).

## Contributing

Anyone can contribute to libecoli. See [`CONTRIBUTING.md`](/CONTRIBUTING.md).

## Projects that use libecoli

* [6WIND Virtual Service Router](https://doc.6wind.com/new/vsr-3/latest/vsr-guide/user-guide/cli/index.html)
* [grout # a graph router based on DPDK](https://github.com/DPDK/grout)
