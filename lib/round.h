#ifndef __ROUND_H__
#define __ROUND_H__

//For X >= 0 and STEP >= 1

#define ROUND_UP(X, STEP) (((X) + (STEP) - 1) / (STEP) * (STEP))

#define ROUND_DOWN(X, STEP) ((X) / (STEP) * (STEP))

//For NOM >= 0 and DENOM >= 1
#define DIV_ROUND_UP(NOM, DENOM) (((NOM) + (DENOM) - 1) / (DENOM))

#endif