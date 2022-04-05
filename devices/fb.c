#include <string.h>
#include <round.h>
#include <stddef.h>
#include <stdint.h>
#include <asm/io.h>
#include "kernel/vaddr.h"
#include "devices/fb.h"

#define COL_CNT 80
#define ROW_CNT 25

#define GRAY_ON_BLACK 0x07

static size_t cx, cy;               //Cursor coordinates
static uint8_t (*fb)[COL_CNT][2];   //Characters buffer pointer

static void init_cursor(void);
static void find_cursor(size_t *x, size_t *y);
static void move_cursor(void);
static void cls(void);
static void clear_row(size_t y);
static void newline(void);

void fb_putc(char ch)
{
    init_cursor();

    switch(ch){
        case '\r':
            cx = 0;
            break;

        case '\n':
            newline();
            break;
        
        case '\f':
            cls();
            break;

        case '\b':
            if(cx > 0)
                cx--;
            break;

        case '\t':
            cx = ROUND_UP(cx + 1, 8);
            if(cx >= COL_CNT)
                newline();
            break;

        default:
            fb[cy][cx][0] = ch;
            fb[cy][cx][1] = GRAY_ON_BLACK;
            if(++cx >= COL_CNT)
                newline();
            break;
    }

    move_cursor();
}

static void clear_row(size_t y)
{
    size_t x;
    for(x = 0; x < COL_CNT; ++x){
        fb[y][x][0] = ' ';
        fb[y][x][1] = GRAY_ON_BLACK;
    }
}

static void cls(void)
{
    size_t y;
    
    for(y = 0; y < ROW_CNT; ++y)
        clear_row(y);
    
    cx = cy = 0;
    move_cursor();
}

static void newline(void)
{
    cx = 0;
    cy++;
    if(cy >= ROW_CNT){
        cy = ROW_CNT - 1;
        memmove(&fb[0], &fb[1], sizeof(fb[0]) * (ROW_CNT - 1));
        clear_row(ROW_CNT - 1);
    }
}

static void init_cursor(void)
{
    static bool inited = false;

    if(!inited){
        fb = ptov(0xb8000);
        find_cursor(&cx, &cy);
        cy += 3;
        cx = 0;
        move_cursor();

        inited = true;
    }
}

static void find_cursor(size_t *x, size_t *y)
{
    uint16_t cp;
    outb(0x3d4, 0x0e);
    cp = inb(0x3d5) << 8;

    outb(0x3d4, 0x0f);
    cp |= inb(0x3d5);

    *x = cp % COL_CNT;
    *y = cp / COL_CNT;
}

static void move_cursor(void)
{
    uint16_t cp = cx + COL_CNT * cy;
    outw(0x3d4, 0x0e | (cp & 0xff00));
    outw(0x3d4, 0x0f | (cp << 8));
}