// license:BSD-3-Clause
// copyright-holders:Luca Elia, David Haywood, Angelo Salese, Roberto Fresca
#ifndef SUBSINO_CRYPT_H
#define SUBSINO_CRYPT_H

extern const UINT8 crsbingo_xors[8];
extern const UINT8 sharkpy_xors[8];
extern const UINT8 victor5_xors[8];

void crsbingo_bitswaps(UINT8 *decrypt, int i);
void sharkpy_bitswaps(UINT8 *decrypt, int i);
void victor5_bitswaps(UINT8 *decrypt, int i);

void subsino_decrypt(UINT8 *region, void (*bitswaps)(UINT8 *decrypt, int i), const UINT8 *xors, int size);

#endif
