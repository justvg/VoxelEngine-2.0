#pragma once

// TODO(georgy): Remove math.h
#include <math.h>
// TODO(georgy): This is for rand. Remove this!
#include <cstdlib>

#if COMPILER_MSVC
#define CompletePreviousWritesBeforeFutureWrites _WriteBarrier()
inline u32 AtomicCompareExchangeU32(u32 volatile *Value, u32 New, u32 Expected)
{
    u32 Result = _InterlockedCompareExchange((long *)Value, New, Expected);
    
	return(Result);
}
inline u32 AtomicExchangeU32(u32 volatile *Value, u32 New)
{
    u32 Result = _InterlockedExchange((long volatile *)Value, New);

    return(Result);
}
inline u64 AtomicExchangeU64(u64 volatile *Value, u64 New)
{
    u64 Result = _InterlockedExchange64((__int64 volatile *)Value, New);

    return(Result);
}
inline u32 AtomicIncrementU32(u32 volatile *Value)
{
    // NOTE(georgy): The return value is the resulting incremented value
    u32 Result = _InterlockedIncrement((long *)Value);
    return(Result);
}
inline u64 AtomicAddU64(u64 volatile *Value, u64 Addend)
{
    u64 Result = _InterlockedExchangeAdd64((__int64 volatile *)Value, Addend);
    return(Result);
}
inline u32 GetThreadID(void)
{
    u8 *ThreadLocalStorage = (u8 *)__readgsqword(0x30);
    u32 ThreadID = *(u32 *)(ThreadLocalStorage + 0x48);
    
    return(ThreadID);
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

inline r32
Tan(r32 Angle)
{
    r32 Result = tanf(Angle);
    return(Result);
}

inline u32
FindLeastSignificantSetBitIndex(u32 Value)
{
    u32 Result = 0;

#if COMPILER_MSVC
    _BitScanForward((unsigned long *)&Result, Value);
#endif

    return(Result);
}