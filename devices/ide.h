#ifndef __IDE_H__
#define __IDE_H__

#include "devices/block.h"
#include "kernel/sync.h"
#include <stdint.h>
#include <stdbool.h>

#define NR_ATA_ADAPTORS 2
#define NR_ATA_DISKS    2

/* ATA disk device connected to a specific ATA interface. */
struct ata_disk {
    char name[8];           /* i.e., "hda","hdb" */
    struct ata_adaptor *ata_adaptor;
    int32_t dev_no;
    bool is_ata_disk;
    size_t nr_sectors;
};

/* ATA interface containing a single host. */
struct ata_adaptor {
    char name[8];           /* i.e., "ide0","ide1" */
    uint16_t reg_base;       /* Io port base */
    uint16_t irq;
    spinlock_t slock;
    semaphore_t sema_wait;  /* Semaphore for interrupt signal waiting. */
    semaphore_t sema_rw;    /* Semaphore for */
    struct ata_disk ata_disks[2];
};

extern void ide_init(void);

#endif