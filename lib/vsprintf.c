#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#define ZEROPAD		1			/* Pad with zeros */
#define LOWERCASE	(1<<1)		/* Use lowercase letters for hexadecimals */

#define is_digit_char(c) ((c) >= '0' && (c) <= '9')

/*	Convert a string to integer if possible while moving the string pointer.	*/
static int atoi_mod(const char **s)
{
	int res = 0;
	while(is_digit_char(**s)){
		res = res * 10 + **s - '0';
		(*s)++;
	}
	return res;
}

/*	'divl' : Divides the 64 bits value accross EDX:EAX by a value. 
	The result of the division is stored in EAX and the remainder in EDX.	*/
#define divl_rem(num, base) \
	({int rem; \
	asm ("divl %4" : "=a" (num), "=d" (rem) : "a" (num), "d" (0), "m" (base)); \
	rem;})

static char* itoa_base(char *str, unsigned long num, int base, int width, int flags)
{
	int num_width = 0;
	int i, zeros;
	char temp_buf[32];
	char *digit_chars = "0123456789ABCDEF";
	
	if(base < 2 || base > 16)
		return NULL;

	if(flags & LOWERCASE)
		digit_chars = "0123456789abcdef";

	if(num == 0)
		temp_buf[num_width++] = '0';
	else
		while(num != 0){
			temp_buf[num_width++] = digit_chars[divl_rem(num, base)];
		}

	if(flags & ZEROPAD){
		zeros = width - num_width;
		for(i = 0; i < zeros; ++i)
			*str++ = '0';
	}

	while(--num_width >= 0)
		*str++ = temp_buf[num_width];

	return str;
}

/* A simplified implementation of vsprintf, limited to support simple format like "%08lx", "%d", "%s", "c", etc.*/
int vsprintf(char *buf, const char *fmt, va_list args)
{
	char *str, *s;
	int flags = 0;
	int width = 0;
	int len, i, num;

	for(str = buf; *fmt; ++fmt){
		if(*fmt != '%'){
			*str++ = *fmt;
			continue;
		}
	
flag_settings:
		++fmt;
		switch(*fmt){
			case '0':
				flags |= ZEROPAD;
				goto flag_settings;
		}

		width = atoi_mod(&fmt);

		//Ignore l, L, h prefix before d,x.
		if(*fmt == 'l' || *fmt == 'L' || *fmt == 'h'){
			fmt++;
		}

		switch(*fmt){
			case 'c':
				*str++ = (char)va_arg(args, int);
				break;

			case 's':
				s = va_arg(args, char *);
				len = strlen(s);
				for(i = 0; i < len; ++i){
					*str++ = *s++;
				}
				break;

			case 'd':
			case 'i':
			case 'u':
				num = va_arg(args, int);
				if(*fmt != 'u' && num < 0){
					*str++ = '-';
					num = -num;
				}
				str = itoa_base(str, num, 10, width, flags);
				break;
			
			case 'o':
				num = va_arg(args, unsigned long);
				str = itoa_base(str, num, 8, width, flags);
				break;

			case 'x':
				flags |= LOWERCASE;
				//fallthrough
			case 'X':
				num = va_arg(args, unsigned long);
				str = itoa_base(str, num, 16, width, flags);
				break;

			case 'p':
				num = (unsigned long)va_arg(args, void *);
				str = itoa_base(str, num, 16, 8, flags);
				break;
		}
	}

	*str = '\0';
	return str - buf;
}

int sprintf(char *buf, const char *fmt, ...)
{
	va_list args;
    int num;

    va_start(args, fmt);
    num = vsprintf(buf, fmt, args);
    va_end(args);
    
    return num;
}