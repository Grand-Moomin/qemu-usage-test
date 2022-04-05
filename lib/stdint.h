#ifndef __STDINT_H__
#define __STDINT_H__

typedef signed char int8_t;
#define INT8_MAX 127
#define INT8_MIN (-INT8_MAX - 1)

typedef unsigned char uint8_t;
#define UINT8_MAX 255

typedef signed short int int16_t;
#define INT16_MAX 32767
#define INT16_MIN (-INT16_MAX - 1)

typedef unsigned short int uint16_t;
#define UINT16_MAX 65535

typedef signed int int32_t;
#define INT32_MAX 2147483647
#define INT32_MIN (-INT32_MAX - 1)

typedef unsigned int uint32_t;
#define UINT32_MAX 4294967295U

typedef signed long long int int64_t;
#define INT64_MAX 9223372036854775807LL
#define INT64_MIN (-INT64_MAX - 1)

typedef unsigned long long int uint64_t;
#define UINT64_MAX 18446744073709551615ULL

typedef uint32_t uintptr_t;
#define UINTPTR_MAX UINT32_MAX

typedef int32_t intptr_t;
#define INTPTR_MIN INT32_MIN
#define INTPTR_MAX INT32_MAX

#endif