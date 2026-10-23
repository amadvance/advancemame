#include "driver.h"
#include <assert.h>
enum { FN1GK = 38, FN2GK = 32, BUFFER_SIZE = 2, FLAG_COMPRESSED = 0x20000 };
struct sbox { UINT8 table[64]; int inputs[6]; int outputs[2]; };
static UINT32 key = 0x05226d41, prot_cur_address, dec_header;
static UINT16 subkey, dec_hist, buffer2a;
static UINT8 buffer[2], buffer2[2], lines[2][512];
static UINT8 *line_buffer = lines[0], *line_buffer_prev = lines[1];
static int enc_ready, buffer_pos, line_buffer_pos, line_buffer_size;
static int buffer_bit, buffer_bit2, block_pos, block_size, block_numlines, done_compression;
static UINT16 get_decrypted_16(void);
static void enc_start(void);
static void enc_fill(void);
static void line_fill(void);
static int get_compressed_bit(void);
static UINT16 crypt_rom_read(UINT32 addr)
{
	UINT16 value = program_read_word_32be(0x02000000 + addr * 2);
	return (value >> 8) | (value << 8);
}
static const struct sbox fn1_sboxes[4][4] = {
	{
		{
			{
				0,3,2,2,1,3,1,2,3,2,1,2,1,2,3,1,3,2,2,0,2,1,3,0,0,3,2,3,2,1,2,0,
				2,3,1,1,2,2,1,1,1,0,2,3,3,0,2,1,1,1,1,1,3,0,3,2,1,0,1,2,0,3,1,3,
			},
			{3,4,5,7,-1,-1},
			{0,4}
		},

		{
			{
				2,2,2,0,3,3,0,1,2,2,3,2,3,0,2,2,1,1,0,3,3,2,0,2,0,1,0,1,2,3,1,1,
				0,1,3,3,1,3,3,1,2,3,2,0,0,0,2,2,0,3,1,3,0,3,2,2,0,3,0,3,1,1,0,2,
			},
			{0,1,2,5,6,7},
			{1,6}
		},

		{
			{
				0,1,3,0,3,1,1,1,1,2,3,1,3,0,2,3,3,2,0,2,1,1,2,1,1,3,1,0,0,2,0,1,
				1,3,1,0,0,3,2,3,2,0,3,3,0,0,0,0,1,2,3,3,2,0,3,2,1,0,0,0,2,2,3,3,
			},
			{0,2,5,6,7,-1},
			{2,3}
		},

		{
			{
				3,2,1,2,1,2,3,2,0,3,2,2,3,1,3,3,0,2,3,0,3,3,2,1,1,1,2,0,2,2,0,1,
				1,3,3,0,0,3,0,3,0,2,1,3,2,1,0,0,0,1,1,2,0,1,0,0,0,1,3,3,2,0,3,3,
			},
			{1,2,3,4,6,7},
			{5,7}
		},
	},
	{
		{
			{
				3,3,1,2,0,0,2,2,2,1,2,1,3,1,1,3,3,0,0,3,0,3,3,2,1,1,3,2,3,2,1,3,
				2,3,0,1,3,2,0,1,2,1,3,1,2,2,3,3,3,1,2,2,0,3,1,2,2,1,3,0,3,0,1,3,
			},
			{0,1,3,4,5,7},
			{0,4}
		},

		{
			{
				2,0,1,0,0,3,2,0,3,3,1,2,1,3,0,2,0,2,0,0,0,2,3,1,3,1,1,2,3,0,3,0,
				3,0,2,0,0,2,2,1,0,2,3,3,1,3,1,0,1,3,3,0,0,1,3,1,0,2,0,3,2,1,0,1,
			},
			{0,1,3,4,6,-1},
			{1,5}
		},

		{
			{
				2,2,2,3,1,1,0,1,3,3,1,1,2,2,2,0,0,3,2,3,3,0,2,1,2,2,3,0,1,3,0,0,
				3,2,0,3,2,0,1,0,0,1,2,2,3,3,0,2,2,1,3,1,1,1,1,2,0,3,1,0,0,2,3,2,
			},
			{1,2,5,6,7,6},
			{2,7}
		},

		{
			{
				0,1,3,3,3,1,3,3,1,0,2,0,2,0,0,3,1,2,1,3,1,2,3,2,2,0,1,3,0,3,3,3,
				0,0,0,2,1,1,2,3,2,2,3,1,1,2,0,2,0,2,1,3,1,1,3,3,1,1,3,0,2,3,0,0,
			},
			{2,3,4,5,6,7},
			{3,6}
		},
	},
	{
		{
			{
				0,0,1,0,1,0,0,3,2,0,0,3,0,1,0,2,0,3,0,0,2,0,3,2,2,1,3,2,2,1,1,2,
				0,0,0,3,0,1,1,0,0,2,1,0,3,1,2,2,2,0,3,1,3,0,1,2,2,1,1,1,0,2,3,1,
			},
			{1,2,3,4,5,7},
			{0,5}
		},

		{
			{
				1,2,1,0,3,1,1,2,0,0,2,3,2,3,1,3,2,0,3,2,2,3,1,1,1,1,0,3,2,0,0,1,
				1,0,0,1,3,1,2,3,0,0,2,3,3,0,1,0,0,2,3,0,1,2,0,1,3,3,3,1,2,0,2,1,
			},
			{0,2,4,5,6,7},
			{1,6}
		},

		{
			{
				0,3,0,2,1,2,0,0,1,1,0,0,3,1,1,0,0,3,0,0,2,3,3,2,3,1,2,0,0,2,3,0,

				255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
			},
			{0,2,4,6,7,-1},
			{2,3}
		},

		{
			{
				0,0,1,0,0,1,0,2,3,3,0,3,3,2,3,0,2,2,2,0,3,2,0,3,1,0,0,3,3,0,0,0,
				2,2,1,0,2,0,3,2,0,0,3,1,3,3,0,0,2,1,1,2,1,0,1,1,0,3,1,2,0,2,0,3,
			},
			{0,1,2,3,6,-1},
			{4,7}
		},
	},
	{
		{
			{
				0,3,3,3,3,3,2,0,0,1,2,0,2,2,2,2,1,1,0,2,2,1,3,2,3,2,0,1,2,3,2,1,
				3,2,2,3,1,0,1,0,0,2,0,1,2,1,2,3,1,2,1,1,2,2,1,0,1,3,2,3,2,0,3,1,
			},
			{0,1,3,4,5,6},
			{0,5}
		},

		{
			{
				0,3,0,0,2,0,3,1,1,1,2,2,2,1,3,1,2,2,1,3,2,2,3,3,0,3,1,0,3,2,0,1,
				3,0,2,0,1,0,2,1,3,3,1,2,2,0,2,3,3,2,3,0,1,1,3,3,0,2,1,3,0,2,2,3,
			},
			{0,1,2,3,5,7},
			{1,7}
		},

		{
			{
				0,1,2,3,3,3,3,1,2,0,2,3,2,1,0,1,2,2,1,2,0,3,2,0,1,1,0,1,3,1,3,1,
				3,1,0,0,1,0,0,0,0,1,2,2,1,1,3,3,1,2,3,3,3,2,3,0,2,2,1,3,3,0,2,0,
			},
			{2,3,4,5,6,7},
			{2,3}
		},

		{
			{
				0,2,1,1,3,2,0,3,1,0,1,0,3,2,1,1,2,2,0,3,1,0,1,2,2,2,3,3,0,0,0,0,
				1,2,1,0,2,1,2,2,2,3,2,3,0,1,3,0,0,1,3,0,0,1,1,0,1,0,0,0,0,2,0,1,
			},
			{0,1,2,4,6,7},
			{4,6}
		},
	},
};

static const struct sbox fn2_sboxes[4][4] = {
	{
		{
			{
				3,3,0,1,0,1,0,0,0,3,0,0,1,3,1,2,0,3,3,3,2,1,0,1,1,1,2,2,2,3,2,2,
				2,1,3,3,1,3,1,1,0,0,1,2,0,2,2,1,1,2,3,1,2,1,3,1,2,2,0,1,3,0,2,2,
			},
			{1,3,4,5,6,7},
			{0,7}
		},

		{
			{
				0,1,3,0,1,1,2,3,2,0,0,3,2,1,3,1,3,3,0,0,1,0,0,3,0,3,3,2,3,2,0,1,
				3,2,3,2,2,1,3,1,1,1,0,3,3,2,2,1,1,2,0,2,0,1,1,0,1,0,1,1,2,0,3,0,
			},
			{0,3,5,6,5,0},
			{1,2}
		},

		{
			{
				0,2,2,1,0,1,2,1,2,0,1,2,3,3,0,1,3,1,1,2,1,2,1,3,3,2,3,3,2,1,0,1,
				0,1,0,2,0,1,1,3,2,0,3,2,1,1,1,3,2,3,0,2,3,0,2,2,1,3,0,1,1,2,2,2,
			},
			{0,2,3,4,7,-1},
			{3,4}
		},

		{
			{
				2,3,1,3,2,0,1,2,0,0,3,3,3,3,3,1,2,0,2,1,2,3,0,2,0,1,0,3,0,2,1,0,
				2,3,0,1,3,0,3,2,3,1,2,0,3,1,1,2,0,3,0,0,2,0,2,1,2,2,3,2,1,2,3,1,
			},
			{1,2,5,6,-1,-1},
			{5,6}
		},
	},
	{
		{
			{
				2,3,1,3,1,0,3,3,3,2,3,3,2,0,0,3,2,3,0,3,1,1,2,3,1,1,2,2,0,1,0,0,
				2,1,0,1,2,0,1,2,0,3,1,1,2,3,1,2,0,2,0,1,3,0,1,0,2,2,3,0,3,2,3,0,
			},
			{0,1,4,5,6,7},
			{0,7}
		},

		{
			{
				0,2,2,0,2,2,0,3,2,3,2,1,3,2,3,3,1,1,0,0,3,0,2,1,1,3,3,2,3,2,0,1,
				1,2,3,0,1,0,3,0,3,1,0,2,1,2,0,3,2,3,1,2,2,0,3,2,3,0,0,1,2,3,3,3,
			},
			{0,2,3,6,7,-1},
			{1,5}
		},

		{
			{
				1,0,3,0,0,1,2,1,0,0,1,0,0,0,2,3,2,2,0,2,0,1,3,0,2,0,1,3,2,3,0,1,
				1,2,2,2,1,3,0,3,0,1,1,0,3,2,3,3,2,0,0,3,1,2,1,3,3,2,1,0,2,1,2,3,
			},
			{2,3,4,6,7,2},
			{2,3}
		},

		{
			{
				2,3,1,3,1,1,2,3,3,1,1,0,1,0,2,3,2,1,0,0,2,2,0,1,0,2,2,2,0,2,1,0,
				3,1,2,3,1,3,0,2,1,0,1,0,0,1,2,2,3,2,3,1,3,2,1,1,2,0,2,1,3,3,1,0,
			},
			{1,2,3,4,5,6},
			{4,6}
		},
	},
	{
		{
			{
				0,3,0,1,3,0,0,2,1,0,1,3,2,2,2,0,3,3,3,0,2,2,0,3,0,0,2,3,0,3,2,1,
				3,3,0,3,0,2,3,3,1,1,1,0,2,2,1,1,3,0,3,1,2,0,2,0,0,0,3,2,1,1,0,0,
			},
			{1,4,5,6,7,5},
			{0,5}
		},

		{
			{
				0,3,0,1,3,0,3,1,3,2,2,2,3,0,3,2,2,1,2,2,0,3,2,2,0,0,2,1,1,3,2,3,
				2,3,3,1,2,0,1,2,2,1,0,0,0,0,2,3,1,2,0,3,1,3,1,2,3,2,1,0,3,0,0,2,
			},
			{0,2,3,4,6,7},
			{1,7}
		},

		{
			{
				2,2,0,3,0,3,1,0,1,1,2,3,2,3,1,0,0,0,3,2,2,0,2,3,1,3,2,0,3,3,1,3,

				255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
			},
			{1,2,4,7,2,-1},
			{2,4}
		},

		{
			{
				0,2,3,1,3,1,1,0,0,1,3,0,2,1,3,3,2,0,2,1,1,2,3,3,0,0,0,2,0,2,3,0,
				3,3,3,3,2,3,3,2,3,0,1,0,2,3,3,2,0,1,3,1,0,1,2,3,3,0,2,0,3,0,3,3,
			},
			{0,1,2,3,5,7},
			{3,6}
		},
	},
	{
		{
			{
				0,1,1,0,0,1,0,2,3,3,0,1,2,3,0,2,1,0,3,3,2,0,3,0,0,2,1,0,1,0,1,3,
				0,3,3,1,2,0,3,0,1,3,2,0,3,3,1,3,0,2,3,3,2,1,1,2,2,1,2,1,2,0,1,1,
			},
			{0,1,2,4,7,-1},
			{0,5}
		},

		{
			{
				2,0,0,2,3,0,2,3,3,1,1,1,2,1,1,0,0,2,1,0,0,3,1,0,0,3,3,0,1,0,1,2,
				0,2,0,2,0,1,2,3,2,1,1,0,3,3,3,3,3,3,1,0,3,0,0,2,0,3,2,0,2,2,0,1,
			},
			{0,1,3,5,6,-1},
			{1,3}
		},

		{
			{
				0,1,1,2,1,3,1,1,0,0,3,1,1,1,2,0,3,2,0,1,1,2,3,3,3,0,3,0,0,2,0,3,
				3,2,0,0,3,2,3,1,2,3,0,3,2,0,1,2,2,2,0,2,0,1,2,2,3,1,2,2,1,1,1,1,
			},
			{0,2,3,4,5,7},
			{2,7}
		},

		{
			{
				0,1,2,0,3,3,0,3,2,1,3,3,0,3,1,1,3,2,3,2,3,0,0,0,3,0,2,2,3,2,2,3,
				2,2,3,1,2,3,1,2,0,3,0,2,3,1,0,0,3,2,1,2,1,2,1,3,1,0,2,3,3,1,3,2,
			},
			{2,3,4,5,6,7},
			{4,6}
		},
	},
};

static const int fn1_game_key_scheduling[FN1GK][2] = {
	{1,29},  {1,71},  {2,4},   {2,54},  {3,8},   {4,56},  {4,73},  {5,11},
	{6,51},  {7,92},  {8,89},  {9,9},   {9,39},  {9,58},  {10,90}, {11,6},
	{12,64}, {13,49}, {14,44}, {15,40}, {16,69}, {17,15}, {18,23}, {18,43},
	{19,82}, {20,81}, {21,32}, {22,5},  {23,66}, {24,13}, {24,45}, {25,12},
	{25,35}, {26,61}, {27,10}, {27,59}, {28,25}, {29,86}
};

static const int fn2_game_key_scheduling[FN2GK][2] = {
	{0,0},   {1,3},   {2,11},  {3,20},  {4,22},  {5,23},  {6,29},  {7,38},
	{8,39},  {9,55},  {9,86},  {9,87},  {10,50}, {11,57}, {12,59}, {13,61},
	{14,63}, {15,67}, {16,72}, {17,83}, {18,88}, {19,94}, {20,35}, {21,17},
	{22,6},  {23,85}, {24,16}, {25,25}, {26,92}, {27,47}, {28,28}, {29,90}
};

static const int fn1_sequence_key_scheduling[20][2] = {
	{0,52},  {1,34},  {2,17},  {3,36}, {4,84},  {4,88},  {5,57},  {6,48},
	{6,68},  {7,76},  {8,83},  {9,30}, {10,22}, {10,41}, {11,38}, {12,55},
	{13,74}, {14,19}, {14,80}, {15,26}
};

static const int fn2_sequence_key_scheduling[16] = {77,34,8,42,36,27,69,66,13,9,79,31,49,7,24,64};

static const int fn2_middle_result_scheduling[16] = {1,10,44,68,74,78,81,95,2,4,30,40,41,51,53,58};

static int feistel_function(int input, const struct sbox *sboxes, UINT32 subkeys)
{
	int k,m;
	int aux;
	int result=0;

	for (m=0; m<4; ++m) {
		for (k=0, aux=0; k<6; ++k)
			if (sboxes[m].inputs[k]!=-1)
				aux |= BIT(input, sboxes[m].inputs[k]) << k;

		aux = sboxes[m].table[(aux^subkeys)&0x3f];

		for (k=0; k<2; ++k)
			result |= BIT(aux,k) << sboxes[m].outputs[k];

		subkeys >>=6;
	}

	return result;
}

static UINT16 block_decrypt(UINT32 game_key, UINT16 sequence_key, UINT16 counter, UINT16 data)
{
	int j;
	int aux, aux2;
	int A, B;
	int middle_result;
	UINT32 fn1_subkeys[4];
	UINT32 fn2_subkeys[4];

	memset(fn1_subkeys, 0, sizeof(UINT32) * 4);
	memset(fn2_subkeys, 0, sizeof(UINT32) * 4);

	for (j = 0; j < FN1GK; ++j) {
		if (BIT(game_key, fn1_game_key_scheduling[j][0]) != 0) {
			aux = fn1_game_key_scheduling[j][1] % 24;
			aux2 = fn1_game_key_scheduling[j][1] / 24;
			fn1_subkeys[aux2] ^= (1 << aux);
		}
	}

	for (j = 0; j < FN2GK; ++j) {
		if (BIT(game_key, fn2_game_key_scheduling[j][0]) != 0) {
			aux = fn2_game_key_scheduling[j][1] % 24;
			aux2 = fn2_game_key_scheduling[j][1] / 24;
			fn2_subkeys[aux2] ^= (1 << aux);
		}
	}

	for (j = 0; j < 20; ++j) {
		if (BIT(sequence_key, fn1_sequence_key_scheduling[j][0]) != 0) {
			aux = fn1_sequence_key_scheduling[j][1] % 24;
			aux2 = fn1_sequence_key_scheduling[j][1] / 24;
			fn1_subkeys[aux2] ^= (1 << aux);
		}
	}

	for (j = 0; j < 16; ++j) {
		if (BIT(sequence_key, j) != 0) {
			aux = fn2_sequence_key_scheduling[j] % 24;
			aux2 = fn2_sequence_key_scheduling[j] / 24;
			fn2_subkeys[aux2] ^= (1 << aux);
		}
	}

	aux = BITSWAP16(counter, 5, 12, 14, 13, 9, 3, 6, 4, 8, 1, 15, 11, 0, 7, 10, 2);

	B = aux >> 8;
	A = (aux & 0xff) ^ feistel_function(B, fn1_sboxes[0], fn1_subkeys[0]);

	B ^= feistel_function(A, fn1_sboxes[1], fn1_subkeys[1]);

	A ^= feistel_function(B, fn1_sboxes[2], fn1_subkeys[2]);

	B ^= feistel_function(A, fn1_sboxes[3], fn1_subkeys[3]);

	middle_result = (B << 8) | A;

	for (j = 0; j < 16; ++j) {
		if (BIT(middle_result, j) != 0) {
			aux = fn2_middle_result_scheduling[j] % 24;
			aux2 = fn2_middle_result_scheduling[j] / 24;
			fn2_subkeys[aux2] ^= (1 << aux);
		}
	}

	aux = BITSWAP16(data, 14, 3, 8, 12, 13, 7, 15, 4, 6, 2, 9, 5, 11, 0, 1, 10);

	B = aux >> 8;
	A = (aux & 0xff) ^ feistel_function(B, fn2_sboxes[0], fn2_subkeys[0]);

	B ^= feistel_function(A, fn2_sboxes[1], fn2_subkeys[1]);

	A ^= feistel_function(B, fn2_sboxes[2], fn2_subkeys[2]);

	B ^= feistel_function(A, fn2_sboxes[3], fn2_subkeys[3]);

	aux = (B << 8) | A;

	aux = BITSWAP16(aux, 15, 7, 6, 14, 13, 12, 5, 4, 3, 2, 11, 10, 9, 1, 0, 8);

	return aux;
}

static UINT16 get_decrypted_16()
{
	UINT16 enc;

	enc = crypt_rom_read(prot_cur_address);

	UINT16 dec = block_decrypt(key, subkey, prot_cur_address, enc);
	UINT16 res = (dec & 3) | (dec_hist & 0xfffc);
	dec_hist = dec;

	prot_cur_address ++;

	return res;
}

static void enc_start()
{
	block_pos = 0;
	done_compression = 0;
	buffer_pos = BUFFER_SIZE;

	if (buffer_bit2 < 14)
	{

		dec_header = (buffer2a & 0x0003) << 16;
	}
	else
	{
		dec_hist = 0;
		dec_header = get_decrypted_16() << 16;
	}

	dec_header |= get_decrypted_16();

	block_numlines = ((dec_header & 0x000000ff) >> 0) + 1;
	int blocky = ((dec_header & 0x0001ff00) >> 8) + 1;
	block_size = block_numlines * blocky;

	if(dec_header & FLAG_COMPRESSED) {
		line_buffer_size = blocky;
		line_buffer_pos = line_buffer_size;
		buffer_bit = 7;
		buffer_bit2 = 15;
	}

	enc_ready = 1;
}

static void enc_fill()
{
	assert(buffer_pos == BUFFER_SIZE);
	for(int i = 0; i != BUFFER_SIZE; i+=2) {
		UINT16 val = get_decrypted_16();
		buffer[i] = val;
		buffer[i+1] = val >> 8;
		block_pos+=2;

		if (!(dec_header & FLAG_COMPRESSED))
		{
			if (block_pos == block_size)
			{

				enc_start();
			}
		}
	}
	buffer_pos = 0;
}

static const UINT8 trees[9][2][32] = {
	{
		{0x01,0x10,0x0f,0x05,0xc4,0x13,0x87,0x0a,0xcc,0x81,0xce,0x0c,0x86,0x0e,0x84,0xc2,
			0x11,0xc1,0xc3,0xcf,0x15,0xc8,0xcd,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,},
		{0xc7,0x02,0x03,0x04,0x80,0x06,0x07,0x08,0x09,0xc9,0x0b,0x0d,0x82,0x83,0x85,0xc0,
			0x12,0xc6,0xc5,0x14,0x16,0xca,0xcb,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,},
	},
	{
		{0x02,0x80,0x05,0x04,0x81,0x10,0x15,0x82,0x09,0x83,0x0b,0x0c,0x0d,0xdc,0x0f,0xde,
			0x1c,0xcf,0xc5,0xdd,0x86,0x16,0x87,0x18,0x19,0x1a,0xda,0xca,0xc9,0x1e,0xce,0xff,},
		{0x01,0x17,0x03,0x0a,0x08,0x06,0x07,0xc2,0xd9,0xc4,0xd8,0xc8,0x0e,0x84,0xcb,0x85,
			0x11,0x12,0x13,0x14,0xcd,0x1b,0xdb,0xc7,0xc0,0xc1,0x1d,0xdf,0xc3,0xc6,0xcc,0xff,},
	},
	{
		{0xc6,0x80,0x03,0x0b,0x05,0x07,0x82,0x08,0x15,0xdc,0xdd,0x0c,0xd9,0xc2,0x14,0x10,
			0x85,0x86,0x18,0x16,0xc5,0xc4,0xc8,0xc9,0xc0,0xcc,0xff,0xff,0xff,0xff,0xff,0xff,},
		{0x01,0x02,0x12,0x04,0x81,0x06,0x83,0xc3,0x09,0x0a,0x84,0x11,0x0d,0x0e,0x0f,0x19,
			0xca,0xc1,0x13,0xd8,0xda,0xdb,0x17,0xde,0xcd,0xcb,0xff,0xff,0xff,0xff,0xff,0xff,},
	},
	{
		{0x01,0x80,0x0d,0x04,0x05,0x15,0x83,0x08,0xd9,0x10,0x0b,0x0c,0x84,0x0e,0xc0,0x14,
			0x12,0xcb,0x13,0xca,0xc8,0xc2,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,},
		{0xc5,0x02,0x03,0x07,0x81,0x06,0x82,0xcc,0x09,0x0a,0xc9,0x11,0xc4,0x0f,0x85,0xd8,
			0xda,0xdb,0xc3,0xdc,0xdd,0xc1,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,},
	},
	{
		{0x01,0x80,0x06,0x0c,0x05,0x81,0xd8,0x84,0x09,0xdc,0x0b,0x0f,0x0d,0x0e,0x10,0xdb,
			0x11,0xca,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,},
		{0xc4,0x02,0x03,0x04,0xcb,0x0a,0x07,0x08,0xd9,0x82,0xc8,0x83,0xc0,0xc1,0xda,0xc2,
			0xc9,0xc3,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,},
	},
	{
		{0x01,0x02,0x06,0x0a,0x83,0x0b,0x07,0x08,0x09,0x82,0xd8,0x0c,0xd9,0xda,0xff,0xff,
			0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,},
		{0xc3,0x80,0x03,0x04,0x05,0x81,0xca,0xc8,0xdb,0xc9,0xc0,0xc1,0x0d,0xc2,0xff,0xff,
			0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,},
	},
	{
		{0x01,0x02,0x03,0x04,0x81,0x07,0x08,0xd8,0xda,0xd9,0xff,0xff,0xff,0xff,0xff,0xff,
			0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,},
		{0xc2,0x80,0x05,0xc9,0xc8,0x06,0x82,0xc0,0x09,0xc1,0xff,0xff,0xff,0xff,0xff,0xff,
			0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,},
	},
	{
		{0x01,0x80,0x04,0xc8,0xc0,0xd9,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
			0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,},
		{0xc1,0x02,0x03,0x81,0x05,0xd8,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
			0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,},
	},
	{
		{0x01,0xd8,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
			0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,},
		{0xc0,0x80,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
			0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,},
	},
};

static int get_compressed_bit()
{

	if (buffer_bit2 == 15)
	{
		buffer_bit2 = 0;
		buffer2a = get_decrypted_16();
		buffer2[0] = buffer2a;
		buffer2[1] = buffer2a >> 8;

		buffer_pos = 0;

	}
	else
	{
		buffer_bit2++;
	}

	int res = (buffer2[(buffer_pos&1)^1] >> buffer_bit) & 1;
	buffer_bit--;
	if(buffer_bit == -1) {
		buffer_bit = 7;
		buffer_pos++;
	}
	return res;
}
static void line_fill()
{
	assert(line_buffer_pos == line_buffer_size);
	UINT8 *lp = line_buffer;
	UINT8 *lc = line_buffer_prev;

	{ UINT8 *swap = line_buffer; line_buffer = line_buffer_prev; line_buffer_prev = swap; }

	line_buffer_pos = 0;

	for(int i=0; i != line_buffer_size;) {

		int slot = i ? i < line_buffer_size - 7 ? 1 : (i & 7) + 1 : 0;

		UINT32 tmp = 0;
		while (!(tmp&0x80))
			if(get_compressed_bit())
				tmp = trees[slot][1][tmp];
			else
				tmp = trees[slot][0][tmp];
		if(tmp != 0xff) {
			int count = (tmp & 7) + 1;

			if(tmp&0x40) {

				static int offsets[4] = {0, 1, 0, -1};
				int offset = offsets[(tmp & 0x18) >> 3];
				for(int j=0; j != count; j++) {
					lc[i^1] = lp[((i + offset + line_buffer_size) % line_buffer_size)^1];
					i++;
				}

			} else {

				UINT8 byte;
				byte =         get_compressed_bit()  << 1;
				byte = (byte | get_compressed_bit()) << 1;
				byte = (byte | get_compressed_bit()) << 1;
				byte = (byte | get_compressed_bit()) << 1;
				byte = (byte | get_compressed_bit()) << 1;
				byte = (byte | get_compressed_bit()) << 1;
				byte = (byte | get_compressed_bit()) << 1;
				byte =  byte | get_compressed_bit();
				for(int j=0; j != count; j++)
					lc[(i++)^1] = byte;

			}
		}
	}

	block_pos++;
	if (block_numlines == block_pos)
	{
		done_compression = 1;
	}
	else
	{
	}
}

static UINT32 registers[4];
static UINT16 stream_read(void)
{
	UINT8 *base;
	if (!enc_ready) enc_start();
	if (dec_header & FLAG_COMPRESSED)
	{
		if (line_buffer_pos == line_buffer_size)
		{
			if (done_compression) enc_start();
			line_fill();
		}
		base = line_buffer + line_buffer_pos;
		line_buffer_pos += 2;
	}
	else
	{
		if (buffer_pos == BUFFER_SIZE) enc_fill();
		base = buffer + buffer_pos;
		buffer_pos += 2;
	}
	return (base[0] << 8) | base[1];
}
READ32_HANDLER( stv_5881_r )
{
	if (registers[0] & 0x10000)
	{
		if (offset == 3)
		{
			UINT16 a = stream_read(), b = stream_read();
			return ((UINT32)((a >> 8) | (a << 8)) << 16) | (UINT16)((b >> 8) | (b << 8));
		}
		return registers[offset];
	}
	return registers[offset] ? registers[offset] : ((UINT32 *)memory_region(REGION_USER1))[0x2fffff0 / 4 + offset];
}
WRITE32_HANDLER( stv_5881_w )
{
	COMBINE_DATA(&registers[offset]);
	if (offset == 2)
	{
		if (!(mem_mask & 0xffff0000)) prot_cur_address = (prot_cur_address & 0xffff0000) | (data >> 16);
		if (!(mem_mask & 0x0000ffff))
		{
			prot_cur_address = (prot_cur_address & 0xffff) | (data << 16);
			buffer_bit = 7;
			buffer_bit2 = 15;
		}
		enc_ready = 0;
	}
	else if (offset == 3) { subkey = registers[3] >> 16; enc_ready = 0; }
}
void stv_5881_reset(void)
{
	memset(registers, 0, sizeof(registers));
	memset(lines, 0, sizeof(lines));
	line_buffer = lines[0];
	line_buffer_prev = lines[1];
	prot_cur_address = dec_hist = dec_header = subkey = enc_ready = 0;
	buffer_bit = 7; buffer_bit2 = 15;
	buffer_pos = BUFFER_SIZE;
	line_buffer_pos = line_buffer_size = 0;
}
