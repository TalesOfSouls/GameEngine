/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_ACCOUNT_PERMISSION_TYPE_H
#define COMS_ACCOUNT_PERMISSION_TYPE_H

#include "../stdlib/Types.h"

enum PermissionType {
    PERMISSION_TYPE_NONE = 0,
    PERMISSION_TYPE_READ = 1 << 0,
    PERMISSION_TYPE_UPDATE = 1 << 1,
    PERMISSION_TYPE_CREATE = 1 << 2,
    PERMISSION_TYPE_DELETE = 1 << 3,
    PERMISSION_TYPE_PERMISSION = 1 << 4,
};

#endif