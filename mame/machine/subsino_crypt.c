// license:BSD-3-Clause
// copyright-holders:Luca Elia, David Haywood, Angelo Salese, Roberto Fresca
/***************************************************************************

Subsino XOR + Bitswap Encryption
by David Haywood

***************************************************************************/

#include "driver.h"
#include "subsino_crypt.h"

/* XORs */

const UINT8 crsbingo_xors[8] = { 0xbb, 0xcc, 0xcc, 0xdd, 0xaa, 0x11, 0x44, 0xee };
const UINT8 sharkpy_xors[8] = { 0xcc, 0xaa, 0x66, 0xaa, 0xee, 0x33, 0xff, 0xff };
const UINT8 victor5_xors[8] = { 0x99, 0x99, 0x33, 0x44, 0xbb, 0x88, 0x88, 0xbb };

/* Bitswaps */

void crsbingo_bitswaps(UINT8 *decrypt, int i)
{
	if ((i & 7) == 0) decrypt[i] = BITSWAP8(decrypt[i], 7, 2, 5, 4, 3, 6, 1, 0);
	if ((i & 7) == 1) decrypt[i] = BITSWAP8(decrypt[i], 7, 2, 1, 0, 3, 6, 5, 4);
	if ((i & 7) == 2) decrypt[i] = BITSWAP8(decrypt[i], 3, 2, 5, 0, 7, 6, 1, 4);
	if ((i & 7) == 3) decrypt[i] = BITSWAP8(decrypt[i], 7, 2, 5, 0, 3, 6, 1, 4);
	if ((i & 7) == 4) decrypt[i] = BITSWAP8(decrypt[i], 7, 6, 5, 0, 3, 2, 1, 4);
	if ((i & 7) == 5) decrypt[i] = BITSWAP8(decrypt[i], 7, 2, 1, 4, 3, 6, 5, 0);
	if ((i & 7) == 6) decrypt[i] = BITSWAP8(decrypt[i], 7, 2, 1, 0, 3, 6, 5, 4);
	if ((i & 7) == 7) decrypt[i] = BITSWAP8(decrypt[i], 3, 2, 1, 0, 7, 6, 5, 4);
}

void sharkpy_bitswaps(UINT8 *decrypt, int i)
{
	if ((i & 7) == 0) decrypt[i] = BITSWAP8(decrypt[i], 3, 2, 1, 0, 7, 6, 5, 4);
	if ((i & 7) == 1) decrypt[i] = BITSWAP8(decrypt[i], 7, 2, 5, 4, 3, 6, 1, 0);
	if ((i & 7) == 2) decrypt[i] = BITSWAP8(decrypt[i], 7, 2, 1, 4, 3, 6, 5, 0);
	if ((i & 7) == 3) decrypt[i] = BITSWAP8(decrypt[i], 3, 6, 1, 0, 7, 2, 5, 4);
	if ((i & 7) == 4) decrypt[i] = BITSWAP8(decrypt[i], 7, 2, 5, 4, 3, 6, 1, 0);
	if ((i & 7) == 5) decrypt[i] = BITSWAP8(decrypt[i], 3, 2, 5, 4, 7, 6, 1, 0);
	if ((i & 7) == 6) decrypt[i] = BITSWAP8(decrypt[i], 7, 6, 1, 4, 3, 2, 5, 0);
	if ((i & 7) == 7) decrypt[i] = BITSWAP8(decrypt[i], 3, 6, 1, 4, 7, 2, 5, 0);
}

void victor5_bitswaps(UINT8 *decrypt, int i)
{
	if ((i & 7) == 0) decrypt[i] = BITSWAP8(decrypt[i], 7, 2, 5, 4, 3, 6, 1, 0);
	if ((i & 7) == 1) decrypt[i] = BITSWAP8(decrypt[i], 7, 6, 5, 0, 3, 2, 1, 4);
	if ((i & 7) == 2) decrypt[i] = BITSWAP8(decrypt[i], 7, 2, 1, 0, 3, 6, 5, 4);
	if ((i & 7) == 3) decrypt[i] = BITSWAP8(decrypt[i], 7, 2, 1, 0, 3, 6, 5, 4);
	if ((i & 7) == 4) decrypt[i] = BITSWAP8(decrypt[i], 3, 2, 1, 0, 7, 6, 5, 4);
	if ((i & 7) == 5) decrypt[i] = BITSWAP8(decrypt[i], 7, 6, 5, 0, 3, 2, 1, 4);
	if ((i & 7) == 6) decrypt[i] = BITSWAP8(decrypt[i], 3, 6, 1, 0, 7, 2, 5, 4);
	if ((i & 7) == 7) decrypt[i] = BITSWAP8(decrypt[i], 7, 2, 1, 4, 3, 6, 5, 0);
}

/* Decrypt */

void subsino_decrypt(UINT8 *region, void (*bitswaps)(UINT8 *decrypt, int i), const UINT8 *xors, int size)
{
	UINT8 *decrypt = malloc_or_die(size);
	int i;

	for (i = 0; i < size; i++)
	{
		decrypt[i] = region[i] ^ xors[i & 7];
		bitswaps(decrypt, i);
	}

	memcpy(region, decrypt, size);
	free(decrypt);
}
