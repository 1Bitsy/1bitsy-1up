#ifndef UTIL_H
#define UTIL_H

#define CAT2_H(a, b) a##b
#define CAT_H(a, b) CAT2_H(a, b)
#define TMPVAR_H() CAT_H(tmp__, __COUNTER__)


#define MIN(a, b)           MIN_H(a, b, TMPVAR_H(), TMPVAR_H())
#define MIN_H(a, b, t1, t2) ({                                          \
                                __typeof__ (a) t1 = (a);                \
                                __typeof__ (b) t2 = (b);                \
                                t1 < t2 ? t1 : t2;                      \
                            })

#define MAX(a, b)           MAX_H(a, b, TMPVAR_H(), TMPVAR_H())
#define MAX_H(a, b, t1, t2) ({                                          \
                                __typeof__ (a) t1 = (a);                \
                                __typeof__ (b) t2 = (b);                \
                                t1 > t2 ? t1 : t2;                      \
                            })

#define CLAMP(min, max, x) MAX(min, MIN(max, x))

// SIGN(x) * ABS(x) == x

#define ABS(a)              ABS_H(a, TMPVAR_H())
#define ABS_H(a, t)         ({                                          \
                                __typeof__ (a) t = (a);                 \
                                t >= 0 ? t: -t;                         \
                            })

#define SIGN(x)             SIGN_H(x, TMPVAR_H())
#define SIGN_H(x, t)        ({                                          \
                                __typeof__ (x) t = (x);                 \
                                t < 0 ? -1 : t > 0 ? +1 : 0;            \
                            })

// FLOOR(x) + FRAC(x) == x

#define FLOOR(x)            FLOOR_H(x, TMPVAR_H())
#define FLOOR_H(x, t)       ({                                          \
                                __typeof__ (x) t = (x);                 \
                                t >= 0 ? (int)t : -(int)-t;             \
                            })

#define CEIL(x)            CEIL_H(x, TMPVAR_H())
#define CEIL_H(x, t)       ({                                           \
                                __typeof__ (x) t = (x);                 \
                                t >= 0 ? -(int)-t : (int)t;             \
                            })

#define FRAC(x)             FRAC_H(x, TMPVAR_H())
#define FRAC_H(x, t)        ({                                          \
                                __typeof__ (x) t = (x);                 \
                                t >= 0 ? t - (int)t : t + (int)-t;      \
                            })

// FRAC(x) + RFRAC(x) == 1
#define RFRAC(x)            (1 - FRAC(x))

#define ROUND(x)            FLOOR(x + 0.5)

#endif /* UTIL_H */
