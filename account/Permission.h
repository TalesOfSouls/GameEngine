/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_ACCOUNT_PERMISSION_H
#define COMS_ACCOUNT_PERMISSION_H

#include "../stdlib/Types.h"

struct Permission {
    uint32 id;
    uint32 uint;
    uint32 app;
    uint16 module;

    // Providing module
    uint16 from;

    // Used by the module to internally handle permssions for different areas
    // e.g. In the news module one category could be news to specify the permissions for news, or news category, ...
    uint16 category;

    // Specific element
    uint32 element;

    // Component of an element (e.g. only allowed to change content but not title of news)
    uint32 component;

    /**
     * 0x00000001 = read permission
     * 0x00000010 = update permission
     * 0x00000100 = create permission
     * 0x00001000 = delete permission
     * 0x00010000 = can change permission
     */
    byte permission_flag;

    // @question Is this defaultCPermissions used?
};

#endif