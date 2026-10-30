#!/usr/bin/env python3
# license:GPL-2.0-only
# copyright-holders:FinalDave, Gražvydas Ignotas
"""Regenerate the checked-in v0.99 core; never needed by normal builds.

Run from the repository root. The reference generator is copied to /tmp so
resource/cyclone68000 and its pre-generated Cyclone.s remain untouched.
"""
from pathlib import Path
import shutil
import subprocess
import tempfile

here = Path(__file__).resolve().parent
reference = here.parents[2] / 'resource' / 'cyclone68000'
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
	subprocess.run(['make', '-B', 'Cyclone.s'], cwd=work, check=True)
	assembly = (work / 'Cyclone.s').read_text()

# Reserve internal[0:2] for the safe host fetch interval, and internal[2] for
# a request to finish at the next instruction boundary. The slow case gives
# Musashi the rest of the timeslice at bank edges/unmapped PCs. Extension words can then
# use Cyclone's native loads without crossing the bank (max instruction: 10B).
# Preserve CPSR and all scratch registers: some fetch sites have live values.
assembly = assembly.replace('  .text\n', '  .text\n  .arm\n', 1)
assembly = assembly.replace('  ldrh r8,[r4],#2', '  bl CycloneMameFetch')
import re
assembly = re.sub(r'  ldr(eq|le|gt)? pc,\[r6,r8,asl #2\]',
				  lambda m: '  b' + (m[1] or '') + ' CycloneMameDispatch', assembly)
helper = '''
;@ AdvanceMAME: bounded instruction fetch, see generate.py.
CycloneMameFetch:
  stmfd sp!,{r0,r1,r12,lr}
  mrs r12,cpsr
  ldr r0,[r7,#0xa4]
  cmp r4,r0
  blo CycloneMameFetchSlow
  ldr r0,[r7,#0xa8]
  cmp r4,r0
  bhi CycloneMameFetchSlow
  ldrh r8,[r4],#2
  msr cpsr_f,r12
  ldmfd sp!,{r0,r1,r12,pc}
CycloneMameFetchSlow:
  mov r0,#1
  str r0,[r7,#0xac]
  mov r8,#0
  add r4,r4,#2
  msr cpsr_f,r12
  ldmfd sp!,{r0,r1,r12,pc}
CycloneMameDispatch:
  ldr r12,[r7,#0xac]
  tst r12,r12
  bne CycloneEnd
  ldr pc,[r6,r8,asl #2]

'''
assembly = assembly.replace('CycloneInitJT:\n', helper + 'CycloneInitJT:\n', 1)
assembly += '\n.section .note.GNU-stack,"",%progbits\n'
(here / 'cyclone.s').write_text(assembly)
