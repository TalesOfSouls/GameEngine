/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_INPUT_MODE_H
#define COMS_INPUT_MODE_H

#include "../stdlib/Stdlib.h"

enum InputMode : byte {
    INPUT_MODE_EVENT,
    INPUT_MODE_POLLING,
};

#endif