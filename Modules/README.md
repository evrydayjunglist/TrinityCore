# TrinityCore Modules

Opt-in native module root for this TrinityCore fork.

Full reference standards: [`docs/midnight-assessment/reference-trees-and-standards.md`](../docs/midnight-assessment/reference-trees-and-standards.md)

## Layout

A directory under `modules/` is a module only when it has a `src/` subfolder:

```text
modules/
  mod-example/
    src/
      Script/
        example_loader.cpp    # void Addmod_exampleScripts()
    conf/
      example.conf.dist
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

Module configs copy to flat `modules/` beside `worldserver.exe`, **keeping `.dist`**:

```text
modules/mod-playerbots/conf/playerbots.conf.dist
  → bin/RelWithDebInfo/modules/playerbots.conf.dist
```

`worldserver` loads `modules/` separately from `worldserver.conf.d`:

- `modules/*.conf` — your overrides (survive rebuilds)
- `modules/*.conf.dist` — shipped defaults when no matching `.conf` exists

## Phase 2

Playerbots compiles under `modules/mod-playerbots/` after this shell is proven.
See [`docs/midnight-assessment/module-support-prereq.md`](../docs/midnight-assessment/module-support-prereq.md).
