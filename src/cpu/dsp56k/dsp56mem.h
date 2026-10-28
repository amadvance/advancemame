// license:BSD-3-Clause
// copyright-holders:Andrew Gardner, Ryan Holtz

#ifndef MAME_CPU_DSP56156_DSP56MEM_H
#define MAME_CPU_DSP56156_DSP56MEM_H

void mem_reset(dsp56156_core* cpustate);

#define A2O(a) (a - 0xffc0)

#define O2A(a) (a + 0xffc0)

#define PBC      (cpustate->peripheral_ram[A2O(0xffc0)])
#define PCC      (cpustate->peripheral_ram[A2O(0xffc1)])
#define PBDDR    (cpustate->peripheral_ram[A2O(0xffc2)])
#define PCDDR    (cpustate->peripheral_ram[A2O(0xffc3)])
#define HCR      (cpustate->peripheral_ram[A2O(0xffc4)])
#define COCR     (cpustate->peripheral_ram[A2O(0xffc8)])
#define CRASSI0  (cpustate->peripheral_ram[A2O(0xffd0)])
#define CRBSSI0  (cpustate->peripheral_ram[A2O(0xffd1)])
#define CRASSI1  (cpustate->peripheral_ram[A2O(0xffd8)])
#define CRBSSI1  (cpustate->peripheral_ram[A2O(0xffd9)])
#define PLCR     (cpustate->peripheral_ram[A2O(0xffdc)])
#define BCR      (cpustate->peripheral_ram[A2O(0xffde)])
#define IPR      (cpustate->peripheral_ram[A2O(0xffdf)])
#define PBD      (cpustate->peripheral_ram[A2O(0xffe2)])
#define PCD      (cpustate->peripheral_ram[A2O(0xffe3)])
#define HSR      (cpustate->peripheral_ram[A2O(0xffe4)])
#define HTXHRX   (cpustate->peripheral_ram[A2O(0xffe5)])
#define COSR     (cpustate->peripheral_ram[A2O(0xffe8)])
#define CRXCTX   (cpustate->peripheral_ram[A2O(0xffe9)])
#define TCR      (cpustate->peripheral_ram[A2O(0xffec)])
#define TCTR     (cpustate->peripheral_ram[A2O(0xffed)])
#define TCPR     (cpustate->peripheral_ram[A2O(0xffee)])
#define TPR      (cpustate->peripheral_ram[A2O(0xffef)])
#define TSRSSI0  (cpustate->peripheral_ram[A2O(0xfff0)])
#define TRXSSI0  (cpustate->peripheral_ram[A2O(0xfff1)])
#define RSMA0    (cpustate->peripheral_ram[A2O(0xfff2)])
#define RSMB0    (cpustate->peripheral_ram[A2O(0xfff3)])
#define TSMA0    (cpustate->peripheral_ram[A2O(0xfff4)])
#define TSMB0    (cpustate->peripheral_ram[A2O(0xfff5)])
#define TSRSSI1  (cpustate->peripheral_ram[A2O(0xfff8)])
#define TRXSSI1  (cpustate->peripheral_ram[A2O(0xfff9)])
#define RSMA1    (cpustate->peripheral_ram[A2O(0xfffa)])
#define RSMB1    (cpustate->peripheral_ram[A2O(0xfffb)])
#define TSMA1    (cpustate->peripheral_ram[A2O(0xfffc)])
#define TSMB1    (cpustate->peripheral_ram[A2O(0xfffd)])

void IPR_set(dsp56156_core* cpustate, UINT16 value);

INT8 irqa_ipl(dsp56156_core* cpustate);
INT8 irqb_ipl(dsp56156_core* cpustate);
UINT8 irqa_trigger(dsp56156_core* cpustate);
UINT8 irqb_trigger(dsp56156_core* cpustate);

INT8 codec_ipl(dsp56156_core* cpustate);
INT8 host_ipl(dsp56156_core* cpustate);
INT8 ssi0_ipl(dsp56156_core* cpustate);
INT8 ssi1_ipl(dsp56156_core* cpustate);
INT8 tm_ipl(dsp56156_core* cpustate);

void dsp56156_host_interface_reset(dsp56156_core* cpustate);
#define HTX (HTXHRX)
#define HRX (HTXHRX)

#define ICR (cpustate->HI.icr)
#define CVR (cpustate->HI.cvr)
#define ISR (cpustate->HI.isr)
#define IVR (cpustate->HI.ivr)
#define TXH (cpustate->HI.trxh)
#define TXL (cpustate->HI.trxl)
#define RXH (cpustate->HI.trxh)
#define RXL (cpustate->HI.trxl)

void HCR_set(dsp56156_core* cpustate, UINT16 value);

UINT16 HCIE_bit(dsp56156_core* cpustate);
UINT16 HTIE_bit(dsp56156_core* cpustate);
UINT16 HRIE_bit(dsp56156_core* cpustate);

void HF3_bit_set(dsp56156_core* cpustate, UINT16 value);
void HF2_bit_set(dsp56156_core* cpustate, UINT16 value);
void HCIE_bit_set(dsp56156_core* cpustate, UINT16 value);
void HTIE_bit_set(dsp56156_core* cpustate, UINT16 value);
void HRIE_bit_set(dsp56156_core* cpustate, UINT16 value);

UINT16 HTDE_bit(dsp56156_core* cpustate);
UINT16 HRDF_bit(dsp56156_core* cpustate);

void DMA_bit_set(dsp56156_core* cpustate, UINT16 value);
void HF1_bit_set(dsp56156_core* cpustate, UINT16 value);
void HF0_bit_set(dsp56156_core* cpustate, UINT16 value);
void HCP_bit_set(dsp56156_core* cpustate, UINT16 value);
void HTDE_bit_set(dsp56156_core* cpustate, UINT16 value);
void HRDF_bit_set(dsp56156_core* cpustate, UINT16 value);

void ICR_set(dsp56156_core* cpustate, UINT8 value);

void HF1_bit_host_set(dsp56156_core* cpustate, UINT8 value);
void HF0_bit_host_set(dsp56156_core* cpustate, UINT8 value);
void TREQ_bit_set(dsp56156_core* cpustate, UINT8 value);
void RREQ_bit_set(dsp56156_core* cpustate, UINT8 value);

void CVR_set(dsp56156_core* cpustate, UINT8 value);

UINT8 HV_bits(dsp56156_core* cpustate);

void HC_bit_set(dsp56156_core* cpustate, UINT8 value);
void HV_bits_set(dsp56156_core* cpustate, UINT8 value);

UINT8 TXDE_bit(dsp56156_core* cpustate);
UINT8 RXDF_bit(dsp56156_core* cpustate);

void HF3_bit_host_set(dsp56156_core* cpustate, UINT8 value);
void HF2_bit_host_set(dsp56156_core* cpustate, UINT8 value);

void TXDE_bit_set(dsp56156_core* cpustate, UINT8 value);
void RXDF_bit_set(dsp56156_core* cpustate, UINT8 value);

void dsp56156_host_interface_HTX_to_host(dsp56156_core* cpustate);
void dsp56156_host_interface_host_to_HTX(dsp56156_core* cpustate);

void dsp56156_io_reset(dsp56156_core* cpustate);

void BCR_set(dsp56156_core* cpustate, UINT16 value);

void RH_bit_set(dsp56156_core* cpustate, UINT16 value);
void BS_bit_set(dsp56156_core* cpustate, UINT16 value);
void external_x_wait_states_set(dsp56156_core* cpustate, UINT16 value);
void external_p_wait_states_set(dsp56156_core* cpustate, UINT16 value);

void PBC_set(dsp56156_core* cpustate, UINT16 value);

void PBDDR_set(dsp56156_core* cpustate, UINT16 value);

void PBD_set(dsp56156_core* cpustate, UINT16 value);

void PCC_set(dsp56156_core* cpustate, UINT16 value);

void PCDDR_set(dsp56156_core* cpustate, UINT16 value);

void PCD_set(dsp56156_core* cpustate, UINT16 value);

#endif
