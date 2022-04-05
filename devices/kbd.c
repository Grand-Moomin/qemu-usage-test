#include "devices/kbd.h"
#include "kernel/interrupt.h"
#include "kernel/utils.h"
#include "asm/io.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>

#define DATA_REG 0x60

struct keymap{
    uint16_t scan_code_first;    /* Scan code of first character in string chars. */
    const char *chars;          /* scan_code_first <=> chars[0], scan_code_first+1 <=> chars[1] ... */
};

struct special_keymap{
    uint16_t scan_code;
    bool *state;
};

static const struct keymap invariant_keymap[] = {
    {0x01, "\x1b"}, /* Escape */
    {0x0e, "\b"},
    {0x0f, "\tQWERTYUIOP"},
    {0x1c, "\r"},
    {0x1e, "ASDFGHJKL"},
    {0x2c, "ZXCVBNM"},
    {0x37, "*"},    /* Keypad * */
    {0x39, " "},
    {0x53, "\x7f"}, /* Delete */
    {0, NULL}
};

static const struct keymap unshifted_keymap[] = {
    {0x02, "1234567890-="},
    {0x1a, "[]"},
    {0x27, ";'`"},
    {0x2b, "\\"},
    {0x33, ",./"},
    {0, NULL}
};

static const struct keymap shifted_keymap[] = {
    {0x02, "!@#$%^&*()_+"},
    {0x1a, "{}"},
    {0x27, ":\"~"},
    {0x2b, "|"},
    {0x33, "<>?"},
    {0, NULL}
};

static bool left_shift, right_shift;
static bool left_alt, right_alt;
static bool left_ctrl, right_ctrl;
static bool caps_locks;

static const struct special_keymap special_keymap[] = {
    {0x1d, &left_ctrl},
    {0x2a, &left_shift},
    {0x36, &right_shift},
    {0x38, &left_alt},
    {0xe01d, &right_ctrl},
    {0xe038, &right_alt},
    {0, NULL}
};

static void kbd_interrupt(interrupt_stack_frame *stack __attribute__((unused)));
static bool map_scan_code(const struct keymap *km, uint16_t scan_code, char *ch);

void kbd_init(void)
{
    set_interrupt_gate(0x20 + 1, kbd_interrupt);
}

static bool map_scan_code(const struct keymap *km, uint16_t scan_code, char *ch)
{
    for(; km->chars; km++){
        size_t code_group_len = strlen(km->chars);
        if(scan_code >= km->scan_code_first && scan_code < km->scan_code_first + code_group_len){
            *ch = km->chars[scan_code - km->scan_code_first];
            return true;
        }
    }
    return false;
}

static void kbd_interrupt(interrupt_stack_frame *stack __attribute__((unused)))
{
    bool shift = left_shift || right_shift;
    bool alt = left_alt || right_alt;
    bool ctrl = left_ctrl || right_alt;
    bool release;
    uint16_t scan_code;
    char ch;
    const struct special_keymap *sp_kmap;
    
    scan_code = inb(DATA_REG);
    /* For prefix code */
    if(scan_code == 0xe0)
        scan_code = (scan_code << 8) | inb(DATA_REG);

    /* Bit 7 set for key release. */
    release = scan_code & 0x80;
    scan_code &= ~0x80u;

    /* Cap locks */
    if(scan_code == 0x3a){
        if(!release)
            caps_locks = !caps_locks;
    }
    
    else if(map_scan_code(invariant_keymap, scan_code, &ch) || 
            (!shift && map_scan_code(unshifted_keymap, scan_code, &ch)) ||
            (shift && map_scan_code(shifted_keymap, scan_code, &ch))){
        if(!release){
            if(ctrl && ch >= 0x40 && ch < 0x60)
                ch -= 0x40;
            else if(shift == caps_locks)
                ch = tolower(ch);

            if(alt)
                ch += 0x80;
            
            printk("%c", ch);
        }
    }

    else{
        for(sp_kmap = special_keymap; sp_kmap->state; ++sp_kmap){
            if(sp_kmap->scan_code == scan_code){
                *sp_kmap->state = !release;
                break;
            }
        }
    }
}

