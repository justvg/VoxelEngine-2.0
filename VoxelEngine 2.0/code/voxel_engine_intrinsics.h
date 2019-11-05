#pragma once

// TODO(georgy): Remove math.h
#include <math.h>

#if COMPILER_MSVC
#define CompletePreviousWritesBeforeFutureWrites _WriteBarrier()
inline u32 AtomicCompareExchangeU32(u32 volatile *Value, u32 New, u32 Expected)
{
    u32 Result = _InterlockedCompareExchange((long *)Value, New, Expected);

    return(Result);
}
#endif

inline r32
SquareRoot(r32 Value)
{
    r32 Result = sqrtf(Value);
    return(Result);
}

inline i32
FloorReal32ToInt32(r32 Value)
{
    i32 Result = (i32)Value;
    if((r32)Result > Value)
    {
        Result--;
    }
    return(Result);
}

inline r32
Sin(r32 Angle)
{
    r32 Result = sinf(Angle);
    return(Result);
}

inline r32
Cos(r32 Angle)
{
    r32 Result = cosf(Angle);
    return(Result);
}

inline r32
ArcCos(r32 Value) 
{
    r32 Result = acosf(Value);
    return(Result);
}

inline r32
ATan2(r32 Y, r32 X)
{
    r32 Result = atan2f(Y, X);
    return(Result);
}