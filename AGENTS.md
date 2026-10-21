# AdvanceMAME Developer & Agent Guide

This document contains essential project overview, architecture details, build/test workflows, and tips for AI agents and developers working on this codebase.

---

## 1. Project Overview

**AdvanceMAME** is an advanced arcade emulator built on top of the classic MAME 0.106 core, designed and maintained by Andrea Mazzoleni. Its main strengths include advanced video processing (support for CRT monitors, modeline generation, scale effects like xBR/ScaleHQ), flexible audio synchronization, and cross-platform support (Linux, DOS, Windows, Raspberry Pi).

- **Core Version**: MAME 0.106 baseline (C-based driver architecture, before MAME transitioned to C++ devices and modern memory systems).
- **Target OS / Build Environment**: Linux (x86_64 / ARM / Raspberry Pi), GCC toolchain.

---

## 2. Source Code Organization

- `src/drivers/`: Driver implementations for arcade hardware (e.g. `src/drivers/raiden2.c`, `src/drivers/cps1.c`, etc.).
- `src/mamedriv.c`: Master list of all enabled game drivers.
  - **Important**: Any newly added, renamed, or restored game driver MUST be registered here using the `DRIVER( <name> )` macro; otherwise, the binary will not recognize the ROM set.
- `src/vidhrdw/`: Video hardware emulation routines.
- `src/sndhrdw/`: Sound hardware implementations.
- `src/machine/`: Cryptography, co-processor, and peripheral MCU emulation.
- `src/includes/`: Driver header definitions and shared structures.
- `resource/`: Reference repositories and materials:
  - `resource/mame/mame0XXXs/`: Historical MAME source code drops (e.g. `resource/mame/mame0155s/` contains MAME 0.155).
  - `resource/mamegit/`: Git repository containing full MAME history for researching commits, diffs, and promotion logs. Always use `--` before the path (e.g. `git -C resource/mamegit log -- src/mame/drivers/raiden2.c`), otherwise Git will fail if the file was moved, renamed, or does not exist at that path in HEAD.

---

## 3. Build Instructions

AdvanceMAME uses a standard Makefile:

```bash
# Build the project (executable placed at obj/mame/linux/blend/advmame)
make -j$(nproc)
```

The resulting executable can be invoked as `./obj/mame/linux/blend/advmame`.

To build with debugger support, use:

```bash
make -j$(nproc) CONF_DEBUGGER=yes
```

This defines `MAME_DEBUG` and enables `-debug_cpu`, `-debug_watch`, and `-debug_break`. The debugger executable is placed at `obj/mame/linux/blend-debugger/advmame`; invoke it explicitly when using these options:

```bash
./obj/mame/linux/blend-debugger/advmame <driver_name> -output window -debug_cpu 0 -debug_exit 120
```

The debugger build uses a separate output directory, so it can coexist with the standard build. Use `make -j$(nproc)` for the standard build and keep `CONF_DEBUGGER=yes` on subsequent debugger build commands.

---

## 4. Running & Testing Games

> [!IMPORTANT]
> **Always use `-output window`** whenever launching or running any game with AdvanceMAME.

### ROM Paths
The ROM paths and configurations are defined in `~/.advance/advmame.rc` (e.g., `/extra/advmame/rom`, `/extra/advmame/rom_cln`, etc.).

### Running Games & ROM Verification
To run a game interactively:
```bash
./obj/mame/linux/blend/advmame <driver_name> -output window
```

To quickly verify driver recognition and ROM set completeness without leaving the game running (exits cleanly at frame 1 if OK, or prints missing ROM details and exits):
```bash
./obj/mame/linux/blend/advmame <driver_name> -output window -debug_exit 1
```
If ROMs are missing or mismatched, AdvanceMAME will list all required ROM filenames and hashes.

### Automated Frame Debugging & Snapshots
AdvanceMAME includes built-in options for deterministic frame capture and automated exit:
- `-debug_snap_step <X>`: Save game frames as PNG images every `X` frames (if 1, saves every frame; default 0 = disabled). Saved to `/home/am/.advance/snap/<driver_name>_<frame:05d>.png`.
- `-debug_sound`: Record game audio automatically to WAV in `dir_snap` until exit. Independent of manual recording keys and time limits; overwrites `dir_snap/<driver_name>.wav` on each run.
- `-debug_start <X>`: Enable CPU traces, memory watches, PC breakpoints, automatic PNG snapshots and debug WAV capture at frame `X` (default 0 = immediately). Frames are numbered from 1. Snapshot intervals and `-debug_exit` remain absolute; for example, start 100 with step 10 first captures frame 100. Manual recording and scripted inputs are not delayed.
- `-debug_exit <X>`: Automatically force a clean exit at frame `X` (default 0 = disabled).
- `-debug_cpu <N>`: Trace CPU instruction execution and registers in real time to `dir_snap/<driver_name>.cpu`, overwriting the file on each run (`<N>` is CPU index: `0` for CPU 0 / main CPU, `1` for CPU 1, `-1` for all CPUs; default `-2` = disabled). Available when built with debugger support (`make CONF_DEBUGGER=yes`, which defines `MAME_DEBUG`), ensuring standard release builds incur zero CPU overhead.
- `-debug_watch <cpu>:<address>:<length>`: Log reads and writes overlapping one byte range in the CPU's program memory space to `dir_snap/<driver_name>.watch`. Addresses are hexadecimal (optional `0x`), lengths are decimal or `0x` hexadecimal, and CPU indices are decimal. Each line contains frame, CPU, instruction PC, direction, access width, byte address and actual data. Includes banked RAM and handler accesses through the core memory accessors; direct pointer accesses and opcode fetches that bypass those accessors are not recorded. Requires `CONF_DEBUGGER=yes`.
- `-debug_break <cpu>:<address>`: On the first matching PC, save the selected CPU's instruction/register state to `dir_snap/<driver_name>.break`, then save `dir_snap/<driver_name>.break.png` and exit. Uses the same CPU/address notation as `-debug_watch` and requires `CONF_DEBUGGER=yes`. Registers are captured at the instruction hook; CPU cores may finish that instruction before suspension. The PNG is captured at the next video update with CPUs suspended, including when the match occurs before the first frame. It is not an instruction-level rendering of the screen.

Both options work independently of `-debug_cpu`, are gated by `-debug_start`, and overwrite their output files each run. Enabling `-debug_break` also removes its old PNG so a missed breakpoint cannot leave a stale screenshot. Use `-debug_exit` as a fallback if a breakpoint is never reached. Only one watched range and one breakpoint can be configured per run; watch addresses/lengths use bytes, while breakpoint addresses use the CPU's native PC units.

Example: capture every 10th frame up to frame 200 and exit:
```bash
./obj/mame/linux/blend/advmame <driver_name> -output window -debug_snap_step 10 -debug_exit 200
```
The saved PNG snapshots can be viewed using image viewer tools to inspect tilemaps, sprites, colors, and text layer integrity.

Example: trace CPU 0 and capture PNG/WAV from frame 100 to frame 120 (requires a debugger-enabled build):
```bash
./obj/mame/linux/blend-debugger/advmame <driver_name> -output window -debug_cpu 0 -debug_start 100 -debug_snap_step 10 -debug_sound -debug_exit 120
```

Example: watch main CPU RAM and exit on a PC match (addresses below are illustrative):
```bash
./obj/mame/linux/blend-debugger/advmame loverboy -output window -debug_watch 0:8000:0x100 -debug_break 0:1234 -debug_start 100 -debug_exit 600
```

### Scripting & Input Simulation (-script_play)
AdvanceMAME supports automated scripting via `-script_play`. This is useful for simulating inputs such as inserting coins (`coin1`) and pressing start (`start1`) without manual interaction, allowing agents to navigate menus or transition from attract mode into active gameplay:

```bash
# Insert coin after 5s, press start 1s later, capture every 20 frames up to frame 600, and exit
./obj/mame/linux/blend/advmame <driver_name> -output window -script_play '{ delay(5000); simulate_event(coin1, 100); delay(1000); simulate_event(start1, 100); }' -debug_snap_step 20 -debug_exit 600
```
Common simulated event identifiers:
- `coin1`, `coin2`: Coin slot inputs.
- `start1`, `start2`: Player 1 / Player 2 Start buttons.
- `p1_button1`, `p1_up`, `p1_down`, `p1_left`, `p1_right`: Player 1 directional and action buttons.

### Comparing Execution with Official MAME (-debugscript)
When diagnosing boot freezes, protection traps, or emulation bugs, comparing instruction execution traces between AdvanceMAME and official MAME is extremely effective.

Official MAME (`/usr/local/bin/mame`) includes a built-in debugger that can run commands non-interactively using `-debug -debugscript <script_file>`.

#### 1. Generating an Execution Trace in Official MAME
Create a debugger command script (e.g. `/tmp/trace.cmd`):
```text
trace /tmp/mame_cpu.tr,maincpu,,{tracelog "%06X: A=%04X X=%04X Y=%04X S=%04X D=%04X P=%02X",pc,a,x,y,s,d,p}
go
```
Run official MAME with a limited execution duration:
```bash
/usr/local/bin/mame <driver_name> -rompath "/extra/advmame/rom;/extra/mame/rom" -debug -debugscript /tmp/trace.cmd -seconds_to_run 3 -window -nomaximize
```
> [!NOTE]
> In official MAME on Linux, multiple paths in `-rompath` must be separated by semicolons (`;`) enclosed in quotes, not colons (`:`).

#### 2. Comparing with AdvanceMAME
Log instructions or register state in AdvanceMAME (e.g., via debug logging in the CPU core or driver memory handlers) and compare the trace logs side-by-side using `diff` or head-to-head text comparison. This quickly pinpoints the exact branch condition, unmapped memory read, or protection check where AdvanceMAME diverges.

### Simulating Inputs & Snapshots in Official MAME (-autoboot_script)
Official MAME (`/usr/local/bin/mame`) supports automated Lua scripting via `-autoboot_script <script.lua>`. This allows simulating coin insertion, start/directional inputs, and capturing reference snapshots at specific frames for head-to-head visual comparison with AdvanceMAME:

1. Create a Lua script (e.g. `/tmp/test_mame.lua`):
```lua
local frame = 0
emu.register_frame_done(function()
    frame = frame + 1
    -- Insert coin at frame 300, release at frame 310
    if frame == 300 then
        manager.machine.ioport.ports[":SYSTEM"]:field(0x10):set_value(1)
    end
    if frame == 310 then
        manager.machine.ioport.ports[":SYSTEM"]:field(0x10):set_value(0)
    end
    -- Take screenshot at frame 340
    if frame == 340 then
        manager.machine.video:snapshot()
    end
end)
```

2. Run official MAME with the script:
```bash
/usr/local/bin/mame <driver_name> -rompath "/extra/advmame/rom;/extra/mame/rom" -autoboot_script /tmp/test_mame.lua -seconds_to_run 6 -window -nomaximize -snapname /tmp/mame_test
```

> [!TIP]
> To find the exact input port tag (e.g. `":SYSTEM"`, `":IN0"`, `":DSW"`) and field bitmask (e.g. `0x10` for `IPT_COIN1`), inspect the driver's `INPUT_PORTS_START` macro in `resource/mamegit/`.

---

## 5. Backporting Guidelines (Modern MAME -> AdvanceMAME)

When porting fixes from modern MAME (e.g., MAME 0.155+) into AdvanceMAME:

1. **C vs C++ Device Hierarchy**:
   - Modern MAME uses C++ classes inheriting from `driver_device` and `device_t` with `MCFG_*` macros and member handlers.
   - AdvanceMAME uses C-style functions (`READ8_HANDLER`, `WRITE8_HANDLER`, `DRIVER_INIT`, `MACHINE_RESET`, `VIDEO_START`, `VIDEO_UPDATE`).
2. **Memory Map Differences**:
   - Modern MAME uses 16-bit word addresses (`AS_PROGRAM, 16`) where word offsets are passed to read/write members.
   - AdvanceMAME uses byte-level memory maps (`ADDRESS_SPACE_PROGRAM, 8`) with `AM_RANGE` in bytes.
   - Be mindful of address bus swaps (e.g. CRTC register address swaps: word offset bit 3/4 swap corresponds to byte offset bit 4/5 swap).
3. **Hardware Registers & Reset Hooks**:
   - Ensure hardware state variables (e.g. `r2_is_raidendx`) are consistently set across **both** `DRIVER_INIT` and `MACHINE_RESET`.
   - Watch out for `DRIVER_INIT(child)` calling `DRIVER_INIT(parent)` if the parent initializes or resets variant flags.
4. **DMA & Video Buffering**:
   - When co-processor DMA (COP) or private buffer transfers are required (e.g. tilemap or palette DMA), ensure the DMA trigger handler actually copies the memory rather than assuming CPU writes directly mutate video RAM.
5. **Driver Registration**:
   - When adding or renaming sets, always update `src/mamedriv.c` to match the exact `GAME()` macro identifiers.

---

## 6. Non-Obvious Emulation & Debugging Pitfalls

The following are subtle, non-obvious traps frequently encountered when debugging arcade drivers or porting code to AdvanceMAME:

1. **Stale NVRAM/CFG Invalidation (`nvram/` & `cfg/`)**:
   - *Trap*: If a game driver is tested while it has memory bugs or incomplete protection emulation, AdvanceMAME persists this corrupt state to `nvram/<driver>.nv` and `cfg/<driver>.cfg`.
   - *Symptom*: Even after the source code bug is completely fixed, the game continues to fail self-tests, freeze at boot, or show factory reset errors because it reloads the stale NVRAM saved during previous failed runs.
   - *Rule*: Always delete `nvram/<driver>.nv` (and `cfg/<driver>.cfg`) when testing fixes for boot freezes, EEPROM checksums, or protection routines.

2. **Unmapped Memory Reads: 0x00 vs. Open Bus / Float**:
   - *Trap*: In classic MAME 0.106, unmapped address space or unpopulated RAM (such as missing battery SRAM on arcade bootlegs) frequently returns `0x00` by default instead of open bus (floating data bus) or `0xFF`.
   - *Symptom*: Anti-copier or anti-piracy routines intentionally read unmapped areas expecting bus noise or non-zero values. Reading `0x00` triggers protection traps (e.g. infinite `BRA $` loops, forced screen blanks via display disable registers, or continuous resets) with no obvious error message.
   - *Fix*: Check read handlers for unmapped regions or bypass the protection check in `DRIVER_INIT` (e.g. patch with an immediate `RTS`: `0x60` on 65816, `0x4E75` on 68000, `0xC9` on Z80).

3. **Audio Handshake & Sub-CPU Synchronization Delays**:
   - *Trap*: Arcade systems with dedicated sound CPUs (Z80, 68000 sound CPU, SPC700) exchange data via I/O latch registers with handshake status flags (e.g. ACK/Ready bits).
   - *Symptom*: While the sound CPU initializes, transfers sound samples, or waits for an interrupt, the main CPU can spin in a `while (!ACK)` loop for 50,000–300,000 instructions (corresponding to 50–100 frames). This delay is easily mistaken for a CPU freeze or crash.
   - *Rule*: Before assuming a boot freeze, verify whether the PC is cycling on an audio I/O latch and run the test with a higher frame count (e.g. `-debug_exit 600`), ensuring the sound CPU reset lines and clocks are properly running.

4. **Address Bus Swaps & Word-vs-Byte Offsets**:
   - *Trap*: Modern MAME uses word offsets (`offset 1` = byte 2 in 16-bit spaces), while AdvanceMAME uses byte offsets in memory ranges.
   - *Symptom*: When porting address bit swaps or multiplexed registers (e.g. CRTC register address swaps), a modern MAME mask like `offset & 0x08` corresponds to `offset & 0x10` in AdvanceMAME byte space. Incorrect bit masks compile without any warnings or errors, but silently scramble tilemaps, palettes, or sprite attributes.

5. **Interrupt Line Management (`ASSERT_LINE` vs. `HOLD_LINE`)**:
   - *Trap*: Using `ASSERT_LINE` keeps the CPU interrupt line active indefinitely until software explicitly acknowledges and clears it via a specific I/O port (`CLEAR_LINE`).
   - *Symptom*: If the driver lacks the exact write handler that clears the interrupt line, the CPU will immediately re-enter the Interrupt Service Routine (ISR) infinitely upon exit, completely halting game logic.
   - *Rule*: Prefer `HOLD_LINE` for standard raster/VBLANK interrupts unless explicit hardware latching and clearing behavior is verified and fully implemented.

6. **Sprite & Palette Bufferization at Frame End (VBLANK / DMA)**:
   - *Trap*: Many classic arcade video chips do not render directly from CPU-writable RAM; they read from an internal private buffer or rely on end-of-frame buffering.
   - *Symptom*: The CPU writes sprite coordinates and palettes correctly, but the screen remains blank or sprites lag by one frame.
   - *Fix*: Ensure sprite/palette buffering is invoked at VBLANK (e.g., calling `buffer_spriteram_w` or configuring `video_eof_*`), or ensure DMA trigger register handlers actually transfer the buffer rather than waiting for CPU writes.

7. **Tracking Driver History in `resource/mamegit/` with `--follow`**:
   - *Trap*: In the Git history of MAME (`resource/mamegit/`), drivers were repeatedly moved (e.g., from `src/mame/drivers/` into vendor-specific directories like `src/mame/nintendo/`, `src/mame/capcom/`, etc.).
   - *Symptom*: Running `git log` on an old path in HEAD may return fatal errors or miss older commits.
   - *Rule*: Always use `--` before the path and pass `--follow` (e.g. `git -C resource/mamegit log --follow -- <path>`), or search commit messages and code changes with `git -C resource/mamegit log -S "<symbol_or_driver>"` to find the commit where a game was promoted from NOT_WORKING.

---

## 7. Development & Agent Workflow Rules

- **Always Use Window Mode**: Always use `-output window` whenever launching or running games with AdvanceMAME.
- **Commit Messages**: Every time a change is done, a single line commit description should be provided for that change.
- **Git Commits**: Never commit changes to git.
- **Temporary files** Create temporary files and directories under `/tmp/advancemame/`.
