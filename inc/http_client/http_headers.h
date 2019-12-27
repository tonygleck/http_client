// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef HTTP_HEADER_H
#define HTTP_HEADER_H

#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"

typedef struct HTTP_HEADERS_INFO_TAG* HTTP_HEADERS_HANDLE;

MOCKABLE_FUNCTION(, HTTP_HEADERS_HANDLE, http_header_create);
MOCKABLE_FUNCTION(, void, http_header_destroy, HTTP_HEADERS_HANDLE, handle);

MOCKABLE_FUNCTION(, int, http_header_add, HTTP_HEADERS_HANDLE, handle, const char*, name, const char*, value);
MOCKABLE_FUNCTION(, int, http_header_add_partial, HTTP_HEADERS_HANDLE, handle, const char*, name, size_t, name_len, const char*, value, size_t, value_len);
MOCKABLE_FUNCTION(, int, http_header_remove, HTTP_HEADERS_HANDLE, handle, const char*, name);

MOCKABLE_FUNCTION(, const char*, http_header_get_value, HTTP_HEADERS_HANDLE, handle, const char*, name);

MOCKABLE_FUNCTION(, size_t, http_header_get_count, HTTP_HEADERS_HANDLE, handle);
MOCKABLE_FUNCTION(, int, http_header_get_name_value_pair, HTTP_HEADERS_HANDLE, handle, size_t, index, const char**, name, const char**, value);

MOCKABLE_FUNCTION(, int, http_header_clear, HTTP_HEADERS_HANDLE, handle);

#endif // HTTP_HEADER_H
