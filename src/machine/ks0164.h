// license:BSD-3-Clause
// copyright-holders:Olivier Galibert, David Carne

// Samsung KS0164 wavetable chip
// KS0164 core

#ifndef ADV_KS0164_H
#define ADV_KS0164_H
extern const struct CustomSound_interface ks0164_interface;
READ32_HANDLER(ks0164_data_r);
READ32_HANDLER(ks0164_status_r);
WRITE32_HANDLER(ks0164_data_w);
WRITE32_HANDLER(ks0164_ctrl_w);
#endif
