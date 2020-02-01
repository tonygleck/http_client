// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stddef.h>

#include "lib-util-c/sys_debug_shim.h"
#include "lib-util-c/app_logging.h"
#include "lib-util-c/item_list.h"
#include "lib-util-c/crt_extensions.h"

#include "http_client/http_headers.h"

typedef struct HTTP_HEADERS_INFO_TAG
{
    ITEM_LIST_HANDLE list_items;
} HTTP_HEADERS_INFO;

typedef struct NAME_VALUE_PAIR_TAG
{
    char* name;
    char* value;
} NAME_VALUE_PAIR;

static void destroy_list_item(void* user_ctx, void* remove_item)
{
    (void)user_ctx;
    NAME_VALUE_PAIR* nvp = (NAME_VALUE_PAIR*)remove_item;
    free(nvp->name);
    free(nvp->value);
    free(nvp);
}

HTTP_HEADERS_HANDLE http_header_create(void)
{
    HTTP_HEADERS_INFO* result;
    if ((result = (HTTP_HEADERS_INFO*)malloc(sizeof(HTTP_HEADERS_INFO))) == NULL)
    {
        log_error("Failure allocating http header");
    }
    else if ((result->list_items = item_list_create(destroy_list_item, result)) == NULL)
    {
        log_error("Failure creating http header list");
        free(result);
        result = NULL;
    }
    return result;
}

void http_header_destroy(HTTP_HEADERS_HANDLE handle)
{
    if (handle != NULL)
    {
        item_list_destroy(handle->list_items);
        free(handle);
    }
}

int http_header_add(HTTP_HEADERS_HANDLE handle, const char* name, const char* value)
{
    int result;
    if (handle == NULL || name == NULL || value == NULL)
    {
            log_error("Failure invalid parameter specified handle: %p, name: %p, value: %p", handle, name, value);
            result = __LINE__;
    }
    else
    {
        NAME_VALUE_PAIR* nvp = (NAME_VALUE_PAIR*)malloc(sizeof(NAME_VALUE_PAIR));
        if (nvp == NULL)
        {
            log_error("Failure invalid parameter specified nvp: %p", nvp);
            result = __LINE__;
        }
        // todo validate header name
        else if (clone_string(&nvp->name, name) != 0)
        {
            free(nvp);
            log_error("Failure allocating name value");
            result = __LINE__;
        }
        else if (clone_string(&nvp->value, value) != 0)
        {
            free(nvp->name);
            free(nvp);
            log_error("Failure allocating name value");
            result = __LINE__;
        }
        else if (item_list_add_item(handle->list_items, nvp) )
        {
            free(nvp->name);
            free(nvp->value);
            free(nvp);
            log_error("Failure allocating name value");
            result = __LINE__;
        }
        else
        {
            result = 0;
        }
    }
    return result;
}

int http_header_add_partial(HTTP_HEADERS_HANDLE handle, const char* name, size_t name_len, const char* value, size_t value_len)
{
    int result;
    if (handle == NULL || name == NULL || name_len == 0 || value == NULL || value_len == 0)
    {
            log_error("Failure invalid parameter specified handle: %p, name: %p, value: %p", handle, name, value);
            result = __LINE__;
    }
    else
    {
        NAME_VALUE_PAIR* nvp = (NAME_VALUE_PAIR*)malloc(sizeof(NAME_VALUE_PAIR));
        if (nvp == NULL)
        {
            log_error("Failure invalid parameter specified nvp: %p", nvp);
            result = __LINE__;
        }
        else if (clone_string_with_size(&nvp->name, name, name_len) != 0)
        {
            free(nvp);
            log_error("Failure allocating name value");
            result = __LINE__;
        }
        else if (clone_string_with_size(&nvp->value, value, value_len) != 0)
        {
            free(nvp->name);
            free(nvp);
            log_error("Failure allocating name value");
            result = __LINE__;
        }
        else if (item_list_add_item(handle->list_items, nvp) )
        {
            free(nvp->name);
            free(nvp->value);
            free(nvp);
            log_error("Failure allocating name value");
            result = __LINE__;
        }
        else
        {
            result = 0;
        }
    }
    return result;
}

int http_header_remove(HTTP_HEADERS_HANDLE handle, const char* name)
{
    int result;
    if (handle == NULL || name == NULL)
    {
        log_error("Invalid parameter specified handle: %p, name: %p", handle, name);
        result = __LINE__;
    }
    else
    {
        result = 0;
        const NAME_VALUE_PAIR* nvp;
        size_t list_count = item_list_item_count(handle->list_items);
        for (size_t index = 0; index < list_count; index++)
        {
            nvp = item_list_get_item(handle->list_items, index);
            if (strcmp(nvp->name, name) == 0)
            {
                item_list_remove_item(handle->list_items, index);
                break;
            }
        }
    }
    return result;
}

const char* http_header_get_value(HTTP_HEADERS_HANDLE handle, const char* name)
{
    const char* result;
    if (handle == NULL || name == NULL)
    {
        log_error("Invalid parameter specified handle: %p, name: %p", handle, name);
        result = NULL;
    }
    else
    {
        result = NULL;
        const NAME_VALUE_PAIR* nvp;
        size_t list_count = item_list_item_count(handle->list_items);
        for (size_t index = 0; index < list_count; index++)
        {
            if ((nvp = item_list_get_item(handle->list_items, index)) != NULL)
            {
                if (strcmp(nvp->name, name) == 0)
                {
                    result = nvp->value;
                    break;
                }
            }
        }
    }
    return result;
}

size_t http_header_get_count(HTTP_HEADERS_HANDLE handle)
{
    size_t result;
    if (handle == NULL)
    {
        log_error("Invalid parameter specified handle NULL");
        result = 0;
    }
    else
    {
        result = item_list_item_count(handle->list_items);
    }
    return result;
}

int http_header_get_name_value_pair(HTTP_HEADERS_HANDLE handle, size_t index, const char** name, const char** value)
{
    int result;
    if (handle == NULL || name == NULL || value == NULL)
    {
        log_error("Invalid parameter specified handle: %p, name: %p, value %p", handle, name, value);
        result = __LINE__;
    }
    else
    {
        const NAME_VALUE_PAIR* nvp;
        nvp = item_list_get_item(handle->list_items, index);
        if (nvp != NULL)
        {
            *name = nvp->name;
            *value = nvp->value;
            result = 0;
        }
        else
        {
            log_error("Failued getting list item");
            result = __LINE__;
        }
    }
    return result;
}

int http_header_clear(HTTP_HEADERS_HANDLE handle)
{
    int result;
    if (handle == NULL)
    {
        log_error("Invalid parameter specified handle NULL");
        result = __LINE__;
    }
    else
    {
        result = item_list_clear(handle->list_items);
    }
    return result;
}
