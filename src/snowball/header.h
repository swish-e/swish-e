
#include <limits.h>

#include "api.h"
#include "../swishtypes.h"

#define MAXINT INT_MAX
#define MININT INT_MIN

#define HEAD 2*sizeof(SWINT_T)

#define SIZE(p)        ((SWINT_T *)(p))[-1]
#define SET_SIZE(p, n) ((SWINT_T *)(p))[-1] = n
#define CAPACITY(p)    ((SWINT_T *)(p))[-2]

struct among
{   SWINT_T s_size;     /* number of chars in string */
    const symbol * s;       /* search string */
    SWINT_T substring_i;/* index to longest matching substring */
    SWINT_T result;     /* result of the lookup */
    SWINT_T (* function)(struct SN_env *);
};

extern symbol * create_s(void);
extern void lose_s(symbol * p);

extern SWINT_T skip_utf8(const symbol * p, SWINT_T c, SWINT_T lb, SWINT_T l, SWINT_T n);

extern SWINT_T in_grouping_U(struct SN_env * z, const unsigned char * s, SWINT_T min, SWINT_T max, SWINT_T repeat);
extern SWINT_T in_grouping_b_U(struct SN_env * z, const unsigned char * s, SWINT_T min, SWINT_T max, SWINT_T repeat);
extern SWINT_T out_grouping_U(struct SN_env * z, const unsigned char * s, SWINT_T min, SWINT_T max, SWINT_T repeat);
extern SWINT_T out_grouping_b_U(struct SN_env * z, const unsigned char * s, SWINT_T min, SWINT_T max, SWINT_T repeat);

extern SWINT_T in_grouping(struct SN_env * z, const unsigned char * s, SWINT_T min, SWINT_T max, SWINT_T repeat);
extern SWINT_T in_grouping_b(struct SN_env * z, const unsigned char * s, SWINT_T min, SWINT_T max, SWINT_T repeat);
extern SWINT_T out_grouping(struct SN_env * z, const unsigned char * s, SWINT_T min, SWINT_T max, SWINT_T repeat);
extern SWINT_T out_grouping_b(struct SN_env * z, const unsigned char * s, SWINT_T min, SWINT_T max, SWINT_T repeat);

extern SWINT_T eq_s(struct SN_env * z, SWINT_T s_size, const symbol * s);
extern SWINT_T eq_s_b(struct SN_env * z, SWINT_T s_size, const symbol * s);
extern SWINT_T eq_v(struct SN_env * z, const symbol * p);
extern SWINT_T eq_v_b(struct SN_env * z, const symbol * p);

extern SWINT_T find_among(struct SN_env * z, const struct among * v, SWINT_T v_size);
extern SWINT_T find_among_b(struct SN_env * z, const struct among * v, SWINT_T v_size);

extern SWINT_T replace_s(struct SN_env * z, SWINT_T c_bra, SWINT_T c_ket, SWINT_T s_size, const symbol * s, SWINT_T * adjustment);
extern SWINT_T slice_from_s(struct SN_env * z, SWINT_T s_size, const symbol * s);
extern SWINT_T slice_from_v(struct SN_env * z, const symbol * p);
extern SWINT_T slice_del(struct SN_env * z);

extern SWINT_T insert_s(struct SN_env * z, SWINT_T bra, SWINT_T ket, SWINT_T s_size, const symbol * s);
extern SWINT_T insert_v(struct SN_env * z, SWINT_T bra, SWINT_T ket, const symbol * p);

extern symbol * slice_to(struct SN_env * z, symbol * p);
extern symbol * assign_to(struct SN_env * z, symbol * p);

extern void debug(struct SN_env * z, SWINT_T number, SWINT_T line_count);

