#ifndef COMS_UI_LANGUAGE_H
#define COMS_UI_LANGUAGE_H

#include "../stdlib/Stdlib.h"

#define LANGUAGE_VERSION 1

// Size is limited to 4GB
struct Language {
    // WARNING: the actual start of data is data -= sizeof(Language); see file loading below
    // The reason for this is we store the Language struct in the beginning of the data/file
    byte* data;

    int32 count;
    int32 size;
    char** lang;
};

#endif