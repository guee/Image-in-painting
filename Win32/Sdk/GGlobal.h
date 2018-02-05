#ifndef GGLOBAL_H
#define GGLOBAL_H
#define	G_EXPORT 


#include <stdint.h>
#include <math.h>
typedef float real_t;

template <typename T>
inline T gAbs(const T &t) { return t >= 0 ? t : -t; }

inline int gRound( float_t d)
{ return d >= real_t(0.0) ? int(d + real_t(0.5)) : int(d - int(d-1) + real_t(0.5)) + int(d-1); }
inline int gRound( double_t d )
{ return d >= real_t(0.0) ? int(d + real_t(0.5)) : int(d - int(d-1) + real_t(0.5)) + int(d-1); }

inline int64_t gRound64(double_t d)
{ return d >= 0.0 ? int64_t(d + 0.5) : int64_t(d - real_t( int64_t(d-1)) + 0.5) + int64_t(d-1); }


template <typename T>
inline const T &gMin(const T &a, const T &b) { return (a < b) ? a : b; }
template <typename T>
inline const T &gMax(const T &a, const T &b) { return (a < b) ? b : a; }
template <typename T>
inline const T &gBound(const T &min, const T &val, const T &max)
{ return gMax(min, gMin(max, val)); }

static inline bool gIsNull(double d)
{
    union U {
        double_t d;
		int64_t u;
    };
    U val;
    val.d = d;
    return val.u == int64_t(0);
}
static inline bool gIsNull(float f)
{
    union U {
        float_t f;
        uint32_t u;
    };
    U val;
    val.f = f;
    return val.u == 0u;
}

static inline bool gFuzzyCompare(double_t p1, double_t p2)
{
    return (gAbs(p1 - p2) <= 0.000000000001 * gMin(gAbs(p1), gAbs(p2)));
}

static inline bool gFuzzyCompare(float_t p1, float_t p2)
{
    return (gAbs(p1 - p2) <= 0.00001f * gMin(gAbs(p1), gAbs(p2)));
}

static inline bool gFuzzyIsNull( double_t d)
{
    return gAbs(d) <= 0.000000000001;
}

static inline bool gFuzzyIsNull( float_t f)
{
    return gAbs(f) <= 0.00001f;
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2 (M_PI / 2)
#endif

enum AspectRatioMode {
    IgnoreAspectRatio,
    KeepAspectRatio,
    KeepAspectRatioByExpanding
};

#endif GGLOBAL_H