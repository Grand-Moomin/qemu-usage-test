#include <kernel/bitmap.h>

bitmap *bitmap_create_in_buf(size_t bit_count, void *buf, size_t buf_size)
{
    bitmap *bm = buf;

    if(buf_size - sizeof(bitmap) < bitmap_required_buf_size(bit_count))
        return NULL;
    bm->bit_count = bit_count;
    return bm;
}

void bitmap_config_all(bitmap *bm, bool val)
{
    size_t i, u32_bound, u32_res, res_bit_start_idx;
    uint32_t v = (val) ? -1 : 0;

    u32_bound = bm->bit_count / BITS_PER_UINT32;
    u32_res = bm->bit_count % BITS_PER_UINT32;

    for(i = 0; i < u32_bound; ++i)
        bm->bits[i] = v;
    
    res_bit_start_idx = u32_bound * BITS_PER_UINT32;
    for(i = 0; i < u32_res; ++i)
        bitmap_config_bit(bm, res_bit_start_idx + i, val);
}

bool bitmap_test_bit(bitmap *bm, size_t idx, bool val)
{
    size_t idx_buf = idx / BITS_PER_UINT32;
    uint32_t bit_mask = 1 << (idx % BITS_PER_UINT32);

    if(val)
        return bm->bits[idx_buf] & bit_mask;
    else
        return ~(bm->bits[idx_buf]) & bit_mask;
}

size_t bitmap_find_first_bit(bitmap *bm, size_t idx_start, bool val)
{
    return bitmap_find_first_bits_group(bm, idx_start, 1, val);
}

size_t bitmap_find_first_bits_group(bitmap *bm, size_t idx_start, size_t bit_count, bool val)
{
    size_t idx_cur = idx_start;
    size_t idx_last = idx_start + bit_count;
    bool test_flag = false;

    while(idx_last <= bm->bit_count && idx_cur < bm->bit_count){
        test_flag = bitmap_test_bit(bm, idx_cur, val);
        if(test_flag){
            idx_cur++;
            if(idx_cur == idx_last)
                return idx_cur - bit_count;
        }
        else{
            idx_cur++;
            idx_last = idx_cur + bit_count;
        }
    }
    return BITMAP_ERR;
}

void bitmap_config_bit(bitmap *bm, size_t idx, bool val)
{
    size_t idx_buf = idx / BITS_PER_UINT32;
    uint32_t bit_mask = 1 << (idx % BITS_PER_UINT32);

    if(idx >= bm->bit_count)
        return;

    if(val)
        bm->bits[idx_buf] |= bit_mask;        
    else
        bm->bits[idx_buf] &= ~bit_mask;
}

/* Required byte counts to store bit_count bits. */
size_t bitmap_required_buf_size(size_t bit_count)
{
    return DIV_ROUND_UP(bit_count, BITS_PER_UINT8);
}

/* Set specific bit to 1. */
void bitmap_set_bit(bitmap *bm, size_t idx)
{
    bitmap_config_bit(bm, idx, true);
}

void bitmap_set_multi_bits(bitmap *bm, size_t idx_start, size_t bit_count)
{
    size_t i;
    size_t idx_last = idx_start + bit_count;

    if(idx_last > bm->bit_count)
        return;
    
    for(i = idx_start; i < idx_last; ++i)
        bitmap_set_bit(bm, i);
}

/* Set specific bit to 0. */
void bitmap_reset_bit(bitmap *bm, size_t idx)
{
    bitmap_config_bit(bm, idx, false);
}

void bitmap_reset_multi_bits(bitmap *bm, size_t idx_start, size_t bit_count)
{
    size_t i;
    size_t idx_last = idx_start + bit_count;

    if(idx_last > bm->bit_count)
        return;
    
    for(i = idx_start; i < idx_last; ++i)
        bitmap_reset_bit(bm, i);
}

size_t bitmap_get_bits_count(bitmap *bm)
{
    return bm->bit_count;
}