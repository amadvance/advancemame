// license:BSD-3-Clause
// copyright-holders:Andrew Gardner, Ryan Holtz

#define LOG_INVALID             (1U << 1)
#define LOG_PERIPHERAL_READS    (1U << 2)
#define LOG_PERIPHERAL_WRITES   (1U << 3)
#define LOG_HOST_READS          (1U << 4)

void IPR_set(dsp56156_core* cpustate, UINT16 value)
{

	IPR = value;
}

INT8  irqa_ipl(dsp56156_core* cpustate)   { return ((IPR & 0x0003) >> 0) - 1;  }
UINT8 irqa_trigger(dsp56156_core* cpustate){ return  (IPR & 0x0004) >> 2;       }
INT8  irqb_ipl(dsp56156_core* cpustate)   { return ((IPR & 0x0018) >> 3) - 1;  }
UINT8 irqb_trigger(dsp56156_core* cpustate){ return  (IPR & 0x0002) >> 5;       }
INT8  codec_ipl(dsp56156_core* cpustate)  { return ((IPR & 0x00c0) >> 6) - 1;  }
INT8  host_ipl(dsp56156_core* cpustate)   { return ((IPR & 0x0300) >> 8) - 1;  }
INT8  ssi0_ipl(dsp56156_core* cpustate)   { return ((IPR & 0x0c00) >> 10) - 1; }
INT8  ssi1_ipl(dsp56156_core* cpustate)   { return ((IPR & 0x3000) >> 12) - 1; }
INT8  tm_ipl(dsp56156_core* cpustate)     { return ((IPR & 0xc000) >> 14) - 1; }

void mem_reset(dsp56156_core* cpustate)
{

	dsp56156_host_interface_reset(cpustate);

	dsp56156_io_reset(cpustate);
}

void HCR_set(dsp56156_core* cpustate, UINT16 value)
{
	LOG("%s: HCR_set: %02x\n", "DSP56156", value);
	HF3_bit_set (cpustate, (value & 0x0010) >> 4);
	HF2_bit_set (cpustate, (value & 0x0008) >> 3);
	HCIE_bit_set(cpustate, (value & 0x0004) >> 2);
	HTIE_bit_set(cpustate, (value & 0x0002) >> 1);
	HRIE_bit_set(cpustate, (value & 0x0001) >> 0);
}

UINT16 HCIE_bit(dsp56156_core* cpustate) { return ((HCR & 0x0004) != 0); }
UINT16 HTIE_bit(dsp56156_core* cpustate) { return ((HCR & 0x0002) != 0); }
UINT16 HRIE_bit(dsp56156_core* cpustate) { return ((HCR & 0x0001) != 0); }

void HF3_bit_set(dsp56156_core* cpustate, UINT16 value)
{
	value = value & 0x01;
	HCR &= ~(0x0010);
	HCR |=  (value << 4);

	LOG("%s: HF3_bit_set: %d\n", "DSP56156", value);
	HF3_bit_host_set(cpustate, value);
}
void HF2_bit_set(dsp56156_core* cpustate, UINT16 value)
{
	value = value & 0x01;
	HCR &= ~(0x0008);
	HCR |=  (value << 3);

	LOG("%s: HF2_bit_set: %d\n", "DSP56156", value);
	HF2_bit_host_set(cpustate, value);
}
void HCIE_bit_set(dsp56156_core* cpustate, UINT16 value)
{
	value = value & 0x01;
	HCR &= ~(0x0004);
	HCR |=  (value << 2);
}
void HTIE_bit_set(dsp56156_core* cpustate, UINT16 value)
{
	value = value & 0x01;
	HCR &= ~(0x0002);
	HCR |=  (value << 1);
}
void HRIE_bit_set(dsp56156_core* cpustate, UINT16 value)
{
	value = value & 0x01;
	HCR &= ~(0x0001);
	HCR |=  (value << 0);
}

UINT16 HTDE_bit(dsp56156_core* cpustate) { return ((HSR & 0x0002) != 0); }
UINT16 HRDF_bit(dsp56156_core* cpustate) { return ((HSR & 0x0001) != 0); }

void DMA_bit_set(dsp56156_core* cpustate, UINT16 value)
{
	value = value & 0x01;
	HSR &= ~(0x0080);
	HSR |= (value << 7);

}
void HF1_bit_set(dsp56156_core* cpustate, UINT16 value)
{
	value = value & 0x01;
	HSR &= ~(0x0010);
	HSR |= (value << 4);
}
void HF0_bit_set(dsp56156_core* cpustate, UINT16 value)
{
	value = value & 0x01;
	HSR &= ~(0x0008);
	HSR |= (value << 3);
}
void HCP_bit_set(dsp56156_core* cpustate, UINT16 value)
{
	value = value & 0x01;
	HSR &= ~(0x0004);
	HSR |=  (value << 2);

	if (value && HCIE_bit(cpustate))
		dsp56156_add_pending_interrupt(cpustate, "Host Command");
}
void HTDE_bit_set(dsp56156_core* cpustate, UINT16 value)
{
	value = value & 0x01;
	HSR &= ~(0x0002);
	HSR |=  (value << 1);

	if (value && HTIE_bit(cpustate))
		dsp56156_add_pending_interrupt(cpustate, "Host Transmit Data");

	if (!value && !RXDF_bit(cpustate))
		dsp56156_host_interface_HTX_to_host(cpustate);
}
void HRDF_bit_set(dsp56156_core* cpustate, UINT16 value)
{
	value = value & 0x01;
	HSR &= ~(0x0001);
	HSR |=  (value << 0);

	if (value && HRIE_bit(cpustate))
		dsp56156_add_pending_interrupt(cpustate, "Host Receive Data");

	if (!value && !TXDE_bit(cpustate))
		dsp56156_host_interface_host_to_HTX(cpustate);
}

void ICR_set(dsp56156_core* cpustate, UINT8 value)
{
	HF1_bit_host_set(cpustate, (value & 0x10) >> 4);
	HF0_bit_host_set(cpustate, (value & 0x08) >> 3);
	TREQ_bit_set(cpustate, (value & 0x02) >> 1);
	RREQ_bit_set(cpustate, (value & 0x01) >> 0);
}

void HF1_bit_host_set(dsp56156_core* cpustate, UINT8 value)
{
	value = value & 0x01;
	ICR &= ~(0x10);
	ICR |=  (value << 4);

	HF1_bit_set(cpustate, value);
}
void HF0_bit_host_set(dsp56156_core* cpustate, UINT8 value)
{
	value = value & 0x01;
	ICR &= ~(0x08);
	ICR |=  (value << 3);

	HF0_bit_set(cpustate, value);
}
void TREQ_bit_set(dsp56156_core* cpustate, UINT8 value)
{
	value = value & 0x01;
	ICR &= ~(0x02);
	ICR |=  (value << 1);
}
void RREQ_bit_set(dsp56156_core* cpustate, UINT8 value)
{
	value = value & 0x01;
	ICR &= ~(0x01);
	ICR |=  (value << 0);

	if (value)
	{

	}
}

UINT8 HV_bits(dsp56156_core* cpustate) { return (CVR & 0x1f); }

void CVR_set(dsp56156_core* cpustate, UINT8 value)
{

	HC_bit_set(cpustate, (value & 0x80) >> 7);
	HV_bits_set(cpustate, (value & 0x1f));
}

void HC_bit_set(dsp56156_core* cpustate, UINT8 value)
{
	value = value & 0x01;
	CVR &= ~(0x80);
	CVR |=  (value << 7);

	HCP_bit_set(cpustate, value);
}
void HV_bits_set(dsp56156_core* cpustate, UINT8 value)
{
	value = value & 0x1f;
	CVR &= ~(0x1f);
	CVR |=  (value << 0);
}

UINT8 TXDE_bit(dsp56156_core* cpustate) { return ((ISR & 0x0002) != 0); }
UINT8 RXDF_bit(dsp56156_core* cpustate) { return ((ISR & 0x0001) != 0); }

void HF3_bit_host_set(dsp56156_core* cpustate, UINT8 value)
{
	value = value & 0x01;
	ISR &= ~(0x0010);
	ISR |=  (value << 4);
}
void HF2_bit_host_set(dsp56156_core* cpustate, UINT8 value)
{
	value = value & 0x01;
	ISR &= ~(0x0008);
	ISR |=  (value << 3);
}

void TXDE_bit_set(dsp56156_core* cpustate, UINT8 value)
{
	value = value & 0x01;
	ISR &= ~(0x0002);
	ISR |=  (value << 1);

	if (!value && !HRDF_bit(cpustate))
		dsp56156_host_interface_host_to_HTX(cpustate);
}

void RXDF_bit_set(dsp56156_core* cpustate, UINT8 value)
{
	value = value & 0x01;
	ISR &= ~(0x0001);
	ISR |=  (value << 0);

	if (!value && !HTDE_bit(cpustate))
		dsp56156_host_interface_HTX_to_host(cpustate);
}

void dsp56156_host_interface_reset(dsp56156_core* cpustate)
{

	cpustate->HI.hcr = &cpustate->peripheral_ram[A2O(0xffc4)];
	cpustate->HI.hsr = &cpustate->peripheral_ram[A2O(0xffe4)];
	cpustate->HI.htrx = &cpustate->peripheral_ram[A2O(0xffe5)];

	cpustate->HI.bootstrap_offset = 0x0000;

	HCR_set(cpustate, 0x0000);

	HRDF_bit_set(cpustate, 0);
	HTDE_bit_set(cpustate, 1);
	HCP_bit_set(cpustate, 0);
	HF0_bit_set(cpustate, 0);
	HF1_bit_set(cpustate, 0);
	DMA_bit_set(cpustate, 0);

	HV_bits_set(cpustate, 0x16);
	HC_bit_set(cpustate, 0);

}

void dsp56156_host_interface_HTX_to_host(dsp56156_core* cpustate)
{
	RXH = ((HTX & 0xff00) >> 8);
	RXL = ((HTX & 0x00ff));
	RXDF_bit_set(cpustate, 1);
	HTDE_bit_set(cpustate, 1);
}

void dsp56156_host_interface_host_to_HTX(dsp56156_core* cpustate)
{
	HRX &= 0x00ff;
	HRX |= (TXH << 8);
	HRX &= 0xff00;
	HRX |= TXL;
	TXDE_bit_set(cpustate, 1);
	HRDF_bit_set(cpustate, 1);
}

void BCR_set(dsp56156_core* cpustate, UINT16 value)
{
	RH_bit_set(cpustate, (value & 0x8000) >> 15);
	BS_bit_set(cpustate, (value & 0x4000) >> 14);
	external_x_wait_states_set(cpustate, (value & 0x03e0) >> 5);
	external_p_wait_states_set(cpustate, (value & 0x001f) >> 0);
}

void RH_bit_set(dsp56156_core* cpustate, UINT16 value)
{
	value = value & 0x0001;
	BCR &= ~(0x8000);
	BCR |= (value << 15);

}
void BS_bit_set(dsp56156_core* cpustate, UINT16 value)
{
	value = value & 0x0001;
	BCR &= ~(0x4000);
	BCR |= (value << 14);

}
void external_x_wait_states_set(dsp56156_core* cpustate, UINT16 value)
{
	value = value & 0x001f;
	BCR &= ~(0x03e0);
	BCR |= (value << 5);
}
void external_p_wait_states_set(dsp56156_core* cpustate, UINT16 value)
{
	value = value & 0x001f;
	BCR &= ~(0x001f);
	BCR |= (value << 0);
}

void PBC_set(dsp56156_core* cpustate, UINT16 value)
{
	if (value & 0xfffe)
	{
		LOGMASKED(LOG_INVALID, "%s: Attempting to set reserved bits in PBC. Ignoring.\n", "DSP56156");
	}

	value = value & 0x0001;
	PBC &= ~(0x0001);
	PBC |= (value << 0);
}

#ifdef UNUSED_FUNCTION
int host_interface_active(dsp56156_core* cpustate)
{

	return PBC & 0x0001;
}
#endif

void PBDDR_set(dsp56156_core* cpustate, UINT16 value)
{
	if (value & 0x8000)
	{
		LOGMASKED(LOG_INVALID, "%s: Attempting to set reserved bits in PBDDR. Ignoring.\n", "DSP56156");
	}

	value = value & 0x7fff;
	PBDDR &= ~(0x7fff);
	PBDDR |= (value << 0);

}

void PBD_set(dsp56156_core* cpustate, UINT16 value)
{
	if (value & 0x8000)
	{
		LOGMASKED(LOG_INVALID, "%s: Attempting to set reserved bits in PBD. Ignoring.\n", "DSP56156");
	}

	value = value & 0x7fff;
	PBD &= ~(0x7fff);
	PBD |= (value << 0);

}

void PCC_set(dsp56156_core* cpustate, UINT16 value)
{
	if (value & 0xf000)
	{
		LOGMASKED(LOG_INVALID, "%s: Attempting to set reserved bits in PCC. Ignoring.\n", "DSP56156");
	}

	value = value & 0x0fff;
	PCC &= ~(0x0fff);
	PCC |= (value << 0);

}

void PCDDR_set(dsp56156_core* cpustate, UINT16 value)
{
	if (value & 0xf000)
	{
		LOGMASKED(LOG_INVALID, "%s: Attempting to set reserved bits in PCDDR. Ignoring.\n", "DSP56156");
	}

	value = value & 0x0fff;
	PCDDR &= ~(0x0fff);
	PCDDR |= (value << 0);

}

void PCD_set(dsp56156_core* cpustate, UINT16 value)
{
	if (value & 0xf000)
	{
		LOGMASKED(LOG_INVALID, "%s: Attempting to set reserved bits in PCD. Ignoring.\n", "DSP56156");
	}

	value = value & 0x0fff;
	PCD &= ~(0x0fff);
	PCD |= (value << 0);

	if (cpustate->portc) cpustate->portc(PCD);
}

void dsp56156_io_reset(dsp56156_core* cpustate)
{

	RH_bit_set(cpustate, 0);
	BS_bit_set(cpustate, 1);
	external_x_wait_states_set(cpustate, 0x1f);
	external_p_wait_states_set(cpustate, 0x1f);
}

READ16_HANDLER( dsp56k_peripheral_r )
{
	dsp56156_core* cpustate = &dsp56k;
	LOGMASKED(LOG_PERIPHERAL_READS, "%s: Peripheral read %04x\n", "DSP56156", O2A(offset));

	switch (O2A(offset))
	{

		case 0xffc0: break;

		case 0xffc1: break;

		case 0xffc2: break;

		case 0xffc3: break;

		case 0xffc4: break;

		case 0xffc8: break;

		case 0xffc9: break;

		case 0xffd0: break;

		case 0xffd1: break;

		case 0xffd8: break;

		case 0xffd9: break;

		case 0xffdc: break;

		case 0xffdd: break;

		case 0xffde: break;

		case 0xffdf: break;

		case 0xffe2: break;

		case 0xffe3: break;

		case 0xffe4: break;

		case 0xffe5:

			if (!HRDF_bit(cpustate))
				return 0xbeef;
			else
			{
				UINT16 value = HRX;
				HRDF_bit_set(cpustate, 0);
				return value;
			}

		case 0xffe8: break;

		case 0xffe9: break;

		case 0xffec: break;

		case 0xffed: break;

		case 0xffee: break;

		case 0xffef: break;

		case 0xfff0: break;

		case 0xfff1: break;

		case 0xfff2: break;

		case 0xfff3: break;

		case 0xfff4: break;

		case 0xfff5: break;

		case 0xfff8: break;

		case 0xfff9: break;

		case 0xfffa: break;

		case 0xfffb: break;

		case 0xfffc: break;

		case 0xfffd: break;

		case 0xffff: break;
	}

	return cpustate->peripheral_ram[offset];
}

WRITE16_HANDLER( dsp56k_peripheral_w )
{
	dsp56156_core* cpustate = &dsp56k;

	LOGMASKED(LOG_PERIPHERAL_WRITES, "%s: Peripheral write %04x = %04x\n", "DSP56156", O2A(offset), data);

	switch (O2A(offset))
	{

		case 0xffc0:
			PBC_set(cpustate, data);
			break;

		case 0xffc1:
			PCC_set(cpustate, data);
			break;

		case 0xffc2:
			PBDDR_set(cpustate, data);
			break;

		case 0xffc3:
			PCDDR_set(cpustate, data);
			break;

		case 0xffc4:
			HCR_set(cpustate, data);
			break;

		case 0xffc8: break;

		case 0xffc9:
			LOGMASKED(LOG_INVALID, "%s: Writing to register 0xffc9 is reserved for test purposes only. Ignoring.\n", "DSP56156");
			break;

		case 0xffd0: break;

		case 0xffd1: break;

		case 0xffd8: break;

		case 0xffd9: break;

		case 0xffdc: break;

		case 0xffdd:
			LOGMASKED(LOG_INVALID, "%s: Writing to register 0xffdd is reserved for future chipset implementations. Ignoring.\n", "DSP56156");
			break;

		case 0xffde:
			BCR_set(cpustate, data);
			break;

		case 0xffdf:
			IPR_set(cpustate, data);
			break;

		case 0xffe2:
			PBD_set(cpustate, data);
			break;

		case 0xffe3:
			PCD_set(cpustate, data);
			break;

		case 0xffe4: break;

		case 0xffe5:
			HTX = data;
			HTDE_bit_set(cpustate, 0);
			break;

		case 0xffe8: break;

		case 0xffe9: break;

		case 0xffec: break;

		case 0xffed: break;

		case 0xffee: break;

		case 0xffef: break;

		case 0xfff0: break;

		case 0xfff1: break;

		case 0xfff2: break;

		case 0xfff3: break;

		case 0xfff4: break;

		case 0xfff5: break;

		case 0xfff8: break;

		case 0xfff9: break;

		case 0xfffa: break;

		case 0xfffb: break;

		case 0xfffc: break;

		case 0xfffd: break;

		case 0xffff:
			LOGMASKED(LOG_INVALID, "%s: Writing to register 0xffff is reserved for on-chip emulation features. Ignoring.\n", "DSP56156");
			break;
	}
}

static void host_interface_write(UINT8 offset, UINT8 data)
{
	dsp56156_core* cpustate = &dsp56k;

	switch (offset)
	{

		case 0x00:

			if (cpustate->bootstrap_mode == BOOTSTRAP_HI)
			{

				if (data & 0x08)
				{
					cpustate->bootstrap_mode = BOOTSTRAP_OFF;
					PC = 0x0000;

					break;
				}
			}
			ICR_set(cpustate, data);
			break;

		case 0x01:
			CVR_set(cpustate, data);
			break;

		case 0x02:
			LOGMASKED(LOG_INVALID, "%s: Attempting to write read-only Interrupt Status Register. Ignoring.\n", "DSP56156");
			break;

		case 0x03: break;

		case 0x04:
			LOGMASKED(LOG_INVALID, "%s: Attempting to write unused host-side register 0x4. Ignoring.\n", "DSP56156");
			break;

		case 0x05:
			LOGMASKED(LOG_INVALID, "%s: Attempting to write reserved host-side register 0x5. Ignoring.\n", "DSP56156");
			break;

		case 0x06:

			if (cpustate->bootstrap_mode == BOOTSTRAP_HI)
			{
				cpustate->program_ram[cpustate->HI.bootstrap_offset] &= 0x00ff;
				cpustate->program_ram[cpustate->HI.bootstrap_offset] |= (data << 8);
				break;
			}

			if (TXDE_bit(cpustate))
			{
				TXH = data;
			}
			break;

		case 0x07:

			if (cpustate->bootstrap_mode == BOOTSTRAP_HI)
			{
				cpustate->program_ram[cpustate->HI.bootstrap_offset] &= 0xff00;
				cpustate->program_ram[cpustate->HI.bootstrap_offset] |= data;
				cpustate->HI.bootstrap_offset++;

				if (cpustate->HI.bootstrap_offset == 0x800)
				{
					cpustate->bootstrap_mode = BOOTSTRAP_OFF;
				}
				break;
			}

			if (TXDE_bit(cpustate))
			{
				TXL = data;
				TXDE_bit_set(cpustate, 0);
			}
			break;

		default:
			LOGMASKED(LOG_INVALID, "%s: Attempting to write invalid host-side register 0x%x. Ignoring.\n", "DSP56156", offset);
			break;
	}
}

static UINT8 host_interface_read(UINT8 offset)
{
	dsp56156_core* cpustate = &dsp56k;

	switch (offset)
	{

		case 0x00:
			LOGMASKED(LOG_HOST_READS, "%s: Returning Interrupt Control Register (ICR): %02x.\n", "DSP56156", ICR);
			return ICR;

		case 0x01:
			LOGMASKED(LOG_HOST_READS, "%s: Returning Command Vector Register (CVR): %02x.\n", "DSP56156", CVR);
			return CVR;

		case 0x02:
			LOGMASKED(LOG_HOST_READS, "%s: Returning Interrupt Status Register (ISR): %02x.\n", "DSP56156", ISR);
			return ISR;

		case 0x03:
			LOGMASKED(LOG_HOST_READS, "%s: Returning Interrupt Vector2 Register (IVR): %02x.\n", "DSP56156", IVR);
			return IVR;

		case 0x04:
			return 0x00;

		case 0x05:
			LOGMASKED(LOG_INVALID, "%s: Attempting to read reserved host-side register 0x5. Ignoring.\n", "DSP56156");
			break;

		case 0x06:

			if (!RXDF_bit(cpustate))
				return 0xbf;
			else
				return RXH;

		case 0x07:

			if (!RXDF_bit(cpustate))
				return 0xbf;
			else
			{
				UINT8 value = RXL;
				RXDF_bit_set(cpustate, 0);
				return value;
			}

		default:
			LOGMASKED(LOG_INVALID, "%s: Attempting to read invalid host-side register 0x%x. Ignoring.\n", "DSP56156", offset);
			break;
	}

	return 0xff;
}
