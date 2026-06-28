# mod-playerbots (Gate 1)

Compile-only Playerbots stub for this TrinityCore fork. **Disabled by default.**

## Build (opt-in)

```powershell
cmake -B "D:/WOWEmulation/Emulators/Builds/TrinityCore-evry" `
      -DMODULES=static -DMODULE_MOD_PLAYERBOTS=static
cmake --build "D:/WOWEmulation/Emulators/Builds/TrinityCore-evry" `
      --config RelWithDebInfo --target worldserver modules
```

Default path (`MODULES=none`) does not link this module.

## Runtime config

- Shipped default: `bin/RelWithDebInfo/modules/mod-playerbots.conf.dist`
- User override: `bin/RelWithDebInfo/modules/mod-playerbots.conf`

Loaded via `ConfigMgr::LoadModuleConfigDir("modules")` — separate from `worldserver.conf.d`.

## Gate 1 scope

- `.playerbots status` — reports loaded + enabled/disabled state
- `Playerbots.Enable = 0` by default

## Not in Gate 1

Bot login/logout, Playerbots database, manager, AI, random bots, LFG/BG, WotLK imports.

Design: `docs/midnight-assessment/playerbots/playerbots-integration-plan.md`
