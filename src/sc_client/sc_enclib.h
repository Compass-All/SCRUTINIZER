#ifndef SC_ENC_H
#define SC_ENC_H

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define AUTH_TEST_KEY 0x6296828

struct mem_buffer {
	void *virt_base;
	size_t size;
    int flag;
    int hash;
};

void decrypt_buffer(struct mem_buffer* buffer);
void encrypt_buffer(struct mem_buffer* buffer);

void key_expansion(uint32_t *kv);
void aes128_block(uint32_t *expandedkeys, uint64_t *data, size_t size, uint32_t isdec);
void sha256(uint32_t *ctx, const void *in, size_t size);
void sha256_final(uint32_t *ctx, const void *in, size_t remain_size, size_t tot_size);
void sha256_block_data_order(uint32_t *ctx, const void *in, size_t num);
void protect_str_buffer(struct mem_buffer* buf);
void restore_str_buffer(struct mem_buffer* buf);

void fast_memset(void *dst, unsigned long val, size_t size);
void fast_memcpy(void *dst, void *src, size_t size);

#endif