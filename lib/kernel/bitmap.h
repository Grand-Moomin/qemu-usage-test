#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <stddef.h>
#include <stdint.h>
#include <round.h>
#include <stdbool.h>

#define BITS_PER_UINT8  (sizeof(uint8_t)*8)
#define BITS_PER_UINT32 (sizeof(uint32_t)*8)

#define BITMAP_ERR  UINT32_MAX

typedef struct {
    size_t bit_count;
    uint32_t bits[0];
} bitmap;


extern bitmap *bitmap_create_in_buf(size_t bit_count, void *buf, size_t buf_size);
extern size_t bitmap_required_buf_size(size_t bit_count);   /* Required byte counts to store bit_count bits. */

size_t bitmap_get_bits_count(bitmap *bm);

extern void bitmap_config_all(bitmap *bm, bool val);
extern void bitmap_config_bit(bitmap *bm, size_t idx, bool val);
extern void bitmap_set_bit(bitmap *bm, size_t idx);         /* Set specific bit to 1. */
extern void bitmap_reset_bit(bitmap *bm, size_t idx);       /* Set specific bit to 0. */
extern void bitmap_set_multi_bits(bitmap *bm, size_t idx_start, size_t bit_count);      /* Set several specific bits to 1. */
extern void bitmap_reset_multi_bits(bitmap *bm, size_t idx_start, size_t bit_count);    /* Set several specific bits to 0. */
extern bool bitmap_test_bit(bitmap *bm, size_t idx, bool val);  /* Test if the bit @ idx is equal to val. */

extern size_t bitmap_find_first_bit(bitmap *bm, size_t idx_start, bool val);           /* Find first bit starting from idx_start which is equal to val. */
extern size_t bitmap_find_first_bits_group(bitmap *bm, size_t idx_start, size_t bit_count, bool val); /* Find first group of bits all of which are equal to val. */

#endif