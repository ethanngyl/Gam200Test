# Code Review — Game Engine/Game Segregation Refactor

**Commits reviewed**: `aefde8a`, `f61b94c`
**Branch**: `claude/code-review-LndEd`
**Date**: 2026-03-17

---

## Summary

The two most recent commits introduce a significant architectural refactor: hardcoded paths are moved to a project manifest, and ~1400 lines of mixed-responsibility code in `GameStateManager.cpp` are extracted into a new `GameRuntime/` module. The direction is correct and improves maintainability, but the refactor has left behind several bugs and maintenance hazards detailed below.

---

## Critical Issues

### 1. Duplicate Lua API Registrations — `GameScriptAPIProvider.cpp`

Seven functions are registered twice:

- `GetEntityAP` (registered at both blocks)
- `GetEntityHP`
- `SetEntityHP`
- `RefillEntityAP`
- `RefillEntityAttackAP`
- `ConsumeEntityAP`

The second registration silently overwrites the first. In debug builds this causes no crash, but it wastes registration slots and indicates a copy-paste error during the refactor. Remove the duplicate block (lines ~133–141).

---

### 2. Memory Leak in `main.cpp` on Init Failure

```cpp
engine = new CoreEngine();
if (!engine->InitializeAllSystems()) {
    return -1;  // engine is never deleted
}
```

If `InitializeAllSystems()` fails, `engine` is leaked. Fix with RAII (`std::unique_ptr`) or an explicit `delete engine` before the early return.

---

### 3. Incomplete `Level3Hybrid` State Initialization — `StateDispatchTable.cpp`

`ConfigureLevel3HybridState()` calls `StandardScriptedStateHooks::Configure(state)` to populate all six function pointers, then overwrites only three (`fpInitialize`, `fpUpdate`, `fpFree`). This leaves `fpLoad`, `fpDraw`, and `fpUnload` pointing to the standard scripted implementations, which may be correct but is not documented. If it is intentional, add a comment; if not, explicitly assign all six pointers.

---

### 4. State ID Defined in Four Separate Locations

The same set of state identifiers appears in:

1. `StateProfiles.cpp` — hardcoded array
2. `StateScriptRegistry.cpp:IsScriptedState()` — switch statement
3. `StateScriptRegistry.cpp:GetFallbackStateName()` — switch statement
4. `StateScriptRegistry.cpp:GetFallbackLevelScript()` — switch statement

Adding a new state requires updating all four locations; missing any one causes silent misbehavior. Consolidate into a single table (e.g., in `StateProfiles`) and derive the others from it.

---

## High-Severity Issues

### 5. Null Function Pointers in `SpecialStateHooks` — `SpecialStateHooks.cpp`

`ConfigureUnknownState()` logs an error then sets all six function pointers to `nullptr`. If the GSM subsequently calls any of them the game crashes. Replace `nullptr` with no-op stubs (log and return) so the game degrades gracefully instead of crashing.

`ConfigureRestartState()` and `ConfigureQuitState()` only log — they configure nothing. If these states are reached, all function pointers remain whatever they were previously. Either implement the logic or document clearly that these states are handled by a higher-level mechanism before the function pointers are called.

---

### 6. Unimplemented Lua Functions Declared in `LevelLoader.h`

`LevelLoader.h` declares `Lua_ToggleEditor`, `Lua_IsEditorEnabled`, and `Lua_SetEditorMode` as public static methods but they have no implementation in `LevelLoader.cpp`. Calling them will fail to link or crash at runtime. Implement or remove them.

---

### 7. Static String Shared Across Calls — `StateScriptRegistry.cpp`

```cpp
static std::string fallbackPath;
// ... modified each call to buildPath()
```

`buildPath()` modifies a `static std::string` and returns a `c_str()` pointer into it. Any subsequent call (or returned pointer use after another call) invalidates prior results. This is also not thread-safe. Return a `std::string` by value instead.

---

### 8. JSON Parsed via String Search in `AudioLoader.cpp`

`ReloadAudioLibrary()` parses JSON by searching for substring positions manually rather than using the nlohmann::json object model already present in the project. Manual string parsing is fragile and breaks on whitespace/formatting variations. Use the existing JSON library:

```cpp
auto j = nlohmann::json::parse(fileContents);
for (auto& entry : j["sounds"]) { ... }
```

---

## Medium-Severity Issues

### 9. Dangling FMOD Channel Pointer — `AudioSystem.cpp`

`musicChannel` is stored as `FMOD::Channel*`. FMOD recycles channel handles internally; the pointer can become invalid after the sound finishes or the channel group is reset without the engine being notified. Use `FMOD::Channel::isValid()` before every access, or track validity via a callback/flag.

---

### 10. Inconsistent Logging — Multiple Files

`LevelLoader.cpp` and `AudioSystem.cpp` mix `std::cout`, `std::cerr`, and the project's `LOG_*` macros. This makes filtering log output in a running game impossible. Standardize to `LOG_*` macros exclusively.

---

### 11. Hardcoded State Names in `ConfigReader.cpp`

```cpp
// Lines 487-489
if (state == "mainmenu") ...
if (state == "level2") ...
if (state == "quit") ...
```

These duplicate the state list already in `StateProfiles`. Use `StateProfiles::GetKind()` or the registry instead.

---

### 12. `lua_touserdata` Cast Without Null Check — `LevelLoader.cpp`

```cpp
auto* obj = static_cast<SomeType*>(lua_touserdata(L, 1));
// obj used without null check
```

`lua_touserdata` returns `nullptr` if the stack value is not a userdata. Dereference without checking is undefined behavior. Add `if (!obj) { luaL_error(...); return 0; }`.

---

### 13. Implicit Dependencies via `Precompiled.h`

Multiple files use `Framework::KEY_F9`, `Framework::CORE`, `current`, `next`, `previous`, `fpLoad`, etc. without explicit includes. These are assumed to come through `Precompiled.h`. This is fragile: if the precompiled header changes, compilation of unrelated files breaks silently. Add explicit includes for the headers that define these symbols.

---

### 14. Magic Numbers in `main.cpp`

```cpp
if (dt > 0.25f) dt = 0.25f;   // line ~192
for (int i = 0; i < 5; ++i) { // line ~230
```

Move these to named constants (`MAX_DELTA_TIME`, `MAX_FIXED_STEPS`) at the top of the file.

---

## Low-Severity / Style Issues

### 15. Naming Inconsistency in `GameScriptAPIProvider.cpp`

All Lua-registered functions use the `Lua_FunctionName` prefix except `lua_ToggleEditorMode` and `lua_IsEditorMode` (lowercase `lua_`). Pick one convention and apply it uniformly.

### 16. `ConfigReader.h` — Missing `const` on Getters

`GetString()`, `GetInt()`, `GetFloat()` are logically read-only but are not marked `const`. Mark them `const` to enable use on `const ConfigReader&` references.

### 17. Resource Leak in `AudioLoader.cpp`

If an exception is thrown inside `ParseAudioJSON()` after the file is opened but before it is closed, the file handle leaks. Use RAII (`std::ifstream` already is RAII, but ensure scope exits properly) or wrap the body in a try/finally pattern.

### 18. `GetKind()` Linear Search — `StateProfiles.cpp`

The 14-entry array lookup is O(n) but negligible in practice. No action required unless the list grows significantly.

---

## Positive Observations

- **Architecture direction is sound**: Extracting game-specific hooks from `GameStateManager` into `GameRuntime/` is a meaningful improvement. The facade pattern in `GameStateRuntimeFacade` correctly isolates the engine from game-layer concerns.
- **`StateScriptRegistry`**: Proper use of try-catch on JSON parsing and fallback strategies is good defensive code.
- **`Level3StateHooks`**: The guard pattern (`if (em && pc && spawner && input)`) before using raw pointers is correct.
- **`project_paths.json`**: Moving hardcoded paths to a manifest is the right call and will simplify environment-specific configuration.
- **`GameBootstrap`**: Clean, minimal, well-scoped.

---

## Priority Fix List

| Priority | File | Issue |
|----------|------|-------|
| P0 | `GameScriptAPIProvider.cpp` | Remove duplicate Lua registrations |
| P0 | `main.cpp` | Fix engine leak on init failure |
| P0 | `LevelLoader.h/.cpp` | Remove or implement undeclared Lua functions |
| P1 | `StateDispatchTable.cpp` | Document or fix Level3Hybrid partial initialization |
| P1 | `SpecialStateHooks.cpp` | Replace nullptr function pointers with no-op stubs |
| P1 | `StateScriptRegistry.cpp` | Fix static string lifetime / consolidate state lists |
| P2 | `AudioLoader.cpp` | Replace manual JSON parsing with nlohmann::json |
| P2 | `AudioSystem.cpp` | Guard all FMOD channel accesses with `isValid()` |
| P2 | Multiple | Standardize logging to `LOG_*` macros |
| P3 | `ConfigReader.cpp` | Remove hardcoded state name strings |
| P3 | `LevelLoader.cpp` | Add null checks after `lua_touserdata` |
| P3 | Multiple | Add explicit includes; remove implicit Precompiled.h reliance |
