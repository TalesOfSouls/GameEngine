/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_HTML_DISPATCHER_H
#define COMS_HTML_DISPATCHER_H

#include "../stdlib/Types.h"
#include "../memory/BufferMemory.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "../module/WebModule.h"

typedef void* (*ControllerFunction)(WebModule* module, HttpRequest* request, HttpResponse* response, void* data);

struct DispatchData {
    WebModule* module;
    HttpRequest* request;
    HttpResponse* response;
    void* data;
};

struct Dispatcher {
    void* app;

    // This is an array of arrays to make looking up functions faster
    // The first index defines the module id, the second level defines the function
    //      functions[module_id][function_id]
    // The 0 index is used for custom functions
    // The other indeces are filled by the WebModuleManager
    ControllerFunction** functions;
    int16 category_count;
    int16 base_func_count;
};

void dispatcher_alloc(Dispatcher* dispatcher, void* app, int32 category_count, int32 func_count, BufferMemory* buf, int32 alignment = 64) {
    dispatcher->app = app;
    dispatcher->base_func_count = func_count;
    dispatcher->functions = (ControllerFunction **) buffer_get_memory(buf, sizeof(ControllerFunction *) * category_count, alignment, true);
    dispatcher->functions[0] = (ControllerFunction *) buffer_get_memory(buf, sizeof(ControllerFunction) * func_count, alignment, true);
}

void dispatcher_set_func(Dispatcher* dispatcher, uint32 id, ControllerFunction func) {
    /**
     * The 16 high bits of function_id define the module id
     * and the lower 16 bits define the relative function id in that module.
     */
    uint16 category_id = id >> 16;
    if (category_id >= dispatcher->category_count) {
        return;
    }

    uint16 function_id = id & 0x0000FFFF;
    if (category_id == 0 && function_id >= dispatcher->base_func_count) {
        return;
    }

    dispatcher->functions[category_id][function_id] = func;
}

void dispatcher_set_category(Dispatcher* dispatcher, uint16 category, ControllerFunction* functions) {
    if (category >= dispatcher->category_count) {
        return;
    }

    dispatcher->functions[category] = functions;
}

ControllerFunction dispatcher_get_function(Dispatcher* dispatcher, uint32 id) {
    uint16 category_id = id >> 16;
    if (category_id >= dispatcher->category_count) {
        return NULL;
    }

    uint16 function_id = id & 0x0000FFFF;
    if (category_id == 0 && function_id >= dispatcher->base_func_count) {
        return NULL;
    }

    return dispatcher->functions[category_id][function_id];
}

void* dispatcher_dispatch(Dispatcher* dispatcher, uint32 id, DispatchData* dispatch_data) {
    ControllerFunction func = dispatcher_get_function(dispatcher, id);

    if (func) {
        return func(dispatch_data->module, dispatch_data->request, dispatch_data->response, dispatch_data->data);
    }

    return NULL;
}

#endif