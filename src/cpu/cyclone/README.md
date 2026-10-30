Cyclone 68000 execution backend
===============================

`cyclone_imp.c` is a pure C port of the PSP-MAME4ALL adapter in
`resource/PSP-MAME4ALL/src/cyclone/m68000.cpp`, adapted to AdvanceMAME's
0.106 CPU and memory interfaces. `cyclone.h` and the v0.99 generator come
from `resource/cyclone68000`. See `License.txt` for the upstream dual license.

Configure automatically sets `CONF_CYCLONE=yes` for a 32-bit little-endian
ARM compiler target. There is no configure option. All other targets and
all other 68k variants retain Musashi. The normal build uses checked-in C
and ARM assembly; no C++ compiler or Cyclone generator is needed for it.

The existing Musashi context remains authoritative between timeslices.
The adapter transfers registers, SR, stack pointers, interrupt pins, and
STOP/HALT state to/from Cyclone. This retains the existing CPU interface,
CPU-family context switching, reset behavior, and save-state registrations.
No rebased PCs, jump tables, or callback pointers are added to save files.
Instruction counters and PC/register queries are synchronized before memory
handlers; cycle changes are carried back to the ARM core. Long accesses use
two big-endian word accesses with 24-bit address wrap, as in Musashi.

Opcode mapping is rebased on jumps and after memory handlers. Bounded opcode
fetches leave space for instruction extensions. At a bank edge, Musashi's safe
fetch path executes the remainder of the timeslice. Debugger builds, separate
decrypted opcode/argument streams, encrypted PC-relative reads, custom memory
interfaces, and custom RESET/CMPI.L/RTE callbacks use Musashi. Level-7 interrupt
edges also use Musashi, since upstream Cyclone treats level 7 as continuously
non-maskable. Other interrupts use the original MAME acknowledgement callback,
including explicit vectors, autovectors, spurious vectors and HOLD/ASSERT lines.

The supplied upstream pre-generated `Cyclone.s` disables live memory-handler
cycles and interrupt acknowledgement callbacks, so it cannot be used unchanged.
The checked-in assembly is regenerated from the same v0.99 sources with
`cyclone_config.h`. To reproduce it from the repository root:

    python3 src/cpu/cyclone/generate.py

The script builds the reference generator in `/tmp/advancemame/`, enables
interrupt checks after memory instructions, and adds bounded instruction-fetch
helpers. Reference files remain untouched. The generated file uses ARMv4T
instructions and supports interworking callbacks and a non-executable stack.

Run the standalone regression suite on 32-bit little-endian ARM:

    sh src/cpu/cyclone/tests/run.sh

Or cross-compile and run under QEMU (static ARM libc required):

    CC=arm-linux-gnueabi-gcc RUNNER=qemu-arm sh src/cpu/cyclone/tests/run.sh

`BUILD_CC`, `CFLAGS`, and `LDFLAGS` can be supplied separately. Tests link the
actual Cyclone and Musashi cores, compare execution and cycle results, and
exercise memory access, bank edges/switching, interrupt callbacks, STOP,
context switching, and the existing save-state registrations.
