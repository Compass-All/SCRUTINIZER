#include "sc_enclib.h"


void fast_memset(void *dst, unsigned long val, size_t size) {
	size_t _size = size / 64 * 64;
	for (unsigned long addr = (unsigned long)dst ; addr < (unsigned long)dst + _size ; addr += 64) {
		((unsigned long*)addr)[0] = val; ((unsigned long*)addr)[1] = val; ((unsigned long*)addr)[2] = val; ((unsigned long*)addr)[3] = val;
		((unsigned long*)addr)[4] = val; ((unsigned long*)addr)[5] = val; ((unsigned long*)addr)[6] = val; ((unsigned long*)addr)[7] = val;
	}
	memset(dst+_size, val, size-_size);
}
void fast_memcpy(void *dst, void *src, size_t size) {
	for (long long remain = size ; remain-32 >= 0 ; dst += 32, src += 32, remain -= 32) {
		((unsigned long*)dst)[0] = ((unsigned long*)src)[0]; ((unsigned long*)dst)[1] = ((unsigned long*)src)[1];
		((unsigned long*)dst)[2] = ((unsigned long*)src)[2]; ((unsigned long*)dst)[3] = ((unsigned long*)src)[3];
	}
}

uint32_t init_RC[10] = {0x01000000,0x02000000,0x04000000,0x08000000,0x10000000,0x20000000,0x40000000,0x80000000,0x1b000000,0x36000000};
uint32_t init_H[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
uint32_t sig[16];

uint64_t aes_key[2] = {0x1234567890abcdef, 0xfedcba9876543210};

uint32_t Sbox[256] = {
	0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
	0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
	0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
	0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
	0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
	0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
	0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
	0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
	0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
	0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
	0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
	0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
	0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
	0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
	0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
	0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

void key_expansion(uint32_t *kv) {
	*(uint64_t*)kv = aes_key[0]; *(((uint64_t*)kv) + 1) = aes_key[1];
	int indekz=4;

	uint32_t ktv;
	int cnt=0;
	while(indekz<44){
		if(indekz%4==0){
			ktv=((kv[indekz-1]<<8)&(0xffffffff))|((kv[indekz-1]>>24)&(0xffffffff));//rotate
			//subbytes
			uint32_t tmp78=(ktv&0xff000000)>>24;
			uint32_t tmp56=(ktv&0x00ff0000)>>16;
			uint32_t tmp34=(ktv&0x0000ff00)>>8;
			uint32_t tmp12=(ktv&0x000000ff)>>0;
			tmp78=Sbox[tmp78];
			tmp56=Sbox[tmp56];
			tmp34=Sbox[tmp34];
			tmp12=Sbox[tmp12];
			ktv=(tmp78<<24)+(tmp56<<16)+(tmp34<<8)+tmp12;
			ktv=ktv^init_RC[cnt];//eor with rc
			kv[indekz]=kv[indekz-4]^ktv;
			cnt+=1;
		}
		else{
			kv[indekz]=kv[indekz-1]^kv[indekz-4];
		}
		indekz+=1;
	}
}

void decrypt_buffer(struct mem_buffer* buffer) {
	uint32_t kv[44];
	if (buffer->flag & 0x1) key_expansion(kv);

	uint32_t H[8];
	if ((buffer->flag & 0x1) || (buffer->flag & 0x4)) fast_memcpy(H, init_H, sizeof(H));

	int offset = 0;
	int thisstepsize = 0;
	while (offset < buffer->size) {
		void *current_ptr = (void*)((uint64_t)buffer->virt_base + offset);
		offset += 4096;
		if (offset > buffer->size) thisstepsize = buffer->size - (offset-4096);
		else thisstepsize = 4096;
		// flush_dcache_range((uint64_t)(current_ptr), thisstepsize); 
		if (buffer->flag & 0x1) {
			aes128_block(kv, current_ptr, thisstepsize, 1);// 1 decryption
		}
		if ((buffer->flag & 0x1) || (buffer->flag & 0x4)) { // 0x2 only protect, no integrity check
			if (offset < buffer->size) sha256(H, current_ptr, thisstepsize);
			else sha256_final(H, current_ptr, thisstepsize, buffer->size);
		}
		// flush_dcache_range((uint64_t)(current_ptr), thisstepsize);
    }
}

void encrypt_buffer(struct mem_buffer* buffer){
	uint32_t kv[44];
	uint32_t H[8];
	if (buffer->flag & 0x8) {
		key_expansion(kv);
		fast_memcpy(H, init_H, sizeof(H));
	}

	int offset = 0;
	int thisstepsize = 0;
	while (offset < buffer->size) {
		void *current_ptr = (void*)((uint64_t)buffer->virt_base + offset);
		offset += 4096;
		if (offset > buffer->size) thisstepsize = buffer->size - (offset-4096);
		else thisstepsize = 4096;
		// flush_dcache_range((uint64_t)(current_ptr), thisstepsize); 
		if (buffer->flag & 0x8) {
			if (offset < buffer->size) sha256(H, current_ptr, thisstepsize);
			else sha256_final(H, current_ptr, thisstepsize, buffer->size);
			aes128_block(kv, current_ptr, thisstepsize, 0); //0 encryption
		}
		else { // 0x10
			fast_memset(current_ptr, 0, thisstepsize); 
		}
		// flush_dcache_range((uint64_t)(current_ptr), thisstepsize); 
    }
}

void sha256(uint32_t *ctx, const void *in, size_t size) {
	size_t block_num = size / 64;
	if (block_num) sha256_block_data_order(ctx, in, block_num); 
}

void sha256_final(uint32_t *ctx, const void *in, size_t remain_size, size_t tot_size) {
	size_t block_num = remain_size / 64;
	sha256(ctx, in, block_num*64);

	size_t remainder = remain_size % 64;
	size_t tot_bits = tot_size * 8;
	char last_block[64]; 
	fast_memset(last_block, 0, sizeof(last_block));
	fast_memcpy(last_block, (void*)in+block_num*64, remainder);
	last_block[remainder] = 0x80;
	if (remainder < 56) {}
	else {
		sha256_block_data_order(ctx, last_block, 1);
		fast_memset(last_block, 0, sizeof(last_block));
	}
	for (int i = 0 ; i < 8 ; ++ i) last_block[63-i] = tot_bits >> (i * 8);
	sha256_block_data_order(ctx, last_block, 1);
}


#define SHA256_BLOCK_SIZE 32            // SHA256 outputs a 32 byte digest

/**************************** DATA TYPES ****************************/
typedef unsigned char BYTE;             // 8-bit byte
typedef unsigned int  WORD;             // 32-bit word, change to "long" for 16-bit machines

typedef struct {
	BYTE data[64];
	WORD datalen;
	unsigned long long bitlen;
	WORD state[8];
} SHA256_CTX;

//0x6296828
BYTE true_sig[SHA256_BLOCK_SIZE] = {0xcc,  0x7e,  0x8e,  0x7c,  0xc5,  0x89,  0x6e,  0xc7,  0x7d,  0x8e,  0x1d,  0x55,  0x61,  0x5,  0xed,  0xa8,  0xed,  0x68,  0xc5,  0xe6,  0x6a,  0xa0,  0x24,  0xa4,  0x1d,  0x7a,  0xdf,  0x39,  0x14,  0x6c,  0xda,  0xc2};
BYTE user_sig[SHA256_BLOCK_SIZE];

/****************************** MACROS ******************************/
#define ROTLEFT(a,b) (((a) << (b)) | ((a) >> (32-(b))))
#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b))))

#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22))
#define EP1(x) (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25))
#define SIG0(x) (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))

/**************************** VARIABLES *****************************/
static const WORD k[64] = {
	0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
	0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
	0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
	0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
	0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
	0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
	0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
	0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

/*********************** FUNCTION DEFINITIONS ***********************/
void sha256_transform(SHA256_CTX *ctx, const BYTE data[])
{
	WORD a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];

	// initialization 
	for (i = 0, j = 0; i < 16; ++i, j += 4)
		m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
	for ( ; i < 64; ++i)
		m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];

	a = ctx->state[0];
	b = ctx->state[1];
	c = ctx->state[2];
	d = ctx->state[3];
	e = ctx->state[4];
	f = ctx->state[5];
	g = ctx->state[6];
	h = ctx->state[7];

	for (i = 0; i < 64; ++i) {
		t1 = h + EP1(e) + CH(e,f,g) + k[i] + m[i];
		t2 = EP0(a) + MAJ(a,b,c);
		h = g;
		g = f;
		f = e;
		e = d + t1;
		d = c;
		c = b;
		b = a;
		a = t1 + t2;
	}

	ctx->state[0] += a;
	ctx->state[1] += b;
	ctx->state[2] += c;
	ctx->state[3] += d;
	ctx->state[4] += e;
	ctx->state[5] += f;
	ctx->state[6] += g;
	ctx->state[7] += h;
}

void sha256_init(SHA256_CTX *ctx)
{
	ctx->datalen = 0;
	ctx->bitlen = 0;
	ctx->state[0] = 0x6a09e667;
	ctx->state[1] = 0xbb67ae85;
	ctx->state[2] = 0x3c6ef372;
	ctx->state[3] = 0xa54ff53a;
	ctx->state[4] = 0x510e527f;
	ctx->state[5] = 0x9b05688c;
	ctx->state[6] = 0x1f83d9ab;
	ctx->state[7] = 0x5be0cd19;
}

void sha256_update(SHA256_CTX *ctx, BYTE* data, size_t len)
{
	WORD i;

	for (i = 0; i < len; ++i) {
		ctx->data[ctx->datalen] = data[i];
		ctx->datalen++;
		if (ctx->datalen == 64) {
			sha256_transform(ctx, ctx->data);
			ctx->bitlen += 512;
			ctx->datalen = 0;
		}
	}
}

void sha256_f(SHA256_CTX *ctx, BYTE* hash)
{
	WORD i;

	i = ctx->datalen;

	if (ctx->datalen < 56) {
		ctx->data[i++] = 0x80;  // pad 10000000 = 0x80
		while (i < 56)
			ctx->data[i++] = 0x00;
	}
	else {
		ctx->data[i++] = 0x80;
		while (i < 64)
			ctx->data[i++] = 0x00;
		sha256_transform(ctx, ctx->data);
		memset(ctx->data, 0, 56);
	}

	ctx->bitlen += ctx->datalen * 8;
	ctx->data[63] = ctx->bitlen;
	ctx->data[62] = ctx->bitlen >> 8;
	ctx->data[61] = ctx->bitlen >> 16;
	ctx->data[60] = ctx->bitlen >> 24;
	ctx->data[59] = ctx->bitlen >> 32;
	ctx->data[58] = ctx->bitlen >> 40;
	ctx->data[57] = ctx->bitlen >> 48;
	ctx->data[56] = ctx->bitlen >> 56;
	sha256_transform(ctx, ctx->data);

	for (i = 0; i < 4; ++i) {
		hash[i]      = (ctx->state[0] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 4]  = (ctx->state[1] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 8]  = (ctx->state[2] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 12] = (ctx->state[3] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 16] = (ctx->state[4] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 20] = (ctx->state[5] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 24] = (ctx->state[6] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 28] = (ctx->state[7] >> (24 - i * 8)) & 0x000000ff;
	}

}

void protect_str_buffer(struct mem_buffer* buf)
{
    if (buf->flag & 0x18) {
        encrypt_buffer(buf);
        buf->flag = 0x7;
    }
}

void restore_str_buffer(struct mem_buffer* buf)
{
    if (buf->flag & 0x7) {
        decrypt_buffer(buf);
        buf->flag = 0x18;
    }
}