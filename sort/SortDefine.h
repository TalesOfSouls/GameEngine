#ifndef COMS_SORT_DEFINE_H
#define COMS_SORT_DEFINE_H

#include "../stdlib/Types.h"

typedef int32 (*SortCompareFunc)(const void* __restrict a, const void* __restrict b) NO_EXCEPT;

#endif