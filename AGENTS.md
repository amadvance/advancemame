# AdvanceMAME Agent Guide

**AdvanceMAME** is an arcade emulator by Andrea Mazzoleni built on the MAME 0.106 core (C-based drivers, before MAME moved to C++ devices and modern memory systems). It focuses on advanced video output (CRT modelines, scale effects like xBR/ScaleHQ), audio synchronization and portability (Linux x86_64/ARM/Raspberry Pi, DOS, Windows), built with GCC.

> [!IMPORTANT]
> Before backporting code from modern MAME or debugging a driver (debug options, CPU traces, memory watches, comparison with official MAME, known emulation pitfalls), read [BACKPORT.md](BACKPORT.md).

---

## 1. Source Code Organization

- `mame/drivers/`: Game drivers (e.g. `raiden2.c`, `cps1.c`).
- `mame/vidhrdw/`, `mame/sndhrdw/`: Video and sound hardware.
- `mame/machine/`: Cryptography, co-processors and peripheral MCUs.
- `mame/includes/`: Driver headers and shared structures.
- `mame/mamedriv.c`: Master list of enabled drivers. Every added, renamed or restored set MUST be registered with `DRIVER( <name> )` matching its `GAME()` identifier, otherwise the ROM set is not recognized.
- `resource/mame/`, `resource/hbmame/`: Git repositories with the full MAME and HBMAME history (see BACKPORT.md for how to search them).

---

## 2. Build & Run

```bash
make -j$(nproc)                     # executable: obj/mame/linux/blend/advmame
make -j$(nproc) CONF_DEBUGGER=yes   # executable: obj/mame/linux/blend-debugger/advmame
```
The debugger build defines `MAME_DEBUG`, uses a separate output directory and is required by `-debug_cpu`, `-debug_watch` and `-debug_break`.

**Always use `-output window`** when running AdvanceMAME. ROM paths are configured in `~/.advance/advmame.rc` (e.g. `/extra/advmame/rom`, `/extra/advmame/rom_cln`).

```bash
# Verify driver recognition and ROMs: exits at frame 1, or lists missing/mismatched ROMs with hashes
./obj/mame/linux/blend/advmame <driver_name> -output window -debug_exit 1
# Save a PNG every 10 frames to ~/.advance/snap/<driver_name>_<frame:05d>.png and exit at frame 200
./obj/mame/linux/blend/advmame <driver_name> -output window -debug_snap_step 10 -debug_exit 200
# Insert coin after 5s, press start 1s later, capture every 20 frames, exit at frame 600
./obj/mame/linux/blend/advmame <driver_name> -output window -script_play '{ delay(5000); simulate_event(coin1, 100); delay(1000); simulate_event(start1, 100); }' -debug_snap_step 20 -debug_exit 600
```
Common `simulate_event` identifiers: `coin1`, `coin2`, `start1`, `start2`, `p1_button1`, `p1_up`, `p1_down`, `p1_left`, `p1_right`.

`-debug_exit` disables throttling and prints the average FPS, so it is also used for performance measurements (see BACKPORT.md).

---

## 3. Development & Agent Workflow Rules

- **Efficiency First**: Prioritize maximum efficiency, even when this reduces emulation accuracy, provided the game remains functional.
- **Coding Style**: Preserve the coding style and indentation of the file being modified. Use tabs for indentation in new files, except for backported files, which must preserve the indentation of the original source file. Avoid reformatting unrelated code.
- **Game Database**: When adding, removing, renaming, or modifying an AdvanceMAME game or its metadata, read GAMES.md. For standard games supported by MAME, metadata is generated automatically. Modify `support/curated.json` only when necessary (e.g. hacks/bootlegs that must be standalone root records rather than clones, curated descriptions, or specific links/modifications). Do not regenerate `games.json`.
- **Commit Messages**: For every change, provide a single line commit description. Do not use subsystem prefixes such as "psx spu: ..."; start directly with an imperative verb (e.g. "Fix ...", "Add ...", "Deactivate ...").
- **Git Commits**: Do not commit changes to Git unless explicitly requested to commit a specific change.
- **License and Copyright Headers**: All source files under `mame/` (C, C++ and headers) must start with these two lines, also when creating, backporting or updating them:
  ```c
  // license:<license-id>
  // copyright-holders:<Author 1>, <Author 2>, ...
  ```
- **File Search Scope**: Search for files only in `/extra/advmame`, `/extra/mame` and the current directory. Never search the whole filesystem (e.g. `find /`). In the current directory, always prefer `git grep` over `grep`.
- **Temporary Files**: Create temporary files and directories under `/tmp/advancemame/`.

### 3.1 Development for the advance/ directory

- **Language**: C99 (not C11/C++).
- **Format**: Enforce via `uncrustify -c uncrustify.cfg --no-backup *.c *.h`.
- **Naming**: snake_case for functions, UPPER_CASE for macros/constants.
- **Indentation**: Tabs, no alignment.
- **Comments**: Use `/** ... */` for multiline top comments preceding declarations and `/* ... */` for multiline comments within code. Use `/* first letter lowercase */` for single-line inline notes.
- **Critical Comments**: Always add a comment at non-obvious critical points, especially around data-integrity invariants, crash recovery, fallback behavior, concurrency, and fatal versus best-effort error handling. Explain why the logic is required, not merely what the code does.
- **Preferences**: Use 0 instead of NULL and '\0'. Use prefix ++variable and --variable instead of postfix where equivalent.
- **Safety Checks**: Avoid adding safety checks for conditions that never happen.
- **Type Casts**: Avoid type casts unless strictly necessary for compilation with -Werror and unavoidable by other means.
