#ifndef STEMMER_API_H
#define STEMMER_API_H 1
#include "../swishtypes.h"

typedef unsigned char symbol;

/* Or replace 'char' above with 'short' for 16 bit characters.

   More precisely, replace 'char' with whatever type guarantees the
   character width you need. Note however that sizeof(symbol) should divide
   HEAD, defined in header.h as 2*sizeof(SWINT_T), without remainder, otherwise
   there is an alignment problem. In the unlikely event of a problem here,
   consult Martin Porter.

*/

struct SN_env {
    symbol * p;
    SWINT_T c; SWINT_T l; SWINT_T lb; SWINT_T bra; SWINT_T ket;
    symbol * * S;
    SWINT_T * I;
    unsigned char * B;
};

extern struct SN_env * SN_create_env(SWINT_T S_size, SWINT_T I_size, SWINT_T B_size);
extern void SN_close_env(struct SN_env * z, SWINT_T S_size);

extern SWINT_T SN_set_current(struct SN_env * z, SWINT_T size, const symbol * s);

#endif
