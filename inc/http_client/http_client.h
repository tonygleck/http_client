// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#ifdef __cplusplus
extern "C" {
#include <cstddef>
#include <cstdint>
#else
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#endif /* __cplusplus */

#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"
#include "patchcords/xio_client.h"
#include "http_client/http_headers.h"

#define HTTP_CLIENT_RESULT_VALUES       \
    HTTP_CLIENT_OK,                     \
    HTTP_CLIENT_INVALID_ARG,            \
    HTTP_CLIENT_ERROR,                  \
    HTTP_CLIENT_OPEN_FAILED,            \
    HTTP_CLIENT_SEND_FAILED,            \
    HTTP_CLIENT_ALREADY_INIT,           \
    HTTP_CLIENT_HTTP_HEADERS_FAILED,    \
    HTTP_CLIENT_INVALID_STATE,          \
    HTTP_CLIENT_DISCONNECTION,          \
    HTTP_CLIENT_MEMORY

MU_DEFINE_ENUM(HTTP_CLIENT_RESULT, HTTP_CLIENT_RESULT_VALUES);

#define HTTP_CLIENT_REQUEST_TYPE_VALUES \
    HTTP_CLIENT_REQUEST_OPTIONS,        \
    HTTP_CLIENT_REQUEST_GET,            \
    HTTP_CLIENT_REQUEST_POST,           \
    HTTP_CLIENT_REQUEST_PUT,            \
    HTTP_CLIENT_REQUEST_DELETE,         \
    HTTP_CLIENT_REQUEST_PATCH

MU_DEFINE_ENUM(HTTP_CLIENT_REQUEST_TYPE, HTTP_CLIENT_REQUEST_TYPE_VALUES);

typedef struct HTTP_CLIENT_INFO_TAG* HTTP_CLIENT_HANDLE;

typedef void(*ON_HTTP_OPEN_COMPLETE_CALLBACK)(void* callback_ctx, HTTP_CLIENT_RESULT open_result);
typedef void(*ON_HTTP_ERROR_CALLBACK)(void* callback_ctx, HTTP_CLIENT_RESULT error_result);
typedef void(*ON_HTTP_REQUEST_CALLBACK)(void* callback_ctx, HTTP_CLIENT_RESULT request_result, const unsigned char* content, size_t content_length, unsigned int status_code,
    HTTP_HEADERS_HANDLE response_headers);
typedef void(*ON_HTTP_CLIENT_CLOSE)(void* callback_context);

MOCKABLE_FUNCTION(, HTTP_CLIENT_HANDLE, http_client_create);
MOCKABLE_FUNCTION(, void, http_client_destroy, HTTP_CLIENT_HANDLE, handle);

MOCKABLE_FUNCTION(, int, http_client_open, HTTP_CLIENT_HANDLE, handle, XIO_INSTANCE_HANDLE, xio_handle, ON_HTTP_OPEN_COMPLETE_CALLBACK, on_open_complete_cb, void*, user_ctx, ON_HTTP_ERROR_CALLBACK, on_error_cb, void*, err_user_ctx);
MOCKABLE_FUNCTION(, int, http_client_close, HTTP_CLIENT_HANDLE, handle, ON_HTTP_CLIENT_CLOSE, http_close_cb, void*, user_ctx);

MOCKABLE_FUNCTION(, int, http_client_execute_request, HTTP_CLIENT_HANDLE, handle, HTTP_CLIENT_REQUEST_TYPE, request_type, const char*, relative_path,
    HTTP_HEADERS_HANDLE, http_header, const unsigned char*, content, size_t, content_length, ON_HTTP_REQUEST_CALLBACK, on_request_callback, void*, callback_ctx);

MOCKABLE_FUNCTION(, void, http_client_process_item, HTTP_CLIENT_HANDLE, handle);

MOCKABLE_FUNCTION(, int, http_client_set_trace, HTTP_CLIENT_HANDLE, handle, bool, set_trace);

#endif // HTTP_CLIENT_H