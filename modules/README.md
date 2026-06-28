# TrinityCore Modules

Opt-in native module root for this TrinityCore fork.

This is part of the **primary work track** (AzerothCore-like architecture +
native Playerbots). See [`docs/midnight-assessment/project-focus.md`](../docs/midnight-assessment/project-focus.md)
for fork steering; retail-parity work usually lives in `src/server/scripts/` and
`sql/updates/` instead.

Full reference standards: [`docs/midnight-assessment/reference-trees-and-standards.md`](../docs/midnight-assessment/reference-trees-and-standards.md)

**Platform note:** The git path is lowercase **`modules/`** (matches CMake
`add_subdirectory(modules)` and AzerothCore convention). Previously tracked as
`Modules/` on Windows-only case-insensitive checkouts; renamed for Linux parity.

## Layout

A directory under `modules/` is a module only when it has a `src/` subfolder:

```text
modules/
  mod-example/
    src/
      Script/
        example_loader.cpp    # void Addmod_exampleScripts()
    conf/
      mod-example.conf.dist
```

CMake `string(MAKE_C_IDENTIFIER)` maps directory names to loader symbols:

- `mod-example` → `Addmod_exampleScripts()`, cache `MODULE_MOD_EXAMPLE`
- `mod-playerbots` → `Addmod_playerbotsScripts()`, cache `MODULE_MOD_PLAYERBOTS`

## Build options

- `-DMODULES=none` — default; empty `AddModulesScripts()`, no module sources linked
- `-DMODULES=static` — link enabled modules into `worldserver`
- `-DMODULES=dynamic` — build enabled modules as shared libraries under `bin/.../modules/`
- `-DMODULE_<NAME>=disabled|static|dynamic` — per-module override

```powershell
.\scripts\build-trinitycore-master.ps1

.\scripts\build-trinitycore-master.ps1 -Modules static
# plus per-module flags when modules exist, e.g.:
# cmake -B ... -DMODULES=static -DMODULE_MOD_PLAYERBOTS=static
```

## Runtime config

Module configs copy to flat `modules/` beside `worldserver.exe`, **keeping `.dist`**.
Config basename must match the module directory name (e.g. `mod-playerbots.conf.dist`):

```text
modules/mod-playerbots/conf/mod-playerbots.conf.dist
  → bin/RelWithDebInfo/modules/mod-playerbots.conf.dist
```

`worldserver` loads `modules/` separately from `worldserver.conf.d`:

- `modules/*.conf` — your overrides (survive rebuilds)
- `modules/*.conf.dist` — shipped defaults when no matching `.conf` exists

## Playerbots (Gate 1 complete)

The compile-only stub lives at `modules/mod-playerbots/` (loader, `.playerbots status`,
`mod-playerbots.conf.dist`). Opt-in via `-DMODULES=static -DMODULE_MOD_PLAYERBOTS=static`;
default `MODULES=none` leaves it under **disabled**, not linked.

Gate result: [`docs/midnight-assessment/playerbots-gate-01-compile-result.md`](../docs/midnight-assessment/playerbots-gate-01-compile-result.md).
Guardrails: [`docs/midnight-assessment/module-support-prereq.md`](../docs/midnight-assessment/module-support-prereq.md).
