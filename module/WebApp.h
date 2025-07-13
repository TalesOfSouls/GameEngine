#ifndef COMS_MODULE_WEB_APP_H
#define COMS_MODULE_WEB_APP_H

#include "../system/Library.cpp"
#include "../log/DebugContainer.h"
#include "../http/HttpRequest.h"
#include "../http/HttpResponse.h"
#include "../network/SocketConnection.h"

struct ApplicationData;

typedef void dll_app_load(DebugContainer*);

// INFO: Requires pointer to pointer since we may want to resize the request or response object
//      The response object is intuitive but even the response object might need this if we are parsing a file upload
typedef void dll_app_handle_request(void*, SocketConnection*, HttpRequest**, HttpResponse**);

struct WebApp {
    dll_app_load* app_load;
    dll_app_handle_request* app_handle_request;

    // @todo It's a little bit nasty, that we have this thing here
    //      Remember lib is now back referencing to *codetable
    //      It would probably better to have a library manager that holds all libs
    Library lib;
};

// null || '' = must not be set
// * any
// actual name
struct WebAppMatchPattern {
    char subdomain[16];
    char domain[24];
    char path[24];

    int32 app_id;
};

const char* web_app_function_name_table[] = {
    "app_load",
    "app_handle_request",
};

#endif