#include "devices/ide.h"
#include "kernel/interrupt.h"
#include "kernel/utils.h"
#include <stdio.h>
#include <asm/io.h>

/* ATA command block registers */
#define reg_data(channel)   ((channel)->reg_base)
#define reg_error(channel)  ((channel)->reg_base + 1)
#define reg_nsect(channel)  ((channel)->reg_base + 2)   /* Requested sector count */
#define reg_lbal(channel)   ((channel)->reg_base + 3)   /* LBA 0:7 */
#define reg_lbam(channel)   ((channel)->reg_base + 4)   /* LBA 8:15 */
#define reg_lbah(channel)   ((channel)->reg_base + 5)   /* LBA 16:23 */
#define reg_device(channel) ((channel)->reg_base + 6)   /* Device | LBA 24:27 */
#define reg_status(channel) ((channel)->reg_base + 7)   /* Status, read only */
#define reg_command(channel)    reg_status(channel)     /* Command, write only */

/* ATA control block registers */
#define reg_ctl(channel)    ((channel)->reg_base + 0x206)   /* Device control, write only */
#define reg_alt_status(channel) reg_ctl(channel)            /* Alternate status, read only */

/* Alternate status register bits */
#define STA_BSY 0x80    /* Device busy */
#define STA_DRDY 0x40   /* Device ready */
#define STA_DRQ 0x08    /* Data request */

/* Control register bits*/
#define CTL_SRST 0x04   /* Software reset */

/* Device register bits */
#define DEV_MBS 0xa0    /* Must be set */
#define DEV_LBA 0x40    /* Linear based addressing */
#define DEV_DEV 0x10    /* Select device: 0: master, 1: slave */

/* Commands */
#define CMD_IDENTIFY_DEVICE 0xec
#define CMD_READ_SECTOR_RETRY 0x20
#define CMD_WRITE_SECTOR_RETRY 0x30

static struct ata_adaptor channels[NR_ATA_ADAPTORS];

static void reset_adaptor(struct ata_adaptor *chan);
static void select_disk(struct ata_disk *disk);
static void select_disk_wait(struct ata_disk *disk);
static bool check_disk_type(struct ata_disk *disk);
static void identify_ata_device(struct ata_disk *disk);
static void issue_pio_cmd(struct ata_adaptor *chan, uint8_t cmd);
static char *manipulate_info_string(char *orig, size_t size);

static void input_sector(struct ata_adaptor *chan, void *buf);
static void output_sector(struct ata_adaptor *chan, const void *buf);
static size_t ide_read(void *buf, size_t sect_no, void *param);
static size_t ide_write(const void *buf, size_t sect_no, void *param);

static bool wait_while_busy(struct ata_disk *disk);
static void wait_until_idle(struct ata_disk *disk);

static void ide_interrupt(interrupt_stack_frame *stack);

static struct block ide_disks[NR_ATA_ADAPTORS * NR_ATA_DISKS];

static struct block_operations ide_ops = {
    .read = ide_read,
    .write = ide_write
};

void ide_init(void)
{
    size_t chan_no, dev_no;
    struct ata_adaptor *chan;
    struct ata_disk *disk;
    uint16_t chan_reg_bases[] = {0x1f0, 0x170}; /* IO ports base of primary and secondary ATA harddisk controller */
    uint16_t chan_irqs[] = {14 + 0x20, 15 + 0x20}; /* ATA harddisk interrupt vec_no */

    for(chan_no = 0; chan_no < NR_ATA_ADAPTORS; ++chan_no){
        chan = channels + chan_no;

        sprintf(chan->name, "ide%d", chan_no);
        chan->reg_base = chan_reg_bases[chan_no];
        chan->irq = chan_irqs[chan_no];

        spin_lock_init(&chan->slock);
        sema_init(&chan->sema_wait, 0);
        sema_init(&chan->sema_rw, 1);

        for(dev_no = 0; dev_no < NR_ATA_DISKS; ++dev_no){
            disk = chan->ata_disks + dev_no;
            disk->ata_adaptor = chan;
            disk->dev_no = dev_no;
            sprintf(disk->name, "hd%c", 'a' + chan_no * NR_ATA_DISKS + dev_no);
        }

        set_interrupt_gate(chan_irqs[chan_no], ide_interrupt);

        reset_adaptor(chan);

        if(check_disk_type(&chan->ata_disks[0]))
            check_disk_type(&chan->ata_disks[1]);

        for(dev_no = 0; dev_no < NR_ATA_DISKS; ++dev_no){
            struct ata_disk *disk = &chan->ata_disks[dev_no];
            if(disk->is_ata_disk){
                identify_ata_device(disk);
                block_register(&ide_disks[chan_no * NR_ATA_DISKS + dev_no],
                                disk->name, disk->nr_sectors, &ide_ops, disk);
            }
        }
    }
    /*char buf[512];
    //ide_read(buf, 0, &channels->ata_disks[0]);
    //printk("read sector 0 %02x\n", (uint8_t)buf[510]);
    struct block *blk = blcok_get_by_name("hda");
    printk("blk %d\n",blk->nr_sectors);
    block_read(blk, buf, 0);
    printk("read sector 0 %02x\n", (uint8_t)buf[510]);*/
}

static void ide_interrupt(interrupt_stack_frame *stack)
{
    int i;
    struct ata_adaptor *chan;
    
    for(i = 0; i < NR_ATA_ADAPTORS; ++i){
        chan = &channels[i];
        if(chan->irq == stack->vec_no){
            /*Reading status register when an interrupt is pending causes the interrupt to be cleared. */
            inb(reg_status(chan));
            sema_up(&chan->sema_wait);
            return;
        }
    }
}

static void ide_udelay(uint32_t n)
{
    for(uint32_t i = 0; i < n; ++i){
        barrier();
        for(uint32_t j = 0; j < 100; ++j);
    }
}

static void reset_adaptor(struct ata_adaptor *chan)
{
    size_t dev_no;
    struct ata_disk *disk;
    bool present[NR_ATA_DISKS];
    
    for(dev_no = 0; dev_no < NR_ATA_DISKS; dev_no++){
        disk = &chan->ata_disks[dev_no];
        select_disk(disk);
        
        /* Try to detect device presence. */
        outb(reg_nsect(chan), 0x55);
        outb(reg_lbal(chan), 0xaa);

        outb(reg_nsect(chan), 0xaa);
        outb(reg_lbal(chan), 0x55);

        outb(reg_nsect(chan), 0x55);
        outb(reg_lbal(chan), 0xaa);

        barrier();

        present[dev_no] = (inb(reg_nsect(chan)) == 0x55 && inb(reg_lbal(chan)) == 0xaa);
    }

    /* Software reset */
    outb(reg_ctl(chan), 0);
    ide_udelay(10);
    outb(reg_ctl(chan), CTL_SRST);
    ide_udelay(10);
    outb(reg_ctl(chan), 0);
    ide_udelay(150*1000);

    /* Wait for disk 0 to clear BSY. */
    if(present[0]){
        select_disk(&chan->ata_disks[0]);
        wait_while_busy(&chan->ata_disks[0]);
    }

    /* Wait for disk 1 to clear BSY. */
    if(present[1]){
        int i;
        select_disk(&chan->ata_disks[1]);
        for(i = 0; i < 3000; ++i){
            if(inb(reg_nsect(chan)) == 1 && inb(reg_lbal(chan)) == 1)
                break;
            ide_udelay(10);
        }
        wait_while_busy(&chan->ata_disks[1]);
    }
}

static bool check_disk_type(struct ata_disk *disk)
{
    struct ata_adaptor *chan = disk->ata_adaptor;
    uint8_t error, lbam, lbah, status;

    select_disk(disk);
    error = inb(reg_error(chan));
    lbam = inb(reg_lbam(chan));
    lbah = inb(reg_lbah(chan));
    status = inb(reg_status(chan));

    if((error != 1 && (error != 0x81 || disk->dev_no == 1))
        || !(status & STA_DRDY)
        || (status & STA_BSY)){
            disk->is_ata_disk = false;
            return error != 0x81;
    }
    else{
        disk->is_ata_disk = (lbam == 0 && lbah == 0) || (lbam == 0x3c && lbah == 0xc3);
        return true;
    }
}

static void identify_ata_device(struct ata_disk *disk)
{
    struct ata_adaptor *chan = disk->ata_adaptor;
    char id[BLOCK_SECTOR_SIZE];
    size_t nr_sectors;
    char *serial_no, *model_no;

    if(!disk->is_ata_disk)
        return;
    
    printk("Identify ata disk (%s) ... ", disk->name);
    select_disk_wait(disk);
    issue_pio_cmd(chan, CMD_IDENTIFY_DEVICE);
    sema_down(&chan->sema_wait);
    if(!wait_while_busy(disk)){
        disk->is_ata_disk = false;
        return;
    }

    input_sector(chan, id);

    disk->nr_sectors = nr_sectors = *(size_t *)&id[60 * 2];
    serial_no = manipulate_info_string(&id[10 * 2], 20);
    model_no = manipulate_info_string(&id[27 * 2], 40);

    printk("sectors %d \n", nr_sectors);
    printk("Serial no. %s\nModel no. %s\n",serial_no, model_no);    
}

static void issue_pio_cmd(struct ata_adaptor *chan, uint8_t cmd)
{
    interrupt_enable();
    outb(reg_command(chan), cmd);
}

static bool wait_while_busy(struct ata_disk *disk)
{
    struct ata_adaptor *chan = disk->ata_adaptor;
    int i;

    printk("busy waiting %s ", disk->name);
    for(i = 0; i < 3000; ++i){
        if((inb(reg_alt_status(chan)) & STA_BSY) == 0){
            printk("ok\n");
            return inb(reg_alt_status(chan)) & STA_DRQ;
        }
        ide_udelay(10);
    }

    printk("Failed\n");
    return false;
}

/* Wait for the ata adaptor until it is available to handle the next request. */
static void wait_until_idle(struct ata_disk *disk)
{
    int i;
    for(i = 0; i < 1000; ++i){
        if((inb(reg_status(disk->ata_adaptor)) & (STA_BSY | STA_DRQ)) == 0)
            return;
        ide_udelay(10);
    }
    printk("Wait timeout %s\n", disk->name);
}

/* Select one of the devices (disks) connected to the adaptor. */
static void select_disk(struct ata_disk *disk)
{
    struct ata_adaptor *chan = disk->ata_adaptor;
    uint8_t dev_reg_val = DEV_MBS;

    /* DEV bit set for slave device (device 1)*/
    if(disk->dev_no == 1)
        dev_reg_val |= DEV_DEV;
    outb(reg_device(chan), dev_reg_val);    /* Here, the genuine command to select the device in the channel. */
    ide_udelay(1);
}

static void select_disk_wait(struct ata_disk *disk)
{
    wait_until_idle(disk);
    select_disk(disk);
    wait_until_idle(disk);
}

static void input_sector(struct ata_adaptor *chan, void *buf)
{
    insw(reg_data(chan), buf, BLOCK_SECTOR_SIZE / 2);
}

static void output_sector(struct ata_adaptor *chan, const void *buf)
{
    outsw(reg_data(chan), buf, BLOCK_SECTOR_SIZE / 2);
}

static char *manipulate_info_string(char *orig, size_t size)
{
    size_t i;
    char tmp;

    /* ATA standard stipulates the order of an ACSII string, 
    which is not the convenrtional order for a string in C. 
    as the 2 bytes in a single word need to be reversed. */
    for(i = 0; i < size - 1; i += 2){
        tmp = orig[i + 1];
        orig[i + 1] = orig[i];
        orig[i] = tmp;
    }

    for(i = size - 1; i > 0; i--){
        tmp = orig[i];
        if(tmp != '\0')
            break;
    }
    orig[i] = '\0';

    return orig;
}

static void select_sector(struct ata_disk *disk, size_t sect_no)
{
    struct ata_adaptor *chan = disk->ata_adaptor;

    select_disk_wait(disk);
    outb(reg_nsect(chan), 1);               /* Sector count : Here, we request one sector. */
    /* Specify LBA (sector number) and device. */
    outb(reg_lbal(chan), sect_no);
    outb(reg_lbam(chan), sect_no >> 8);
    outb(reg_lbah(chan), sect_no >> 16);
    outb(reg_device(chan), DEV_MBS | DEV_LBA | (disk->dev_no == 1 ? DEV_DEV : 0) | (sect_no) >> 24);
}

static size_t ide_read(void *buf, size_t sect_no, void *param)
{
    struct ata_disk *disk = (struct ata_disk *)param;
    struct ata_adaptor *chan = disk->ata_adaptor;
    int32_t ret = 0;

    sema_down(&chan->sema_rw);
    select_sector(disk, sect_no);               /* Select the spcific sector of the disk. */
    issue_pio_cmd(chan, CMD_READ_SECTOR_RETRY); /* Issue a read-sector-with-retry command. */
    sema_down(&chan->sema_wait);                /* Wait for interrupt from ata adaptor signaling the completion of read. */
    if(wait_while_busy(disk)){
        input_sector(chan, buf);                /* Multiple read ops to read words from data register. */
        ret = BLOCK_SECTOR_SIZE;
    }
    else
        ret = -1;
    sema_up(&chan->sema_rw);
    return ret;
}

static size_t ide_write(const void *buf, size_t sect_no, void *param)
{
    struct ata_disk *disk = (struct ata_disk *)param;
    struct ata_adaptor *chan = disk->ata_adaptor;
    int32_t ret = 0;

    sema_down(&chan->sema_rw);
    select_sector(disk, sect_no);               /* Select the spcific sector of the disk. */
    issue_pio_cmd(chan, CMD_WRITE_SECTOR_RETRY);    /* Issue a write-sector-with-retry command. */
    if(wait_while_busy(disk)){
        output_sector(chan, buf);               /* Multiple write ops to write words to data register. */
        sema_down(&chan->sema_wait);            /* Wait for interrupt from ada adaptor signaling the completion of write. */
        ret = BLOCK_SECTOR_SIZE;
    }
    else
        ret = -1;
    sema_up(&chan->sema_rw);
    return ret;
}

