#!/usr/bin/env python3
# license:GPL-2.0-only
# copyright-holders:FinalDave, Gražvydas Ignotas
"""Regenerate the checked-in ARMv7-A core from the v0.99 reference.

Normal builds do not run this C++ generator. Reference files remain untouched.
"""
from pathlib import Path
import shutil
import subprocess
import tempfile

here = Path(__file__).resolve().parent
reference = here.parents[2] / 'resource' / 'cyclone68000'
Path('/tmp/advancemame').mkdir(exist_ok=True)

# Every fetch site in v0.99 is followed by TST/SUBS/CMP, before any conditional
# instruction consumes CPSR. r12 is dead there; r0/r1 can still be live. Keep
# the usual fetch and dispatch inline, and call a leaf helper only at an edge.
fetch = '''  ldr r12,[r7,#0xa4] ;@ safe host fetch interval
  cmp r4,r12
  ldrhs r12,[r7,#0xa8]
  cmphs r12,r4
  ldrhsh r8,[r4],#2
  bllo CycloneMameFetchSlow'''
helper = '''
;@ AdvanceMAME: only unsafe fetches come here; r0/r1 and CPSR are dead/preserved.
CycloneMameFetchSlow:
  mov r12,#1
  str r12,[r7,#0xac] ;@ request Musashi for the rest of the timeslice
  mov r8,#0x10000 ;@ extra dispatch entry, outside the 16-bit opcode space
  add r4,r4,#2
  bx lr

'''
branch = '''#if USE_CHECKPC_CALLBACK
  // All CheckPc callers allow r0-r3 to be clobbered. In-range targets keep
  // the existing base; only a bank/mirror boundary needs the C callback.
  ot("  add r0,r%d,r%d%s ;@ r0 = New PC\\n",r_base,r_ofs,s_ofs);
  ot("  ldr r3,[r7,#0xa4]\\n");
  ot("  cmp r0,r3\\n");
  ot("  ldrhs r3,[r7,#0xa8]\\n");
  ot("  cmphs r3,r0\\n");
  ot("  ldrlo r3,[r7,#0x64] ;@ slow checkpc handler\\n");
#if HAVE_ARMv5
  ot("  blxlo r3\\n");
#else
  ot("  movlo lr,pc\\n");
  ot("  bxlo r3\\n");
#endif
  if (writeback)
    ot("  mov r4,r0\\n");
'''

with tempfile.TemporaryDirectory(prefix='cyclone-', dir='/tmp/advancemame') as work:
	work = Path(work)
	for source in reference.glob('*.cpp'):
		shutil.copy(source, work)
	for name in ('app.h', 'Cyclone.h', 'Makefile'):
		shutil.copy(reference / name, work)
	shutil.copytree(reference / 'Disa', work / 'Disa')
	shutil.copy(here / 'cyclone_config.h', work / 'config.h')
	source = work / 'OpAny.cpp'
	source.write_text(source.read_text().replace('int did_fetch=0;', '''int did_fetch=0;
  // MAME handlers may assert an interrupt during any memory access.
  if ((sea >= 0x10 && sea != 0x3c) || (tea >= 0x10 && tea != 0x3c))
    opend_check_interrupt = 1;'''))
	source = work / 'OpBranch.cpp'
	text = source.read_text()
	start = text.index('#if USE_CHECKPC_CALLBACK', text.index('void CheckPc('))
	end = text.index('#else\n  int r_out=', start)
	source.write_text(text[:start] + branch + text[end:])

	subprocess.run(['make', '-B', 'Cyclone.s'], cwd=work, check=True)
	assembly = (work / 'Cyclone.s').read_text()
	assembly = assembly.replace('  .text\n', '  .text\n  .arm\n  .arch armv7-a\n', 1)
	assembly = assembly.replace('  ldrh r8,[r4],#2', fetch)
	assembly = assembly.replace('CycloneInitJT:\n', helper + 'CycloneInitJT:\n', 1)
	# The decompressor fills only 65536 real opcodes. Its reserved padding
	# holds one additional exit entry, so dispatch needs no separate test.
	needle = '  ldmfd sp!,{r7,pc}\n  .ltorg'
	assert assembly.count(needle) == 1
	assembly = assembly.replace(needle, '''  adr r0,CycloneEnd
  add r1,r7,#0x40000
  str r0,[r1] ;@ synthetic opcode 0x10000 exits at an instruction boundary
''' + needle, 1)
	assembly += '''\n.if (. - CycloneJumpTab) < 0x40004
  .space 0x40004 - (. - CycloneJumpTab)
.endif
.section .note.GNU-stack,"",%progbits
'''
	(here / 'cyclone.s').write_text(assembly)
