// license:BSD-3-Clause
// copyright-holders: Andreas Naive, David Haywood


/****************************************************************************
IGS036 encryption emulation

The encryption used by the IGS036 seems to be another iteration over
previous IGS encryption schemes. Basically, it consists on a rotation-based
non-trivial obfuscation layered upon a simple address-based XOR encryption
(similar to the ones found in previous IGS circuits).

The scheme works on 16-bits words and is probably designed to depend on 24 bits of
(word-) address; in what follows, we will refer to the 8 lowest ones simply as the
lowest bits of the address, and the other 16 as the highest bits the address.

The address-based XOR can be thought as 16 one-bit XORs against key bits
controlled by certain combinations of up to three address bits. The game key is
comprised of 256 8-bits values provided by the internal ARM; every 8-bit value
in the key is used on those words whose address match the index modulus 256;
in a given key byte, every bit affects two positions of the corresponding 16-bits
encrypted words.
This use of the key is similar to the one found in previous instantiations of
IGS circuits.

What is new in the IGS036 is the use of an obfuscation working this way:

1) The highest bits of the address are split in 4 groups, every of which controls
a rotation by a shift of (plus or minus) 9, 1, 2 and 4 respectively.
2) For every address, the highest bit of the group set in the address controls
the activation/deactivation of the rotation for his group, using an associated
(and fixed) boolean function depending on the lowest bits of the address.
3) If the group rotation is to be activated according to 2), then another
fixed group-level boolean functions (again, depending on the lowest bits of the
address) control the direction (left or right) of the rotation.
4) One of the groups (the associated with the shift by 9) interacts with the other
three by inverting (when active itself) the activation/deactivation patterns of
the other three.
5) The lowest bits of the address control a further rotation(independent
on the highest bits of the address).
6) Finally, a global bitswap is applied.

All the associated boolean functions are clearly of low complexity, so it should
be expected that the hardware is calculating them that way rather than using
lookup tables or otherwise. It should be stressed that this obfuscation is done
system-wide without dependence on the game keys.

On a different note, the unused tail of the ROMs are pattern-filled and, more interestingly,
that region appears to be hiding 20-bytes values (SHA-1 hashes?) located at
positions which vary per set. See the table below.

driver      20-bytes value position in the ROM
---------   ----------------------------------
orleg2o     $763984-$763997
orleg2      $76C77C-$76C78F
kov3        $718040-$718053
kof98umh    $E50E60-$E50E73

TO-DO:
- complete the table with the 20-bytes values
- decryption tables for the programs using less than 0x80000 of code (huoqilin, jhzb)

*****************************************************************************/

#include "driver.h"
#include "machine/igs036crypt.h"
static UINT16 decrypt(UINT16 cipherword, int word_address, const UINT8 *key);
static UINT16 deobfuscate(UINT16 cipherword, int word_address);
static int rotation(int address);
static int rot_enabled(int address, const int *group);
static int rot_group(int address, const int *group);
static UINT16 rol(UINT16 num, int shift);
static const UINT32 triggers[16][2];
static int (*rot_enabling[16][4])(int);
static int (*rot_direction[4][8])(int);
void igs036_decrypt_rom(UINT16 *rom, int size, int offset, const UINT8 *key)
{
	for (int i = 0; i < size / 2; i++)
	{
		rom[i] = decrypt(rom[i], i + offset, key);
	}
}

static UINT16 decrypt(UINT16 cipherword, int word_address, const UINT8 *key)
{
	// key-independent manipulation
	int aux = deobfuscate(cipherword, word_address);

	// key-dependent manipulation
	for (int i = 0; i < 16; ++i)
	{
		if ((word_address & triggers[i][0]) == triggers[i][1])
			aux ^= BIT(key[word_address&0xff], i & 7) << i;
		else
			aux ^= BIT(0x1a3a, i) << i;
	}

	return aux;
}

static UINT16 deobfuscate(UINT16 cipherword, int word_address)
{
	// key-independent manipulation
	const int shift = rotation(word_address);
	int aux = rol(cipherword, shift);
	aux = BITSWAP16(aux, 10,9,8,7,0,15,6,5,   14,13,4,3,12,11,2,1);

	return aux;
}

static int rotation(int address)
{
	const int group15[] = {15,11, 7, 5};  // 15 is a guess
	const int group14[] = {14, 9, 3, 2};
	const int group13[] = {13,10, 6, 1};
	const int group12[] = {12, 8, 4, 0};

	// rotation depending on all the address bits
	const int enabled0 = rot_enabled(address, group15);
	int rot = enabled0 * rot_group(address, group15) * 9;

	const int enabled1 = enabled0 ^ rot_enabled(address, group14);
	rot += enabled1 * rot_group(address, group14) * 1;

	const int enabled2 = enabled0 ^ rot_enabled(address, group13);
	rot += enabled2 * rot_group(address, group13) * 2;

	const int enabled3 = enabled0 ^ rot_enabled(address, group12);
	rot += enabled3 * rot_group(address, group12) * 4;

	// block-independent rotation (just depending on the lowest 8 bits)
	int rot2  = 4 * BIT(address, 0);
	rot2 += 1 * BIT(address, 4) * (BIT(address, 0) * 2 - 1);
	rot2 += 4 * BIT(address, 3) * (BIT(address, 0) * 2 - 1);
	rot2 *= (BIT(address, 7) | (BIT(address, 0) ^ BIT(address, 1) ^ 1)) * 2 - 1;
	rot2 += 2 * ((BIT(address, 0) ^ BIT(address, 1)) & (BIT(address, 7) ^ 1));

	return (rot + rot2) & 0xf;
}

static int rot_enabled(int address, const int *group)
{
	int enabled = 0;
	for (int j = 0; j <4 ; ++j)
	{
		if (BIT(address, 8 + group[j]))
		{
			const int aux = address ^ (0x1b * BIT(address, 2));
			enabled = rot_enabling[group[j]][aux & 3](aux);
			break;
		}
	}

	return enabled;
}

static int rot_group(int address, const int *group)
{
	const int aux = rot_direction[group[0] & 3][address & 7](address);
	return (aux * 2) - 1;
}

static UINT16 rol(UINT16 num, int shift)
{
	const UINT16 r = num << shift;
	const UINT16 l = num >> (16 - shift);

	return r | l;
}

// the triggers describe under what conditions are every one of the 16 XORs activated

static const UINT32 triggers[16][2] = {
	{0x000101, 0x000001}, {0x000802, 0x000800}, {0x000204, 0x000004}, {0x000408, 0x000408},
	{0x010010, 0x000010}, {0x020020, 0x000020}, {0x040040, 0x000040}, {0x080080, 0x080080},
	{0x100100, 0x000100}, {0x200200, 0x200000}, {0x400400, 0x400000}, {0x800801, 0x000001},
	{0x001004, 0x001000}, {0x002010, 0x002000}, {0x004040, 0x000040}, {0x008100, 0x008100}
};


// The rotation depending on the 16 highest address bits depends on a series
// of function on the 8 lowest word-address bits. Some comments:
// * Bits #5 & #6 are unused so, in fact, they only depend on 6 address bits
// * The functions are clearly low-complexity boolean functions on those 6 bits
//   rather than, say, random lookup tables
// * There are quite a number of functionally equivalent ways to implement
//   those boolean functions, so the given implementation (by multiplexing
//   over some simple functions) shouldn't be taken too seriously: while it's
//   functionally correct, it doesn't necessarily represent the way the hardware
//   is calculating them.

static int unknown(int address) { return 0; }
static int cZero  (int address) { return 0; }
static int cOne   (int address) { return 1; }
static int bit_3  (int address) { return BIT(address, 3); }
static int bit_4  (int address) { return BIT(address, 4); }
static int bit_7  (int address) { return BIT(address, 7); }
static int not_3  (int address) { return BIT(address, 3) ^ 1; }
static int not_4  (int address) { return BIT(address, 4) ^ 1; }
static int not_7  (int address) { return BIT(address, 7) ^ 1; }
static int xor_37 (int address) { return BIT(address, 3) ^ BIT(address, 7); }
static int xnor_37(int address) { return BIT(address, 3) ^ BIT(address, 7) ^ 1; }
static int xor_47 (int address) { return BIT(address, 4) ^ BIT(address, 7); }
static int xnor_47(int address) { return BIT(address, 4) ^ BIT(address, 7) ^ 1; }
static int nor_34 (int address) { return (BIT(address, 3) | BIT(address, 4)) ^ 1; }
static int impl_43(int address) { return BIT(address, 3) || (BIT(address, 4) ^ 1); }


static int (*rot_enabling[16][4])(int)  = {
	{bit_3  , not_3  , bit_3  , not_3  },
	{bit_3  , not_3  , bit_3  , not_3  },
	{bit_4  , bit_4  , bit_4  , bit_4  },
	{bit_4  , not_4  , bit_4  , not_4  },
	{bit_3  , bit_3  , bit_3  , bit_3  },
	{nor_34 , bit_7  , bit_7  , cZero  },
	{cZero  , cOne   , cZero  , cOne   },
	{impl_43, xor_37 , xnor_37, not_3  },
	{bit_3  , bit_3  , not_3  , not_3  },
	{bit_4  , bit_4  , not_4  , not_4  },
	{cZero  , cZero  , cZero  , cZero  },
	{nor_34 , bit_7  , not_7  , cOne   },
	{bit_3  , not_3  , bit_3  , not_3  },
	{cZero  , cOne   , cOne   , cZero  },
	{bit_4  , not_4  , bit_4  , not_4  },
	{unknown, unknown, unknown, unknown},
};

static int (*rot_direction[4][8])(int)  = {
	{bit_3  , xor_37 , xnor_37, not_3  , bit_3  , xor_37 , xnor_37, not_3  },
	{cZero  , not_7  , not_7  , cZero  , cZero  , not_7  , not_7  , cZero  },
	{bit_4  , xor_47 , xnor_47, not_4  , bit_4  , xor_47 , xnor_47, not_4  },
	{bit_3  , not_7  , bit_7  , cZero  , cOne   , not_7  , bit_7  , cZero  },
};

// ------------------------GAME KEYS---------------------------

// The keys below have been obtained by an automatic process
// exploiting the simple XOR scheme used by the system. Overall, the process,
// while simple, seems to be pretty robust, so few errors should be expected,
// if any.

