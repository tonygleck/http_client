// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef HTTP_CODEC_H
#define HTTP_CODEC_H

#ifdef __cplusplus
extern "C" {
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"
#include "patchcords/patchcord_client.h"
#include "http_client/http_headers.h"

typedef struct HTTP_CODEC_INFO_TAG* HTTP_CODEC_HANDLE;

#define HTTP_CODEC_CB_RESULT_VALUES     \
    HTTP_CODEC_CB_RESULT_OK,            \
    HTTP_CODEC_CB_RESULT_ERROR

MU_DEFINE_ENUM(HTTP_CODEC_CB_RESULT, HTTP_CODEC_CB_RESULT_VALUES);

typedef struct HTTP_RECV_DATA_TAG
{
    HTTP_HEADERS_HANDLE recv_header;
    BYTE_BUFFER http_content;
    uint32_t status_code;
} HTTP_RECV_DATA;

typedef void(*ON_HTTP_DATA_CALLBACK)(void* callback_ctx, HTTP_CODEC_CB_RESULT result, const HTTP_RECV_DATA* http_recv_data);

MOCKABLE_FUNCTION(, HTTP_CODEC_HANDLE, http_codec_create, ON_HTTP_DATA_CALLBACK, data_callback, void*, user_ctx);
MOCKABLE_FUNCTION(, void, http_codec_destroy, HTTP_CODEC_HANDLE, handle);

MOCKABLE_FUNCTION(, int, http_codec_reintialize, HTTP_CODEC_HANDLE, handle);

MOCKABLE_FUNCTION(, ON_BYTES_RECEIVED, http_codec_get_recv_function);

MOCKABLE_FUNCTION(, int, http_codec_set_trace, HTTP_CODEC_HANDLE, handle, bool, set_trace);


#endif // HTTP_CODEC_H
