# Pyronia Library

Pyronia is a security framework for Linux-based systems that provides fine-grained access control.

## Overview

This repository provides a userspace library for integrating Pyronia into
language runtimes, such as Python, Javascript etc.

## Pyronia API

The packages in this library implement the various components of the Pyronia language runtime
framework.

TODO

## Installation

TODO

## Policy Generation

To facilitate writing Pyronia security policies, we provide a small generation tool that
creates sandbox profile templates for you to complete with your application's rules.

```
python generate-pyr-profile-template.py /abspath/to/application/executable /relpath/to/profiles
```

This script will generate two policy templates and store them at the given destination path:
_abspath.to.application.executable_ and _abspath.to.executable-lib.prof_. The first template
contains the application-wide policy, and is needed for Pyronia to

### Policy Specification Guidelines

For Pyronia to be able to enforce your security rules for individual libraries, your final
policies should follows these guidelines:

* One rule per line, comma-separated
* The library name `d` is reserved for default access rules. Do not remove any of the default access rules as this will break the Pyronia userspace backend.
* Application-wide file system access rule format: `<absolute path> <access perms>` where access
perms can be any one r more of r, w, a, m, ux, ix, px.
* Application-wide network connection rule format: `network <af>` where af can be `inet` or `udp`
* Library file system access rule format: `<library name> <absolute path>`. For the library to
have access to the resource, the absolute path must match one of the file rules in the
application-wide policy. Note that the library then gains the same access rights as indicated in
the policy-wide policy, so make sure to use only grant those access permissions that you want
the library to have whenever possible.
* Library network connection rule format: `<library name> network <destination address>`. For `inet` connections, the destination address must be an IP address (URLs not currently supported).

## Disclaimer

Please keep in mind that this Pyronia library is under active development.
The repository may contain experimental features that aren't fully tested.
We recommend using a [tagged release](https://github.com/masomel/libpyronia/releases).
