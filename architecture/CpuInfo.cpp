/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_ARCHITECTURE_CPU_INFO_C
#define COMS_ARCHITECTURE_CPU_INFO_C

#include "CpuInfo.h"

#ifdef __aarch64__
    #include "arm/CpuInfo.cpp"
#else
    #include "x86/CpuInfo.cpp"
#endif

#endif