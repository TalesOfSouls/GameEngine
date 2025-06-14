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

#include "../../../stdlib/Types.h"
#include "../../../compiler/CompilerUtils.h"
#include <windows.h>

// We need the following helper types to "cast" between float and long.
// We can't just perform a "normal" cast since that re-interprets the bits. We need to maintain the bits
// @question Don't we also have to do this for signed vs unsigned?
typedef union { f32 f; LONG l; } _atomic_32;
typedef union { f64 f; LONG64 l; } _atomic_64;

// WARNING: Windows doesn't really have relaxed, release, acquire function on x86_64.
// You can see that by checking out how they are defined
// @bug As a result we are not always using the correct fenced/unfenced version on ARM
// (e.g. see _InterlockedCompareExchange8, it should be _InterlockedCompareExchange8_nf/rel/acq)
// To solve this we would probably have to make some of these functions Architecture specific in addition to platform specific

FORCE_INLINE void atomic_set_relaxed(void** target, void* new_pointer) noexcept { InterlockedExchangePointerNoFence(target, new_pointer); }
FORCE_INLINE void* atomic_get_relaxed(void** target) noexcept { return InterlockedCompareExchangePointerNoFence(target, NULL, NULL); }
FORCE_INLINE void atomic_set_relaxed(volatile int8* value, int8 new_value) noexcept { InterlockedExchangeNoFence8((volatile char *) value, new_value); }
FORCE_INLINE void atomic_set_relaxed(volatile int16* value, int16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeNoFence16((volatile short *) value, new_value); }
FORCE_INLINE void atomic_set_relaxed(volatile int32* value, int32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedExchangeNoFence((volatile long *) value, new_value); }
FORCE_INLINE void atomic_set_relaxed(volatile int64* value, int64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedExchangeNoFence64((volatile LONG64 *) value, (LONG64) new_value); }
FORCE_INLINE void atomic_set_relaxed(volatile f32* value, f32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); _atomic_32 temp = {.f = new_value}; InterlockedExchangeNoFence((volatile long *) value, (long) temp.l); }
FORCE_INLINE void atomic_set_relaxed(volatile f64* value, f64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); _atomic_64 temp = {.f = new_value}; InterlockedExchangeNoFence64((volatile LONG64 *) value, (LONG64) temp.l); }
FORCE_INLINE int8 atomic_fetch_set_relaxed(volatile int8* value, int8 new_value) noexcept { return (int8) InterlockedExchangeNoFence8((volatile char *) value, (char) new_value); }
FORCE_INLINE int16 atomic_fetch_set_relaxed(volatile int16* value, int16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (int16) InterlockedExchangeNoFence16((volatile short *) value, (short) new_value); }
FORCE_INLINE int32 atomic_fetch_set_relaxed(volatile int32* value, int32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedExchangeNoFence((volatile long *) value, new_value); }
FORCE_INLINE int64 atomic_fetch_set_relaxed(volatile int64* value, int64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedExchangeNoFence64((volatile LONG64 *) value, (LONG64) new_value); }
FORCE_INLINE int8 atomic_get_relaxed(volatile int8* value) noexcept { return (int8) _InterlockedCompareExchange8((volatile char *) value, 0, 0); }
FORCE_INLINE int16 atomic_get_relaxed(volatile int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (int16) InterlockedCompareExchangeNoFence16((volatile short *) value, 0, 0); }
FORCE_INLINE int32 atomic_get_relaxed(volatile int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedCompareExchangeNoFence((volatile long *) value, 0, 0); }
FORCE_INLINE int64 atomic_get_relaxed(volatile int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedCompareExchangeNoFence64((volatile LONG64 *) value, 0, 0); }
FORCE_INLINE f32 atomic_get_relaxed(volatile f32* value) noexcept { _atomic_32 temp = {.l = InterlockedCompareExchangeNoFence((volatile long *) value, 0, 0)}; return temp.f; }
FORCE_INLINE f64 atomic_get_relaxed(volatile f64* value) noexcept { _atomic_64 temp = {.l = InterlockedCompareExchangeNoFence64((volatile LONG64 *) value, 0, 0)}; return temp.f; }
FORCE_INLINE int8 atomic_increment_relaxed(volatile int8* value) noexcept { return InterlockedExchangeAdd8((volatile char *) value, 1); }
FORCE_INLINE int8 atomic_decrement_relaxed(volatile int8* value) noexcept { return InterlockedExchangeAdd8((volatile char *) value, -1); }
FORCE_INLINE int16 atomic_increment_relaxed(volatile int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return InterlockedIncrementNoFence16((volatile short *) value); }
FORCE_INLINE int16 atomic_decrement_relaxed(volatile int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return InterlockedDecrementNoFence16((volatile short *) value); }
FORCE_INLINE int32 atomic_increment_relaxed(volatile int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return InterlockedIncrementNoFence((volatile long *) value); }
FORCE_INLINE int32 atomic_decrement_relaxed(volatile int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return InterlockedDecrementNoFence((volatile long *) value); }
FORCE_INLINE int64 atomic_increment_relaxed(volatile int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return InterlockedIncrementNoFence64((volatile LONG64 *) value); }
FORCE_INLINE int64 atomic_decrement_relaxed(volatile int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return InterlockedDecrementNoFence64((volatile LONG64 *) value); }
FORCE_INLINE void atomic_add_relaxed(volatile int8* value, int8 increment) noexcept { InterlockedExchangeAdd8((volatile char *) value, (char) increment); }
FORCE_INLINE void atomic_sub_relaxed(volatile int8* value, int8 decrement) noexcept { InterlockedExchangeAdd8((volatile char *) value, -((char) decrement)); }
FORCE_INLINE void atomic_add_relaxed(volatile int16* value, int16 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeAdd16((volatile short *) value, (short) increment); }
FORCE_INLINE void atomic_sub_relaxed(volatile int16* value, int16 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeAdd16((volatile short *) value, -((short) decrement)); }
FORCE_INLINE void atomic_add_relaxed(volatile int32* value, int32 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAddNoFence((volatile long *) value, increment); }
FORCE_INLINE void atomic_sub_relaxed(volatile int32* value, int32 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAddNoFence((volatile long *) value, -decrement); }
FORCE_INLINE void atomic_add_relaxed(volatile int64* value, int64 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAddNoFence64((volatile LONG64 *) value, (LONG64) increment); }
FORCE_INLINE void atomic_sub_relaxed(volatile int64* value, int64 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAddNoFence64((volatile LONG64 *) value, -((LONG64) decrement)); }
FORCE_INLINE f32 atomic_compare_exchange_strong_relaxed(volatile f32* value, f32* expected, f32 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); _atomic_32 temp = {.l = InterlockedCompareExchangeNoFence((volatile long *) value, (long) desired, (long) *expected) }; return temp.f; }
FORCE_INLINE f64 atomic_compare_exchange_strong_relaxed(volatile f64* value, f64* expected, f64 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); _atomic_64 temp = {.l = InterlockedCompareExchangeNoFence64((volatile LONG64 *) value, (LONG64) desired, (LONG64) *expected) }; return temp.f; }
FORCE_INLINE int32 atomic_compare_exchange_strong_relaxed(volatile int32* value, int32* expected, int32 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedCompareExchangeNoFence((volatile long *) value, desired, *expected); }
FORCE_INLINE int64 atomic_compare_exchange_strong_relaxed(volatile int64* value, int64* expected, int64 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedCompareExchangeNoFence64((volatile LONG64 *) value, (LONG64) desired, (LONG64) *expected); }
FORCE_INLINE int8 atomic_fetch_add_relaxed(volatile int8* value, int8 operand) noexcept { return (int8) InterlockedExchangeAdd8((volatile char *) value, (char) operand); }
FORCE_INLINE int8 atomic_fetch_sub_relaxed(volatile int8* value, int8 operand) noexcept { return (int8) InterlockedExchangeAdd8((volatile char *) value, -((char) operand)); }
FORCE_INLINE int16 atomic_fetch_add_relaxed(volatile int16* value, int16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (int16) InterlockedExchangeAdd16((volatile short *) value, (short) operand); }
FORCE_INLINE int16 atomic_fetch_sub_relaxed(volatile int16* value, int16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (int16) InterlockedExchangeAdd16((volatile short *) value, -((short) operand)); }
FORCE_INLINE int32 atomic_fetch_add_relaxed(volatile int32* value, int32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedExchangeAddNoFence((volatile long *) value, operand); }
FORCE_INLINE int32 atomic_fetch_sub_relaxed(volatile int32* value, int32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedExchangeAddNoFence((volatile unsigned long *) value, -((long) operand)); }
FORCE_INLINE int64 atomic_fetch_add_relaxed(volatile int64* value, int64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedExchangeAddNoFence64((volatile LONG64 *) value, (LONG64) operand); }
FORCE_INLINE int64 atomic_fetch_sub_relaxed(volatile int64* value, int64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedExchangeAdd64((volatile LONG64 *) value, -((LONG64) operand)); }
FORCE_INLINE void atomic_set_relaxed(volatile uint8* value, uint8 new_value) noexcept { InterlockedExchangeNoFence8((volatile char *) value, (char) new_value); }
FORCE_INLINE void atomic_set_relaxed(volatile uint16* value, uint16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeNoFence16((volatile short *) value, (short) new_value); }
FORCE_INLINE void atomic_set_relaxed(volatile uint32* value, uint32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedExchangeNoFence((volatile long *) value, new_value); }
FORCE_INLINE void atomic_set_relaxed(volatile uint64* value, uint64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedExchangeNoFence64((volatile LONG64 *) value, (LONG64) new_value); }
FORCE_INLINE uint8 atomic_fetch_set_relaxed(volatile uint8* value, uint8 new_value) noexcept { return (uint8) InterlockedExchangeNoFence8((volatile char *) value, (char) new_value); }
FORCE_INLINE uint16 atomic_fetch_set_relaxed(volatile uint16* value, uint16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (uint16) InterlockedExchangeNoFence16((volatile short *) value, (short) new_value); }
FORCE_INLINE uint32 atomic_fetch_set_relaxed(volatile uint32* value, uint32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedExchangeNoFence((volatile long *) value, new_value); }
FORCE_INLINE uint64 atomic_fetch_set_relaxed(volatile uint64* value, uint64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedExchangeNoFence64((volatile LONG64 *) value, (LONG64) new_value); }
FORCE_INLINE uint8 atomic_get_relaxed(volatile uint8* value) noexcept { return (uint8) _InterlockedCompareExchange8((volatile char *) value, 0, 0); }
FORCE_INLINE uint16 atomic_get_relaxed(volatile uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (uint16) InterlockedCompareExchangeNoFence16((volatile short *) value, 0, 0); }
FORCE_INLINE uint32 atomic_get_relaxed(volatile uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedCompareExchangeNoFence((volatile long *) value, 0, 0); }
FORCE_INLINE uint64 atomic_get_relaxed(volatile uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedCompareExchangeNoFence64((volatile LONG64 *) value, 0, 0); }
FORCE_INLINE uint8 atomic_increment_relaxed(volatile uint8* value) noexcept { return InterlockedExchangeAdd8((volatile char *) value, 1); }
FORCE_INLINE uint8 atomic_decrement_relaxed(volatile uint8* value) noexcept { return InterlockedExchangeAdd8((volatile char *) value, -1); }
FORCE_INLINE uint16 atomic_increment_relaxed(volatile uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return InterlockedIncrementNoFence16((volatile short *) value); }
FORCE_INLINE uint16 atomic_decrement_relaxed(volatile uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return InterlockedDecrementNoFence16((volatile short *) value); }
FORCE_INLINE uint32 atomic_increment_relaxed(volatile uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return InterlockedIncrementNoFence((volatile long *) value); }
FORCE_INLINE uint32 atomic_decrement_relaxed(volatile uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return InterlockedDecrementNoFence((volatile long *) value); }
FORCE_INLINE uint64 atomic_increment_relaxed(volatile uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return InterlockedIncrementNoFence64((volatile LONG64 *) value); }
FORCE_INLINE uint64 atomic_decrement_relaxed(volatile uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return InterlockedDecrementNoFence64((volatile LONG64 *) value); }
FORCE_INLINE void atomic_add_relaxed(volatile uint8* value, uint8 increment) noexcept { InterlockedExchangeAdd8((volatile char *) value, (char) increment); }
FORCE_INLINE void atomic_sub_relaxed(volatile uint8* value, uint8 decrement) noexcept { InterlockedExchangeAdd8((volatile char *) value, -((char) decrement)); }
FORCE_INLINE void atomic_add_relaxed(volatile uint16* value, uint16 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeAdd16((volatile short *) value, (short) increment); }
FORCE_INLINE void atomic_sub_relaxed(volatile uint16* value, uint16 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeAdd16((volatile short *) value, -((short) decrement)); }
FORCE_INLINE void atomic_add_relaxed(volatile uint32* value, uint32 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAddNoFence((volatile long *) value, increment); }
FORCE_INLINE void atomic_sub_relaxed(volatile uint32* value, uint32 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAddNoFence((volatile long *) value, -1 * ((int32) decrement)); }
FORCE_INLINE void atomic_add_relaxed(volatile uint64* value, uint64 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAddNoFence64((volatile LONG64 *) value, (LONG64) increment); }
FORCE_INLINE void atomic_sub_relaxed(volatile uint64* value, uint64 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAddNoFence64((volatile LONG64 *) value, -((LONG64) decrement)); }
FORCE_INLINE uint32 atomic_compare_exchange_strong_relaxed(volatile uint32* value, uint32* expected, uint32 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedCompareExchangeNoFence((volatile long *) value, desired, *expected); }
FORCE_INLINE uint64 atomic_compare_exchange_strong_relaxed(volatile uint64* value, uint64* expected, uint64 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedCompareExchangeNoFence64((volatile LONG64 *) value, (LONG64) desired, (LONG64) *expected); }
FORCE_INLINE uint8 atomic_fetch_add_relaxed(volatile uint8* value, uint8 operand) noexcept { return (uint8) InterlockedExchangeAdd8((volatile char *) value, (char) operand); }
FORCE_INLINE uint8 atomic_fetch_sub_relaxed(volatile uint8* value, uint8 operand) noexcept { return (uint8) InterlockedExchangeAdd8((volatile char *) value, -((char) operand)); }
FORCE_INLINE uint16 atomic_fetch_add_relaxed(volatile uint16* value, uint16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (uint16) InterlockedExchangeAdd16((volatile short *) value, (short) operand); }
FORCE_INLINE uint16 atomic_fetch_sub_relaxed(volatile uint16* value, uint16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (uint16) InterlockedExchangeAdd16((volatile short *) value, -((short) operand)); }
FORCE_INLINE uint32 atomic_fetch_add_relaxed(volatile uint32* value, uint32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedExchangeAddNoFence((volatile long *) value, operand); }
FORCE_INLINE uint32 atomic_fetch_sub_relaxed(volatile uint32* value, uint32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedExchangeAddNoFence((volatile unsigned long *) value, -((long) operand)); }
FORCE_INLINE uint64 atomic_fetch_add_relaxed(volatile uint64* value, uint64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedExchangeAddNoFence64((volatile LONG64 *) value, (LONG64) operand); }
FORCE_INLINE uint64 atomic_fetch_sub_relaxed(volatile uint64* value, uint64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedExchangeAdd64((volatile LONG64 *) value, -((LONG64) operand)); }
FORCE_INLINE void atomic_and_relaxed(volatile uint8* value, uint8 mask) noexcept { InterlockedAnd8((volatile char *) value, mask); }
FORCE_INLINE void atomic_and_relaxed(volatile int8* value, int8 mask) noexcept { InterlockedAnd8((volatile char *) value, mask); }
FORCE_INLINE void atomic_and_relaxed(volatile uint16* value, uint16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedAnd16((volatile short *) value, mask); }
FORCE_INLINE void atomic_and_relaxed(volatile int16* value, int16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedAnd16((volatile short *) value, mask); }
FORCE_INLINE void atomic_and_relaxed(volatile uint32* value, uint32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAndNoFence((volatile LONG *) value, mask); }
FORCE_INLINE void atomic_and_relaxed(volatile int32* value, int32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAndNoFence((volatile LONG *) value, (LONG)mask); }
FORCE_INLINE void atomic_and_relaxed(volatile uint64* value, uint64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAnd64NoFence((volatile LONG64 *) value, mask); }
FORCE_INLINE void atomic_and_relaxed(volatile int64* value, int64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAnd64NoFence((volatile LONG64 *) value, mask); }
FORCE_INLINE void atomic_or_relaxed(volatile uint8* value, uint8 mask) noexcept { InterlockedOr8((volatile char *) value, mask); }
FORCE_INLINE void atomic_or_relaxed(volatile int8* value, int8 mask) noexcept { InterlockedOr8((volatile char *) value, mask); }
FORCE_INLINE void atomic_or_relaxed(volatile uint16* value, uint16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedOr16((volatile short *) value, mask); }
FORCE_INLINE void atomic_or_relaxed(volatile int16* value, int16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedOr16((volatile short *) value, mask); }
FORCE_INLINE void atomic_or_relaxed(volatile uint32* value, uint32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedOrNoFence((volatile LONG *) value, mask); }
FORCE_INLINE void atomic_or_relaxed(volatile int32* value, int32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedOrNoFence((volatile LONG *) value, (LONG)mask); }
FORCE_INLINE void atomic_or_relaxed(volatile uint64* value, uint64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedOr64NoFence((volatile LONG64 *) value, mask); }
FORCE_INLINE void atomic_or_relaxed(volatile int64* value, int64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedOr64NoFence((volatile LONG64 *) value, mask); }

FORCE_INLINE void* atomic_get_acquire(void** target) noexcept { return InterlockedCompareExchangePointerAcquire(target, NULL, NULL); }
FORCE_INLINE int8 atomic_fetch_set_acquire(volatile int8* value, int8 new_value) noexcept { return (int8) InterlockedExchangeAcquire8((volatile char *) value, (char) new_value); }
FORCE_INLINE int16 atomic_fetch_set_acquire(volatile int16* value, int16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (int16) InterlockedExchangeAcquire16((volatile short *) value, (short) new_value); }
FORCE_INLINE int32 atomic_fetch_set_acquire(volatile int32* value, int32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedExchangeAcquire((volatile long *) value, new_value); }
FORCE_INLINE int64 atomic_fetch_set_acquire(volatile int64* value, int64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedExchangeAcquire64((volatile LONG64 *) value, (LONG64) new_value); }
FORCE_INLINE int8 atomic_get_acquire(volatile int8* value) noexcept { return (int8) _InterlockedCompareExchange8((volatile char *) value, 0, 0); }
FORCE_INLINE int16 atomic_get_acquire(volatile int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (int16) InterlockedCompareExchangeAcquire16((volatile short *) value, 0, 0); }
FORCE_INLINE int32 atomic_get_acquire(volatile int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedCompareExchangeAcquire((volatile long *) value, 0, 0); }
FORCE_INLINE int64 atomic_get_acquire(volatile int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedCompareExchangeAcquire64((volatile LONG64 *) value, 0, 0); }
FORCE_INLINE f32 atomic_get_acquire(volatile f32* value) noexcept { _atomic_32 temp = {.l = InterlockedCompareExchangeAcquire((volatile long *) value, 0, 0)}; return temp.f; }
FORCE_INLINE f64 atomic_get_acquire(volatile f64* value) noexcept { _atomic_64 temp = {.l = InterlockedCompareExchangeAcquire64((volatile LONG64 *) value, 0, 0)}; return temp.f; }
FORCE_INLINE int8 atomic_increment_acquire(volatile int8* value) noexcept { return InterlockedExchangeAdd8((volatile char *) value, 1); }
FORCE_INLINE int8 atomic_decrement_acquire(volatile int8* value) noexcept { return InterlockedExchangeAdd8((volatile char *) value, -1); }
FORCE_INLINE int16 atomic_increment_acquire(volatile int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return InterlockedIncrementAcquire16((volatile short *) value); }
FORCE_INLINE int16 atomic_decrement_acquire(volatile int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return InterlockedDecrementAcquire16((volatile short *) value); }
FORCE_INLINE int32 atomic_increment_acquire(volatile int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return InterlockedIncrementAcquire((volatile long *) value); }
FORCE_INLINE int32 atomic_decrement_acquire(volatile int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return InterlockedDecrementAcquire((volatile long *) value); }
FORCE_INLINE int64 atomic_increment_acquire(volatile int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return InterlockedIncrementAcquire64((volatile LONG64 *) value); }
FORCE_INLINE int64 atomic_decrement_acquire(volatile int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return InterlockedDecrementAcquire64((volatile LONG64 *) value); }
FORCE_INLINE void atomic_add_acquire(volatile int8* value, int8 increment) noexcept { InterlockedExchangeAdd8((volatile char *) value, (char) increment); }
FORCE_INLINE void atomic_sub_acquire(volatile int8* value, int8 decrement) noexcept { InterlockedExchangeAdd8((volatile char *) value, -((char) decrement)); }
FORCE_INLINE void atomic_add_acquire(volatile int16* value, int16 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeAdd16((volatile short *) value, (short) increment); }
FORCE_INLINE void atomic_sub_acquire(volatile int16* value, int16 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeAdd16((volatile short *) value, -((short) decrement)); }
FORCE_INLINE void atomic_add_acquire(volatile int32* value, int32 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAddAcquire((volatile long *) value, increment); }
FORCE_INLINE void atomic_sub_acquire(volatile int32* value, int32 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAddAcquire((volatile long *) value, -decrement); }
FORCE_INLINE void atomic_add_acquire(volatile int64* value, int64 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAddAcquire64((volatile LONG64 *) value, (LONG64) increment); }
FORCE_INLINE void atomic_sub_acquire(volatile int64* value, int64 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAddAcquire64((volatile LONG64 *) value, -((LONG64) decrement)); }
FORCE_INLINE f32 atomic_compare_exchange_strong_acquire(volatile f32* value, f32* expected, f32 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); _atomic_32 temp = {.l = InterlockedCompareExchangeAcquire((volatile long *) value, (long) desired, (long) *expected) }; return temp.f; }
FORCE_INLINE f64 atomic_compare_exchange_strong_acquire(volatile f64* value, f64* expected, f64 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); _atomic_64 temp = {.l = InterlockedCompareExchangeAcquire64((volatile LONG64 *) value, (LONG64) desired, (LONG64) *expected) }; return temp.f; }
FORCE_INLINE int32 atomic_compare_exchange_strong_acquire(volatile int32* value, int32* expected, int32 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedCompareExchangeAcquire((volatile long *) value, desired, *expected); }
FORCE_INLINE int64 atomic_compare_exchange_strong_acquire(volatile int64* value, int64* expected, int64 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedCompareExchangeAcquire64((volatile LONG64 *) value, (LONG64) desired, (LONG64) *expected); }
FORCE_INLINE int8 atomic_fetch_add_acquire(volatile int8* value, int8 operand) noexcept { return (int8) InterlockedExchangeAdd8((volatile char *) value, (char) operand); }
FORCE_INLINE int8 atomic_fetch_sub_acquire(volatile int8* value, int8 operand) noexcept { return (int8) InterlockedExchangeAdd8((volatile char *) value, -((char) operand)); }
FORCE_INLINE int16 atomic_fetch_add_acquire(volatile int16* value, int16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (int16) InterlockedExchangeAdd16((volatile short *) value, (short) operand); }
FORCE_INLINE int16 atomic_fetch_sub_acquire(volatile int16* value, int16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (int16) InterlockedExchangeAdd16((volatile short *) value, -((short) operand)); }
FORCE_INLINE int32 atomic_fetch_add_acquire(volatile int32* value, int32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedExchangeAddAcquire((volatile long *) value, operand); }
FORCE_INLINE int32 atomic_fetch_sub_acquire(volatile int32* value, int32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedExchangeAddAcquire((volatile unsigned long *) value, -((long) operand)); }
FORCE_INLINE int64 atomic_fetch_add_acquire(volatile int64* value, int64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedExchangeAddAcquire64((volatile LONG64 *) value, (LONG64) operand); }
FORCE_INLINE int64 atomic_fetch_sub_acquire(volatile int64* value, int64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedExchangeAdd64((volatile LONG64 *) value, -((LONG64) operand)); }
FORCE_INLINE void atomic_set_acquire(volatile uint8* value, uint8 new_value) noexcept { InterlockedExchangeAcquire8((volatile char *) value, (char) new_value); }
FORCE_INLINE void atomic_set_acquire(volatile uint16* value, uint16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeAcquire16((volatile short *) value, (short) new_value); }
FORCE_INLINE void atomic_set_acquire(volatile uint32* value, uint32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedExchangeAcquire((volatile long *) value, new_value); }
FORCE_INLINE void atomic_set_acquire(volatile uint64* value, uint64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedExchangeAcquire64((volatile LONG64 *) value, (LONG64) new_value); }
FORCE_INLINE uint8 atomic_fetch_set_acquire(volatile uint8* value, uint8 new_value) noexcept { return (uint8) InterlockedExchangeAcquire8((volatile char *) value, (char) new_value); }
FORCE_INLINE uint16 atomic_fetch_set_acquire(volatile uint16* value, uint16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (uint16) InterlockedExchangeAcquire16((volatile short *) value, (short) new_value); }
FORCE_INLINE uint32 atomic_fetch_set_acquire(volatile uint32* value, uint32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedExchangeAcquire((volatile long *) value, new_value); }
FORCE_INLINE uint64 atomic_fetch_set_acquire(volatile uint64* value, uint64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedExchangeAcquire64((volatile LONG64 *) value, (LONG64) new_value); }
FORCE_INLINE uint8 atomic_get_acquire(volatile uint8* value) noexcept { return (uint8) _InterlockedCompareExchange8((volatile char *) value, 0, 0); }
FORCE_INLINE uint16 atomic_get_acquire(volatile uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (uint16) InterlockedCompareExchangeAcquire16((volatile short *) value, 0, 0); }
FORCE_INLINE uint32 atomic_get_acquire(volatile uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedCompareExchangeAcquire((volatile long *) value, 0, 0); }
FORCE_INLINE uint64 atomic_get_acquire(volatile uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedCompareExchangeAcquire64((volatile LONG64 *) value, 0, 0); }
FORCE_INLINE uint8 atomic_increment_acquire(volatile uint8* value) noexcept { return InterlockedExchangeAdd8((volatile char *) value, 1); }
FORCE_INLINE uint8 atomic_decrement_acquire(volatile uint8* value) noexcept { return InterlockedExchangeAdd8((volatile char *) value, -1); }
FORCE_INLINE uint16 atomic_increment_acquire(volatile uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return InterlockedIncrementAcquire16((volatile short *) value); }
FORCE_INLINE uint16 atomic_decrement_acquire(volatile uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return InterlockedDecrementAcquire16((volatile short *) value); }
FORCE_INLINE uint32 atomic_increment_acquire(volatile uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return InterlockedIncrementAcquire((volatile long *) value); }
FORCE_INLINE uint32 atomic_decrement_acquire(volatile uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return InterlockedDecrementAcquire((volatile long *) value); }
FORCE_INLINE uint64 atomic_increment_acquire(volatile uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return InterlockedIncrementAcquire64((volatile LONG64 *) value); }
FORCE_INLINE uint64 atomic_decrement_acquire(volatile uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return InterlockedDecrementAcquire64((volatile LONG64 *) value); }
FORCE_INLINE void atomic_add_acquire(volatile uint8* value, uint8 increment) noexcept { InterlockedExchangeAdd8((volatile char *) value, (char) increment); }
FORCE_INLINE void atomic_sub_acquire(volatile uint8* value, uint8 decrement) noexcept { InterlockedExchangeAdd8((volatile char *) value, -((char) decrement)); }
FORCE_INLINE void atomic_add_acquire(volatile uint16* value, uint16 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeAdd16((volatile short *) value, (short) increment); }
FORCE_INLINE void atomic_sub_acquire(volatile uint16* value, uint16 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeAdd16((volatile short *) value, -((short) decrement)); }
FORCE_INLINE void atomic_add_acquire(volatile uint32* value, uint32 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAddAcquire((volatile long *) value, increment); }
FORCE_INLINE void atomic_sub_acquire(volatile uint32* value, uint32 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAddAcquire((volatile long *) value, -1 * ((int32) decrement)); }
FORCE_INLINE void atomic_add_acquire(volatile uint64* value, uint64 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAddAcquire64((volatile LONG64 *) value, (LONG64) increment); }
FORCE_INLINE void atomic_sub_acquire(volatile uint64* value, uint64 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAddAcquire64((volatile LONG64 *) value, -((LONG64) decrement)); }
FORCE_INLINE uint32 atomic_compare_exchange_strong_acquire(volatile uint32* value, uint32* expected, uint32 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedCompareExchangeAcquire((volatile long *) value, desired, *expected); }
FORCE_INLINE uint64 atomic_compare_exchange_strong_acquire(volatile uint64* value, uint64* expected, uint64 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedCompareExchangeAcquire64((volatile LONG64 *) value, (LONG64) desired, (LONG64) *expected); }
FORCE_INLINE uint8 atomic_fetch_add_acquire(volatile uint8* value, uint8 operand) noexcept { return (uint8) InterlockedExchangeAdd8((volatile char *) value, (char) operand); }
FORCE_INLINE uint8 atomic_fetch_sub_acquire(volatile uint8* value, uint8 operand) noexcept { return (uint8) InterlockedExchangeAdd8((volatile char *) value, -((char) operand)); }
FORCE_INLINE uint16 atomic_fetch_add_acquire(volatile uint16* value, uint16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (uint16) InterlockedExchangeAdd16((volatile short *) value, (short) operand); }
FORCE_INLINE uint16 atomic_fetch_sub_acquire(volatile uint16* value, uint16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (uint16) InterlockedExchangeAdd16((volatile short *) value, -((short) operand)); }
FORCE_INLINE uint32 atomic_fetch_add_acquire(volatile uint32* value, uint32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedExchangeAddAcquire((volatile long *) value, operand); }
FORCE_INLINE uint32 atomic_fetch_sub_acquire(volatile uint32* value, uint32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedExchangeAddAcquire((volatile unsigned long *) value, -((long) operand)); }
FORCE_INLINE uint64 atomic_fetch_add_acquire(volatile uint64* value, uint64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedExchangeAddAcquire64((volatile LONG64 *) value, (LONG64) operand); }
FORCE_INLINE uint64 atomic_fetch_sub_acquire(volatile uint64* value, uint64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedExchangeAdd64((volatile LONG64 *) value, -((LONG64) operand)); }
FORCE_INLINE void atomic_and_acquire(volatile uint8* value, uint8 mask) noexcept { InterlockedAnd8((volatile char *) value, mask); }
FORCE_INLINE void atomic_and_acquire(volatile int8* value, int8 mask) noexcept { InterlockedAnd8((volatile char *) value, mask); }
FORCE_INLINE void atomic_and_acquire(volatile uint16* value, uint16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedAnd16((volatile short *) value, mask); }
FORCE_INLINE void atomic_and_acquire(volatile int16* value, int16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedAnd16((volatile short *) value, mask); }
FORCE_INLINE void atomic_and_acquire(volatile uint32* value, uint32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAndAcquire((volatile LONG *) value, mask); }
FORCE_INLINE void atomic_and_acquire(volatile int32* value, int32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAndAcquire((volatile LONG *) value, (LONG)mask); }
FORCE_INLINE void atomic_and_acquire(volatile uint64* value, uint64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAnd64Acquire((volatile LONG64 *) value, mask); }
FORCE_INLINE void atomic_and_acquire(volatile int64* value, int64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAnd64Acquire((volatile LONG64 *) value, mask); }
FORCE_INLINE void atomic_or_acquire(volatile uint8* value, uint8 mask) noexcept { InterlockedOr8((volatile char *) value, mask); }
FORCE_INLINE void atomic_or_acquire(volatile int8* value, int8 mask) noexcept { InterlockedOr8((volatile char *) value, mask); }
FORCE_INLINE void atomic_or_acquire(volatile uint16* value, uint16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedOr16((volatile short *) value, mask); }
FORCE_INLINE void atomic_or_acquire(volatile int16* value, int16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedOr16((volatile short *) value, mask); }
FORCE_INLINE void atomic_or_acquire(volatile uint32* value, uint32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedOrAcquire((volatile LONG *) value, mask); }
FORCE_INLINE void atomic_or_acquire(volatile int32* value, int32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedOrAcquire((volatile LONG *) value, (LONG)mask); }
FORCE_INLINE void atomic_or_acquire(volatile uint64* value, uint64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedOr64Acquire((volatile LONG64 *) value, mask); }
FORCE_INLINE void atomic_or_acquire(volatile int64* value, int64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedOr64Acquire((volatile LONG64 *) value, mask); }

FORCE_INLINE void atomic_set_release(void** target, void* new_pointer) noexcept { InterlockedExchangePointer(target, new_pointer); }
FORCE_INLINE void* atomic_get_release(void** target) noexcept { return InterlockedCompareExchangePointerRelease(target, NULL, NULL); }
FORCE_INLINE void atomic_set_release(volatile int8* value, int8 new_value) noexcept { InterlockedExchange8((volatile char *) value, new_value); }
FORCE_INLINE void atomic_set_release(volatile int16* value, int16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchange16((volatile short *) value, new_value); }
FORCE_INLINE void atomic_set_release(volatile int32* value, int32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedExchange((volatile long *) value, new_value); }
FORCE_INLINE void atomic_set_release(volatile int64* value, int64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedExchange64((volatile LONG64 *) value, (LONG64) new_value); }
FORCE_INLINE void atomic_set_release(volatile f32* value, f32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); _atomic_32 temp = {.f = new_value}; InterlockedExchange((volatile long *) value, (long) temp.l); }
FORCE_INLINE void atomic_set_release(volatile f64* value, f64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); _atomic_64 temp = {.f = new_value}; InterlockedExchange64((volatile LONG64 *) value, (LONG64) temp.l); }
FORCE_INLINE int8 atomic_fetch_set_release(volatile int8* value, int8 new_value) noexcept { return (int8) InterlockedExchange8((volatile char *) value, (char) new_value); }
FORCE_INLINE int16 atomic_fetch_set_release(volatile int16* value, int16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (int16) InterlockedExchange16((volatile short *) value, (short) new_value); }
FORCE_INLINE int32 atomic_fetch_set_release(volatile int32* value, int32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedExchange((volatile long *) value, new_value); }
FORCE_INLINE int64 atomic_fetch_set_release(volatile int64* value, int64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedExchange64((volatile LONG64 *) value, (LONG64) new_value); }
FORCE_INLINE int8 atomic_get_release(volatile int8* value) noexcept { return (int8) _InterlockedCompareExchange8((volatile char *) value, 0, 0); }
FORCE_INLINE int16 atomic_get_release(volatile int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (int16) InterlockedCompareExchangeRelease16((volatile short *) value, 0, 0); }
FORCE_INLINE int32 atomic_get_release(volatile int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedCompareExchangeRelease((volatile long *) value, 0, 0); }
FORCE_INLINE int64 atomic_get_release(volatile int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedCompareExchangeRelease64((volatile LONG64 *) value, 0, 0); }
FORCE_INLINE f32 atomic_get_release(volatile f32* value) noexcept { _atomic_32 temp = {.l = InterlockedCompareExchangeRelease((volatile long *) value, 0, 0)}; return temp.f; }
FORCE_INLINE f64 atomic_get_release(volatile f64* value) noexcept { _atomic_64 temp = {.l = InterlockedCompareExchangeRelease64((volatile LONG64 *) value, 0, 0)}; return temp.f; }
FORCE_INLINE int8 atomic_increment_release(volatile int8* value) noexcept { return InterlockedExchangeAdd8((volatile char *) value, 1); }
FORCE_INLINE int8 atomic_decrement_release(volatile int8* value) noexcept { return InterlockedExchangeAdd8((volatile char *) value, -1); }
FORCE_INLINE int16 atomic_increment_release(volatile int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return InterlockedIncrementRelease16((volatile short *) value); }
FORCE_INLINE int16 atomic_decrement_release(volatile int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return InterlockedDecrementRelease16((volatile short *) value); }
FORCE_INLINE int32 atomic_increment_release(volatile int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return InterlockedIncrementRelease((volatile long *) value); }
FORCE_INLINE int32 atomic_decrement_release(volatile int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return InterlockedDecrementRelease((volatile long *) value); }
FORCE_INLINE int64 atomic_increment_release(volatile int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return InterlockedIncrementRelease64((volatile LONG64 *) value); }
FORCE_INLINE int64 atomic_decrement_release(volatile int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return InterlockedDecrementRelease64((volatile LONG64 *) value); }
FORCE_INLINE void atomic_add_release(volatile int8* value, int8 increment) noexcept { InterlockedExchangeAdd8((volatile char *) value, (char) increment); }
FORCE_INLINE void atomic_sub_release(volatile int8* value, int8 decrement) noexcept { InterlockedExchangeAdd8((volatile char *) value, -((char) decrement)); }
FORCE_INLINE void atomic_add_release(volatile int16* value, int16 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeAdd16((volatile short *) value, (short) increment); }
FORCE_INLINE void atomic_sub_release(volatile int16* value, int16 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeAdd16((volatile short *) value, -((short) decrement)); }
FORCE_INLINE void atomic_add_release(volatile int32* value, int32 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAddRelease((volatile long *) value, increment); }
FORCE_INLINE void atomic_sub_release(volatile int32* value, int32 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAddRelease((volatile long *) value, -decrement); }
FORCE_INLINE void atomic_add_release(volatile int64* value, int64 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAddRelease64((volatile LONG64 *) value, (LONG64) increment); }
FORCE_INLINE void atomic_sub_release(volatile int64* value, int64 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAddRelease64((volatile LONG64 *) value, -((LONG64) decrement)); }
FORCE_INLINE f32 atomic_compare_exchange_strong_release(volatile f32* value, f32* expected, f32 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); _atomic_32 temp = {.l = InterlockedCompareExchangeRelease((volatile long *) value, (long) desired, (long) *expected) }; return temp.f; }
FORCE_INLINE f64 atomic_compare_exchange_strong_release(volatile f64* value, f64* expected, f64 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); _atomic_64 temp = {.l = InterlockedCompareExchangeRelease64((volatile LONG64 *) value, (LONG64) desired, (LONG64) *expected) }; return temp.f; }
FORCE_INLINE int32 atomic_compare_exchange_strong_release(volatile int32* value, int32* expected, int32 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedCompareExchangeRelease((volatile long *) value, desired, *expected); }
FORCE_INLINE int64 atomic_compare_exchange_strong_release(volatile int64* value, int64* expected, int64 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedCompareExchangeRelease64((volatile LONG64 *) value, (LONG64) desired, (LONG64) *expected); }
FORCE_INLINE int8 atomic_fetch_add_release(volatile int8* value, int8 operand) noexcept { return (int8) InterlockedExchangeAdd8((volatile char *) value, (char) operand); }
FORCE_INLINE int8 atomic_fetch_sub_release(volatile int8* value, int8 operand) noexcept { return (int8) InterlockedExchangeAdd8((volatile char *) value, -((char) operand)); }
FORCE_INLINE int16 atomic_fetch_add_release(volatile int16* value, int16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (int16) InterlockedExchangeAdd16((volatile short *) value, (short) operand); }
FORCE_INLINE int16 atomic_fetch_sub_release(volatile int16* value, int16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (int16) InterlockedExchangeAdd16((volatile short *) value, -((short) operand)); }
FORCE_INLINE int32 atomic_fetch_add_release(volatile int32* value, int32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedExchangeAddRelease((volatile long *) value, operand); }
FORCE_INLINE int32 atomic_fetch_sub_release(volatile int32* value, int32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedExchangeAddRelease((volatile unsigned long *) value, -((long) operand)); }
FORCE_INLINE int64 atomic_fetch_add_release(volatile int64* value, int64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedExchangeAddRelease64((volatile LONG64 *) value, (LONG64) operand); }
FORCE_INLINE int64 atomic_fetch_sub_release(volatile int64* value, int64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedExchangeAdd64((volatile LONG64 *) value, -((LONG64) operand)); }
FORCE_INLINE int64 atomic_fetch_and_release(volatile int64* value, uint64 mask) noexcept { return (int64) InterlockedAnd64Release((volatile LONG64 *) value, (LONG64) mask); }
FORCE_INLINE int64 atomic_fetch_or_release(volatile int64* value, uint64 mask) noexcept { return (int64) InterlockedOr64Release((volatile LONG64 *) value, (LONG64) mask); }
FORCE_INLINE uint64 atomic_fetch_and_release(volatile uint64* value, uint64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedAnd64Release((volatile LONG64 *) value, (LONG64) mask); }
FORCE_INLINE uint64 atomic_fetch_or_release(volatile uint64* value, uint64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedOr64Release((volatile LONG64 *) value, (LONG64) mask); }
FORCE_INLINE void atomic_set_release(volatile uint8* value, uint8 new_value) noexcept { InterlockedExchange8((volatile char *) value, (char) new_value); }
FORCE_INLINE void atomic_set_release(volatile uint16* value, uint16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchange16((volatile short *) value, (short) new_value); }
FORCE_INLINE void atomic_set_release(volatile uint32* value, uint32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedExchange((volatile long *) value, new_value); }
FORCE_INLINE void atomic_set_release(volatile uint64* value, uint64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedExchange64((volatile LONG64 *) value, (LONG64) new_value); }
FORCE_INLINE uint8 atomic_fetch_set_release(volatile uint8* value, uint8 new_value) noexcept { return (uint8) InterlockedExchange8((volatile char *) value, (char) new_value); }
FORCE_INLINE uint16 atomic_fetch_set_release(volatile uint16* value, uint16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (uint16) InterlockedExchange16((volatile short *) value, (short) new_value); }
FORCE_INLINE uint32 atomic_fetch_set_release(volatile uint32* value, uint32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedExchange((volatile long *) value, new_value); }
FORCE_INLINE uint64 atomic_fetch_set_release(volatile uint64* value, uint64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedExchange64((volatile LONG64 *) value, (LONG64) new_value); }
FORCE_INLINE uint8 atomic_get_release(volatile uint8* value) noexcept { return (uint8) _InterlockedCompareExchange8((volatile char *) value, 0, 0); }
FORCE_INLINE uint16 atomic_get_release(volatile uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (uint16) InterlockedCompareExchangeRelease16((volatile short *) value, 0, 0); }
FORCE_INLINE uint32 atomic_get_release(volatile uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedCompareExchangeRelease((volatile long *) value, 0, 0); }
FORCE_INLINE uint64 atomic_get_release(volatile uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedCompareExchangeRelease64((volatile LONG64 *) value, 0, 0); }
FORCE_INLINE uint8 atomic_increment_release(volatile uint8* value) noexcept { return InterlockedExchangeAdd8((volatile char *) value, 1); }
FORCE_INLINE uint8 atomic_decrement_release(volatile uint8* value) noexcept { return InterlockedExchangeAdd8((volatile char *) value, -1); }
FORCE_INLINE uint16 atomic_increment_release(volatile uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return InterlockedIncrementRelease16((volatile short *) value); }
FORCE_INLINE uint16 atomic_decrement_release(volatile uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return InterlockedDecrementRelease16((volatile short *) value); }
FORCE_INLINE uint32 atomic_increment_release(volatile uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return InterlockedIncrementRelease((volatile long *) value); }
FORCE_INLINE uint32 atomic_decrement_release(volatile uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return InterlockedDecrementRelease((volatile long *) value); }
FORCE_INLINE uint64 atomic_increment_release(volatile uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return InterlockedIncrementRelease64((volatile LONG64 *) value); }
FORCE_INLINE uint64 atomic_decrement_release(volatile uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return InterlockedDecrementRelease64((volatile LONG64 *) value); }
FORCE_INLINE void atomic_add_release(volatile uint8* value, uint8 increment) noexcept { InterlockedExchangeAdd8((volatile char *) value, (char) increment); }
FORCE_INLINE void atomic_sub_release(volatile uint8* value, uint8 decrement) noexcept { InterlockedExchangeAdd8((volatile char *) value, -((char) decrement)); }
FORCE_INLINE void atomic_add_release(volatile uint16* value, uint16 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeAdd16((volatile short *) value, (short) increment); }
FORCE_INLINE void atomic_sub_release(volatile uint16* value, uint16 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeAdd16((volatile short *) value, -((short) decrement)); }
FORCE_INLINE void atomic_add_release(volatile uint32* value, uint32 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAddRelease((volatile long *) value, increment); }
FORCE_INLINE void atomic_sub_release(volatile uint32* value, uint32 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAddRelease((volatile long *) value, -1 * ((int32) decrement)); }
FORCE_INLINE void atomic_add_release(volatile uint64* value, uint64 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAddRelease64((volatile LONG64 *) value, (LONG64) increment); }
FORCE_INLINE void atomic_sub_release(volatile uint64* value, uint64 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAddRelease64((volatile LONG64 *) value, -((LONG64) decrement)); }
FORCE_INLINE uint32 atomic_compare_exchange_strong_release(volatile uint32* value, uint32* expected, uint32 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedCompareExchangeRelease((volatile long *) value, desired, *expected); }
FORCE_INLINE uint64 atomic_compare_exchange_strong_release(volatile uint64* value, uint64* expected, uint64 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedCompareExchangeRelease64((volatile LONG64 *) value, (LONG64) desired, (LONG64) *expected); }
FORCE_INLINE uint8 atomic_fetch_add_release(volatile uint8* value, uint8 operand) noexcept { return (uint8) InterlockedExchangeAdd8((volatile char *) value, (char) operand); }
FORCE_INLINE uint8 atomic_fetch_sub_release(volatile uint8* value, uint8 operand) noexcept { return (uint8) InterlockedExchangeAdd8((volatile char *) value, -((char) operand)); }
FORCE_INLINE uint16 atomic_fetch_add_release(volatile uint16* value, uint16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (uint16) InterlockedExchangeAdd16((volatile short *) value, (short) operand); }
FORCE_INLINE uint16 atomic_fetch_sub_release(volatile uint16* value, uint16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (uint16) InterlockedExchangeAdd16((volatile short *) value, -((short) operand)); }
FORCE_INLINE uint32 atomic_fetch_add_release(volatile uint32* value, uint32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedExchangeAddRelease((volatile long *) value, operand); }
FORCE_INLINE uint32 atomic_fetch_sub_release(volatile uint32* value, uint32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedExchangeAddRelease((volatile unsigned long *) value, -((long) operand)); }
FORCE_INLINE uint64 atomic_fetch_add_release(volatile uint64* value, uint64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedExchangeAddRelease64((volatile LONG64 *) value, (LONG64) operand); }
FORCE_INLINE uint64 atomic_fetch_sub_release(volatile uint64* value, uint64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedExchangeAdd64((volatile LONG64 *) value, -((LONG64) operand)); }
FORCE_INLINE void atomic_and_release(volatile uint8* value, uint8 mask) noexcept { InterlockedAnd8((volatile char *) value, mask); }
FORCE_INLINE void atomic_and_release(volatile int8* value, int8 mask) noexcept { InterlockedAnd8((volatile char *) value, mask); }
FORCE_INLINE void atomic_and_release(volatile uint16* value, uint16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedAnd16((volatile short *) value, mask); }
FORCE_INLINE void atomic_and_release(volatile int16* value, int16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedAnd16((volatile short *) value, mask); }
FORCE_INLINE void atomic_and_release(volatile uint32* value, uint32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAndRelease((volatile LONG *) value, mask); }
FORCE_INLINE void atomic_and_release(volatile int32* value, int32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAndRelease((volatile LONG *) value, (LONG)mask); }
FORCE_INLINE void atomic_and_release(volatile uint64* value, uint64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAnd64Release((volatile LONG64 *) value, mask); }
FORCE_INLINE void atomic_and_release(volatile int64* value, int64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAnd64Release((volatile LONG64 *) value, mask); }
FORCE_INLINE void atomic_or_release(volatile uint8* value, uint8 mask) noexcept { InterlockedOr8((volatile char *) value, mask); }
FORCE_INLINE void atomic_or_release(volatile int8* value, int8 mask) noexcept { InterlockedOr8((volatile char *) value, mask); }
FORCE_INLINE void atomic_or_release(volatile uint16* value, uint16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedOr16((volatile short *) value, mask); }
FORCE_INLINE void atomic_or_release(volatile int16* value, int16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedOr16((volatile short *) value, mask); }
FORCE_INLINE void atomic_or_release(volatile uint32* value, uint32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedOrRelease((volatile LONG *) value, mask); }
FORCE_INLINE void atomic_or_release(volatile int32* value, int32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedOrRelease((volatile LONG *) value, (LONG)mask); }
FORCE_INLINE void atomic_or_release(volatile uint64* value, uint64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedOr64Release((volatile LONG64 *) value, mask); }
FORCE_INLINE void atomic_or_release(volatile int64* value, int64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedOr64Release((volatile LONG64 *) value, mask); }

FORCE_INLINE void atomic_set_acquire_release(void** target, void* new_pointer) noexcept { InterlockedExchangePointer(target, new_pointer); }
FORCE_INLINE void* atomic_get_acquire_release(void** target) noexcept { return InterlockedCompareExchangePointer(target, NULL, NULL); }
FORCE_INLINE void atomic_set_acquire_release(volatile int8* value, int8 new_value) noexcept { InterlockedExchange8((volatile char *) value, new_value); }
FORCE_INLINE void atomic_set_acquire_release(volatile int16* value, int16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchange16((volatile short *) value, new_value); }
FORCE_INLINE void atomic_set_acquire_release(volatile int32* value, int32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedExchange((volatile long *) value, new_value); }
FORCE_INLINE void atomic_set_acquire_release(volatile int64* value, int64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedExchange64((volatile LONG64 *) value, (LONG64) new_value); }
FORCE_INLINE void atomic_set_acquire_release(volatile f32* value, f32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); _atomic_32 temp = {.f = new_value}; InterlockedExchange((volatile long *) value, (long) temp.l); }
FORCE_INLINE void atomic_set_acquire_release(volatile f64* value, f64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); _atomic_64 temp = {.f = new_value}; InterlockedExchange64((volatile LONG64 *) value, (LONG64) temp.l); }
FORCE_INLINE int8 atomic_fetch_set_acquire_release(volatile int8* value, int8 new_value) noexcept { return (int8) InterlockedExchange8((volatile char *) value, (char) new_value); }
FORCE_INLINE int16 atomic_fetch_set_acquire_release(volatile int16* value, int16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (int16) InterlockedExchange16((volatile short *) value, (short) new_value); }
FORCE_INLINE int32 atomic_fetch_set_acquire_release(volatile int32* value, int32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedExchange((volatile long *) value, new_value); }
FORCE_INLINE int64 atomic_fetch_set_acquire_release(volatile int64* value, int64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedExchange64((volatile LONG64 *) value, (LONG64) new_value); }
FORCE_INLINE int8 atomic_get_acquire_release(volatile int8* value) noexcept { return (int8) _InterlockedCompareExchange8((volatile char *) value, 0, 0); }
FORCE_INLINE int16 atomic_get_acquire_release(volatile int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (int16) InterlockedCompareExchange16((volatile short *) value, 0, 0); }
FORCE_INLINE int32 atomic_get_acquire_release(volatile int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedCompareExchange((volatile long *) value, 0, 0); }
FORCE_INLINE int64 atomic_get_acquire_release(volatile int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedCompareExchange64((volatile LONG64 *) value, 0, 0); }
FORCE_INLINE f32 atomic_get_acquire_release(volatile f32* value) noexcept { _atomic_32 temp = {.l = InterlockedCompareExchange((volatile long *) value, 0, 0)}; return temp.f; }
FORCE_INLINE f64 atomic_get_acquire_release(volatile f64* value) noexcept { _atomic_64 temp = {.l = InterlockedCompareExchange64((volatile LONG64 *) value, 0, 0)}; return temp.f; }
FORCE_INLINE int8 atomic_increment_acquire_release(volatile int8* value) noexcept { return InterlockedExchangeAdd8((volatile char *) value, 1); }
FORCE_INLINE int8 atomic_decrement_acquire_release(volatile int8* value) noexcept { return InterlockedExchangeAdd8((volatile char *) value, -1); }
FORCE_INLINE int16 atomic_increment_acquire_release(volatile int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return InterlockedIncrement16((volatile short *) value); }
FORCE_INLINE int16 atomic_decrement_acquire_release(volatile int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return InterlockedDecrement16((volatile short *) value); }
FORCE_INLINE int32 atomic_increment_acquire_release(volatile int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return InterlockedIncrement((volatile long *) value); }
FORCE_INLINE int32 atomic_decrement_acquire_release(volatile int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return InterlockedDecrement((volatile long *) value); }
FORCE_INLINE int64 atomic_increment_acquire_release(volatile int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return InterlockedIncrement64((volatile LONG64 *) value); }
FORCE_INLINE int64 atomic_decrement_acquire_release(volatile int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return InterlockedDecrement64((volatile LONG64 *) value); }
FORCE_INLINE void atomic_add_acquire_release(volatile int8* value, int8 increment) noexcept { InterlockedExchangeAdd8((volatile char *) value, (char) increment); }
FORCE_INLINE void atomic_sub_acquire_release(volatile int8* value, int8 decrement) noexcept { InterlockedExchangeAdd8((volatile char *) value, -((char) decrement)); }
FORCE_INLINE void atomic_add_acquire_release(volatile int16* value, int16 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeAdd16((volatile short *) value, (short) increment); }
FORCE_INLINE void atomic_sub_acquire_release(volatile int16* value, int16 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeAdd16((volatile short *) value, -((short) decrement)); }
FORCE_INLINE void atomic_add_acquire_release(volatile int32* value, int32 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAdd((volatile long *) value, increment); }
FORCE_INLINE void atomic_sub_acquire_release(volatile int32* value, int32 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAdd((volatile long *) value, -decrement); }
FORCE_INLINE void atomic_add_acquire_release(volatile int64* value, int64 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAdd64((volatile LONG64 *) value, (LONG64) increment); }
FORCE_INLINE void atomic_sub_acquire_release(volatile int64* value, int64 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAdd64((volatile LONG64 *) value, -((LONG64) decrement)); }
FORCE_INLINE f32 atomic_compare_exchange_strong_acquire_release(volatile f32* value, f32 expected, f32 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); _atomic_32 temp = {.l = InterlockedCompareExchange((volatile long *) value, (long) desired, (long) expected) }; return temp.f; }
FORCE_INLINE f64 atomic_compare_exchange_strong_acquire_release(volatile f64* value, f64 expected, f64 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); _atomic_64 temp = {.l = InterlockedCompareExchange64((volatile LONG64 *) value, (LONG64) desired, (LONG64) expected) }; return temp.f; }
FORCE_INLINE int32 atomic_compare_exchange_strong_acquire_release(volatile int32* value, int32 expected, int32 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedCompareExchange((volatile long *) value, desired, expected); }
FORCE_INLINE int64 atomic_compare_exchange_strong_acquire_release(volatile int64* value, int64 expected, int64 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedCompareExchange64((volatile LONG64 *) value, (LONG64) desired, (LONG64) expected); }
FORCE_INLINE int8 atomic_fetch_add_acquire_release(volatile int8* value, int8 operand) noexcept { return (int8) InterlockedExchangeAdd8((volatile char *) value, (char) operand); }
FORCE_INLINE int8 atomic_fetch_sub_acquire_release(volatile int8* value, int8 operand) noexcept { return (int8) InterlockedExchangeAdd8((volatile char *) value, -((char) operand)); }
FORCE_INLINE int16 atomic_fetch_add_acquire_release(volatile int16* value, int16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (int16) InterlockedExchangeAdd16((volatile short *) value, (short) operand); }
FORCE_INLINE int16 atomic_fetch_sub_acquire_release(volatile int16* value, int16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (int16) InterlockedExchangeAdd16((volatile short *) value, -((short) operand)); }
FORCE_INLINE int32 atomic_fetch_add_acquire_release(volatile int32* value, int32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedExchangeAdd((volatile long *) value, operand); }
FORCE_INLINE int32 atomic_fetch_sub_acquire_release(volatile int32* value, int32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (int32) InterlockedExchangeAdd((volatile unsigned long *) value, -((long) operand)); }
FORCE_INLINE int64 atomic_fetch_add_acquire_release(volatile int64* value, int64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedExchangeAdd64((volatile LONG64 *) value, (LONG64) operand); }
FORCE_INLINE int64 atomic_fetch_sub_acquire_release(volatile int64* value, int64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (int64) InterlockedExchangeAdd64((volatile LONG64 *) value, -((LONG64) operand)); }
FORCE_INLINE void atomic_set_acquire_release(volatile uint8* value, uint8 new_value) noexcept { InterlockedExchange8((volatile char *) value, (char) new_value); }
FORCE_INLINE void atomic_set_acquire_release(volatile uint16* value, uint16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchange16((volatile short *) value, (short) new_value); }
FORCE_INLINE void atomic_set_acquire_release(volatile uint32* value, uint32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedExchange((volatile long *) value, new_value); }
FORCE_INLINE void atomic_set_acquire_release(volatile uint64* value, uint64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedExchange64((volatile LONG64 *) value, (LONG64) new_value); }
FORCE_INLINE uint8 atomic_fetch_set_acquire_release(volatile uint8* value, uint8 new_value) noexcept { return (uint8) InterlockedExchange8((volatile char *) value, (char) new_value); }
FORCE_INLINE uint16 atomic_fetch_set_acquire_release(volatile uint16* value, uint16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (uint16) InterlockedExchange16((volatile short *) value, (short) new_value); }
FORCE_INLINE uint32 atomic_fetch_set_acquire_release(volatile uint32* value, uint32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedExchange((volatile long *) value, new_value); }
FORCE_INLINE uint64 atomic_fetch_set_acquire_release(volatile uint64* value, uint64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedExchange64((volatile LONG64 *) value, (LONG64) new_value); }
FORCE_INLINE uint8 atomic_get_acquire_release(volatile uint8* value) noexcept { return (uint8) _InterlockedCompareExchange8((volatile char *) value, 0, 0); }
FORCE_INLINE uint16 atomic_get_acquire_release(volatile uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (uint16) InterlockedCompareExchange16((volatile short *) value, 0, 0); }
FORCE_INLINE uint32 atomic_get_acquire_release(volatile uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedCompareExchange((volatile long *) value, 0, 0); }
FORCE_INLINE uint64 atomic_get_acquire_release(volatile uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedCompareExchange64((volatile LONG64 *) value, 0, 0); }
FORCE_INLINE uint8 atomic_increment_acquire_release(volatile uint8* value) noexcept { return InterlockedExchangeAdd8((volatile char *) value, 1); }
FORCE_INLINE uint8 atomic_decrement_acquire_release(volatile uint8* value) noexcept { return InterlockedExchangeAdd8((volatile char *) value, -1); }
FORCE_INLINE uint16 atomic_increment_acquire_release(volatile uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return InterlockedIncrement16((volatile short *) value); }
FORCE_INLINE uint16 atomic_decrement_acquire_release(volatile uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return InterlockedDecrement16((volatile short *) value); }
FORCE_INLINE uint32 atomic_increment_acquire_release(volatile uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return InterlockedIncrement((volatile long *) value); }
FORCE_INLINE uint32 atomic_decrement_acquire_release(volatile uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return InterlockedDecrement((volatile long *) value); }
FORCE_INLINE uint64 atomic_increment_acquire_release(volatile uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return InterlockedIncrement64((volatile LONG64 *) value); }
FORCE_INLINE uint64 atomic_decrement_acquire_release(volatile uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return InterlockedDecrement64((volatile LONG64 *) value); }
FORCE_INLINE void atomic_add_acquire_release(volatile uint8* value, uint8 increment) noexcept { InterlockedExchangeAdd8((volatile char *) value, (char) increment); }
FORCE_INLINE void atomic_sub_acquire_release(volatile uint8* value, uint8 decrement) noexcept { InterlockedExchangeAdd8((volatile char *) value, -((char) decrement)); }
FORCE_INLINE void atomic_add_acquire_release(volatile uint16* value, uint16 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeAdd16((volatile short *) value, (short) increment); }
FORCE_INLINE void atomic_sub_acquire_release(volatile uint16* value, uint16 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedExchangeAdd16((volatile short *) value, -((short) decrement)); }
FORCE_INLINE void atomic_add_acquire_release(volatile uint32* value, uint32 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAdd((volatile long *) value, increment); }
FORCE_INLINE void atomic_sub_acquire_release(volatile uint32* value, uint32 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAdd((volatile long *) value, -1 * ((int32) decrement)); }
FORCE_INLINE void atomic_add_acquire_release(volatile uint64* value, uint64 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAdd64((volatile LONG64 *) value, (LONG64) increment); }
FORCE_INLINE void atomic_sub_acquire_release(volatile uint64* value, uint64 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAdd64((volatile LONG64 *) value, -((LONG64) decrement)); }
FORCE_INLINE uint32 atomic_compare_exchange_strong_acquire_release(volatile uint32* value, uint32 expected, uint32 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedCompareExchange((volatile long *) value, desired, expected); }
FORCE_INLINE uint64 atomic_compare_exchange_strong_acquire_release(volatile uint64* value, uint64 expected, uint64 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedCompareExchange64((volatile LONG64 *) value, (LONG64) desired, (LONG64) expected); }
FORCE_INLINE uint8 atomic_fetch_add_acquire_release(volatile uint8* value, uint8 operand) noexcept { return (uint8) InterlockedExchangeAdd8((volatile char *) value, (char) operand); }
FORCE_INLINE uint8 atomic_fetch_sub_acquire_release(volatile uint8* value, uint8 operand) noexcept { return (uint8) InterlockedExchangeAdd8((volatile char *) value, -((char) operand)); }
FORCE_INLINE uint16 atomic_fetch_add_acquire_release(volatile uint16* value, uint16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (uint16) InterlockedExchangeAdd16((volatile short *) value, (short) operand); }
FORCE_INLINE uint16 atomic_fetch_sub_acquire_release(volatile uint16* value, uint16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return (uint16) InterlockedExchangeAdd16((volatile short *) value, -((short) operand)); }
FORCE_INLINE uint32 atomic_fetch_add_acquire_release(volatile uint32* value, uint32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedExchangeAdd((volatile long *) value, operand); }
FORCE_INLINE uint32 atomic_fetch_sub_acquire_release(volatile uint32* value, uint32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return (uint32) InterlockedExchangeAdd((volatile unsigned long *) value, -((long) operand)); }
FORCE_INLINE uint64 atomic_fetch_add_acquire_release(volatile uint64* value, uint64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedExchangeAdd64((volatile LONG64 *) value, (LONG64) operand); }
FORCE_INLINE uint64 atomic_fetch_sub_acquire_release(volatile uint64* value, uint64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return (uint64) InterlockedExchangeAdd64((volatile LONG64 *) value, -((LONG64) operand)); }
FORCE_INLINE void atomic_and_acquire_release(volatile uint8* value, uint8 mask) noexcept { InterlockedAnd8((volatile char *) value, mask); }
FORCE_INLINE void atomic_and_acquire_release(volatile int8* value, int8 mask) noexcept { InterlockedAnd8((volatile char *) value, mask); }
FORCE_INLINE void atomic_and_acquire_release(volatile uint16* value, uint16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedAnd16((volatile short *) value, mask); }
FORCE_INLINE void atomic_and_acquire_release(volatile int16* value, int16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedAnd16((volatile short *) value, mask); }
FORCE_INLINE void atomic_and_acquire_release(volatile uint32* value, uint32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAnd((volatile LONG *) value, mask); }
FORCE_INLINE void atomic_and_acquire_release(volatile int32* value, int32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedAnd((volatile LONG *) value, (LONG)mask); }
FORCE_INLINE void atomic_and_acquire_release(volatile uint64* value, uint64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAnd64((volatile LONG64 *) value, mask); }
FORCE_INLINE void atomic_and_acquire_release(volatile int64* value, int64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedAnd64((volatile LONG64 *) value, mask); }
FORCE_INLINE void atomic_or_acquire_release(volatile uint8* value, uint8 mask) noexcept { InterlockedOr8((volatile char *) value, mask); }
FORCE_INLINE void atomic_or_acquire_release(volatile int8* value, int8 mask) noexcept { InterlockedOr8((volatile char *) value, mask); }
FORCE_INLINE void atomic_or_acquire_release(volatile uint16* value, uint16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedOr16((volatile short *) value, mask); }
FORCE_INLINE void atomic_or_acquire_release(volatile int16* value, int16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); InterlockedOr16((volatile short *) value, mask); }
FORCE_INLINE void atomic_or_acquire_release(volatile uint32* value, uint32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedOr((volatile LONG *) value, mask); }
FORCE_INLINE void atomic_or_acquire_release(volatile int32* value, int32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); InterlockedOr((volatile LONG *) value, (LONG)mask); }
FORCE_INLINE void atomic_or_acquire_release(volatile uint64* value, uint64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedOr64((volatile LONG64 *) value, mask); }
FORCE_INLINE void atomic_or_acquire_release(volatile int64* value, int64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); InterlockedOr64((volatile LONG64 *) value, mask); }

// Check out the intrinsic functions fence_memory and fence_write
// These are much faster and could accomplish what you are doing
#define atomic_fence_acquire() MemoryBarrier();

// Check out the intrinsic functions fence_memory and fence_write
// These are much faster and could accomplish what you are doing
#define atomic_fence_release() MemoryBarrier();

#endif