#pragma once
#ifndef COMS_UI_TABLE_H
#define COMS_UI_TABLE_H

#include "../stdlib/Stdlib.h"
#include "attribute/UIAttributeDimension.h"

struct UITableHeadElement {

};

struct UITableHead {
    UICore core;

    int16 column_count;

    // For the head we optimize the default case where the title is only simple text elements
    int16 column_length;
    int32 column_start;


};

struct UITableBody {
    UICore core;
};

struct UITableFoot {
    UICore core;

    int16 column_count;
    int16 column_length;
    int32 column_start;
};

struct UITable {
    UICore core;
    UITableHead head;
    UITableBody body;
    UITableFoot foot;
};

#endif