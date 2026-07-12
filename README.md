# ![logo](https://community.trinitycore.org/public/style_images/1_trinitycore.png) TrinityCore Evry — Midnight

[![Average time to resolve an issue](https://isitmaintained.com/badge/resolution/TrinityCore/TrinityCore.svg)](https://isitmaintained.com/project/TrinityCore/TrinityCore "Average time to resolve an issue") [![Percentage of issues still open](https://isitmaintained.com/badge/open/TrinityCore/TrinityCore.svg)](https://isitmaintained.com/project/TrinityCore/TrinityCore "Percentage of issues still open")

> **Evry — Midnight fork:** upstream-tracking TrinityCore with native modules, Playerbots,
> and modern retail-parity development.

--------------


* [About this fork](#about-this-fork)
* [Fork features](#fork-features)
* [Building this fork](#building-this-fork)
* [Upstream Build Status](#upstream-build-status)
* [Introduction](#introduction)
* [Requirements](#requirements)
* [Install](#install)
* [Reporting issues](#reporting-issues)
* [Submitting fixes](#submitting-fixes)
* [Copyright](#copyright)
* [Authors &amp; Contributors](#authors--contributors)
* [Links](#links)



## About this fork

TrinityCore Evry is a personal modern-retail fork of TrinityCore `master`, currently targeting
World of Warcraft: Midnight. It keeps a clean upstream lineage while adding an opt-in native
module framework, a TrinityCore-native Playerbots implementation, and selected retail-parity
systems.

Module support builds on TrinityCore's existing systems rather than introducing parallel
runtime infrastructure. Both combined static-module and module-less build profiles are
maintained.

This is an active development fork, not an official TrinityCore distribution.

## Fork features

### Native module framework

- Opt-in `modules/` tree with per-module CMake, source, configuration, and SQL ownership.
- `MODULES=none`, `static`, and `dynamic` build modes.
- Flat runtime module configuration with user overrides preserved across builds.
- Configuration reload support for module-owned settings.
- Deterministic module SQL updates for the existing auth, characters, and world databases.
- Dynamic-module loading through TrinityCore's existing script reload lifecycle, including
  replacement reload, unload/reload, duplicate-name rejection, and revision diagnostics.

### Playerbots

`mod-playerbots` is an ongoing native Midnight implementation built around real `Player` and
socketless `WorldSession` objects. Current work includes bot session lifecycle, master-alt
control, random-bot scheduling and persistence, movement/combat foundations, quest interaction,
and an expanding RPG progression loop. It is opt-in and remains statically linked because its
long-lived state and database integration are not safe to unload dynamically.

### Retail-parity work

The fork also contains completed or active work for modern systems including character-select
campsites, account-wide warband data, Dracthyr Forbidden Reach, retail-scaled combat stats,
Skyriding, modern professions groundwork, and Archaeology.

## Building this fork

The normal TrinityCore requirements still apply. A combined static-module configuration can be
generated with:

```shell
cmake -B build -DMODULES=static -DMODULE_MOD_PLAYERBOTS=static -DMODULE_MOD_EXAMPLE=static
cmake --build build --config RelWithDebInfo
```

For a module-less core, explicitly disable sticky per-module cache values:

```shell
cmake -B build -DMODULES=none -DMODULE_MOD_PLAYERBOTS=disabled -DMODULE_MOD_EXAMPLE=disabled
cmake --build build --config RelWithDebInfo
```

When returning to the combined build, explicitly restore both module options to `static`.

## Upstream Build Status

master | 3.3.5 | cata_classic
:------------: | :------------: | :------------:
[![master Build Status](https://circleci.com/gh/TrinityCore/TrinityCore/tree/master.svg?style=shield)](https://circleci.com/gh/TrinityCore/TrinityCore/tree/master) | [![3.3.5 Build Status](https://circleci.com/gh/TrinityCore/TrinityCore/tree/3.3.5.svg?style=shield)](https://circleci.com/gh/TrinityCore/TrinityCore/tree/3.3.5) | [![cata_classic Build Status](https://circleci.com/gh/TrinityCore/TrinityCore/tree/cata_classic.svg?style=shield)](https://circleci.com/gh/TrinityCore/TrinityCore/tree/cata_classic)
[![master Build status](https://ci.appveyor.com/api/projects/status/54d0u1fxe50ad80o/branch/master?svg=true)](https://ci.appveyor.com/project/DDuarte/trinitycore/branch/master) | [![Build status](https://ci.appveyor.com/api/projects/status/54d0u1fxe50ad80o/branch/3.3.5?svg=true)](https://ci.appveyor.com/project/DDuarte/trinitycore/branch/3.3.5) | [![Build status](https://ci.appveyor.com/api/projects/status/54d0u1fxe50ad80o/branch/cata_classic?svg=true)](https://ci.appveyor.com/project/DDuarte/trinitycore/branch/cata_classic)
[![master GCC Build status](https://github.com/TrinityCore/TrinityCore/actions/workflows/gcc-build.yml/badge.svg?branch=master&event=push)](https://github.com/TrinityCore/TrinityCore/actions?query=workflow%3AGCC+branch%3Amaster+event%3Apush) | [![3.3.5 GCC Build status](https://github.com/TrinityCore/TrinityCore/actions/workflows/gcc-build.yml/badge.svg?branch=3.3.5&event=push)](https://github.com/TrinityCore/TrinityCore/actions?query=workflow%3AGCC+branch%3A3.3.5+event%3Apush) | [![cata_classic GCC Build status](https://github.com/TrinityCore/TrinityCore/actions/workflows/gcc-build.yml/badge.svg?branch=cata_classic&event=push)](https://github.com/TrinityCore/TrinityCore/actions?query=workflow%3AGCC+branch%3Acata_classic+event%3Apush)
[![master macOS arm64 Build status](https://github.com/TrinityCore/TrinityCore/actions/workflows/macos-arm-build.yml/badge.svg?branch=master&event=push)](https://github.com/TrinityCore/TrinityCore/actions?query=workflow%3AGCC+branch%3Amaster+event%3Apush) | | [![cata_classic macOS arm64 Build status](https://github.com/TrinityCore/TrinityCore/actions/workflows/macos-arm-build.yml/badge.svg?branch=cata_classic&event=push)](https://github.com/TrinityCore/TrinityCore/actions?query=workflow%3AGCC+branch%3Acata_classic+event%3Apush)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/435/badge.svg)](https://scan.coverity.com/projects/435) | [![Coverity Scan Build Status](https://scan.coverity.com/projects/4656/badge.svg)](https://scan.coverity.com/projects/4656) |

## Introduction

TrinityCore is a *MMORPG* Framework based mostly in C++.

It is derived from *MaNGOS*, the *Massive Network Game Object Server*, and is
based on the code of that project with extensive changes over time to optimize,
improve and cleanup the codebase at the same time as improving the in-game
mechanics and functionality.

It is completely open source; community involvement is highly encouraged.

If you wish to contribute ideas or code, please visit our site linked below or
make pull requests to our [Github repository](https://github.com/TrinityCore/TrinityCore/pulls).

For further information on the TrinityCore project, please visit our project
website at [TrinityCore.org](https://www.trinitycore.org).

## Requirements


Software requirements are available in the [wiki](https://trinitycore.info/en/install/requirements) for
Windows, Linux and macOS.


## Install

Detailed installation guides are available in the [wiki](https://trinitycore.info/en/home) for
Windows, Linux and macOS.


## Reporting issues

Issues can be reported via the [Github issue tracker](https://github.com/TrinityCore/TrinityCore/labels/Branch-master).

Please take the time to review existing issues before submitting your own to
prevent duplicates.

In addition, thoroughly read through the [issue tracker guide](https://community.trinitycore.org/topic/37-the-trinitycore-issuetracker-and-you/) to ensure
your report contains the required information. Incorrect or poorly formed
reports are wasteful and are subject to deletion.


## Submitting fixes

C++ fixes are submitted as pull requests via Github. For more information on how to
properly submit a pull request, read the [how-to: maintain a remote fork](https://community.trinitycore.org/topic/9002-howto-maintain-a-remote-fork-for-pull-requests-tortoisegit/).
For SQL only fixes, open a ticket; if a bug report exists for the bug, post on an existing ticket.


## Copyright

License: GPL 2.0

Read file [COPYING](COPYING).


## Authors &amp; Contributors

Read file [AUTHORS](AUTHORS).


## Links

* [Website](https://www.trinitycore.org)
* [Wiki](https://www.trinitycore.info)
* [Forums](https://talk.trinitycore.org/)
* [Discord](https://discord.trinitycore.org/)
