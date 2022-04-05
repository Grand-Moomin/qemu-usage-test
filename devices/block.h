#ifndef __BLOCK_H__
#define __BLOCK_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <list.h>

#define BLOCK_SECTOR_SIZE   512

struct block_operations {
    size_t (*read)(void *buf, size_t sector, void *param);
    size_t (*write)(const void *buf, size_t sector, void *param);
};

struct block {
    struct list_head block_lists;   /* List of all blocks. */
    char name[16];
    size_t nr_sectors;            /* Number of sectors. */
    struct block_operations *ops;
    void *params;                   /* Private parameters of the block. */
};

extern void block_init(void);
extern struct block *block_register(struct block *blk, const char *name, size_t nr_sectors, \
                                    struct block_operations *ops, void *params);
extern void block_unregister(struct block *blk);
extern size_t block_read(struct block *blk, void *buf, size_t sector);
extern size_t block_write(struct block *blk, const void *buf, size_t sector);
extern struct block *blcok_get_by_name(char *name);

#endif