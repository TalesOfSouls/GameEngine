/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_COMPILER_GCC_ATOMIC_H
#define COMS_COMPILER_GCC_ATOMIC_H

#include "../../stdlib/Stdlib.h"
#include "CompilerUtils.h"

typedef union { f32 f; int32 l; } _atomic_32;
typedef union { f64 f; int64 l; } _atomic_64;

FORCE_INLINE void atomic_set_relaxed(void** target, void* value) noexcept { __atomic_store_n(target, value, __ATOMIC_RELAXED); }
FORCE_INLINE void* atomic_get_relaxed(void** target) noexcept { return __atomic_load_n(target, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_set_relaxed(int8* value, int8 new_value) noexcept { __atomic_store_n(value, new_value, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_set_relaxed(int16* value, int16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_store_n(value, new_value, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_set_relaxed(int32* value, int32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_store_n(value, new_value, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_set_relaxed(int64* value, int64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_store_n(value, new_value, __ATOMIC_RELAXED); }
FORCE_INLINE int8 atomic_fetch_set_relaxed(int8* value, int8 new_value) noexcept { return __atomic_exchange_n(value, new_value, __ATOMIC_RELAXED); }
FORCE_INLINE int16 atomic_fetch_set_relaxed(int16* value, int16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_RELAXED); }
FORCE_INLINE int32 atomic_fetch_set_relaxed(int32* value, int32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_RELAXED); }
FORCE_INLINE int64 atomic_fetch_set_relaxed(int64* value, int64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_RELAXED); }
FORCE_INLINE int8 atomic_get_relaxed(const int8* value) noexcept { return __atomic_load_n((int8 *) value, __ATOMIC_RELAXED); }
FORCE_INLINE int16 atomic_get_relaxed(const int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_load_n((int16 *) value, __ATOMIC_RELAXED); }
FORCE_INLINE int32 atomic_get_relaxed(const int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_load_n((int32 *) value, __ATOMIC_RELAXED); }
FORCE_INLINE int64 atomic_get_relaxed(const int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_load_n((int64 *) value, __ATOMIC_RELAXED); }
FORCE_INLINE int8 atomic_increment_relaxed(int8* value) noexcept { return __atomic_add_fetch(value, 1, __ATOMIC_RELAXED); }
FORCE_INLINE int8 atomic_decrement_relaxed(int8* value) noexcept { return __atomic_sub_fetch(value, 1, __ATOMIC_RELAXED); }
FORCE_INLINE int16 atomic_increment_relaxed(int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_RELAXED); }
FORCE_INLINE int16 atomic_decrement_relaxed(int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_RELAXED); }
FORCE_INLINE int32 atomic_increment_relaxed(int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_RELAXED); }
FORCE_INLINE int32 atomic_decrement_relaxed(int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_RELAXED); }
FORCE_INLINE int64 atomic_increment_relaxed(int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_RELAXED); }
FORCE_INLINE int64 atomic_decrement_relaxed(int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_add_relaxed(int8* value, int8 increment) noexcept { __atomic_add_fetch(value, increment, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_sub_relaxed(int8* value, int8 decrement) noexcept { __atomic_sub_fetch(value, decrement, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_add_relaxed(int16* value, int16 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_add_fetch(value, increment, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_sub_relaxed(int16* value, int16 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_add_relaxed(int32* value, int32 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_add_fetch(value, increment, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_sub_relaxed(int32* value, int32 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_add_relaxed(int64* value, int64 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_add_fetch(value, increment, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_sub_relaxed(int64* value, int64 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_RELAXED); }
FORCE_INLINE f32 atomic_compare_exchange_strong_relaxed(f32* value, f32 expected, f32 desired) noexcept {
    ASSERT_STRICT(((uintptr_t) value % 4) == 0);

    _atomic_32* value_as_union = (_atomic_32*)value;
    _atomic_32* expected_as_union = (_atomic_32*)expected;
    _atomic_32 desired_as_union;
    desired_as_union.f = desired;

    __atomic_compare_exchange_n(
        &value_as_union->l, &expected_as_union->l, desired_as_union.l, 0,
        __ATOMIC_RELAXED, __ATOMIC_RELAXED
    );

    return expected_as_union->f;
}
FORCE_INLINE f64 atomic_compare_exchange_strong_relaxed(f64* value, f64 expected, f64 desired) noexcept {

    _atomic_64* value_as_union = (_atomic_64*)value;
    _atomic_64* expected_as_union = (_atomic_64*)expected;
    _atomic_64 desired_as_union;
    desired_as_union.f = desired;

    __atomic_compare_exchange_n(
        &value_as_union->l, &expected_as_union->l, desired_as_union.l, 0,
        __ATOMIC_RELAXED, __ATOMIC_RELAXED
    );

    return expected_as_union->f;
}
FORCE_INLINE int32 atomic_compare_exchange_strong_relaxed(int32* value, int32 expected, int32 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_compare_exchange_n(value, expected, &desired, 0, __ATOMIC_RELAXED, __ATOMIC_RELAXED); return *expected; }
FORCE_INLINE int64 atomic_compare_exchange_strong_relaxed(int64* value, int64 expected, int64 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_compare_exchange_n(value, expected, &desired, 0, __ATOMIC_RELAXED, __ATOMIC_RELAXED); return *expected; }
FORCE_INLINE int8 atomic_fetch_add_relaxed(int8* value, int8 operand) noexcept { return __atomic_add_fetch(value, operand, __ATOMIC_RELAXED); }
FORCE_INLINE int8 atomic_fetch_sub_relaxed(int8* value, int8 operand) noexcept { return __atomic_sub_fetch(value, operand, __ATOMIC_RELAXED); }
FORCE_INLINE int16 atomic_fetch_add_relaxed(int16* value, int16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_RELAXED); }
FORCE_INLINE int16 atomic_fetch_sub_relaxed(int16* value, int16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_RELAXED); }
FORCE_INLINE int32 atomic_fetch_add_relaxed(int32* value, int32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_RELAXED); }
FORCE_INLINE int32 atomic_fetch_sub_relaxed(int32* value, int32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_RELAXED); }
FORCE_INLINE int64 atomic_fetch_add_relaxed(int64* value, int64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_RELAXED); }
FORCE_INLINE int64 atomic_fetch_sub_relaxed(int64* value, int64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_set_relaxed(uint8* value, uint8 new_value) noexcept { __atomic_store_n(value, new_value, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_set_relaxed(uint16* value, uint16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_store_n(value, new_value, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_set_relaxed(uint32* value, uint32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_store_n(value, new_value, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_set_relaxed(uint64* value, uint64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_store_n(value, new_value, __ATOMIC_RELAXED); }
FORCE_INLINE uint8 atomic_fetch_set_relaxed(uint8* value, uint8 new_value) noexcept { return __atomic_exchange_n(value, new_value, __ATOMIC_RELAXED); }
FORCE_INLINE uint16 atomic_fetch_set_relaxed(uint16* value, uint16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_RELAXED); }
FORCE_INLINE uint32 atomic_fetch_set_relaxed(uint32* value, uint32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_RELAXED); }
FORCE_INLINE uint64 atomic_fetch_set_relaxed(uint64* value, uint64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_RELAXED); }
FORCE_INLINE uint8 atomic_get_relaxed(const uint8* value) noexcept { return __atomic_load_n(value, __ATOMIC_RELAXED); }
FORCE_INLINE uint16 atomic_get_relaxed(const uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_load_n(value, __ATOMIC_RELAXED); }
FORCE_INLINE uint32 atomic_get_relaxed(const uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_load_n(value, __ATOMIC_RELAXED); }
FORCE_INLINE uint64 atomic_get_relaxed(const uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_load_n(value, __ATOMIC_RELAXED); }
FORCE_INLINE uint8 atomic_increment_relaxed(uint8* value) noexcept { return __atomic_add_fetch(value, 1, __ATOMIC_RELAXED); }
FORCE_INLINE uint8 atomic_decrement_relaxed(uint8* value) noexcept { return __atomic_sub_fetch(value, 1, __ATOMIC_RELAXED); }
FORCE_INLINE uint16 atomic_increment_relaxed(uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_RELAXED); }
FORCE_INLINE uint16 atomic_decrement_relaxed(uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_RELAXED); }
FORCE_INLINE uint32 atomic_increment_relaxed(uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_RELAXED); }
FORCE_INLINE uint32 atomic_decrement_relaxed(uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_RELAXED); }
FORCE_INLINE uint64 atomic_increment_relaxed(uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_RELAXED); }
FORCE_INLINE uint64 atomic_decrement_relaxed(uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_add_relaxed(uint8* value, uint8 increment) noexcept { __atomic_add_fetch(value, increment, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_sub_relaxed(uint8* value, uint8 decrement) noexcept { __atomic_sub_fetch(value, decrement, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_add_relaxed(uint16* value, uint16 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_add_fetch(value, increment, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_sub_relaxed(uint16* value, uint16 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_add_relaxed(uint32* value, uint32 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_add_fetch(value, increment, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_sub_relaxed(uint32* value, uint32 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_add_relaxed(uint64* value, uint64 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_add_fetch(value, increment, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_sub_relaxed(uint64* value, uint64 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_RELAXED); }
FORCE_INLINE uint32 atomic_compare_exchange_strong_relaxed(uint32* value, uint32 expected, uint32 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_compare_exchange_n(value, expected, &desired, 0, __ATOMIC_RELAXED, __ATOMIC_RELAXED); return *expected; }
FORCE_INLINE uint64 atomic_compare_exchange_strong_relaxed(uint64* value, uint64 expected, uint64 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_compare_exchange_n(value, expected, &desired, 0, __ATOMIC_RELAXED, __ATOMIC_RELAXED); return *expected; }
FORCE_INLINE uint8 atomic_fetch_add_relaxed(uint8* value, uint8 operand) noexcept { return __atomic_add_fetch(value, operand, __ATOMIC_RELAXED); }
FORCE_INLINE uint8 atomic_fetch_sub_relaxed(uint8* value, uint8 operand) noexcept { return __atomic_sub_fetch(value, operand, __ATOMIC_RELAXED); }
FORCE_INLINE uint16 atomic_fetch_add_relaxed(uint16* value, uint16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_RELAXED); }
FORCE_INLINE uint16 atomic_fetch_sub_relaxed(uint16* value, uint16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_RELAXED); }
FORCE_INLINE uint32 atomic_fetch_add_relaxed(uint32* value, uint32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_RELAXED); }
FORCE_INLINE uint32 atomic_fetch_sub_relaxed(uint32* value, uint32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_RELAXED); }
FORCE_INLINE uint64 atomic_fetch_add_relaxed(uint64* value, uint64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_RELAXED); }
FORCE_INLINE uint64 atomic_fetch_sub_relaxed(uint64* value, uint64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_and_relaxed(uint8* value, uint8 mask) noexcept { __atomic_fetch_and(value, mask, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_and_relaxed(int8* value, int8 mask) noexcept { __atomic_fetch_and(value, mask, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_and_relaxed(uint16* value, uint16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_fetch_and(value, mask, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_and_relaxed(int16* value, int16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_fetch_and(value, mask, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_and_relaxed(uint32* value, uint32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_fetch_and(value, mask, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_and_relaxed(int32* value, int32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_fetch_and(value, mask, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_and_relaxed(uint64* value, uint64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_fetch_and(value, mask, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_and_relaxed(int64* value, int64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_fetch_and(value, mask, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_or_relaxed(uint8* value, uint8 mask) noexcept { __atomic_fetch_or(value, mask, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_or_relaxed(int8* value, int8 mask) noexcept { __atomic_fetch_or(value, mask, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_or_relaxed(uint16* value, uint16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_fetch_or(value, mask, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_or_relaxed(int16* value, int16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_fetch_or(value, mask, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_or_relaxed(uint32* value, uint32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_fetch_or(value, mask, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_or_relaxed(int32* value, int32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_fetch_or(value, mask, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_or_relaxed(uint64* value, uint64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_fetch_or(value, mask, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_or_relaxed(int64* value, int64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_fetch_or(value, mask, __ATOMIC_RELAXED); }

FORCE_INLINE void* atomic_get_acquire(void** target) noexcept { return __atomic_load_n(target, __ATOMIC_ACQUIRE); }
FORCE_INLINE int8 atomic_fetch_set_acquire(int8* value, int8 new_value) noexcept { return __atomic_exchange_n(value, new_value, __ATOMIC_ACQUIRE); }
FORCE_INLINE int16 atomic_fetch_set_acquire(int16* value, int16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_ACQUIRE); }
FORCE_INLINE int32 atomic_fetch_set_acquire(int32* value, int32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_ACQUIRE); }
FORCE_INLINE int64 atomic_fetch_set_acquire(int64* value, int64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_ACQUIRE); }
FORCE_INLINE int8 atomic_get_acquire(const int8* value) noexcept { return __atomic_load_n((int8 *) value, __ATOMIC_ACQUIRE); }
FORCE_INLINE int16 atomic_get_acquire(const int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_load_n((int16 *) value, __ATOMIC_ACQUIRE); }
FORCE_INLINE int32 atomic_get_acquire(const int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_load_n((int32 *) value, __ATOMIC_ACQUIRE); }
FORCE_INLINE int64 atomic_get_acquire(const int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_load_n((int64 *) value, __ATOMIC_ACQUIRE); }
FORCE_INLINE int8 atomic_increment_acquire(int8* value) noexcept { return __atomic_add_fetch(value, 1, __ATOMIC_ACQUIRE); }
FORCE_INLINE int8 atomic_decrement_acquire(int8* value) noexcept { return __atomic_sub_fetch(value, 1, __ATOMIC_ACQUIRE); }
FORCE_INLINE int16 atomic_increment_acquire(int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_ACQUIRE); }
FORCE_INLINE int16 atomic_decrement_acquire(int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_ACQUIRE); }
FORCE_INLINE int32 atomic_increment_acquire(int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_ACQUIRE); }
FORCE_INLINE int32 atomic_decrement_acquire(int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_ACQUIRE); }
FORCE_INLINE int64 atomic_increment_acquire(int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_ACQUIRE); }
FORCE_INLINE int64 atomic_decrement_acquire(int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_add_acquire(int8* value, int8 increment) noexcept { __atomic_add_fetch(value, increment, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_sub_acquire(int8* value, int8 decrement) noexcept { __atomic_sub_fetch(value, decrement, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_add_acquire(int16* value, int16 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_add_fetch(value, increment, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_sub_acquire(int16* value, int16 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_add_acquire(int32* value, int32 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_add_fetch(value, increment, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_sub_acquire(int32* value, int32 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_add_acquire(int64* value, int64 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_add_fetch(value, increment, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_sub_acquire(int64* value, int64 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_ACQUIRE); }
FORCE_INLINE f32 atomic_compare_exchange_strong_acquire(f32* value, f32 expected, f32 desired) noexcept {
    ASSERT_STRICT(((uintptr_t) value % 4) == 0);

    _atomic_32* value_as_union = (_atomic_32*)value;
    _atomic_32* expected_as_union = (_atomic_32*)expected;
    _atomic_32 desired_as_union;
    desired_as_union.f = desired;

    __atomic_compare_exchange_n(
        &value_as_union->l, &expected_as_union->l, desired_as_union.l, 0,
        __ATOMIC_ACQUIRE, __ATOMIC_RELAXED
    );

    return expected_as_union->f;
}
FORCE_INLINE f64 atomic_compare_exchange_strong_acquire(f64* value, f64 expected, f64 desired) noexcept {
    ASSERT_STRICT(((uintptr_t) value % 8) == 0);

    _atomic_64* value_as_union = (_atomic_64*)value;
    _atomic_64* expected_as_union = (_atomic_64*)expected;
    _atomic_64 desired_as_union;
    desired_as_union.f = desired;

    __atomic_compare_exchange_n(
        &value_as_union->l, &expected_as_union->l, desired_as_union.l, 0,
        __ATOMIC_ACQUIRE, __ATOMIC_RELAXED
    );

    return expected_as_union->f;
}
FORCE_INLINE int32 atomic_compare_exchange_strong_acquire(int32* value, int32 expected, int32 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_compare_exchange_n(value, expected, &desired, 0, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE); return *expected; }
FORCE_INLINE int64 atomic_compare_exchange_strong_acquire(int64* value, int64 expected, int64 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_compare_exchange_n(value, expected, &desired, 0, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE); return *expected; }
FORCE_INLINE int8 atomic_fetch_add_acquire(int8* value, int8 operand) noexcept { return __atomic_add_fetch(value, operand, __ATOMIC_ACQUIRE); }
FORCE_INLINE int8 atomic_fetch_sub_acquire(int8* value, int8 operand) noexcept { return __atomic_sub_fetch(value, operand, __ATOMIC_ACQUIRE); }
FORCE_INLINE int16 atomic_fetch_add_acquire(int16* value, int16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_ACQUIRE); }
FORCE_INLINE int16 atomic_fetch_sub_acquire(int16* value, int16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_ACQUIRE); }
FORCE_INLINE int32 atomic_fetch_add_acquire(int32* value, int32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_ACQUIRE); }
FORCE_INLINE int32 atomic_fetch_sub_acquire(int32* value, int32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_ACQUIRE); }
FORCE_INLINE int64 atomic_fetch_add_acquire(int64* value, int64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_ACQUIRE); }
FORCE_INLINE int64 atomic_fetch_sub_acquire(int64* value, int64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_set_acquire(uint8* value, uint8 new_value) noexcept { __atomic_store_n(value, new_value, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_set_acquire(uint16* value, uint16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_store_n(value, new_value, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_set_acquire(uint32* value, uint32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_store_n(value, new_value, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_set_acquire(uint64* value, uint64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_store_n(value, new_value, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint8 atomic_fetch_set_acquire(uint8* value, uint8 new_value) noexcept { return __atomic_exchange_n(value, new_value, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint16 atomic_fetch_set_acquire(uint16* value, uint16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint32 atomic_fetch_set_acquire(uint32* value, uint32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint64 atomic_fetch_set_acquire(uint64* value, uint64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint8 atomic_get_acquire(const uint8* value) noexcept { return __atomic_load_n(value, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint16 atomic_get_acquire(const uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_load_n(value, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint32 atomic_get_acquire(const uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_load_n(value, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint64 atomic_get_acquire(const uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_load_n(value, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint8 atomic_increment_acquire(uint8* value) noexcept { return __atomic_add_fetch(value, 1, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint8 atomic_decrement_acquire(uint8* value) noexcept { return __atomic_sub_fetch(value, 1, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint16 atomic_increment_acquire(uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint16 atomic_decrement_acquire(uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint32 atomic_increment_acquire(uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint32 atomic_decrement_acquire(uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint64 atomic_increment_acquire(uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint64 atomic_decrement_acquire(uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_add_acquire(uint8* value, uint8 increment) noexcept { __atomic_add_fetch(value, increment, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_sub_acquire(uint8* value, uint8 decrement) noexcept { __atomic_sub_fetch(value, decrement, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_add_acquire(uint16* value, uint16 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_add_fetch(value, increment, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_sub_acquire(uint16* value, uint16 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_add_acquire(uint32* value, uint32 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_add_fetch(value, increment, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_sub_acquire(uint32* value, uint32 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_add_acquire(uint64* value, uint64 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_add_fetch(value, increment, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_sub_acquire(uint64* value, uint64 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint32 atomic_compare_exchange_strong_acquire(uint32* value, uint32 expected, uint32 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_compare_exchange_n(value, expected, &desired, 0, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE); return *expected; }
FORCE_INLINE uint64 atomic_compare_exchange_strong_acquire(uint64* value, uint64 expected, uint64 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_compare_exchange_n(value, expected, &desired, 0, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE); return *expected; }
FORCE_INLINE uint8 atomic_fetch_add_acquire(uint8* value, uint8 operand) noexcept { return __atomic_add_fetch(value, operand, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint8 atomic_fetch_sub_acquire(uint8* value, uint8 operand) noexcept { return __atomic_sub_fetch(value, operand, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint16 atomic_fetch_add_acquire(uint16* value, uint16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint16 atomic_fetch_sub_acquire(uint16* value, uint16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint32 atomic_fetch_add_acquire(uint32* value, uint32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint32 atomic_fetch_sub_acquire(uint32* value, uint32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint64 atomic_fetch_add_acquire(uint64* value, uint64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint64 atomic_fetch_sub_acquire(uint64* value, uint64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_and_acquire(uint8* value, uint8 mask) noexcept { __atomic_fetch_and(value, mask, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_and_acquire(int8* value, int8 mask) noexcept { __atomic_fetch_and(value, mask, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_and_acquire(uint16* value, uint16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_fetch_and(value, mask, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_and_acquire(int16* value, int16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_fetch_and(value, mask, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_and_acquire(uint32* value, uint32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_fetch_and(value, mask, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_and_acquire(int32* value, int32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_fetch_and(value, mask, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_and_acquire(uint64* value, uint64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_fetch_and(value, mask, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_and_acquire(int64* value, int64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_fetch_and(value, mask, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_or_acquire(uint8* value, uint8 mask) noexcept { __atomic_fetch_or(value, mask, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_or_acquire(int8* value, int8 mask) noexcept { __atomic_fetch_or(value, mask, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_or_acquire(uint16* value, uint16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_fetch_or(value, mask, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_or_acquire(int16* value, int16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_fetch_or(value, mask, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_or_acquire(uint32* value, uint32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_fetch_or(value, mask, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_or_acquire(int32* value, int32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_fetch_or(value, mask, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_or_acquire(uint64* value, uint64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_fetch_or(value, mask, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_or_acquire(int64* value, int64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_fetch_or(value, mask, __ATOMIC_ACQUIRE); }

FORCE_INLINE void atomic_set_release(void** target, void* value) noexcept { __atomic_store_n(target, value, __ATOMIC_RELEASE); }
FORCE_INLINE void* atomic_get_release(void** target) noexcept { return __atomic_load_n(target, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_set_release(int8* value, int8 new_value) noexcept { __atomic_store_n(value, new_value, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_set_release(int16* value, int16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_store_n(value, new_value, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_set_release(int32* value, int32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_store_n(value, new_value, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_set_release(int64* value, int64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_store_n(value, new_value, __ATOMIC_RELEASE); }
FORCE_INLINE int8 atomic_fetch_set_release(int8* value, int8 new_value) noexcept { return __atomic_exchange_n(value, new_value, __ATOMIC_RELEASE); }
FORCE_INLINE int16 atomic_fetch_set_release(int16* value, int16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_RELEASE); }
FORCE_INLINE int32 atomic_fetch_set_release(int32* value, int32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_RELEASE); }
FORCE_INLINE int64 atomic_fetch_set_release(int64* value, int64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_RELEASE); }
FORCE_INLINE int8 atomic_get_release(const int8* value) noexcept { return __atomic_load_n((int8 *) value, __ATOMIC_RELEASE); }
FORCE_INLINE int16 atomic_get_release(const int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_load_n((int16 *) value, __ATOMIC_RELEASE); }
FORCE_INLINE int32 atomic_get_release(const int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_load_n((int32 *) value, __ATOMIC_RELEASE); }
FORCE_INLINE int64 atomic_get_release(const int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_load_n((int64 *) value, __ATOMIC_RELEASE); }
FORCE_INLINE int8 atomic_increment_release(int8* value) noexcept { return __atomic_add_fetch(value, 1, __ATOMIC_RELEASE); }
FORCE_INLINE int8 atomic_decrement_release(int8* value) noexcept { return __atomic_sub_fetch(value, 1, __ATOMIC_RELEASE); }
FORCE_INLINE int16 atomic_increment_release(int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_RELEASE); }
FORCE_INLINE int16 atomic_decrement_release(int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_RELEASE); }
FORCE_INLINE int32 atomic_increment_release(int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_RELEASE); }
FORCE_INLINE int32 atomic_decrement_release(int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_RELEASE); }
FORCE_INLINE int64 atomic_increment_release(int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_RELEASE); }
FORCE_INLINE int64 atomic_decrement_release(int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_add_release(int8* value, int8 increment) noexcept { __atomic_add_fetch(value, increment, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_sub_release(int8* value, int8 decrement) noexcept { __atomic_sub_fetch(value, decrement, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_add_release(int16* value, int16 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_add_fetch(value, increment, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_sub_release(int16* value, int16 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_add_release(int32* value, int32 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_add_fetch(value, increment, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_sub_release(int32* value, int32 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_add_release(int64* value, int64 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_add_fetch(value, increment, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_sub_release(int64* value, int64 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_RELEASE); }
FORCE_INLINE f32 atomic_compare_exchange_strong_release(f32* value, f32 expected, f32 desired) noexcept {
    ASSERT_STRICT(((uintptr_t) value % 4) == 0);

    _atomic_32* value_as_union = (_atomic_32*)value;
    _atomic_32* expected_as_union = (_atomic_32*)expected;
    _atomic_32 desired_as_union;
    desired_as_union.f = desired;

    __atomic_compare_exchange_n(
        &value_as_union->l, &expected_as_union->l, desired_as_union.l, 0,
        __ATOMIC_RELEASE, __ATOMIC_RELAXED
    );

    return expected_as_union->f;
}
FORCE_INLINE f64 atomic_compare_exchange_strong_release(f64* value, f64 expected, f64 desired) noexcept {
    ASSERT_STRICT(((uintptr_t) value % 8) == 0);

    _atomic_64* value_as_union = (_atomic_64*)value;
    _atomic_64* expected_as_union = (_atomic_64*)expected;
    _atomic_64 desired_as_union;
    desired_as_union.f = desired;

    __atomic_compare_exchange_n(
        &value_as_union->l, &expected_as_union->l, desired_as_union.l, 0,
        __ATOMIC_RELEASE, __ATOMIC_RELAXED
    );

    return expected_as_union->f;
}
FORCE_INLINE int32 atomic_compare_exchange_strong_release(int32* value, int32 expected, int32 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_compare_exchange_n(value, expected, &desired, 0, __ATOMIC_RELEASE, __ATOMIC_RELEASE); return *expected; }
FORCE_INLINE int64 atomic_compare_exchange_strong_release(int64* value, int64 expected, int64 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_compare_exchange_n(value, expected, &desired, 0, __ATOMIC_RELEASE, __ATOMIC_RELEASE); return *expected; }
FORCE_INLINE int8 atomic_fetch_add_release(int8* value, int8 operand) noexcept { return __atomic_add_fetch(value, operand, __ATOMIC_RELEASE); }
FORCE_INLINE int8 atomic_fetch_sub_release(int8* value, int8 operand) noexcept { return __atomic_sub_fetch(value, operand, __ATOMIC_RELEASE); }
FORCE_INLINE int16 atomic_fetch_add_release(int16* value, int16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_RELEASE); }
FORCE_INLINE int16 atomic_fetch_sub_release(int16* value, int16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_RELEASE); }
FORCE_INLINE int32 atomic_fetch_add_release(int32* value, int32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_RELEASE); }
FORCE_INLINE int32 atomic_fetch_sub_release(int32* value, int32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_RELEASE); }
FORCE_INLINE int64 atomic_fetch_add_release(int64* value, int64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_RELEASE); }
FORCE_INLINE int64 atomic_fetch_sub_release(int64* value, int64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_RELEASE); }
FORCE_INLINE int64 atomic_fetch_and_release(int64* value, uint64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_and_fetch(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE int64 atomic_fetch_or_release(int64* value, uint64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_or_fetch(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE uint64 atomic_fetch_and_release(uint64* value, uint64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_and_fetch(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE uint64 atomic_fetch_or_release(uint64* value, uint64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_or_fetch(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_set_release(uint8* value, uint8 new_value) noexcept { __atomic_store_n(value, new_value, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_set_release(uint16* value, uint16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_store_n(value, new_value, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_set_release(uint32* value, uint32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_store_n(value, new_value, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_set_release(uint64* value, uint64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_store_n(value, new_value, __ATOMIC_RELEASE); }
FORCE_INLINE uint8 atomic_fetch_set_release(uint8* value, uint8 new_value) noexcept { return __atomic_exchange_n(value, new_value, __ATOMIC_RELEASE); }
FORCE_INLINE uint16 atomic_fetch_set_release(uint16* value, uint16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_RELEASE); }
FORCE_INLINE uint32 atomic_fetch_set_release(uint32* value, uint32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_RELEASE); }
FORCE_INLINE uint64 atomic_fetch_set_release(uint64* value, uint64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_RELEASE); }
FORCE_INLINE uint8 atomic_get_release(const uint8* value) noexcept { return __atomic_load_n(value, __ATOMIC_RELEASE); }
FORCE_INLINE uint16 atomic_get_release(const uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_load_n(value, __ATOMIC_RELEASE); }
FORCE_INLINE uint32 atomic_get_release(const uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_load_n(value, __ATOMIC_RELEASE); }
FORCE_INLINE uint64 atomic_get_release(const uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_load_n(value, __ATOMIC_RELEASE); }
FORCE_INLINE uint8 atomic_increment_release(uint8* value) noexcept { return __atomic_add_fetch(value, 1, __ATOMIC_RELEASE); }
FORCE_INLINE uint8 atomic_decrement_release(uint8* value) noexcept { return __atomic_sub_fetch(value, 1, __ATOMIC_RELEASE); }
FORCE_INLINE uint16 atomic_increment_release(uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_RELEASE); }
FORCE_INLINE uint16 atomic_decrement_release(uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_RELEASE); }
FORCE_INLINE uint32 atomic_increment_release(uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_RELEASE); }
FORCE_INLINE uint32 atomic_decrement_release(uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_RELEASE); }
FORCE_INLINE uint64 atomic_increment_release(uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_RELEASE); }
FORCE_INLINE uint64 atomic_decrement_release(uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_add_release(uint8* value, uint8 increment) noexcept { __atomic_add_fetch(value, increment, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_sub_release(uint8* value, uint8 decrement) noexcept { __atomic_sub_fetch(value, decrement, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_add_release(uint16* value, uint16 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_add_fetch(value, increment, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_sub_release(uint16* value, uint16 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_add_release(uint32* value, uint32 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_add_fetch(value, increment, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_sub_release(uint32* value, uint32 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_add_release(uint64* value, uint64 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_add_fetch(value, increment, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_sub_release(uint64* value, uint64 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_RELEASE); }
// @bug Wrong implementation, see strong_acquire_release
FORCE_INLINE uint32 atomic_compare_exchange_strong_release(uint32* value, uint32 expected, uint32 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_compare_exchange_n(value, expected, &desired, 0, __ATOMIC_RELEASE, __ATOMIC_RELAXED); return *expected; }
FORCE_INLINE uint64 atomic_compare_exchange_strong_release(uint64* value, uint64 expected, uint64 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_compare_exchange_n(value, expected, &desired, 0, __ATOMIC_RELEASE, __ATOMIC_RELAXED); return *expected; }
FORCE_INLINE uint8 atomic_fetch_add_release(uint8* value, uint8 operand) noexcept { return __atomic_add_fetch(value, operand, __ATOMIC_RELEASE); }
FORCE_INLINE uint8 atomic_fetch_sub_release(uint8* value, uint8 operand) noexcept { return __atomic_sub_fetch(value, operand, __ATOMIC_RELEASE); }
FORCE_INLINE uint16 atomic_fetch_add_release(uint16* value, uint16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_RELEASE); }
FORCE_INLINE uint16 atomic_fetch_sub_release(uint16* value, uint16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_RELEASE); }
FORCE_INLINE uint32 atomic_fetch_add_release(uint32* value, uint32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_RELEASE); }
FORCE_INLINE uint32 atomic_fetch_sub_release(uint32* value, uint32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_RELEASE); }
FORCE_INLINE uint64 atomic_fetch_add_release(uint64* value, uint64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_RELEASE); }
FORCE_INLINE uint64 atomic_fetch_sub_release(uint64* value, uint64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_and_release(uint8* value, uint8 mask) noexcept { __atomic_fetch_and(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_and_release(int8* value, int8 mask) noexcept { __atomic_fetch_and(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_and_release(uint16* value, uint16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_fetch_and(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_and_release(int16* value, int16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_fetch_and(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_and_release(uint32* value, uint32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_fetch_and(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_and_release(int32* value, int32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_fetch_and(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_and_release(uint64* value, uint64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_fetch_and(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_and_release(int64* value, int64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_fetch_and(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_or_release(uint8* value, uint8 mask) noexcept { __atomic_fetch_or(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_or_release(int8* value, int8 mask) noexcept { __atomic_fetch_or(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_or_release(uint16* value, uint16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_fetch_or(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_or_release(int16* value, int16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_fetch_or(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_or_release(uint32* value, uint32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_fetch_or(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_or_release(int32* value, int32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_fetch_or(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_or_release(uint64* value, uint64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_fetch_or(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_or_release(int64* value, int64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_fetch_or(value, mask, __ATOMIC_RELEASE); }

FORCE_INLINE void atomic_set_acquire_release(void** target, void* value) noexcept { __atomic_store_n(target, value, __ATOMIC_SEQ_CST); }
FORCE_INLINE void* atomic_get_acquire_release(void** target) noexcept { return __atomic_load_n(target, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_set_acquire_release(int8* value, int8 new_value) noexcept { __atomic_store_n(value, new_value, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_set_acquire_release(int16* value, int16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_store_n(value, new_value, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_set_acquire_release(int32* value, int32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_store_n(value, new_value, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_set_acquire_release(int64* value, int64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_store_n(value, new_value, __ATOMIC_SEQ_CST); }
FORCE_INLINE int8 atomic_fetch_set_acquire_release(int8* value, int8 new_value) noexcept { return __atomic_exchange_n(value, new_value, __ATOMIC_SEQ_CST); }
FORCE_INLINE int16 atomic_fetch_set_acquire_release(int16* value, int16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_SEQ_CST); }
FORCE_INLINE int32 atomic_fetch_set_acquire_release(int32* value, int32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_SEQ_CST); }
FORCE_INLINE int64 atomic_fetch_set_acquire_release(int64* value, int64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_SEQ_CST); }
FORCE_INLINE int8 atomic_get_acquire_release(const int8* value) noexcept { return __atomic_load_n((int8 *) value, __ATOMIC_SEQ_CST); }
FORCE_INLINE int16 atomic_get_acquire_release(const int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_load_n((int16 *) value, __ATOMIC_SEQ_CST); }
FORCE_INLINE int32 atomic_get_acquire_release(const int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_load_n((int32 *) value, __ATOMIC_SEQ_CST); }
FORCE_INLINE int64 atomic_get_acquire_release(const int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_load_n((int64 *) value, __ATOMIC_SEQ_CST); }
FORCE_INLINE int8 atomic_increment_acquire_release(int8* value) noexcept { return __atomic_add_fetch(value, 1, __ATOMIC_SEQ_CST); }
FORCE_INLINE int8 atomic_decrement_acquire_release(int8* value) noexcept { return __atomic_sub_fetch(value, 1, __ATOMIC_SEQ_CST); }
FORCE_INLINE int16 atomic_increment_acquire_release(int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_SEQ_CST); }
FORCE_INLINE int16 atomic_decrement_acquire_release(int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_SEQ_CST); }
FORCE_INLINE int32 atomic_increment_acquire_release(int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_SEQ_CST); }
FORCE_INLINE int32 atomic_decrement_acquire_release(int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_SEQ_CST); }
FORCE_INLINE int64 atomic_increment_acquire_release(int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_SEQ_CST); }
FORCE_INLINE int64 atomic_decrement_acquire_release(int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_add_acquire_release(int8* value, int8 increment) noexcept { __atomic_add_fetch(value, increment, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_sub_acquire_release(int8* value, int8 decrement) noexcept { __atomic_sub_fetch(value, decrement, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_add_acquire_release(int16* value, int16 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_add_fetch(value, increment, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_sub_acquire_release(int16* value, int16 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_add_acquire_release(int32* value, int32 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_add_fetch(value, increment, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_sub_acquire_release(int32* value, int32 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_add_acquire_release(int64* value, int64 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_add_fetch(value, increment, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_sub_acquire_release(int64* value, int64 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_SEQ_CST); }
FORCE_INLINE f32 atomic_compare_exchange_strong_acquire_release(f32* value, f32 expected, f32 desired) noexcept {
    ASSERT_STRICT(((uintptr_t) value % 4) == 0);

    _atomic_32* value_as_union = (_atomic_32*)value;
    _atomic_32 expected_as_union = (_atomic_32)expected;
    _atomic_32 desired_as_union;
    desired_as_union.f = desired;

    __atomic_compare_exchange_n(
        &value_as_union->l, &expected_as_union.l, desired_as_union.l, 0,
        __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE
    );

    return expected_as_union.f;
}
FORCE_INLINE f64 atomic_compare_exchange_strong_acquire_release(f64* value, f64 expected, f64 desired) noexcept {
    ASSERT_STRICT(((uintptr_t) value % 8) == 0);

    _atomic_64* value_as_union = (_atomic_64*)value;
    _atomic_64 expected_as_union = (_atomic_64)expected;
    _atomic_64 desired_as_union;
    desired_as_union.f = desired;

    __atomic_compare_exchange_n(
        &value_as_union->l, &expected_as_union.l, desired_as_union.l, 0,
        __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE
    );

    return expected_as_union.f;
}
FORCE_INLINE int32 atomic_compare_exchange_strong_acquire_release(int32* value, int32 expected, int32 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_compare_exchange_n(value, &expected, &desired, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE); return expected; }
FORCE_INLINE int64 atomic_compare_exchange_strong_acquire_release(int64* value, int64 expected, int64 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_compare_exchange_n(value, &expected, &desired, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE); return expected; }
FORCE_INLINE int8 atomic_fetch_add_acquire_release(int8* value, int8 operand) noexcept { return __atomic_add_fetch(value, operand, __ATOMIC_SEQ_CST); }
FORCE_INLINE int8 atomic_fetch_sub_acquire_release(int8* value, int8 operand) noexcept { return __atomic_sub_fetch(value, operand, __ATOMIC_SEQ_CST); }
FORCE_INLINE int16 atomic_fetch_add_acquire_release(int16* value, int16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_SEQ_CST); }
FORCE_INLINE int16 atomic_fetch_sub_acquire_release(int16* value, int16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_SEQ_CST); }
FORCE_INLINE int32 atomic_fetch_add_acquire_release(int32* value, int32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_SEQ_CST); }
FORCE_INLINE int32 atomic_fetch_sub_acquire_release(int32* value, int32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_SEQ_CST); }
FORCE_INLINE int64 atomic_fetch_add_acquire_release(int64* value, int64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_SEQ_CST); }
FORCE_INLINE int64 atomic_fetch_sub_acquire_release(int64* value, int64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_set_acquire_release(uint8* value, uint8 new_value) noexcept { __atomic_store_n(value, new_value, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_set_acquire_release(uint16* value, uint16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_store_n(value, new_value, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_set_acquire_release(uint32* value, uint32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_store_n(value, new_value, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_set_acquire_release(uint64* value, uint64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_store_n(value, new_value, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint8 atomic_fetch_set_acquire_release(uint8* value, uint8 new_value) noexcept { return __atomic_exchange_n(value, new_value, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint16 atomic_fetch_set_acquire_release(uint16* value, uint16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint32 atomic_fetch_set_acquire_release(uint32* value, uint32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint64 atomic_fetch_set_acquire_release(uint64* value, uint64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint8 atomic_get_acquire_release(const uint8* value) noexcept { return __atomic_load_n(value, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint16 atomic_get_acquire_release(const uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_load_n(value, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint32 atomic_get_acquire_release(const uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_load_n(value, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint64 atomic_get_acquire_release(const uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_load_n(value, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint8 atomic_increment_acquire_release(uint8* value) noexcept { return __atomic_add_fetch(value, 1, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint8 atomic_decrement_acquire_release(uint8* value) noexcept { return __atomic_sub_fetch(value, 1, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint16 atomic_increment_acquire_release(uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint16 atomic_decrement_acquire_release(uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint32 atomic_increment_acquire_release(uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint32 atomic_decrement_acquire_release(uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint64 atomic_increment_acquire_release(uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint64 atomic_decrement_acquire_release(uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_add_acquire_release(uint8* value, uint8 increment) noexcept { __atomic_add_fetch(value, increment, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_sub_acquire_release(uint8* value, uint8 decrement) noexcept { __atomic_sub_fetch(value, decrement, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_add_acquire_release(uint16* value, uint16 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_add_fetch(value, increment, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_sub_acquire_release(uint16* value, uint16 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_add_acquire_release(uint32* value, uint32 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_add_fetch(value, increment, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_sub_acquire_release(uint32* value, uint32 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_add_acquire_release(uint64* value, uint64 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_add_fetch(value, increment, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_sub_acquire_release(uint64* value, uint64 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint32 atomic_compare_exchange_strong_acquire_release(uint32* value, uint32 expected, uint32 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_compare_exchange_n(value, &expected, &desired, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE); return expected; }
FORCE_INLINE uint64 atomic_compare_exchange_strong_acquire_release(uint64* value, uint64 expected, uint64 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_compare_exchange_n(value, &expected, &desired, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE); return expected; }
FORCE_INLINE uint8 atomic_fetch_add_acquire_release(uint8* value, uint8 operand) noexcept { return __atomic_add_fetch(value, operand, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint8 atomic_fetch_sub_acquire_release(uint8* value, uint8 operand) noexcept { return __atomic_sub_fetch(value, operand, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint16 atomic_fetch_add_acquire_release(uint16* value, uint16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint16 atomic_fetch_sub_acquire_release(uint16* value, uint16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint32 atomic_fetch_add_acquire_release(uint32* value, uint32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint32 atomic_fetch_sub_acquire_release(uint32* value, uint32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint64 atomic_fetch_add_acquire_release(uint64* value, uint64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint64 atomic_fetch_sub_acquire_release(uint64* value, uint64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_and_acquire_release(uint8* value, uint8 mask) noexcept { __atomic_fetch_and(value, mask, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_and_acquire_release(int8* value, int8 mask) noexcept { __atomic_fetch_and(value, mask, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_and_acquire_release(uint16* value, uint16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_fetch_and(value, mask, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_and_acquire_release(int16* value, int16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_fetch_and(value, mask, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_and_acquire_release(uint32* value, uint32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_fetch_and(value, mask, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_and_acquire_release(int32* value, int32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_fetch_and(value, mask, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_and_acquire_release(uint64* value, uint64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_fetch_and(value, mask, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_and_acquire_release(int64* value, int64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_fetch_and(value, mask, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_or_acquire_release(uint8* value, uint8 mask) noexcept { __atomic_fetch_or(value, mask, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_or_acquire_release(int8* value, int8 mask) noexcept { __atomic_fetch_or(value, mask, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_or_acquire_release(uint16* value, uint16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_fetch_or(value, mask, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_or_acquire_release(int16* value, int16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_fetch_or(value, mask, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_or_acquire_release(uint32* value, uint32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_fetch_or(value, mask, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_or_acquire_release(int32* value, int32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_fetch_or(value, mask, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_or_acquire_release(uint64* value, uint64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_fetch_or(value, mask, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_or_acquire_release(int64* value, int64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_fetch_or(value, mask, __ATOMIC_SEQ_CST); }

// Check out the intrinsic functions fence_memory and fence_write
// These are much faster and could accomplish what you are doing
#define atomic_fence_acquire() __atomic_thread_fence(__ATOMIC_ACQUIRE)

// Check out the intrinsic functions fence_memory and fence_write
// These are much faster and could accomplish what you are doing
#define atomic_fence_release() __atomic_thread_fence(__ATOMIC_RELEASE)

#endif