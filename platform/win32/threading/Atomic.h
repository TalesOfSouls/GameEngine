/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_WIN32_THREADING_ATOMIC_H
#define COMS_PLATFORM_WIN32_THREADING_ATOMIC_H

#include "../../../stdlib/Stdlib.h"
#include "../../../compiler/CompilerUtils.h"
#include <windows.h>

// We need the following helper types to "cast" between float and long.
// We can't just perform a "normal" cast since that re-interprets the bits. We need to maintain the bits
typedef union { LONG l; f32 f; } _atomic_32;
typedef union { LONG64 l; f64 f; } _atomic_64;

// We need this version as well because c++17 would make it annoying to define the value at initialization
// temp = { value } is only allowed if value has the same value as the first value in the union in c++17
typedef union { f32 f; LONG l; } _atomic_32f;
typedef union { f64 f; LONG64 l; } _atomic_64f;

// WARNING: Windows doesn't really have relaxed, release, acquire function on x86_64.
// You can see that by checking out how they are defined
// @bug As a result we are not always using the correct fenced/unfenced version on ARM
// (e.g. see _InterlockedCompareExchange8, it should be _InterlockedCompareExchange8_nf/rel/acq)
// To solve this we would probably have to make some of these functions Architecture specific in addition to platform specific

/**
 * We sometimes have basically the same function twice (e.g. _fetch_set_, _set_).
 * This is on purpose since the compiler may optimize the ASM by removing the fetch part
 * if it is not needed despite using the same intrinsic as long as the return value is not used.
 */

FORCE_INLINE void atomic_set_relaxed(void** target, void* new_pointer) NO_EXCEPT
{ InterlockedExchangePointerNoFence(target, new_pointer); }
FORCE_INLINE void* atomic_get_relaxed(void** target) NO_EXCEPT
{ return InterlockedCompareExchangePointerNoFence(target, NULL, NULL); }
FORCE_INLINE void atomic_set_relaxed(int8* value, int8 new_value) NO_EXCEPT
{ InterlockedExchangeNoFence8((char *) value, new_value); }
FORCE_INLINE void atomic_set_relaxed(int16* value, int16 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeNoFence16((short *) value, new_value); }
FORCE_INLINE void atomic_set_relaxed(int32* value, int32 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedExchangeNoFence((long *) value, new_value); }
FORCE_INLINE void atomic_set_relaxed(int64* value, int64 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedExchangeNoFence64((LONG64 *) value, (LONG64) new_value); }
FORCE_INLINE void atomic_set_relaxed(f32* value, f32 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); _atomic_32f temp = {new_value}; InterlockedExchangeNoFence((long *) value, (long) temp.l); }
FORCE_INLINE void atomic_set_relaxed(f64* value, f64 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); _atomic_64f temp = {new_value}; InterlockedExchangeNoFence64((LONG64 *) value, (LONG64) temp.l); }
FORCE_INLINE int8 atomic_fetch_set_relaxed(int8* value, int8 new_value) NO_EXCEPT
{ return (int8) InterlockedExchangeNoFence8((char *) value, (char) new_value); }
FORCE_INLINE int16 atomic_fetch_set_relaxed(int16* value, int16 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (int16) InterlockedExchangeNoFence16((short *) value, (short) new_value); }
FORCE_INLINE int32 atomic_fetch_set_relaxed(int32* value, int32 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedExchangeNoFence((long *) value, new_value); }
FORCE_INLINE int64 atomic_fetch_set_relaxed(int64* value, int64 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedExchangeNoFence64((LONG64 *) value, (LONG64) new_value); }
FORCE_INLINE int8 atomic_get_relaxed(const int8* value) NO_EXCEPT
{ return (int8) _InterlockedCompareExchange8((char *) value, 0, 0); }
FORCE_INLINE int16 atomic_get_relaxed(const int16* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (int16) InterlockedCompareExchangeNoFence16((short *) value, 0, 0); }
FORCE_INLINE int32 atomic_get_relaxed(const int32* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedCompareExchangeNoFence((long *) value, 0, 0); }
FORCE_INLINE int64 atomic_get_relaxed(const int64* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedCompareExchangeNoFence64((LONG64 *) value, 0, 0); }
FORCE_INLINE f32 atomic_get_relaxed(const f32* value) NO_EXCEPT
{ _atomic_32 temp = {InterlockedCompareExchangeNoFence((long *) value, 0, 0)}; return temp.f; }
FORCE_INLINE f64 atomic_get_relaxed(const f64* value) NO_EXCEPT
{ _atomic_64 temp = {InterlockedCompareExchangeNoFence64((LONG64 *) value, 0, 0)}; return temp.f; }
FORCE_INLINE int8 atomic_increment_relaxed(int8* value) NO_EXCEPT
{ return InterlockedExchangeAdd8((char *) value, 1); }
FORCE_INLINE int8 atomic_decrement_relaxed(int8* value) NO_EXCEPT
{ return InterlockedExchangeAdd8((char *) value, -1); }
FORCE_INLINE int16 atomic_increment_relaxed(int16* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return InterlockedIncrementNoFence16((short *) value); }
FORCE_INLINE int16 atomic_decrement_relaxed(int16* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return InterlockedDecrementNoFence16((short *) value); }
FORCE_INLINE int32 atomic_increment_relaxed(int32* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return InterlockedIncrementNoFence((long *) value); }
FORCE_INLINE int32 atomic_decrement_relaxed(int32* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return InterlockedDecrementNoFence((long *) value); }
FORCE_INLINE int64 atomic_increment_relaxed(int64* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return InterlockedIncrementNoFence64((LONG64 *) value); }
FORCE_INLINE int64 atomic_decrement_relaxed(int64* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return InterlockedDecrementNoFence64((LONG64 *) value); }
FORCE_INLINE void atomic_add_relaxed(int8* value, int8 increment) NO_EXCEPT
{ InterlockedExchangeAdd8((char *) value, (char) increment); }
FORCE_INLINE void atomic_sub_relaxed(int8* value, int8 decrement) NO_EXCEPT
{ InterlockedExchangeAdd8((char *) value, -((char) decrement)); }
FORCE_INLINE void atomic_add_relaxed(int16* value, int16 increment) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeAdd16((short *) value, (short) increment); }
FORCE_INLINE void atomic_sub_relaxed(int16* value, int16 decrement) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeAdd16((short *) value, -((short) decrement)); }
FORCE_INLINE void atomic_add_relaxed(int32* value, int32 increment) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAddNoFence((long *) value, increment); }
FORCE_INLINE void atomic_sub_relaxed(int32* value, int32 decrement) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAddNoFence((long *) value, -decrement); }
FORCE_INLINE void atomic_add_relaxed(int64* value, int64 increment) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAddNoFence64((LONG64 *) value, (LONG64) increment); }
FORCE_INLINE void atomic_sub_relaxed(int64* value, int64 decrement) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAddNoFence64((LONG64 *) value, -((LONG64) decrement)); }
FORCE_INLINE f32 atomic_compare_exchange_strong_relaxed(f32* value, f32 expected, f32 desired) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); _atomic_32 temp = {InterlockedCompareExchangeNoFence((long *) value, (long) desired, (long) expected) }; return temp.f; }
FORCE_INLINE f64 atomic_compare_exchange_strong_relaxed(f64* value, f64 expected, f64 desired) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); _atomic_64 temp = {InterlockedCompareExchangeNoFence64((LONG64 *) value, (LONG64) desired, (LONG64) expected) }; return temp.f; }
FORCE_INLINE int32 atomic_compare_exchange_strong_relaxed(int32* value, int32 expected, int32 desired) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedCompareExchangeNoFence((long *) value, desired, expected); }
FORCE_INLINE int64 atomic_compare_exchange_strong_relaxed(int64* value, int64 expected, int64 desired) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedCompareExchangeNoFence64((LONG64 *) value, (LONG64) desired, (LONG64) expected); }
FORCE_INLINE int8 atomic_fetch_add_relaxed(int8* value, int8 operand) NO_EXCEPT
{ return (int8) InterlockedExchangeAdd8((char *) value, (char) operand); }
FORCE_INLINE int8 atomic_fetch_sub_relaxed(int8* value, int8 operand) NO_EXCEPT
{ return (int8) InterlockedExchangeAdd8((char *) value, -((char) operand)); }
FORCE_INLINE int16 atomic_fetch_add_relaxed(int16* value, int16 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (int16) InterlockedExchangeAdd16((short *) value, (short) operand); }
FORCE_INLINE int16 atomic_fetch_sub_relaxed(int16* value, int16 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (int16) InterlockedExchangeAdd16((short *) value, -((short) operand)); }
FORCE_INLINE int32 atomic_fetch_add_relaxed(int32* value, int32 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedExchangeAddNoFence((long *) value, operand); }
FORCE_INLINE int32 atomic_fetch_sub_relaxed(int32* value, int32 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedExchangeAddNoFence((unsigned long *) value, -((long) operand)); }
FORCE_INLINE int64 atomic_fetch_add_relaxed(int64* value, int64 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedExchangeAddNoFence64((LONG64 *) value, (LONG64) operand); }
FORCE_INLINE int64 atomic_fetch_sub_relaxed(int64* value, int64 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedExchangeAdd64((LONG64 *) value, -((LONG64) operand)); }
FORCE_INLINE void atomic_set_relaxed(uint8* value, uint8 new_value) NO_EXCEPT
{ InterlockedExchangeNoFence8((char *) value, (char) new_value); }
FORCE_INLINE void atomic_set_relaxed(uint16* value, uint16 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeNoFence16((short *) value, (short) new_value); }
FORCE_INLINE void atomic_set_relaxed(uint32* value, uint32 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedExchangeNoFence((long *) value, new_value); }
FORCE_INLINE void atomic_set_relaxed(uint64* value, uint64 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedExchangeNoFence64((LONG64 *) value, (LONG64) new_value); }
FORCE_INLINE uint8 atomic_fetch_set_relaxed(uint8* value, uint8 new_value) NO_EXCEPT
{ return (uint8) InterlockedExchangeNoFence8((char *) value, (char) new_value); }
FORCE_INLINE uint16 atomic_fetch_set_relaxed(uint16* value, uint16 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (uint16) InterlockedExchangeNoFence16((short *) value, (short) new_value); }
FORCE_INLINE uint32 atomic_fetch_set_relaxed(uint32* value, uint32 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedExchangeNoFence((long *) value, new_value); }
FORCE_INLINE uint64 atomic_fetch_set_relaxed(uint64* value, uint64 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedExchangeNoFence64((LONG64 *) value, (LONG64) new_value); }
FORCE_INLINE uint8 atomic_get_relaxed(const uint8* value) NO_EXCEPT
{ return (uint8) _InterlockedCompareExchange8((char *) value, 0, 0); }
FORCE_INLINE uint16 atomic_get_relaxed(const uint16* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (uint16) InterlockedCompareExchangeNoFence16((short *) value, 0, 0); }
FORCE_INLINE uint32 atomic_get_relaxed(const uint32* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedCompareExchangeNoFence((long *) value, 0, 0); }
FORCE_INLINE uint64 atomic_get_relaxed(const uint64* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedCompareExchangeNoFence64((LONG64 *) value, 0, 0); }
FORCE_INLINE uint8 atomic_increment_relaxed(uint8* value) NO_EXCEPT
{ return InterlockedExchangeAdd8((char *) value, 1); }
FORCE_INLINE uint8 atomic_decrement_relaxed(uint8* value) NO_EXCEPT
{ return InterlockedExchangeAdd8((char *) value, -1); }
FORCE_INLINE uint16 atomic_increment_relaxed(uint16* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return InterlockedIncrementNoFence16((short *) value); }
FORCE_INLINE uint16 atomic_decrement_relaxed(uint16* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return InterlockedDecrementNoFence16((short *) value); }
FORCE_INLINE uint32 atomic_increment_relaxed(uint32* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return InterlockedIncrementNoFence((long *) value); }
FORCE_INLINE uint32 atomic_decrement_relaxed(uint32* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return InterlockedDecrementNoFence((long *) value); }
FORCE_INLINE uint64 atomic_increment_relaxed(uint64* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return InterlockedIncrementNoFence64((LONG64 *) value); }
FORCE_INLINE uint64 atomic_decrement_relaxed(uint64* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return InterlockedDecrementNoFence64((LONG64 *) value); }
FORCE_INLINE void atomic_add_relaxed(uint8* value, uint8 increment) NO_EXCEPT
{ InterlockedExchangeAdd8((char *) value, (char) increment); }
FORCE_INLINE void atomic_sub_relaxed(uint8* value, uint8 decrement) NO_EXCEPT
{ InterlockedExchangeAdd8((char *) value, -((char) decrement)); }
FORCE_INLINE void atomic_add_relaxed(uint16* value, uint16 increment) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeAdd16((short *) value, (short) increment); }
FORCE_INLINE void atomic_sub_relaxed(uint16* value, uint16 decrement) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeAdd16((short *) value, -((short) decrement)); }
FORCE_INLINE void atomic_add_relaxed(uint32* value, uint32 increment) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAddNoFence((long *) value, increment); }
FORCE_INLINE void atomic_sub_relaxed(uint32* value, uint32 decrement) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAddNoFence((long *) value, -1 * ((int32) decrement)); }
FORCE_INLINE void atomic_add_relaxed(uint64* value, uint64 increment) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAddNoFence64((LONG64 *) value, (LONG64) increment); }
FORCE_INLINE void atomic_sub_relaxed(uint64* value, uint64 decrement) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAddNoFence64((LONG64 *) value, -((LONG64) decrement)); }
FORCE_INLINE uint32 atomic_compare_exchange_strong_relaxed(uint32* value, uint32 expected, uint32 desired) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedCompareExchangeNoFence((long *) value, desired, expected); }
FORCE_INLINE uint64 atomic_compare_exchange_strong_relaxed(uint64* value, uint64 expected, uint64 desired) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedCompareExchangeNoFence64((LONG64 *) value, (LONG64) desired, (LONG64) expected); }
FORCE_INLINE uint8 atomic_fetch_add_relaxed(uint8* value, uint8 operand) NO_EXCEPT
{ return (uint8) InterlockedExchangeAdd8((char *) value, (char) operand); }
FORCE_INLINE uint8 atomic_fetch_sub_relaxed(uint8* value, uint8 operand) NO_EXCEPT
{ return (uint8) InterlockedExchangeAdd8((char *) value, -((char) operand)); }
FORCE_INLINE uint16 atomic_fetch_add_relaxed(uint16* value, uint16 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (uint16) InterlockedExchangeAdd16((short *) value, (short) operand); }
FORCE_INLINE uint16 atomic_fetch_sub_relaxed(uint16* value, uint16 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (uint16) InterlockedExchangeAdd16((short *) value, -((short) operand)); }
FORCE_INLINE uint32 atomic_fetch_add_relaxed(uint32* value, uint32 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedExchangeAddNoFence((long *) value, operand); }
FORCE_INLINE uint32 atomic_fetch_sub_relaxed(uint32* value, uint32 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedExchangeAddNoFence((unsigned long *) value, -((long) operand)); }
FORCE_INLINE uint64 atomic_fetch_add_relaxed(uint64* value, uint64 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedExchangeAddNoFence64((LONG64 *) value, (LONG64) operand); }
FORCE_INLINE uint64 atomic_fetch_sub_relaxed(uint64* value, uint64 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedExchangeAdd64((LONG64 *) value, -((LONG64) operand)); }
FORCE_INLINE void atomic_and_relaxed(uint8* value, uint8 mask) NO_EXCEPT
{ InterlockedAnd8((char *) value, mask); }
FORCE_INLINE void atomic_and_relaxed(int8* value, int8 mask) NO_EXCEPT
{ InterlockedAnd8((char *) value, mask); }
FORCE_INLINE void atomic_and_relaxed(uint16* value, uint16 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedAnd16((short *) value, mask); }
FORCE_INLINE void atomic_and_relaxed(int16* value, int16 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedAnd16((short *) value, mask); }
FORCE_INLINE void atomic_and_relaxed(uint32* value, uint32 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAndNoFence((LONG *) value, mask); }
FORCE_INLINE void atomic_and_relaxed(int32* value, int32 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAndNoFence((LONG *) value, (LONG)mask); }
FORCE_INLINE void atomic_and_relaxed(uint64* value, uint64 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAnd64NoFence((LONG64 *) value, mask); }
FORCE_INLINE void atomic_and_relaxed(int64* value, int64 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAnd64NoFence((LONG64 *) value, mask); }
FORCE_INLINE void atomic_or_relaxed(uint8* value, uint8 mask) NO_EXCEPT
{ InterlockedOr8((char *) value, mask); }
FORCE_INLINE void atomic_or_relaxed(int8* value, int8 mask) NO_EXCEPT
{ InterlockedOr8((char *) value, mask); }
FORCE_INLINE void atomic_or_relaxed(uint16* value, uint16 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedOr16((short *) value, mask); }
FORCE_INLINE void atomic_or_relaxed(int16* value, int16 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedOr16((short *) value, mask); }
FORCE_INLINE void atomic_or_relaxed(uint32* value, uint32 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedOrNoFence((LONG *) value, mask); }
FORCE_INLINE void atomic_or_relaxed(int32* value, int32 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedOrNoFence((LONG *) value, (LONG)mask); }
FORCE_INLINE void atomic_or_relaxed(uint64* value, uint64 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedOr64NoFence((LONG64 *) value, mask); }
FORCE_INLINE void atomic_or_relaxed(int64* value, int64 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedOr64NoFence((LONG64 *) value, mask); }

FORCE_INLINE void* atomic_get_acquire(void** target) NO_EXCEPT
{ return InterlockedCompareExchangePointerAcquire(target, NULL, NULL); }
FORCE_INLINE int8 atomic_fetch_set_acquire(int8* value, int8 new_value) NO_EXCEPT
{ return (int8) InterlockedExchangeAcquire8((char *) value, (char) new_value); }
FORCE_INLINE int16 atomic_fetch_set_acquire(int16* value, int16 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (int16) InterlockedExchangeAcquire16((short *) value, (short) new_value); }
FORCE_INLINE int32 atomic_fetch_set_acquire(int32* value, int32 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedExchangeAcquire((long *) value, new_value); }
FORCE_INLINE int64 atomic_fetch_set_acquire(int64* value, int64 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedExchangeAcquire64((LONG64 *) value, (LONG64) new_value); }
FORCE_INLINE int8 atomic_get_acquire(const int8* value) NO_EXCEPT
{ return (int8) _InterlockedCompareExchange8((char *) value, 0, 0); }
FORCE_INLINE int16 atomic_get_acquire(const int16* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (int16) InterlockedCompareExchangeAcquire16((short *) value, 0, 0); }
FORCE_INLINE int32 atomic_get_acquire(const int32* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedCompareExchangeAcquire((long *) value, 0, 0); }
FORCE_INLINE int64 atomic_get_acquire(const int64* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedCompareExchangeAcquire64((LONG64 *) value, 0, 0); }
FORCE_INLINE f32 atomic_get_acquire(const f32* value) NO_EXCEPT
{ _atomic_32 temp = {InterlockedCompareExchangeAcquire((long *) value, 0, 0)}; return temp.f; }
FORCE_INLINE f64 atomic_get_acquire(const f64* value) NO_EXCEPT
{ _atomic_64 temp = {InterlockedCompareExchangeAcquire64((LONG64 *) value, 0, 0)}; return temp.f; }
FORCE_INLINE int8 atomic_increment_acquire(int8* value) NO_EXCEPT
{ return InterlockedExchangeAdd8((char *) value, 1); }
FORCE_INLINE int8 atomic_decrement_acquire(int8* value) NO_EXCEPT
{ return InterlockedExchangeAdd8((char *) value, -1); }
FORCE_INLINE int16 atomic_increment_acquire(int16* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return InterlockedIncrementAcquire16((short *) value); }
FORCE_INLINE int16 atomic_decrement_acquire(int16* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return InterlockedDecrementAcquire16((short *) value); }
FORCE_INLINE int32 atomic_increment_acquire(int32* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return InterlockedIncrementAcquire((long *) value); }
FORCE_INLINE int32 atomic_decrement_acquire(int32* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return InterlockedDecrementAcquire((long *) value); }
FORCE_INLINE int64 atomic_increment_acquire(int64* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return InterlockedIncrementAcquire64((LONG64 *) value); }
FORCE_INLINE int64 atomic_decrement_acquire(int64* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return InterlockedDecrementAcquire64((LONG64 *) value); }
FORCE_INLINE void atomic_add_acquire(int8* value, int8 increment) NO_EXCEPT
{ InterlockedExchangeAdd8((char *) value, (char) increment); }
FORCE_INLINE void atomic_sub_acquire(int8* value, int8 decrement) NO_EXCEPT
{ InterlockedExchangeAdd8((char *) value, -((char) decrement)); }
FORCE_INLINE void atomic_add_acquire(int16* value, int16 increment) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeAdd16((short *) value, (short) increment); }
FORCE_INLINE void atomic_sub_acquire(int16* value, int16 decrement) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeAdd16((short *) value, -((short) decrement)); }
FORCE_INLINE void atomic_add_acquire(int32* value, int32 increment) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAddAcquire((long *) value, increment); }
FORCE_INLINE void atomic_sub_acquire(int32* value, int32 decrement) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAddAcquire((long *) value, -decrement); }
FORCE_INLINE void atomic_add_acquire(int64* value, int64 increment) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAddAcquire64((LONG64 *) value, (LONG64) increment); }
FORCE_INLINE void atomic_sub_acquire(int64* value, int64 decrement) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAddAcquire64((LONG64 *) value, -((LONG64) decrement)); }
FORCE_INLINE f32 atomic_compare_exchange_strong_acquire(f32* value, f32 expected, f32 desired) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); _atomic_32 temp = {InterlockedCompareExchangeAcquire((long *) value, (long) desired, (long) expected) }; return temp.f; }
FORCE_INLINE f64 atomic_compare_exchange_strong_acquire(f64* value, f64 expected, f64 desired) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); _atomic_64 temp = {InterlockedCompareExchangeAcquire64((LONG64 *) value, (LONG64) desired, (LONG64) expected) }; return temp.f; }
FORCE_INLINE int32 atomic_compare_exchange_strong_acquire(int32* value, int32 expected, int32 desired) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedCompareExchangeAcquire((long *) value, desired, expected); }
FORCE_INLINE int64 atomic_compare_exchange_strong_acquire(int64* value, int64 expected, int64 desired) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedCompareExchangeAcquire64((LONG64 *) value, (LONG64) desired, (LONG64) expected); }
FORCE_INLINE int8 atomic_fetch_add_acquire(int8* value, int8 operand) NO_EXCEPT
{ return (int8) InterlockedExchangeAdd8((char *) value, (char) operand); }
FORCE_INLINE int8 atomic_fetch_sub_acquire(int8* value, int8 operand) NO_EXCEPT
{ return (int8) InterlockedExchangeAdd8((char *) value, -((char) operand)); }
FORCE_INLINE int16 atomic_fetch_add_acquire(int16* value, int16 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (int16) InterlockedExchangeAdd16((short *) value, (short) operand); }
FORCE_INLINE int16 atomic_fetch_sub_acquire(int16* value, int16 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (int16) InterlockedExchangeAdd16((short *) value, -((short) operand)); }
FORCE_INLINE int32 atomic_fetch_add_acquire(int32* value, int32 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedExchangeAddAcquire((long *) value, operand); }
FORCE_INLINE int32 atomic_fetch_sub_acquire(int32* value, int32 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedExchangeAddAcquire((unsigned long *) value, -((long) operand)); }
FORCE_INLINE int64 atomic_fetch_add_acquire(int64* value, int64 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedExchangeAddAcquire64((LONG64 *) value, (LONG64) operand); }
FORCE_INLINE int64 atomic_fetch_sub_acquire(int64* value, int64 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedExchangeAdd64((LONG64 *) value, -((LONG64) operand)); }
FORCE_INLINE void atomic_set_acquire(uint8* value, uint8 new_value) NO_EXCEPT
{ InterlockedExchangeAcquire8((char *) value, (char) new_value); }
FORCE_INLINE void atomic_set_acquire(uint16* value, uint16 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeAcquire16((short *) value, (short) new_value); }
FORCE_INLINE void atomic_set_acquire(uint32* value, uint32 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedExchangeAcquire((long *) value, new_value); }
FORCE_INLINE void atomic_set_acquire(uint64* value, uint64 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedExchangeAcquire64((LONG64 *) value, (LONG64) new_value); }
FORCE_INLINE uint8 atomic_fetch_set_acquire(uint8* value, uint8 new_value) NO_EXCEPT
{ return (uint8) InterlockedExchangeAcquire8((char *) value, (char) new_value); }
FORCE_INLINE uint16 atomic_fetch_set_acquire(uint16* value, uint16 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (uint16) InterlockedExchangeAcquire16((short *) value, (short) new_value); }
FORCE_INLINE uint32 atomic_fetch_set_acquire(uint32* value, uint32 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedExchangeAcquire((long *) value, new_value); }
FORCE_INLINE uint64 atomic_fetch_set_acquire(uint64* value, uint64 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedExchangeAcquire64((LONG64 *) value, (LONG64) new_value); }
FORCE_INLINE uint8 atomic_get_acquire(const uint8* value) NO_EXCEPT
{ return (uint8) _InterlockedCompareExchange8((char *) value, 0, 0); }
FORCE_INLINE uint16 atomic_get_acquire(const uint16* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (uint16) InterlockedCompareExchangeAcquire16((short *) value, 0, 0); }
FORCE_INLINE uint32 atomic_get_acquire(const uint32* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedCompareExchangeAcquire((long *) value, 0, 0); }
FORCE_INLINE uint64 atomic_get_acquire(const uint64* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedCompareExchangeAcquire64((LONG64 *) value, 0, 0); }
FORCE_INLINE uint8 atomic_increment_acquire(uint8* value) NO_EXCEPT
{ return InterlockedExchangeAdd8((char *) value, 1); }
FORCE_INLINE uint8 atomic_decrement_acquire(uint8* value) NO_EXCEPT
{ return InterlockedExchangeAdd8((char *) value, -1); }
FORCE_INLINE uint16 atomic_increment_acquire(uint16* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return InterlockedIncrementAcquire16((short *) value); }
FORCE_INLINE uint16 atomic_decrement_acquire(uint16* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return InterlockedDecrementAcquire16((short *) value); }
FORCE_INLINE uint32 atomic_increment_acquire(uint32* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return InterlockedIncrementAcquire((long *) value); }
FORCE_INLINE uint32 atomic_decrement_acquire(uint32* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return InterlockedDecrementAcquire((long *) value); }
FORCE_INLINE uint64 atomic_increment_acquire(uint64* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return InterlockedIncrementAcquire64((LONG64 *) value); }
FORCE_INLINE uint64 atomic_decrement_acquire(uint64* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return InterlockedDecrementAcquire64((LONG64 *) value); }
FORCE_INLINE void atomic_add_acquire(uint8* value, uint8 increment) NO_EXCEPT
{ InterlockedExchangeAdd8((char *) value, (char) increment); }
FORCE_INLINE void atomic_sub_acquire(uint8* value, uint8 decrement) NO_EXCEPT
{ InterlockedExchangeAdd8((char *) value, -((char) decrement)); }
FORCE_INLINE void atomic_add_acquire(uint16* value, uint16 increment) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeAdd16((short *) value, (short) increment); }
FORCE_INLINE void atomic_sub_acquire(uint16* value, uint16 decrement) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeAdd16((short *) value, -((short) decrement)); }
FORCE_INLINE void atomic_add_acquire(uint32* value, uint32 increment) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAddAcquire((long *) value, increment); }
FORCE_INLINE void atomic_sub_acquire(uint32* value, uint32 decrement) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAddAcquire((long *) value, -1 * ((int32) decrement)); }
FORCE_INLINE void atomic_add_acquire(uint64* value, uint64 increment) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAddAcquire64((LONG64 *) value, (LONG64) increment); }
FORCE_INLINE void atomic_sub_acquire(uint64* value, uint64 decrement) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAddAcquire64((LONG64 *) value, -((LONG64) decrement)); }
FORCE_INLINE uint32 atomic_compare_exchange_strong_acquire(uint32* value, uint32 expected, uint32 desired) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedCompareExchangeAcquire((long *) value, desired, expected); }
FORCE_INLINE uint64 atomic_compare_exchange_strong_acquire(uint64* value, uint64 expected, uint64 desired) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedCompareExchangeAcquire64((LONG64 *) value, (LONG64) desired, (LONG64) expected); }
FORCE_INLINE uint8 atomic_fetch_add_acquire(uint8* value, uint8 operand) NO_EXCEPT
{ return (uint8) InterlockedExchangeAdd8((char *) value, (char) operand); }
FORCE_INLINE uint8 atomic_fetch_sub_acquire(uint8* value, uint8 operand) NO_EXCEPT
{ return (uint8) InterlockedExchangeAdd8((char *) value, -((char) operand)); }
FORCE_INLINE uint16 atomic_fetch_add_acquire(uint16* value, uint16 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (uint16) InterlockedExchangeAdd16((short *) value, (short) operand); }
FORCE_INLINE uint16 atomic_fetch_sub_acquire(uint16* value, uint16 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (uint16) InterlockedExchangeAdd16((short *) value, -((short) operand)); }
FORCE_INLINE uint32 atomic_fetch_add_acquire(uint32* value, uint32 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedExchangeAddAcquire((long *) value, operand); }
FORCE_INLINE uint32 atomic_fetch_sub_acquire(uint32* value, uint32 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedExchangeAddAcquire((unsigned long *) value, -((long) operand)); }
FORCE_INLINE uint64 atomic_fetch_add_acquire(uint64* value, uint64 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedExchangeAddAcquire64((LONG64 *) value, (LONG64) operand); }
FORCE_INLINE uint64 atomic_fetch_sub_acquire(uint64* value, uint64 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedExchangeAdd64((LONG64 *) value, -((LONG64) operand)); }
FORCE_INLINE void atomic_and_acquire(uint8* value, uint8 mask) NO_EXCEPT
{ InterlockedAnd8((char *) value, mask); }
FORCE_INLINE void atomic_and_acquire(int8* value, int8 mask) NO_EXCEPT
{ InterlockedAnd8((char *) value, mask); }
FORCE_INLINE void atomic_and_acquire(uint16* value, uint16 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedAnd16((short *) value, mask); }
FORCE_INLINE void atomic_and_acquire(int16* value, int16 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedAnd16((short *) value, mask); }
FORCE_INLINE void atomic_and_acquire(uint32* value, uint32 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAndAcquire((LONG *) value, mask); }
FORCE_INLINE void atomic_and_acquire(int32* value, int32 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAndAcquire((LONG *) value, (LONG)mask); }
FORCE_INLINE void atomic_and_acquire(uint64* value, uint64 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAnd64Acquire((LONG64 *) value, mask); }
FORCE_INLINE void atomic_and_acquire(int64* value, int64 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAnd64Acquire((LONG64 *) value, mask); }
FORCE_INLINE void atomic_or_acquire(uint8* value, uint8 mask) NO_EXCEPT
{ InterlockedOr8((char *) value, mask); }
FORCE_INLINE void atomic_or_acquire(int8* value, int8 mask) NO_EXCEPT
{ InterlockedOr8((char *) value, mask); }
FORCE_INLINE void atomic_or_acquire(uint16* value, uint16 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedOr16((short *) value, mask); }
FORCE_INLINE void atomic_or_acquire(int16* value, int16 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedOr16((short *) value, mask); }
FORCE_INLINE void atomic_or_acquire(uint32* value, uint32 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedOrAcquire((LONG *) value, mask); }
FORCE_INLINE void atomic_or_acquire(int32* value, int32 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedOrAcquire((LONG *) value, (LONG)mask); }
FORCE_INLINE void atomic_or_acquire(uint64* value, uint64 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedOr64Acquire((LONG64 *) value, mask); }
FORCE_INLINE void atomic_or_acquire(int64* value, int64 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedOr64Acquire((LONG64 *) value, mask); }

FORCE_INLINE void atomic_set_release(void** target, void* new_pointer) NO_EXCEPT
{ InterlockedExchangePointer(target, new_pointer); }
FORCE_INLINE void* atomic_get_release(void** target) NO_EXCEPT
{ return InterlockedCompareExchangePointerRelease(target, NULL, NULL); }
FORCE_INLINE void atomic_set_release(int8* value, int8 new_value) NO_EXCEPT
{ InterlockedExchange8((char *) value, new_value); }
FORCE_INLINE void atomic_set_release(int16* value, int16 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchange16((short *) value, new_value); }
FORCE_INLINE void atomic_set_release(int32* value, int32 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedExchange((long *) value, new_value); }
FORCE_INLINE void atomic_set_release(int64* value, int64 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedExchange64((LONG64 *) value, (LONG64) new_value); }
FORCE_INLINE void atomic_set_release(f32* value, f32 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); _atomic_32f temp = {new_value}; InterlockedExchange((long *) value, (long) temp.l); }
FORCE_INLINE void atomic_set_release(f64* value, f64 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); _atomic_64f temp = {new_value}; InterlockedExchange64((LONG64 *) value, (LONG64) temp.l); }
FORCE_INLINE int8 atomic_fetch_set_release(int8* value, int8 new_value) NO_EXCEPT
{ return (int8) InterlockedExchange8((char *) value, (char) new_value); }
FORCE_INLINE int16 atomic_fetch_set_release(int16* value, int16 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (int16) InterlockedExchange16((short *) value, (short) new_value); }
FORCE_INLINE int32 atomic_fetch_set_release(int32* value, int32 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedExchange((long *) value, new_value); }
FORCE_INLINE int64 atomic_fetch_set_release(int64* value, int64 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedExchange64((LONG64 *) value, (LONG64) new_value); }
FORCE_INLINE int8 atomic_get_release(const int8* value) NO_EXCEPT
{ return (int8) _InterlockedCompareExchange8((char *) value, 0, 0); }
FORCE_INLINE int16 atomic_get_release(const int16* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (int16) InterlockedCompareExchangeRelease16((short *) value, 0, 0); }
FORCE_INLINE int32 atomic_get_release(const int32* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedCompareExchangeRelease((long *) value, 0, 0); }
FORCE_INLINE int64 atomic_get_release(const int64* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedCompareExchangeRelease64((LONG64 *) value, 0, 0); }
FORCE_INLINE f32 atomic_get_release(const f32* value) NO_EXCEPT
{ _atomic_32 temp = {InterlockedCompareExchangeRelease((long *) value, 0, 0)}; return temp.f; }
FORCE_INLINE f64 atomic_get_release(const f64* value) NO_EXCEPT
{ _atomic_64 temp = {InterlockedCompareExchangeRelease64((LONG64 *) value, 0, 0)}; return temp.f; }
FORCE_INLINE int8 atomic_increment_release(int8* value) NO_EXCEPT
{ return InterlockedExchangeAdd8((char *) value, 1); }
FORCE_INLINE int8 atomic_decrement_release(int8* value) NO_EXCEPT
{ return InterlockedExchangeAdd8((char *) value, -1); }
FORCE_INLINE int16 atomic_increment_release(int16* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return InterlockedIncrementRelease16((short *) value); }
FORCE_INLINE int16 atomic_decrement_release(int16* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return InterlockedDecrementRelease16((short *) value); }
FORCE_INLINE int32 atomic_increment_release(int32* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return InterlockedIncrementRelease((long *) value); }
FORCE_INLINE int32 atomic_decrement_release(int32* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return InterlockedDecrementRelease((long *) value); }
FORCE_INLINE int64 atomic_increment_release(int64* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return InterlockedIncrementRelease64((LONG64 *) value); }
FORCE_INLINE int64 atomic_decrement_release(int64* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return InterlockedDecrementRelease64((LONG64 *) value); }
FORCE_INLINE void atomic_add_release(int8* value, int8 increment) NO_EXCEPT
{ InterlockedExchangeAdd8((char *) value, (char) increment); }
FORCE_INLINE void atomic_sub_release(int8* value, int8 decrement) NO_EXCEPT
{ InterlockedExchangeAdd8((char *) value, -((char) decrement)); }
FORCE_INLINE void atomic_add_release(int16* value, int16 increment) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeAdd16((short *) value, (short) increment); }
FORCE_INLINE void atomic_sub_release(int16* value, int16 decrement) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeAdd16((short *) value, -((short) decrement)); }
FORCE_INLINE void atomic_add_release(int32* value, int32 increment) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAddRelease((long *) value, increment); }
FORCE_INLINE void atomic_sub_release(int32* value, int32 decrement) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAddRelease((long *) value, -decrement); }
FORCE_INLINE void atomic_add_release(int64* value, int64 increment) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAddRelease64((LONG64 *) value, (LONG64) increment); }
FORCE_INLINE void atomic_sub_release(int64* value, int64 decrement) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAddRelease64((LONG64 *) value, -((LONG64) decrement)); }
FORCE_INLINE f32 atomic_compare_exchange_strong_release(f32* value, f32 expected, f32 desired) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); _atomic_32 temp = {InterlockedCompareExchangeRelease((long *) value, (long) desired, (long) expected) }; return temp.f; }
FORCE_INLINE f64 atomic_compare_exchange_strong_release(f64* value, f64 expected, f64 desired) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); _atomic_64 temp = {InterlockedCompareExchangeRelease64((LONG64 *) value, (LONG64) desired, (LONG64) expected) }; return temp.f; }
FORCE_INLINE int32 atomic_compare_exchange_strong_release(int32* value, int32 expected, int32 desired) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedCompareExchangeRelease((long *) value, desired, expected); }
FORCE_INLINE int64 atomic_compare_exchange_strong_release(int64* value, int64 expected, int64 desired) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedCompareExchangeRelease64((LONG64 *) value, (LONG64) desired, (LONG64) expected); }
FORCE_INLINE int8 atomic_fetch_add_release(int8* value, int8 operand) NO_EXCEPT
{ return (int8) InterlockedExchangeAdd8((char *) value, (char) operand); }
FORCE_INLINE int8 atomic_fetch_sub_release(int8* value, int8 operand) NO_EXCEPT
{ return (int8) InterlockedExchangeAdd8((char *) value, -((char) operand)); }
FORCE_INLINE int16 atomic_fetch_add_release(int16* value, int16 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (int16) InterlockedExchangeAdd16((short *) value, (short) operand); }
FORCE_INLINE int16 atomic_fetch_sub_release(int16* value, int16 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (int16) InterlockedExchangeAdd16((short *) value, -((short) operand)); }
FORCE_INLINE int32 atomic_fetch_add_release(int32* value, int32 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedExchangeAddRelease((long *) value, operand); }
FORCE_INLINE int32 atomic_fetch_sub_release(int32* value, int32 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedExchangeAddRelease((unsigned long *) value, -((long) operand)); }
FORCE_INLINE int64 atomic_fetch_add_release(int64* value, int64 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedExchangeAddRelease64((LONG64 *) value, (LONG64) operand); }
FORCE_INLINE int64 atomic_fetch_sub_release(int64* value, int64 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedExchangeAdd64((LONG64 *) value, -((LONG64) operand)); }
FORCE_INLINE int64 atomic_fetch_and_release(int64* value, uint64 mask) NO_EXCEPT
{ return (int64) InterlockedAnd64Release((LONG64 *) value, (LONG64) mask); }
FORCE_INLINE int64 atomic_fetch_or_release(int64* value, uint64 mask) NO_EXCEPT
{ return (int64) InterlockedOr64Release((LONG64 *) value, (LONG64) mask); }
FORCE_INLINE uint64 atomic_fetch_and_release(uint64* value, uint64 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedAnd64Release((LONG64 *) value, (LONG64) mask); }
FORCE_INLINE uint64 atomic_fetch_or_release(uint64* value, uint64 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedOr64Release((LONG64 *) value, (LONG64) mask); }
FORCE_INLINE void atomic_set_release(uint8* value, uint8 new_value) NO_EXCEPT
{ InterlockedExchange8((char *) value, (char) new_value); }
FORCE_INLINE void atomic_set_release(uint16* value, uint16 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchange16((short *) value, (short) new_value); }
FORCE_INLINE void atomic_set_release(uint32* value, uint32 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedExchange((long *) value, new_value); }
FORCE_INLINE void atomic_set_release(uint64* value, uint64 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedExchange64((LONG64 *) value, (LONG64) new_value); }
FORCE_INLINE uint8 atomic_fetch_set_release(uint8* value, uint8 new_value) NO_EXCEPT
{ return (uint8) InterlockedExchange8((char *) value, (char) new_value); }
FORCE_INLINE uint16 atomic_fetch_set_release(uint16* value, uint16 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (uint16) InterlockedExchange16((short *) value, (short) new_value); }
FORCE_INLINE uint32 atomic_fetch_set_release(uint32* value, uint32 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedExchange((long *) value, new_value); }
FORCE_INLINE uint64 atomic_fetch_set_release(uint64* value, uint64 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedExchange64((LONG64 *) value, (LONG64) new_value); }
FORCE_INLINE uint8 atomic_get_release(const uint8* value) NO_EXCEPT
{ return (uint8) _InterlockedCompareExchange8((char *) value, 0, 0); }
FORCE_INLINE uint16 atomic_get_release(const uint16* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (uint16) InterlockedCompareExchangeRelease16((short *) value, 0, 0); }
FORCE_INLINE uint32 atomic_get_release(const uint32* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedCompareExchangeRelease((long *) value, 0, 0); }
FORCE_INLINE uint64 atomic_get_release(const uint64* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedCompareExchangeRelease64((LONG64 *) value, 0, 0); }
FORCE_INLINE uint8 atomic_increment_release(uint8* value) NO_EXCEPT
{ return InterlockedExchangeAdd8((char *) value, 1); }
FORCE_INLINE uint8 atomic_decrement_release(uint8* value) NO_EXCEPT
{ return InterlockedExchangeAdd8((char *) value, -1); }
FORCE_INLINE uint16 atomic_increment_release(uint16* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return InterlockedIncrementRelease16((short *) value); }
FORCE_INLINE uint16 atomic_decrement_release(uint16* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return InterlockedDecrementRelease16((short *) value); }
FORCE_INLINE uint32 atomic_increment_release(uint32* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return InterlockedIncrementRelease((long *) value); }
FORCE_INLINE uint32 atomic_decrement_release(uint32* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return InterlockedDecrementRelease((long *) value); }
FORCE_INLINE uint64 atomic_increment_release(uint64* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return InterlockedIncrementRelease64((LONG64 *) value); }
FORCE_INLINE uint64 atomic_decrement_release(uint64* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return InterlockedDecrementRelease64((LONG64 *) value); }
FORCE_INLINE void atomic_add_release(uint8* value, uint8 increment) NO_EXCEPT
{ InterlockedExchangeAdd8((char *) value, (char) increment); }
FORCE_INLINE void atomic_sub_release(uint8* value, uint8 decrement) NO_EXCEPT
{ InterlockedExchangeAdd8((char *) value, -((char) decrement)); }
FORCE_INLINE void atomic_add_release(uint16* value, uint16 increment) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeAdd16((short *) value, (short) increment); }
FORCE_INLINE void atomic_sub_release(uint16* value, uint16 decrement) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeAdd16((short *) value, -((short) decrement)); }
FORCE_INLINE void atomic_add_release(uint32* value, uint32 increment) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAddRelease((long *) value, increment); }
FORCE_INLINE void atomic_sub_release(uint32* value, uint32 decrement) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAddRelease((long *) value, -1 * ((int32) decrement)); }
FORCE_INLINE void atomic_add_release(uint64* value, uint64 increment) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAddRelease64((LONG64 *) value, (LONG64) increment); }
FORCE_INLINE void atomic_sub_release(uint64* value, uint64 decrement) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAddRelease64((LONG64 *) value, -((LONG64) decrement)); }
FORCE_INLINE uint32 atomic_compare_exchange_strong_release(uint32* value, uint32 expected, uint32 desired) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedCompareExchangeRelease((long *) value, desired, expected); }
FORCE_INLINE uint64 atomic_compare_exchange_strong_release(uint64* value, uint64 expected, uint64 desired) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedCompareExchangeRelease64((LONG64 *) value, (LONG64) desired, (LONG64) expected); }
FORCE_INLINE uint8 atomic_fetch_add_release(uint8* value, uint8 operand) NO_EXCEPT
{ return (uint8) InterlockedExchangeAdd8((char *) value, (char) operand); }
FORCE_INLINE uint8 atomic_fetch_sub_release(uint8* value, uint8 operand) NO_EXCEPT
{ return (uint8) InterlockedExchangeAdd8((char *) value, -((char) operand)); }
FORCE_INLINE uint16 atomic_fetch_add_release(uint16* value, uint16 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (uint16) InterlockedExchangeAdd16((short *) value, (short) operand); }
FORCE_INLINE uint16 atomic_fetch_sub_release(uint16* value, uint16 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (uint16) InterlockedExchangeAdd16((short *) value, -((short) operand)); }
FORCE_INLINE uint32 atomic_fetch_add_release(uint32* value, uint32 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedExchangeAddRelease((long *) value, operand); }
FORCE_INLINE uint32 atomic_fetch_sub_release(uint32* value, uint32 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedExchangeAddRelease((unsigned long *) value, -((long) operand)); }
FORCE_INLINE uint64 atomic_fetch_add_release(uint64* value, uint64 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedExchangeAddRelease64((LONG64 *) value, (LONG64) operand); }
FORCE_INLINE uint64 atomic_fetch_sub_release(uint64* value, uint64 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedExchangeAdd64((LONG64 *) value, -((LONG64) operand)); }
FORCE_INLINE void atomic_and_release(uint8* value, uint8 mask) NO_EXCEPT
{ InterlockedAnd8((char *) value, mask); }
FORCE_INLINE void atomic_and_release(int8* value, int8 mask) NO_EXCEPT
{ InterlockedAnd8((char *) value, mask); }
FORCE_INLINE void atomic_and_release(uint16* value, uint16 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedAnd16((short *) value, mask); }
FORCE_INLINE void atomic_and_release(int16* value, int16 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedAnd16((short *) value, mask); }
FORCE_INLINE void atomic_and_release(uint32* value, uint32 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAndRelease((LONG *) value, mask); }
FORCE_INLINE void atomic_and_release(int32* value, int32 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAndRelease((LONG *) value, (LONG)mask); }
FORCE_INLINE void atomic_and_release(uint64* value, uint64 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAnd64Release((LONG64 *) value, mask); }
FORCE_INLINE void atomic_and_release(int64* value, int64 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAnd64Release((LONG64 *) value, mask); }
FORCE_INLINE void atomic_or_release(uint8* value, uint8 mask) NO_EXCEPT
{ InterlockedOr8((char *) value, mask); }
FORCE_INLINE void atomic_or_release(int8* value, int8 mask) NO_EXCEPT
{ InterlockedOr8((char *) value, mask); }
FORCE_INLINE void atomic_or_release(uint16* value, uint16 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedOr16((short *) value, mask); }
FORCE_INLINE void atomic_or_release(int16* value, int16 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedOr16((short *) value, mask); }
FORCE_INLINE void atomic_or_release(uint32* value, uint32 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedOrRelease((LONG *) value, mask); }
FORCE_INLINE void atomic_or_release(int32* value, int32 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedOrRelease((LONG *) value, (LONG)mask); }
FORCE_INLINE void atomic_or_release(uint64* value, uint64 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedOr64Release((LONG64 *) value, mask); }
FORCE_INLINE void atomic_or_release(int64* value, int64 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedOr64Release((LONG64 *) value, mask); }

FORCE_INLINE void atomic_set_acquire_release(void** target, void* new_pointer) NO_EXCEPT
{ InterlockedExchangePointer(target, new_pointer); }
FORCE_INLINE void* atomic_get_acquire_release(void** target) NO_EXCEPT
{ return InterlockedCompareExchangePointer(target, NULL, NULL); }
FORCE_INLINE void atomic_set_acquire_release(int8* value, int8 new_value) NO_EXCEPT
{ InterlockedExchange8((char *) value, new_value); }
FORCE_INLINE void atomic_set_acquire_release(int16* value, int16 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchange16((short *) value, new_value); }
FORCE_INLINE void atomic_set_acquire_release(int32* value, int32 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedExchange((long *) value, new_value); }
FORCE_INLINE void atomic_set_acquire_release(int64* value, int64 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedExchange64((LONG64 *) value, (LONG64) new_value); }
FORCE_INLINE void atomic_set_acquire_release(f32* value, f32 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); _atomic_32f temp = {new_value}; InterlockedExchange((long *) value, (long) temp.l); }
FORCE_INLINE void atomic_set_acquire_release(f64* value, f64 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); _atomic_64f temp = {new_value}; InterlockedExchange64((LONG64 *) value, (LONG64) temp.l); }
FORCE_INLINE int8 atomic_fetch_set_acquire_release(int8* value, int8 new_value) NO_EXCEPT
{ return (int8) InterlockedExchange8((char *) value, (char) new_value); }
FORCE_INLINE int16 atomic_fetch_set_acquire_release(int16* value, int16 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (int16) InterlockedExchange16((short *) value, (short) new_value); }
FORCE_INLINE int32 atomic_fetch_set_acquire_release(int32* value, int32 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedExchange((long *) value, new_value); }
FORCE_INLINE int64 atomic_fetch_set_acquire_release(int64* value, int64 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedExchange64((LONG64 *) value, (LONG64) new_value); }
FORCE_INLINE int8 atomic_get_acquire_release(const int8* value) NO_EXCEPT
{ return (int8) _InterlockedCompareExchange8((char *) value, 0, 0); }
FORCE_INLINE int16 atomic_get_acquire_release(const int16* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (int16) InterlockedCompareExchange16((short *) value, 0, 0); }
FORCE_INLINE int32 atomic_get_acquire_release(const int32* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedCompareExchange((long *) value, 0, 0); }
FORCE_INLINE int64 atomic_get_acquire_release(const int64* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedCompareExchange64((LONG64 *) value, 0, 0); }
FORCE_INLINE f32 atomic_get_acquire_release(const f32* value) NO_EXCEPT
{ _atomic_32 temp = {InterlockedCompareExchange((long *) value, 0, 0)}; return temp.f; }
FORCE_INLINE f64 atomic_get_acquire_release(const f64* value) NO_EXCEPT
{ _atomic_64 temp = {InterlockedCompareExchange64((LONG64 *) value, 0, 0)}; return temp.f; }
FORCE_INLINE int8 atomic_increment_acquire_release(int8* value) NO_EXCEPT
{ return InterlockedExchangeAdd8((char *) value, 1); }
FORCE_INLINE int8 atomic_decrement_acquire_release(int8* value) NO_EXCEPT
{ return InterlockedExchangeAdd8((char *) value, -1); }
FORCE_INLINE int16 atomic_increment_acquire_release(int16* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return InterlockedIncrement16((short *) value); }
FORCE_INLINE int16 atomic_decrement_acquire_release(int16* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return InterlockedDecrement16((short *) value); }
FORCE_INLINE int32 atomic_increment_acquire_release(int32* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return InterlockedIncrement((long *) value); }
FORCE_INLINE int32 atomic_decrement_acquire_release(int32* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return InterlockedDecrement((long *) value); }
FORCE_INLINE int64 atomic_increment_acquire_release(int64* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return InterlockedIncrement64((LONG64 *) value); }
FORCE_INLINE int64 atomic_decrement_acquire_release(int64* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return InterlockedDecrement64((LONG64 *) value); }
FORCE_INLINE void atomic_add_acquire_release(int8* value, int8 increment) NO_EXCEPT
{ InterlockedExchangeAdd8((char *) value, (char) increment); }
FORCE_INLINE void atomic_sub_acquire_release(int8* value, int8 decrement) NO_EXCEPT
{ InterlockedExchangeAdd8((char *) value, -((char) decrement)); }
FORCE_INLINE void atomic_add_acquire_release(int16* value, int16 increment) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeAdd16((short *) value, (short) increment); }
FORCE_INLINE void atomic_sub_acquire_release(int16* value, int16 decrement) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeAdd16((short *) value, -((short) decrement)); }
FORCE_INLINE void atomic_add_acquire_release(int32* value, int32 increment) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAdd((long *) value, increment); }
FORCE_INLINE void atomic_sub_acquire_release(int32* value, int32 decrement) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAdd((long *) value, -decrement); }
FORCE_INLINE void atomic_add_acquire_release(int64* value, int64 increment) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAdd64((LONG64 *) value, (LONG64) increment); }
FORCE_INLINE void atomic_sub_acquire_release(int64* value, int64 decrement) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAdd64((LONG64 *) value, -((LONG64) decrement)); }
FORCE_INLINE f32 atomic_compare_exchange_strong_acquire_release(f32* value, f32 expected, f32 desired) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); _atomic_32 temp = {InterlockedCompareExchange((long *) value, (long) desired, (long) expected) }; return temp.f; }
FORCE_INLINE f64 atomic_compare_exchange_strong_acquire_release(f64* value, f64 expected, f64 desired) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); _atomic_64 temp = {InterlockedCompareExchange64((LONG64 *) value, (LONG64) desired, (LONG64) expected) }; return temp.f; }
FORCE_INLINE int32 atomic_compare_exchange_strong_acquire_release(int32* value, int32 expected, int32 desired) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedCompareExchange((long *) value, desired, expected); }
FORCE_INLINE int64 atomic_compare_exchange_strong_acquire_release(int64* value, int64 expected, int64 desired) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedCompareExchange64((LONG64 *) value, (LONG64) desired, (LONG64) expected); }
FORCE_INLINE int8 atomic_fetch_add_acquire_release(int8* value, int8 operand) NO_EXCEPT
{ return (int8) InterlockedExchangeAdd8((char *) value, (char) operand); }
FORCE_INLINE int8 atomic_fetch_sub_acquire_release(int8* value, int8 operand) NO_EXCEPT
{ return (int8) InterlockedExchangeAdd8((char *) value, -((char) operand)); }
FORCE_INLINE int16 atomic_fetch_add_acquire_release(int16* value, int16 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (int16) InterlockedExchangeAdd16((short *) value, (short) operand); }
FORCE_INLINE int16 atomic_fetch_sub_acquire_release(int16* value, int16 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (int16) InterlockedExchangeAdd16((short *) value, -((short) operand)); }
FORCE_INLINE int32 atomic_fetch_add_acquire_release(int32* value, int32 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedExchangeAdd((long *) value, operand); }
FORCE_INLINE int32 atomic_fetch_sub_acquire_release(int32* value, int32 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedExchangeAdd((unsigned long *) value, -((long) operand)); }
FORCE_INLINE int64 atomic_fetch_add_acquire_release(int64* value, int64 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedExchangeAdd64((LONG64 *) value, (LONG64) operand); }
FORCE_INLINE int64 atomic_fetch_sub_acquire_release(int64* value, int64 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedExchangeAdd64((LONG64 *) value, -((LONG64) operand)); }
FORCE_INLINE void atomic_set_acquire_release(uint8* value, uint8 new_value) NO_EXCEPT
{ InterlockedExchange8((char *) value, (char) new_value); }
FORCE_INLINE void atomic_set_acquire_release(uint16* value, uint16 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchange16((short *) value, (short) new_value); }
FORCE_INLINE void atomic_set_acquire_release(uint32* value, uint32 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedExchange((long *) value, new_value); }
FORCE_INLINE void atomic_set_acquire_release(uint64* value, uint64 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedExchange64((LONG64 *) value, (LONG64) new_value); }
FORCE_INLINE uint8 atomic_fetch_set_acquire_release(uint8* value, uint8 new_value) NO_EXCEPT
{ return (uint8) InterlockedExchange8((char *) value, (char) new_value); }
FORCE_INLINE uint16 atomic_fetch_set_acquire_release(uint16* value, uint16 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (uint16) InterlockedExchange16((short *) value, (short) new_value); }
FORCE_INLINE uint32 atomic_fetch_set_acquire_release(uint32* value, uint32 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedExchange((long *) value, new_value); }
FORCE_INLINE uint64 atomic_fetch_set_acquire_release(uint64* value, uint64 new_value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedExchange64((LONG64 *) value, (LONG64) new_value); }
FORCE_INLINE uint8 atomic_get_acquire_release(const uint8* value) NO_EXCEPT
{ return (uint8) _InterlockedCompareExchange8((char *) value, 0, 0); }
FORCE_INLINE uint16 atomic_get_acquire_release(const uint16* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (uint16) InterlockedCompareExchange16((short *) value, 0, 0); }
FORCE_INLINE uint32 atomic_get_acquire_release(const uint32* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedCompareExchange((long *) value, 0, 0); }
FORCE_INLINE uint64 atomic_get_acquire_release(const uint64* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedCompareExchange64((LONG64 *) value, 0, 0); }
FORCE_INLINE uint8 atomic_increment_acquire_release(uint8* value) NO_EXCEPT
{ return InterlockedExchangeAdd8((char *) value, 1); }
FORCE_INLINE uint8 atomic_decrement_acquire_release(uint8* value) NO_EXCEPT
{ return InterlockedExchangeAdd8((char *) value, -1); }
FORCE_INLINE uint16 atomic_increment_acquire_release(uint16* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return InterlockedIncrement16((short *) value); }
FORCE_INLINE uint16 atomic_decrement_acquire_release(uint16* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return InterlockedDecrement16((short *) value); }
FORCE_INLINE uint32 atomic_increment_acquire_release(uint32* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return InterlockedIncrement((long *) value); }
FORCE_INLINE uint32 atomic_decrement_acquire_release(uint32* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return InterlockedDecrement((long *) value); }
FORCE_INLINE uint64 atomic_increment_acquire_release(uint64* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return InterlockedIncrement64((LONG64 *) value); }
FORCE_INLINE uint64 atomic_decrement_acquire_release(uint64* value) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return InterlockedDecrement64((LONG64 *) value); }
FORCE_INLINE void atomic_add_acquire_release(uint8* value, uint8 increment) NO_EXCEPT
{ InterlockedExchangeAdd8((char *) value, (char) increment); }
FORCE_INLINE void atomic_sub_acquire_release(uint8* value, uint8 decrement) NO_EXCEPT
{ InterlockedExchangeAdd8((char *) value, -((char) decrement)); }
FORCE_INLINE void atomic_add_acquire_release(uint16* value, uint16 increment) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeAdd16((short *) value, (short) increment); }
FORCE_INLINE void atomic_sub_acquire_release(uint16* value, uint16 decrement) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeAdd16((short *) value, -((short) decrement)); }
FORCE_INLINE void atomic_add_acquire_release(uint32* value, uint32 increment) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAdd((long *) value, increment); }
FORCE_INLINE void atomic_sub_acquire_release(uint32* value, uint32 decrement) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAdd((long *) value, -1 * ((int32) decrement)); }
FORCE_INLINE void atomic_add_acquire_release(uint64* value, uint64 increment) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAdd64((LONG64 *) value, (LONG64) increment); }
FORCE_INLINE void atomic_sub_acquire_release(uint64* value, uint64 decrement) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAdd64((LONG64 *) value, -((LONG64) decrement)); }
FORCE_INLINE uint32 atomic_compare_exchange_strong_acquire_release(uint32* value, uint32 expected, uint32 desired) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedCompareExchange((long *) value, desired, expected); }
FORCE_INLINE uint64 atomic_compare_exchange_strong_acquire_release(uint64* value, uint64 expected, uint64 desired) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedCompareExchange64((LONG64 *) value, (LONG64) desired, (LONG64) expected); }
FORCE_INLINE uint8 atomic_fetch_add_acquire_release(uint8* value, uint8 operand) NO_EXCEPT
{ return (uint8) InterlockedExchangeAdd8((char *) value, (char) operand); }
FORCE_INLINE uint8 atomic_fetch_sub_acquire_release(uint8* value, uint8 operand) NO_EXCEPT
{ return (uint8) InterlockedExchangeAdd8((char *) value, -((char) operand)); }
FORCE_INLINE uint16 atomic_fetch_add_acquire_release(uint16* value, uint16 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (uint16) InterlockedExchangeAdd16((short *) value, (short) operand); }
FORCE_INLINE uint16 atomic_fetch_sub_acquire_release(uint16* value, uint16 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (uint16) InterlockedExchangeAdd16((short *) value, -((short) operand)); }
FORCE_INLINE uint32 atomic_fetch_add_acquire_release(uint32* value, uint32 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedExchangeAdd((long *) value, operand); }
FORCE_INLINE uint32 atomic_fetch_sub_acquire_release(uint32* value, uint32 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedExchangeAdd((unsigned long *) value, -((long) operand)); }
FORCE_INLINE uint64 atomic_fetch_add_acquire_release(uint64* value, uint64 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedExchangeAdd64((LONG64 *) value, (LONG64) operand); }
FORCE_INLINE uint64 atomic_fetch_sub_acquire_release(uint64* value, uint64 operand) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedExchangeAdd64((LONG64 *) value, -((LONG64) operand)); }
FORCE_INLINE void atomic_and_acquire_release(uint8* value, uint8 mask) NO_EXCEPT
{ InterlockedAnd8((char *) value, mask); }
FORCE_INLINE void atomic_and_acquire_release(int8* value, int8 mask) NO_EXCEPT
{ InterlockedAnd8((char *) value, mask); }
FORCE_INLINE void atomic_and_acquire_release(uint16* value, uint16 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedAnd16((short *) value, mask); }
FORCE_INLINE void atomic_and_acquire_release(int16* value, int16 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedAnd16((short *) value, mask); }
FORCE_INLINE void atomic_and_acquire_release(uint32* value, uint32 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAnd((LONG *) value, mask); }
FORCE_INLINE void atomic_and_acquire_release(int32* value, int32 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAnd((LONG *) value, (LONG)mask); }
FORCE_INLINE void atomic_and_acquire_release(uint64* value, uint64 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAnd64((LONG64 *) value, mask); }
FORCE_INLINE void atomic_and_acquire_release(int64* value, int64 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAnd64((LONG64 *) value, mask); }
FORCE_INLINE void atomic_or_acquire_release(uint8* value, uint8 mask) NO_EXCEPT
{ InterlockedOr8((char *) value, mask); }
FORCE_INLINE void atomic_or_acquire_release(int8* value, int8 mask) NO_EXCEPT
{ InterlockedOr8((char *) value, mask); }
FORCE_INLINE void atomic_or_acquire_release(uint16* value, uint16 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedOr16((short *) value, mask); }
FORCE_INLINE void atomic_or_acquire_release(int16* value, int16 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedOr16((short *) value, mask); }
FORCE_INLINE void atomic_or_acquire_release(uint32* value, uint32 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedOr((LONG *) value, mask); }
FORCE_INLINE void atomic_or_acquire_release(int32* value, int32 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedOr((LONG *) value, (LONG)mask); }
FORCE_INLINE void atomic_or_acquire_release(uint64* value, uint64 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedOr64((LONG64 *) value, mask); }
FORCE_INLINE void atomic_or_acquire_release(int64* value, int64 mask) NO_EXCEPT
{ ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedOr64((LONG64 *) value, mask); }

// Check out the intrinsic functions fence_memory and fence_write
// These are much faster and could accomplish what you are doing
#define atomic_fence_acquire() MemoryBarrier();

// Check out the intrinsic functions fence_memory and fence_write
// These are much faster and could accomplish what you are doing
#define atomic_fence_release() MemoryBarrier();

#endif