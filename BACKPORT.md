# AdvanceMAME Backporting & Debugging Guide

Reference for porting fixes from modern MAME and debugging game drivers. Basic build, run and workflow rules are in `AGENTS.md`.

---

## 1. Researching MAME History

`resource/mame/` and `resource/hbmame/` are Git repositories with the full MAME and HBMAME history. Drivers were repeatedly moved (e.g. from `src/mame/drivers/` into vendor directories like `src/mame/nintendo/`, `src/mame/capcom/`), so:

- Always put `--` before the path and add `--follow` (e.g. `git -C resource/mame log --follow -- src/mame/drivers/raiden2.c`). Without `--`, Git fails if the path does not exist in HEAD; without `--follow`, older commits are missed.
- To find where a symbol changed or where a game was promoted from `NOT_WORKING`, search code changes with `git -C resource/mame log -S "<symbol_or_driver>"`.

---

## 2. Porting from Modern MAME

1. **C vs C++ Device Hierarchy**: Modern MAME (0.155+) uses C++ classes derived from `driver_device` and `device_t`, `MCFG_*` macros and member handlers. AdvanceMAME uses C functions (`READ8_HANDLER`, `WRITE8_HANDLER`, `DRIVER_INIT`, `MACHINE_RESET`, `VIDEO_START`, `VIDEO_UPDATE`).

2. **Word vs. Byte Offsets**: Modern MAME 16-bit spaces (`AS_PROGRAM, 16`) pass word offsets (`offset 1` = byte 2), while AdvanceMAME memory maps use byte addresses (`ADDRESS_SPACE_PROGRAM, 8`, `AM_RANGE` in bytes). When porting address bit swaps or multiplexed registers, shift the masks: a modern `offset & 0x08` corresponds to `offset & 0x10` (e.g. a CRTC register word offset bit 3/4 swap is a byte offset bit 4/5 swap). Wrong masks compile without warnings but silently scramble tilemaps, palettes or sprite attributes.

3. **Hardware Registers & Reset Hooks**: Set hardware state variables (e.g. `r2_is_raidendx`) consistently in **both** `DRIVER_INIT` and `MACHINE_RESET`. Watch out for `DRIVER_INIT(child)` calling `DRIVER_INIT(parent)` when the parent initializes or resets variant flags.

4. **MCS-48 Oscillator Clocks vs. Machine Cycles (`/ 15`)**:
   - The legacy `mame/cpu/i8039/` core used by `I8039` and `I8048` expects machine-cycle frequencies in `MDRV_CPU_ADD`. Its `CPUINFO_INT_CLOCK_DIVIDER` value is descriptive; the scheduler does not divide the configured clock. Modern MAME's MCS-48 core divides internally and expects the oscillator frequency.
   - Configure `oscillator_hz / I8039_CLOCK_DIVIDER` (the divider is 15: internal divisions by 3 and 5). For Cube Quest, the PR-8210 controller uses `4410000 / I8039_CLOCK_DIVIDER` (294,000 machine cycles/s) and the Simutrek controller uses `6000000 / I8039_CLOCK_DIVIDER` (400,000 machine cycles/s).
   - Copying a modern oscillator clock directly runs the MCU 15 times too fast: serial pulses become too short, handshakes fail, and the main CPU may wait indefinitely for the peripheral.
   - If replacing the core with one that divides internally, pass the oscillator clock and remove the driver-side division exactly once; audit existing drivers to avoid dividing twice. Do not apply this divisor to unrelated CPU families.

---

## 3. AdvanceMAME Debug Options

Options marked *(debugger)* require the debugger build (`make -j$(nproc) CONF_DEBUGGER=yes`, which defines `MAME_DEBUG`), run as `./obj/mame/linux/blend-debugger/advmame`. Output files go to `dir_snap` (`~/.advance/snap/`) and are overwritten on each run.

- `-debug_snap_step <X>`: Save a PNG every `X` frames as `<driver>_<frame:05d>.png` (0 = disabled).
- `-debug_sound`: Record audio to `<driver>.wav` until exit, independent of manual recording keys and time limits.
- `-debug_start <X>`: Enable CPU traces, memory watches, PC breakpoints, PNG snapshots and debug WAV capture from frame `X` (frames are numbered from 1; default 0 = immediately). Snapshot intervals and `-debug_exit` stay absolute (start 100 with step 10 first captures frame 100). Manual recording and scripted inputs are not delayed.
- `-debug_exit <X>`: Exit cleanly at frame `X`. Disables throttling, VSync and frameskip, and prints elapsed emulation time, frame count and average FPS, so it is also suitable for performance measurements. Timing starts at the initial emulation reset (excluding executable/ROM loading and shutdown) and continues across later resets; runs that end before timing starts (like a one-frame ROM check) print no timing. Disable snapshots, audio recording and tracing when measuring raw performance.
- `-debug_cpu <N>` *(debugger)*: Trace instructions and registers to `<driver>.cpu` (`<N>` = CPU index, `-1` = all CPUs; default `-2` = disabled).
- `-debug_watch <cpu>:<address>:<length>` *(debugger)*: Log reads and writes overlapping one byte range of the CPU program space to `<driver>.watch`. Addresses are hex (optional `0x`), lengths decimal or `0x` hex, CPU indices decimal. Each line has frame, CPU, instruction PC, direction, access width, byte address and data. Covers banked RAM and handler accesses through the core memory accessors; direct pointer accesses and opcode fetches are not recorded.
- `-debug_break <cpu>:<address>` *(debugger)*: On the first matching PC, save the CPU instruction/register state to `<driver>.break`, then `<driver>.break.png` at the next video update (with CPUs suspended, even before the first frame), and exit. Registers are captured at the instruction hook, and the core may finish that instruction before suspension; the PNG is not an instruction-level rendering. The old PNG is removed first, so a missed breakpoint leaves no stale screenshot; use `-debug_exit` as a fallback.

`-debug_watch` and `-debug_break` are independent of `-debug_cpu` and gated by `-debug_start`. Only one watched range and one breakpoint per run; watch addresses/lengths are in bytes, breakpoint addresses in the CPU's native PC units.

Examples:
```bash
# Trace CPU 0 and capture PNG/WAV from frame 100 to 120
./obj/mame/linux/blend-debugger/advmame <driver_name> -output window -debug_cpu 0 -debug_start 100 -debug_snap_step 10 -debug_sound -debug_exit 120
# Watch main CPU RAM and exit on a PC match (illustrative addresses)
./obj/mame/linux/blend-debugger/advmame loverboy -output window -debug_watch 0:8000:0x100 -debug_break 0:1234 -debug_start 100 -debug_exit 600
```

---

## 4. Comparing with Official MAME

Official MAME is `/usr/local/bin/mame`. On Linux, multiple `-rompath` entries are separated by semicolons inside quotes, not colons.

### Execution Traces (-debugscript)
Comparing instruction traces between AdvanceMAME and official MAME quickly pinpoints the branch, unmapped read or protection check where emulation diverges (boot freezes, protection traps). Create a debugger script (e.g. `/tmp/advancemame/trace.cmd`):
```text
trace /tmp/advancemame/mame_cpu.tr,maincpu,,{tracelog "%06X: A=%04X X=%04X Y=%04X S=%04X D=%04X P=%02X",pc,a,x,y,s,d,p}
go
```
```bash
/usr/local/bin/mame <driver_name> -rompath "/extra/advmame/rom;/extra/mame/rom" -debug -debugscript /tmp/advancemame/trace.cmd -seconds_to_run 3 -window -nomaximize
```
Then compare with an AdvanceMAME `-debug_cpu` trace (or debug logging in the CPU core or memory handlers) using `diff`.

### Inputs & Snapshots (-autoboot_script)
Create a Lua script (e.g. `/tmp/advancemame/test_mame.lua`) to simulate inputs and capture reference snapshots:
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
```bash
/usr/local/bin/mame <driver_name> -rompath "/extra/advmame/rom;/extra/mame/rom" -autoboot_script /tmp/advancemame/test_mame.lua -seconds_to_run 6 -window -nomaximize -snapname /tmp/advancemame/mame_test
```
Find the port tag (e.g. `":SYSTEM"`, `":IN0"`, `":DSW"`) and field bitmask (e.g. `0x10` for `IPT_COIN1`) in the driver's `INPUT_PORTS_START` in `resource/mame/`.

---

## 5. Emulation & Debugging Pitfalls

1. **Stale NVRAM/CFG**: Runs with memory bugs or incomplete protection persist corrupt state to `nvram/<driver>.nv` and `cfg/<driver>.cfg`. After the fix, the game keeps failing self-tests, freezing at boot or showing factory reset errors because it reloads that state. Always delete both files when testing fixes for boot freezes, EEPROM checksums or protection routines.

2. **Unmapped Memory Reads: 0x00 vs. Open Bus**: In MAME 0.106, unmapped space or unpopulated RAM (e.g. missing battery SRAM on bootlegs) often reads `0x00` instead of open bus or `0xFF`. Anti-piracy routines that expect bus noise or non-zero values then trap silently (infinite `BRA $` loops, display disabled, continuous resets). Check read handlers for unmapped regions, or bypass the check in `DRIVER_INIT` (e.g. patch an immediate `RTS`: `0x60` on 65816, `0x4E75` on 68000, `0xC9` on Z80).

3. **Audio Handshake Delays**: Main and sound CPUs (Z80, 68000, SPC700) exchange data through latches with ACK/Ready flags. While the sound CPU initializes or transfers samples, the main CPU can spin in a `while (!ACK)` loop for 50,000–300,000 instructions (50–100 frames), which looks like a freeze. Before assuming a boot freeze, check whether the PC is cycling on an audio latch, run longer (e.g. `-debug_exit 600`), and verify that the sound CPU reset lines and clocks are running.

4. **Interrupt Lines (`ASSERT_LINE` vs. `HOLD_LINE`)**: `ASSERT_LINE` keeps the interrupt active until software clears it (`CLEAR_LINE`) through a specific port. Without that write handler, the CPU re-enters the ISR forever and game logic halts. Prefer `HOLD_LINE` for standard raster/VBLANK interrupts unless the hardware latching and clearing behavior is verified and implemented.

5. **Sprite, Palette & DMA Buffering**: Many video chips render from a private buffer filled at VBLANK or by DMA, not from CPU-writable RAM. If the CPU writes correct data but the screen stays blank or sprites lag one frame, invoke buffering at VBLANK (e.g. `buffer_spriteram_w` or `video_eof_*`), and make sure DMA trigger handlers (e.g. COP tilemap or palette DMA) actually copy the memory instead of assuming CPU writes mutate video RAM directly.

6. **Signed Positions vs. Unsigned Limits**: Virtual laserdisc lead-in positions can be negative. Comparing a signed position with an unsigned count converts it to a large value: `track >= total_hunks / 2` can be true for a negative `track` when `total_hunks` is unsigned. In the Cube Quest Simutrek player, this clamped the head to the end of the disc on the first slider movement and left the firmware in its initialization or search loop. Keep positions and bounds in a consistent signed domain: derive a signed maximum after establishing that the count fits its type, then compare against it. Do not cast the position to unsigned or clamp valid lead-in positions to zero merely to silence a warning.

7. **Analog Ports Need `PORT_MINMAX`**: In MAME 0.106 an absolute analog port (`IPT_PEDAL*`, `IPT_PADDLE*`, `IPT_AD_STICK_*`, `IPT_LIGHTGUN_*`) without `PORT_MINMAX` keeps `analog.min = analog.max = 0`, while modern MAME defaults them to `0..mask`. The scale factors are computed from `default - min` and `max - default`, so a pedal with default `0x00` gets a zero range and never moves, whatever keys are pressed: the game then waits forever on screens like "STEP ON THE PEDAL TO START" (Final Lap R). Always add `PORT_MINMAX(0x00,0xff)` (or the matching mask, e.g. `0x3ff`) when backporting an analog port. Ports with a mid-scale default (e.g. `0x80`) may still work through the signed fallback, but their direction can be inverted. To find the missing ones, check each `PORT_BIT` of these types, including the modifiers on the following lines.
