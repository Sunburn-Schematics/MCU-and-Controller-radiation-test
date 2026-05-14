# Build Automation Notes

Goal: establish a reliable automation path that stays synchronized with the human VS Code CMake Tools workflow.

Current intended target:
- MCU: STM32F411xE
- Configure preset: `Debug`
- Build graph: `build/Debug/build.ninja`
- Linker script: `STM32F411XX_FLASH.ld`

Primary build flow:
1. Configure the project through VS Code CMake Tools or the STM32 `cube-cmake` frontend so `build/<Preset>/build.ninja` and `compile_commands.json` are generated from `CMakePresets.json` and the top-level `CMakeLists.txt`.
2. Run `Tools\build.ps1`. The script discovers the installed STM32 VS Code extension folders for:
   - `stm32cube-ide-build-cmake`
   - `stm32cube-ide-core`
3. The script prepends the discovered `cube-cmake` and STM32 binary folders to `PATH`, matching the environment override used by the STM32 VS Code extensions.
4. The script resolves `cube-cmake` and the STM32-managed Ninja binary.
5. The default `Commands` backend asks Ninja for the generated target command list:
   - `ninja -C build/<Preset> -t commands <Target>`
6. The script executes each generated command serially through `cmd.exe`, using a per-command timeout and writing timestamped stdout/stderr logs under `build/<Preset>/`.
7. If a command times out, the script stops build-tool child processes started after that command began and exits with code `124`.

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

Progress:
- Replaced `CONFIGURE_DEPENDS` source globs with explicit source lists in `CMakeLists.txt`.
- Reconfigured `Debug`; generated `build.ninja` no longer contains `VerifyGlobs`.
- Wrapper dry-run reaches the generated compile and link actions.
- Wrapper timeout cleanup returns control and leaves no build-tool processes behind.
- Added a command-list backend that reads the generated Ninja commands and executes them one at a time with per-command timeouts.
- Completed a guarded Debug build with:
  - `powershell -NoProfile -ExecutionPolicy Bypass -File Tools\build.ps1 -Preset Debug -Backend Commands -PerCommandTimeoutSeconds 30`
  - Exit code: `0`
  - RAM: `15552 B` / `128 KB` (`11.87%`)
  - FLASH: `108852 B` / `256 KB` (`41.52%`)

Known issue:
- The normal Ninja scheduler path still hangs in this environment. Keep `-Backend Commands` as the default automation path until the scheduler issue is isolated.
- The command-list backend intentionally rebuilds the generated commands instead of using Ninja's incremental scheduler.

Warnings from the first successful guarded build:
- `Services/CommandHandler/hc_datetime.c`: `snprintf` date/time buffer truncation warning in `hc_datetime_ensure_initialized`.
- `Services/jsmn/jsmn_print_utils.c`: `isspace()` called with a `char` subscript expression in `jsmn_flatprint`.

Next diagnostics:
- Investigate why the direct Ninja scheduler path hangs when invoked from automation.
- Fix the two compiler warnings, then rerun the guarded command-list build.
