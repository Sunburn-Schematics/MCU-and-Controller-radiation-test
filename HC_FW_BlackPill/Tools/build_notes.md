# Build Automation Notes

Goal: establish a reliable automation path that stays synchronized with the human VS Code CMake Tools workflow.

Current intended target:
- MCU: STM32F411xE
- Configure preset: `Debug`
- Build graph: `build/Debug/build.ninja`
- Linker script: `STM32F411XX_FLASH.ld`

Complete build-to-hardware loop:
1. Make the smallest source change needed for the test.
2. Run the guarded build:
   - `powershell -NoProfile -ExecutionPolicy Bypass -File Tools\build.ps1 -Preset Debug -Backend Commands -PerCommandTimeoutSeconds 30`
3. Confirm build exit code `0`, review warnings, and check RAM/FLASH usage.
4. Program and verify the target:
   - `powershell -NoProfile -ExecutionPolicy Bypass -File Tools\program.ps1`
5. Confirm programming output includes:
   - `DBGMCU_IDCODE: 0x10006431`
   - `program/verify: OK`
   - `runtime sw_version: ...`
6. For behavior that cannot be fully verified over USB, ask the user for hardware observation.
7. Run the hardware test workflow when protocol/runtime behavior must be checked:
   - `powershell -NoProfile -ExecutionPolicy Bypass -File Tools\test.ps1 -Build -Program -ContinueOnFail`

Proven end-to-end hardware test:
- Source change: `App/fw_app.c` changed `HEARTBEAT_PERIOD_MS` from `500U` to `250U`.
- Expected effect: Blue LED heartbeat toggle period is halved, so the visible flash rate doubles.
- Build result: guarded Debug build completed with exit code `0`.
- Program result: OpenOCD programmed and verified the ELF, then runtime USB CDC `GET sw_version` succeeded.
- Hardware result: user confirmed the actual target BLUE LED flash rate doubled.

Codex build procedure:
1. Do not invoke direct real Ninja builds from Codex for this project:
   - Avoid `ninja -C build/Debug ...`
   - Avoid `cube-cmake --build ...`
   - Avoid `Tools\build.ps1 -Backend Ninja`
   - Avoid `Tools\build.ps1 -Backend CubeCMake`
2. Use the guarded command-list backend:
   - `powershell -NoProfile -ExecutionPolicy Bypass -File Tools\build.ps1 -Preset Debug -Backend Commands -PerCommandTimeoutSeconds 30`
3. Use dry-run/query commands only when inspecting the generated graph:
   - `powershell -NoProfile -ExecutionPolicy Bypass -File Tools\build.ps1 -Preset Debug -Backend Commands -DryRun`
   - `ninja -C build\Debug -t commands HC_FW_BlackPill.elf`
   - `ninja -C build\Debug -n -v HC_FW_BlackPill.elf`
4. Keep an outer command timeout on any Codex shell invocation of the wrapper.
5. After any interrupted or timed-out build diagnostic, check for and stop leftover build tools:
   - `ninja`
   - `cube-cmake`
   - `cmake`
   - `cmd`
   - `arm-none-eabi-gcc`
   - `arm-none-eabi-g++`
   - `arm-none-eabi-ld`
6. Remove stale generated lock files if a Ninja diagnostic was interrupted:
   - `build\Debug\.ninja_lock`
7. Treat a successful manual VS Code/CMake Tools build as the authoritative validation that the normal Ninja scheduler state is healthy.

Primary build flow:
1. Configure the project through VS Code CMake Tools or the STM32 `cube-cmake` frontend so `build/<Preset>/build.ninja` and `compile_commands.json` are generated from `CMakePresets.json` and the top-level `CMakeLists.txt`.
2. Run `Tools\build.ps1`. The script discovers the installed STM32 VS Code extension folders for:
   - `stm32cube-ide-build-cmake`
   - `stm32cube-ide-core`
3. The script prepends the discovered `cube-cmake` and STM32 binary folders to `PATH`, matching the environment override used by the STM32 VS Code extensions.
4. The script resolves `cube-cmake` and the STM32-managed Ninja binary.
5. The default `Commands` backend asks Ninja for the generated target command list:
   - `ninja -C build/<Preset> -t commands <Target>`
6. Before executing commands, the script backs up Ninja scheduler state that may already exist:
   - `.ninja_deps`
   - `.ninja_log`
   - existing `*.obj.d` depfiles
7. The script executes each generated command serially through `cmd.exe`, using a per-command timeout and writing timestamped stdout/stderr logs under `build/<Preset>/`.
8. After the command-list build, the script restores the backed-up Ninja scheduler state and removes any stale `.ninja_lock`.
9. If a command times out, the script stops build-tool child processes started after that command began and exits with code `124`.
10. The generated command-list state backups are stored under `build/<Preset>/.cb_*`. These folders are excluded from later depfile discovery because they are wrapper state, not part of the active CMake/Ninja build graph.
11. The wrapper removes transient `.cb_*`, stale `.codex_ninja_state_backup_*`, and old `.ninja_* .backup_*` files during normal cleanup.
12. The wrapper keeps only the most recent `cube_cmake_build_*.log` and `cube_cmake_dry_run_*.log` runs, controlled by `-KeepDiagnosticLogCount` which defaults to `5`. Use `-NoDiagnosticCleanup` only when preserving diagnostics for investigation.

Build-directory artifact policy:
- Required CMake/Ninja state: `CMakeCache.txt`, `build.ninja`, `compile_commands.json`, `.ninja_deps`, `.ninja_log`, `.cmake/`, `.cache/`, `CMakeFiles/`, and generated object/dependency files.
- Required firmware outputs: `HC_FW_BlackPill.elf` and `HC_FW_BlackPill.map`.
- Disposable wrapper diagnostics: `.cb_*`, `.codex_ninja_state_backup_*`, `.ninja_deps.backup_*`, `.ninja_log.backup_*`, old `cube_cmake_build_*.log`, old `cube_cmake_dry_run_*.log`, and ad hoc `ninja_*` or `direct_compile_*` probe logs.

Synchronization checks:
- `.vscode/settings.json` should keep `cmake.cmakePath` set to `cube-cmake`.
- `.vscode/settings.json` should keep `cmake.preferredGenerators` including `Ninja`.
- `.vscode/settings.json` `cmake.configureEnvironment.PATH` and `cmake.buildEnvironment.PATH` should include the same STM32 `cube-cmake` and core binary paths discovered by `Tools\build.ps1`.
- Project-owned source files are listed explicitly in the top-level `CMakeLists.txt`. When adding, removing, or renaming `.c` files under `App/`, `Bsp/`, `Drivers_Local/`, or `Services/`, update the matching source list in `CMakeLists.txt` before reconfiguring.
- `build/<Preset>/build.ninja` should be regenerated after edits to CMake files, source lists, presets, linker scripts, startup selection, or STM32 extension/toolchain versions.
- `build/<Preset>/compile_commands.json` should show the expected target define, currently `-DSTM32F411xE`.
- `build/<Preset>/build.ninja` should reference `startup_stm32f411xe.s` and `STM32F411XX_FLASH.ld`.
- `build/<Preset>/build.ninja` should not contain `VerifyGlobs` or `cmake.verify_globs`; source registration is intentionally explicit to avoid regeneration hangs.
- The wrapper now warns when the discovered STM32 extension paths drift from the paths configured for VS Code CMake Tools.
- The `Commands` backend should not be used as proof that Ninja's incremental scheduler is healthy; it is a bounded compile/link verification path that preserves existing Ninja state where possible.

Progress:
- Replaced `CONFIGURE_DEPENDS` source globs with explicit source lists in `CMakeLists.txt`.
- Reconfigured `Debug`; generated `build.ninja` no longer contains `VerifyGlobs`.
- Wrapper dry-run reaches the generated compile and link actions.
- Wrapper timeout cleanup returns control and leaves no build-tool processes behind.
- Added a command-list backend that reads the generated Ninja commands and executes them one at a time with per-command timeouts.
- Fixed the VS Code settings comparison to normalize `/` and `\` path separators before warning about drift.
- Added preservation of existing `.ninja_deps`, `.ninja_log`, and `*.obj.d` files around command-list builds so the fallback does not further desynchronize the manual VS Code/Ninja state.
- Excluded wrapper-generated `.cb_*` backup folders from depfile discovery so interrupted or repeated automation runs do not recursively back up old backups.
- Verified the guarded command-list build after a user-performed CMake cache delete and reconfigure.
- Completed a guarded Debug build with:
  - `powershell -NoProfile -ExecutionPolicy Bypass -File Tools\build.ps1 -Preset Debug -Backend Commands -PerCommandTimeoutSeconds 30`
  - Exit code: `0`
  - RAM: `15552 B` / `128 KB` (`11.87%`)
  - FLASH: `108852 B` / `256 KB` (`41.52%`)

Known issue:
- The normal Ninja scheduler path still hangs in this environment. Keep `-Backend Commands` as the default automation path until the scheduler issue is isolated.
- The command-list backend intentionally rebuilds the generated commands instead of using Ninja's incremental scheduler.
- Direct Ninja diagnostics show that `ninja -t ...` graph queries work, but command execution hangs before spawning child commands in this Codex runner. This reproduces even with a tiny standalone probe `build.ninja` that runs `cmd.exe /C echo ...`, so the issue is not specific to the STM32 compile/link commands.
- Earlier diagnostics removed `.ninja_deps` from `build/Debug` after backing it up. A manual VS Code/Ninja build may need to rebuild once to repopulate that database.

Warnings from the first successful guarded build:
- `Services/CommandHandler/hc_datetime.c`: `snprintf` date/time buffer truncation warning in `hc_datetime_ensure_initialized`.

Next diagnostics:
- Investigate why the direct Ninja scheduler path hangs when invoked from automation.
- Fix the remaining compiler warning, then rerun the guarded command-list build.
