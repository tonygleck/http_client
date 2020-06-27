// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "lib-util-c/sys_debug_shim.h"
#include "lib-util-c/app_logging.h"
#include "lib-util-c/item_list.h"
#include "lib-util-c/crt_extensions.h"
#include "lib-util-c/buffer_alloc.h"

#include "patchcords/patchcord_client.h"
#include "patchcords/cord_socket_client.h"

#include "http_client/http_client.h"
#include "http_client/http_headers.h"
#include "http_client/http_codec.h"

static const char* HTTP_HOST = "Host";
static const char* HTTP_CONTENT_LEN = "content-length";
static const char* HTTP_CRLF_VALUE = "\r\n";
static const char* HTTP_REQUEST_LINE_FMT = "%s %s HTTP/1.1\r\n%s";

typedef enum HTTP_CLIENT_STATE_TAG
{
    CLIENT_STATE_NOT_CONN,
    CLIENT_STATE_OPENING,
    CLIENT_STATE_OPENED,
    CLIENT_STATE_OPEN,
    CLIENT_STATE_CLOSING,
    CLIENT_STATE_CLOSED,
    CLIENT_STATE_ERROR
} HTTP_CLIENT_STATE;

typedef struct HTTP_CLIENT_INFO_TAG
{
    PATCH_INSTANCE_HANDLE xio_handle;
    HTTP_CODEC_HANDLE codec_handle;

    HTTP_HEADERS_HANDLE headers;
    ITEM_LIST_HANDLE request_list;
    ITEM_LIST_HANDLE recv_callback_list;

    HTTP_CLIENT_STATE state;
    HTTP_CLIENT_RESULT curr_result;

    ON_HTTP_ERROR_CALLBACK on_error_cb;
    void* err_user_ctx;
    ON_HTTP_OPEN_COMPLETE_CALLBACK on_open_complete_cb;
    void* open_complete_ctx;
    ON_HTTP_CLIENT_CLOSE on_close_cb;
    void* close_user_ctx;

//    HTTP_RECV_DATA recv_data;
    bool logging_enabled;
    uint16_t port;

} HTTP_CLIENT_INFO;

typedef struct HTTP_REQUEST_INFO_TAG
{
    HTTP_CLIENT_INFO* client_info;
    ON_HTTP_REQUEST_CALLBACK on_request_cb;
    void* on_request_ctx;

    HTTP_CLIENT_REQUEST_TYPE request_type;
    char* relative_path;
    STRING_BUFFER header_line;
    BYTE_BUFFER payload;
} HTTP_REQUEST_INFO;

typedef struct HTTP_RESP_INFO_TAG
{
    ON_HTTP_REQUEST_CALLBACK on_request_cb;
    void* on_request_ctx;
} HTTP_RESP_INFO;

static int construct_header_line(HTTP_REQUEST_INFO* request_info, HTTP_HEADERS_HANDLE http_header, size_t content_len, const char* hostname, uint16_t port)
{
    int result = 0;
    bool add_hostname = true;
    size_t header_cnt = http_header_get_count(http_header);
    for (size_t index = 0; index < header_cnt; index++)
    {
        const char* name;
        const char* value;
        if (http_header_get_name_value_pair(http_header, index, &name, &value) != 0)
        {
            log_error("Failure allocating buffer value");
            result = __LINE__;
        }
        else
        {
            if (strcmp(name, HTTP_HOST) == 0)
            {
                add_hostname = false;
            }
            if (string_buffer_construct_sprintf(&request_info->header_line, "%s: %s\r\n", name, value) != 0)
            {
                log_error("Failure allocating buffer value");
                result = __LINE__;
            }
        }
    }
    if (result == 0 && add_hostname)
    {
        // Add the hostname header
        if (string_buffer_construct_sprintf(&request_info->header_line, "%s: %s:%d\r\n", HTTP_HOST, hostname, port) != 0)
        {
            free(request_info->header_line.payload);
            log_error("Failure allocating host line");
            result = __LINE__;
        }
    }
    if (result == 0)
    {
        // Add content length
        if (string_buffer_construct_sprintf(&request_info->header_line, "%s: %u%s%s", HTTP_CONTENT_LEN, (unsigned int)content_len, HTTP_CRLF_VALUE, HTTP_CRLF_VALUE) != 0)
        {
            free(request_info->header_line.payload);
            log_error("Failure allocating host line");
            result = __LINE__;
        }
    }
    return result;
}

static void on_open_complete(void* context, IO_OPEN_RESULT open_result)
{
    if (context != NULL)
    {
        HTTP_CLIENT_INFO* client_info = (HTTP_CLIENT_INFO*)context;
        if (open_result == IO_OPEN_OK)
        {
            client_info->state = CLIENT_STATE_OPENED;
        }
        else
        {
            client_info->state = CLIENT_STATE_ERROR;
            client_info->curr_result = HTTP_CLIENT_OPEN_FAILED;
            log_error("Failure opening http client: %d", open_result);
        }
    }
}

static void on_close_complete(void* context)
{
    if (context != NULL)
    {
        HTTP_CLIENT_INFO* client_info = (HTTP_CLIENT_INFO*)context;
        client_info->state = CLIENT_STATE_CLOSED;
    }
    else
    {
        log_error("Failure context NULL on closed complete");
    }
}

static void on_error(void* context, IO_ERROR_RESULT error_result)
{
    if (context != NULL)
    {
        HTTP_CLIENT_INFO* client_info = (HTTP_CLIENT_INFO*)context;
        if (client_info->on_error_cb != NULL)
        {
            HTTP_CLIENT_RESULT http_error;
            switch (error_result)
            {
                case IO_ERROR_MEMORY:
                    http_error = HTTP_CLIENT_MEMORY;
                    break;
                case IO_ERROR_SERVER_DISCONN:
                    http_error = HTTP_CLIENT_DISCONNECTION;
                    break;
                default:
                    http_error = HTTP_CLIENT_ERROR;
                    break;
            }
            client_info->on_error_cb(client_info->err_user_ctx, http_error);
        }
    }
    else
    {
        log_error("Failure context NULL on on_error");
    }
}

static void on_send_complete(void* context, IO_SEND_RESULT send_result)
{
    if (context != NULL)
    {
        // Only report failures
        if (send_result != IO_SEND_OK)
        {
            HTTP_CLIENT_INFO* client_info = (HTTP_CLIENT_INFO*)context;
            client_info->state = CLIENT_STATE_ERROR;
            client_info->curr_result = HTTP_CLIENT_SEND_FAILED;
            log_error("Failure sending request");
        }
    }
    else
    {
        log_error("Failure context NULL on send complete");
    }
}

static int construct_http_data(const HTTP_REQUEST_INFO* request_info, STRING_BUFFER* http_req_line)
{
    int result = 0;
    const char* method;
    switch (request_info->request_type)
    {
        case HTTP_CLIENT_REQUEST_OPTIONS:
            method = "OPTION";
            break;
        case HTTP_CLIENT_REQUEST_GET:
            method = "GET";
            break;
        case HTTP_CLIENT_REQUEST_POST:
            method = "POST";
            break;
        case HTTP_CLIENT_REQUEST_PUT:
            method = "PUT";
            break;
        case HTTP_CLIENT_REQUEST_DELETE:
            method = "DELETE";
            break;
        case HTTP_CLIENT_REQUEST_PATCH:
            method = "PATCH";
            break;
        case HTTP_CLIENT_REQUEST_TYPE_INVALID:
        default:
            result = __LINE__;
            method = "";
            break;
    }
    if (result == 0)
    {
        if (string_buffer_construct_sprintf(http_req_line, HTTP_REQUEST_LINE_FMT, method, request_info->relative_path, request_info->header_line.payload) != 0)
        {
            log_error("Failure constructing request line");
            result = __LINE__;
        }
        else
        {
            result = 0;
        }
    }
    return result;
}

static int write_http_text(HTTP_CLIENT_INFO* client_info, const char* text)
{
    int result;

    if (text == NULL)
    {
        log_warning("writing Text is NULL");
    }

    if (patchcord_client_send(client_info->xio_handle, text, strlen(text), on_send_complete, client_info) != 0)
    {
        log_error("Failure sending client data");
        result = __LINE__;
    }
    else
    {
        if (client_info->logging_enabled)
        {
            log_trace("==> %s", text);
        }
        result = 0;
    }
    return result;
}

static int send_http_request(HTTP_CLIENT_INFO* client_info, const HTTP_REQUEST_INFO* request_info)
{
    int result;

    STRING_BUFFER request_line = {0};
    if (construct_http_data(request_info, &request_line) != 0)
    {
        log_error("Failure sending client data");
        result = __LINE__;
    }
    else
    {
        // Send the header information
        if (write_http_text(client_info, request_line.payload) != 0)
        {
            log_error("Failure sending client data");
            result = __LINE__;
        }
        // Send the Data
        else if (request_info->payload.payload_size > 0 && patchcord_client_send(client_info->xio_handle, request_info->payload.payload, request_info->payload.payload_size, on_send_complete, client_info) != 0)
        {
            log_error("Failure sending client data");
            result = __LINE__;
        }
        else
        {
            // write trace line
            if (client_info->logging_enabled && request_info->payload.payload_size > 0)
            {
                log_trace("==> %s", request_info->payload.payload);
            }
            result = 0;
        }
        free(request_line.payload);
    }
    return result;
}

static void on_codec_recv_callback(void* context, HTTP_CODEC_CB_RESULT result, const HTTP_RECV_DATA* http_recv_data)
{
    HTTP_CLIENT_INFO* client_info = (HTTP_CLIENT_INFO*)context;
    if (client_info == NULL)
    {
        log_error("Failure invalid user context in codec recieve callback");
    }
    else
    {
        HTTP_RESP_INFO* resp_info = (HTTP_RESP_INFO*)item_list_get_front(client_info->recv_callback_list);
        if (resp_info != NULL)
        {
            HTTP_CLIENT_RESULT request_res = HTTP_CLIENT_OK;
            if (result != HTTP_CODEC_CB_RESULT_OK)
            {
                request_res = HTTP_CLIENT_ERROR;
            }
            resp_info->on_request_cb(resp_info->on_request_ctx, request_res, http_recv_data->http_content.payload, http_recv_data->http_content.payload_size,
                http_recv_data->status_code, http_recv_data->recv_header);
        }
        else
        {
            log_error("Could not retrieve http response info");
        }
    }
}

static void request_list_destroy_cb(void* user_ctx, void* remove_item)
{
    (void)user_ctx;
    HTTP_REQUEST_INFO* request_info = (HTTP_REQUEST_INFO*)remove_item;
    if (request_info == NULL)
    {
        log_error("Failure invalid user context in list destroy");
    }
    else
    {
        free(request_info->payload.payload);
        free(request_info->header_line.payload);
        free(request_info->relative_path);
        free(request_info);
    }
}

static int create_connection(HTTP_CLIENT_INFO* client_info, const HTTP_ADDRESS* http_address)
{
    int result;
    SOCKETIO_CONFIG config = {0};
    config.hostname = http_address->hostname;
    config.port = http_address->port;
    config.address_type = ADDRESS_TYPE_IP;

    PATCHCORD_CALLBACK_INFO callback_info;
    callback_info.on_bytes_received = http_codec_get_recv_function();
    callback_info.on_bytes_received_ctx = client_info->codec_handle;
    callback_info.on_io_error = on_error;
    callback_info.on_io_error_ctx = client_info;

    if ((client_info->xio_handle = patchcord_client_create(cord_socket_get_interface(), &config, &callback_info)) == NULL)
    {
        log_error("Failure creating client connection");
        result = __LINE__;
    }
    else if (patchcord_client_open(client_info->xio_handle, on_open_complete, client_info) != 0)
    {
        log_error("Failure opening http client");
        result = __LINE__;
    }
    else
    {
        result = 0;
    }
    return result;
}

HTTP_CLIENT_HANDLE http_client_create(void)
{
    HTTP_CLIENT_INFO* result;
    if ((result = (HTTP_CLIENT_INFO*)malloc(sizeof(HTTP_CLIENT_INFO))) == NULL)
    {
        log_error("Failure allocating http client info");
    }
    else
    {
        memset(result, 0, sizeof(HTTP_CLIENT_INFO));
        if ((result->codec_handle = http_codec_create(on_codec_recv_callback, result)) == NULL)
        {
            log_error("Failure creating request list");
            free(result);
            result = NULL;
        }
        else if ((result->request_list = item_list_create(request_list_destroy_cb, NULL) ) == NULL)
        {
            log_error("Failure creating request list");

            http_codec_destroy(result->codec_handle);
            free(result);
            result = NULL;
        }
        else if ((result->recv_callback_list = item_list_create(NULL, NULL) ) == NULL)
        {
            log_error("Failure creating request list");

            http_codec_destroy(result->codec_handle);
            item_list_destroy(result->request_list);
            free(result);
            result = NULL;
        }
    }
    return result;
}

void http_client_destroy(HTTP_CLIENT_HANDLE handle)
{
    if (handle != NULL)
    {
        patchcord_client_destroy(handle->xio_handle);
        http_codec_destroy(handle->codec_handle);
        item_list_destroy(handle->recv_callback_list);
        item_list_destroy(handle->request_list);
        free(handle);
    }
}

int http_client_open(HTTP_CLIENT_HANDLE handle, const HTTP_ADDRESS* http_address, ON_HTTP_OPEN_COMPLETE_CALLBACK on_open_complete_cb, void* open_user_ctx, ON_HTTP_ERROR_CALLBACK on_error_cb, void* err_user_ctx)
{
    int result;
    if (handle == NULL || http_address == NULL)
    {
        log_error("Invalid paramenter handle: %p, http_address: %p", handle, http_address);
        result = __LINE__;
    }
    else if (handle->state != CLIENT_STATE_NOT_CONN)
    {
        log_error("Open attempt on a client that is not closed");
        result = __LINE__;
    }
    else if (create_connection(handle, http_address) != 0)
    {
        log_error("Failure attempting to connect to client");
        result = __LINE__;
    }
    else
    {
        handle->port = http_address->port;
        handle->on_open_complete_cb = on_open_complete_cb;
        handle->open_complete_ctx = open_user_ctx;
        handle->on_error_cb = on_error_cb;
        handle->err_user_ctx = err_user_ctx;
        handle->state = CLIENT_STATE_OPENING;
        result = 0;
    }
    return result;
}

int http_client_close(HTTP_CLIENT_HANDLE handle, ON_HTTP_CLIENT_CLOSE on_close_cb, void* user_ctx)
{
    int result;
    if (handle == NULL)
    {
        log_error("Invalid paramenter handle is NULL");
        result = __LINE__;
    }
    else if (handle->state == CLIENT_STATE_NOT_CONN)
    {
        log_error("Close attempt on a client that is not open");
        result = __LINE__;
    }
    else
    {
        handle->on_close_cb = on_close_cb;
        handle->close_user_ctx = user_ctx;
        if (handle->state == CLIENT_STATE_OPEN || handle->state == CLIENT_STATE_OPENING || handle->state == CLIENT_STATE_OPENED)
        {
            if (patchcord_client_close(handle->xio_handle, on_close_complete, handle) != 0)
            {
                log_error("Failure on close attempt");
                handle->curr_result = HTTP_CLIENT_ERROR;
                handle->state = CLIENT_STATE_ERROR;
                result = __LINE__;
            }
            else
            {
                handle->state = CLIENT_STATE_CLOSING;
                result = 0;
            }
        }
        else
        {
            handle->state = CLIENT_STATE_NOT_CONN;
            result = 0;
        }
    }
    return result;
}

int http_client_execute_request(HTTP_CLIENT_HANDLE handle, HTTP_CLIENT_REQUEST_TYPE request_type, const char* relative_path,
    HTTP_HEADERS_HANDLE http_header, const unsigned char* content, size_t content_length, ON_HTTP_REQUEST_CALLBACK on_request_callback, void* callback_ctx)
{
    int result;
    if (handle == NULL)
    {
        log_error("Invalid paramenter handle is NULL");
        result = __LINE__;
    }
    else
    {
        HTTP_RESP_INFO resp_info;
        HTTP_REQUEST_INFO* execute_req;
        if ((execute_req = (HTTP_REQUEST_INFO*)malloc(sizeof(HTTP_REQUEST_INFO))) == NULL)
        {
            log_error("Failure allocating request");
            result = __LINE__;
        }
        else
        {
            memset(execute_req, 0, sizeof(HTTP_REQUEST_INFO));
            uint16_t port;
            bool create_header = false;
            execute_req->request_type = request_type;
            execute_req->client_info = handle;

            if (http_header == NULL)
            {
                if ((http_header = http_header_create()) == NULL)
                {
                    log_error("Failure allocating request");
                    result = __LINE__;
                }
                else
                {
                    result = 0;
                    create_header = true;
                }
            }
            else
            {
                result = 0;
            }

            if (result == 0)
            {
                resp_info.on_request_cb = on_request_callback;
                resp_info.on_request_ctx = callback_ctx;
                if (clone_string(&execute_req->relative_path, relative_path) != 0)
                {
                    log_error("Failure allocating request");
                    free(execute_req);
                    result = __LINE__;
                }
                else if (content_length != 0 && byte_buffer_construct(&execute_req->payload, content, content_length) != 0)
                {
                    log_error("Failure allocating request");
                    free(execute_req->relative_path);
                    free(execute_req);
                    result = __LINE__;
                }
                else if (construct_header_line(execute_req, http_header, content_length, patchcord_client_query_endpoint(handle->xio_handle, &port), handle->port) != 0)
                {
                    log_error("Failure allocating header line");
                    free(execute_req->payload.payload);
                    free(execute_req->relative_path);
                    free(execute_req);
                    result = __LINE__;
                }
                else if (item_list_add_copy(handle->recv_callback_list, &resp_info, sizeof(HTTP_RESP_INFO) ) != 0)
                {
                    log_error("Failure adding to response list");
                    free(execute_req->payload.payload);
                    free(execute_req->header_line.payload);
                    free(execute_req->relative_path);
                    free(execute_req);
                    result = __LINE__;
                }
                else if (item_list_add_item(handle->request_list, execute_req) != 0)
                {
                    log_error("Failure adding to request list");
                    free(execute_req->payload.payload);
                    free(execute_req->header_line.payload);
                    free(execute_req->relative_path);
                    size_t remove_index = item_list_item_count(handle->recv_callback_list);
                    (void)item_list_remove_item(handle->recv_callback_list, remove_index);
                    free(execute_req);
                    result = __LINE__;
                }
                else
                {
                    result = 0;
                }
            }

            if (create_header)
            {
                http_header_destroy(http_header);
            }
        }
    }
    return result;
}

void http_client_process_item(HTTP_CLIENT_HANDLE handle)
{
    if (handle == NULL)
    {
        log_error("Invalid paramenter handle is NULL");
    }
    else
    {
        patchcord_client_process_item(handle->xio_handle);
        switch (handle->state)
        {
            case CLIENT_STATE_OPENING:
            case CLIENT_STATE_CLOSING:
                break;
            case CLIENT_STATE_OPENED:
                if (handle->on_open_complete_cb != NULL)
                {
                    handle->on_open_complete_cb(handle->open_complete_ctx, HTTP_CLIENT_OK);
                }
                handle->state = CLIENT_STATE_OPEN;
                break;
            case CLIENT_STATE_OPEN:
            {
                const HTTP_REQUEST_INFO* execute_req;
                while ((execute_req = item_list_get_front(handle->request_list)) != NULL)
                {
                    // Send the item
                    if (send_http_request(handle, execute_req) != 0)
                    {
                        handle->state = CLIENT_STATE_ERROR;
                        handle->curr_result = HTTP_CLIENT_SEND_FAILED;
                        log_error("Failure sending http request");
                        break;
                    }
                    else if (item_list_remove_item(handle->request_list, 0) != 0)
                    {
                        handle->state = CLIENT_STATE_ERROR;
                        handle->curr_result = HTTP_CLIENT_ERROR;
                        log_error("Invalid paramenter handle is NULL");
                    }
                }
                break;
            }
            case CLIENT_STATE_CLOSED:
                if (handle->on_close_cb != NULL)
                {
                    handle->on_close_cb(handle->close_user_ctx);
                }
                handle->state = CLIENT_STATE_NOT_CONN;
                break;
            case CLIENT_STATE_NOT_CONN:
            default:
                break;
            case CLIENT_STATE_ERROR:
                if (handle->on_error_cb)
                {
                    handle->on_error_cb(handle->err_user_ctx, handle->curr_result);
                }
                handle->state = CLIENT_STATE_NOT_CONN;
                break;
        }
    }
}

int http_client_set_trace(HTTP_CLIENT_HANDLE handle, bool set_trace)
{
    int result;
    if (handle == NULL)
    {
        result = __LINE__;
        log_error("Invalid argument specified handle: NULL");
    }
    else
    {
        handle->logging_enabled = set_trace;
        result = http_codec_set_trace(handle->codec_handle, handle->logging_enabled);
    }
    return result;
}
