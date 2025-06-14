/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_DATABASE_QUERY_BUILDER_H
#define COMS_DATABASE_QUERY_BUILDER_H

#include "../../stdlib/Types.h"
#include "query/grammar/DbQueryGrammar.h"

struct DbQueryValue {
    char column_name[64];
    DbQueryBuilder query;
};

struct DbJoin {
    DbQueryValue join;

    uint8 on_length;
    DbJoinOn* ons;
};

struct DbJoinOn {
    uint8 boolean;
    DbQueryValue column;
    DbOperator operator;
    DbQueryValue value;
};

struct DbWhere {
    uint8 boolean;
    DbQueryValue column;
    DbOperator operator;
    DbQueryValue value;
};

struct DbQueryBuilder {
    // 1 - log
    // 2 - read only
    // 3 - use prepared stmt
    uint8 flags;

    DbQueryType type;

    DbQueryGrammar grammar;

    // Used for select, update, delete, insert, ...
    uint8 value_length;
    DbQueryValue* columns;

    // Used for into and from
    const char table[24];

    // Used for values in insert/update etc.
    uint8 value_length;
    DbQueryValue* values;

    uint8 join_length;
    DbJoin* joins;

    uint8 where_length;
    DbWhere* wheres;

    uint8 group_length;
    DbGroup* groups;

    uint8 order_length;
    DbGroup* orders;

    uint64 limit;
    uint64 offset;
};

#endif