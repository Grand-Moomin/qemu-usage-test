#include "devices/block.h"
#include <string.h>

static struct list_head block_devices;

static inline bool valid_sector_no(struct block *blk, size_t sector)
{
    return (sector < blk->nr_sectors);
}

void block_init(void)
{
    list_head_init(&block_devices);
}

size_t block_read(struct block *blk, void *buf, size_t sector)
{   
    if(!valid_sector_no(blk, sector))
        return -1;

    //TODO: Implement a buffer cache
    return blk->ops->read(buf, sector, blk->params);
}

size_t block_write(struct block *blk, const void *buf, size_t sector)
{
    if(!valid_sector_no(blk, sector))
        return -1;
    
    //TODO: Implement a buffer cache
    return blk->ops->write(buf, sector, blk->params);
}

struct block *block_register(struct block *blk, const char *name, size_t nr_sectors, \
                             struct block_operations *ops, void *params)
{
    if(!blk || !name)
        return NULL;
    
    strlcpy(blk->name, name, sizeof(blk->name));
    blk->nr_sectors = nr_sectors;
    blk->ops = ops;
    blk->params = params;
    list_add_tail(&blk->block_lists, &block_devices);

    return blk;
}

void block_unregister(struct block *blk)
{
    if(!blk)
        return;
    list_del(&blk->block_lists);
}

struct block *blcok_get_by_name(char *name)
{
    struct block *blk;
    list_for_each_entry(blk, &block_devices, block_lists){
        if(!strcmp(name, blk->name)){
            return blk;
        }
    }
    return NULL;
}