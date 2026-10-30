Cyclone 68000 execution backend
===============================

`cyclone_imp.c` is a pure C port of the PSP-MAME4ALL adapter in
`resource/PSP-MAME4ALL/src/cyclone/m68000.cpp`, adapted to AdvanceMAME's
0.106 CPU and memory interfaces. `cyclone.h` and the v0.99 generator come
from `resource/cyclone68000`. See `License.txt` for the upstream dual license.

Configure automatically sets `CONF_CYCLONE=yes` for a 32-bit little-endian
ARM compiler target. There is no configure option. This backend assumes
ARMv7-A or newer in AArch32 mode, including the Raspberry Pi 3. There is one
checked-in assembly core, generated with `HAVE_ARMv6T2=1`. Other architectures
and other 68k variants retain Musashi. Normal builds compile C and assembly;
they do not run the C++ generator.

State and data accesses
-----------------------

The Musashi context remains authoritative between timeslices. The adapter
transfers registers, SR, stack pointers, interrupt pins, and STOP/HALT state
at those boundaries. This retains the CPU interface, CPU-family context
switching, reset behavior, and save-state registrations. No host pointers,
jump tables, or mapping-generation counters are added to save files.

Memory lookup entries distinguish direct banks from handlers, including
second-level subtables. Direct RAM/ROM accesses synchronize only cycles.
Handler accesses export the full CPU state before calling driver code, so
PC/register queries and spin/yield operations continue to work. A longword
access exports at most once, then performs both big-endian word accesses
through the normal memory accessors, retaining masks and 24-bit wrapping.

The memory subsystem increments `memory_opcode_generation` when banks,
decrypted regions, opcode-base callbacks, or CPU memory contexts change.
Dynamic map installation refreshes the live memory context; deferred
`catch_nextBranch()` invalidation also increments the counter. The adapter
checks this counter after accesses and recalculates the opcode base only
when invalidated. The counter and its increments compile out when Cyclone
is disabled.

Instruction fetch and branches
------------------------------

The fast fetch path checks the safe host interval inline using scratch r12,
with no stack save, CPSR save, or function call for an ordinary fetch.
Unsafe fetches call a leaf helper which selects a synthetic exit entry after
the 65536 real opcode entries. Normal dispatch remains a direct table jump,
without a separate exit check at every instruction.

Short/word branches, DBRA and other jumps also check the interval directly
in assembly. Only a destination outside it invokes C `checkpc()`. The interval
respects bank boundaries, opcode-mask mirrors, and contiguous lookup entries,
including holes introduced by dynamic subtables. It reserves room for the
longest instruction and speculative fetch. At an edge, Musashi's safe fetch
path executes the rest of the timeslice.

Debugger builds, separate decrypted opcode/argument streams, encrypted
PC-relative reads, custom memory interfaces, and custom RESET/CMPI.L/RTE
callbacks use Musashi. Level-7 edges also use Musashi, since upstream Cyclone
treats level 7 as continuously non-maskable. Other interrupts use the original
MAME acknowledgement callback, including explicit/autovectors, spurious
vectors and HOLD/ASSERT lines.

Regeneration and verification
-----------------------------

The upstream pre-generated `Cyclone.s` disables live memory-handler cycles
and interrupt acknowledgement callbacks, so it cannot be used unchanged.
To reproduce the checked-in `cyclone.s` from the repository root:

    python3 src/cpu/cyclone/generate.py

The script builds the v0.99 reference generator in `/tmp/advancemame/` using
`cyclone_config.h`, adds inline checks and the exit entry, and leaves reference
files untouched. The output declares `.arch armv7-a` and a non-executable
stack. Aligned operand fetches remain enabled for kernels with alignment traps.

Run the standalone suite on 32-bit little-endian ARMv7-A or newer:

    sh src/cpu/cyclone/tests/run.sh

Or cross-compile and run under QEMU (static ARM libc required):

    CC=arm-linux-gnueabi-gcc RUNNER=qemu-arm sh src/cpu/cyclone/tests/run.sh

`BUILD_CC`, `CFLAGS`, and `LDFLAGS` can be supplied separately. Tests link the
actual Cyclone and Musashi cores and check execution/cycles, memory access,
handler-visible state, mirrors, bank boundaries, dynamic mappings, interrupts,
contexts, save/load continuation, and fast-path synchronization counts.
Assertions stay enabled even if CFLAGS defines NDEBUG.

For timing, use the same command with `TEST_ARGS=--bench`. This disables test
instrumentation and measures register/DBRA loops, direct memory, and handlers,
with 32 timeslices of one million emulated cycles per case. Repeat runs and
compare medians. QEMU timings measure emulator overhead on the host, rather
than Raspberry Pi throughput; use hardware measurements for that comparison.
