// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <ctype.h>

#include "lib-util-c/sys_debug_shim.h"
#include "lib-util-c/app_logging.h"
#include "lib-util-c/crt_extensions.h"
#include "lib-util-c/buffer_alloc.h"

#include "patchcords/xio_client.h"

#include "http_client/http_headers.h"
#include "http_client/http_codec.h"

static const char* HTTP_TRANSFER_ENCODING = "transfer-encoding";
static const char* HTTP_CONTENT_LEN = "content-length";
#define HTTP_CONTENT_LENGTH_LEN     14
#define HTTP_TRANSFER_ENCODING_LEN  17
#define HTTP_END_TOKEN_LEN          4
#define HTTP_CRLF_LEN               2

typedef enum RESPONSE_MESSAGE_STATE_TAG
{
    state_initial,
    state_opening,
    state_open,
    state_process_status_line,
    state_process_headers,
    state_process_body,
    state_process_chunked_body,

    state_send_user_callback,
    state_parse_complete,

    state_closing,
    state_closed,
    state_error
} RESPONSE_MESSAGE_STATE;

typedef enum PARSE_RESULT_TAG
{
    result_success,
    result_failure,
    result_complete
} PARSE_RESULT;

typedef struct HTTP_INCOMING_DATA_TAG
{
    // Parse Data
    BYTE_BUFFER recv_msg;
    size_t buffer_length;
    size_t buffer_offset;

    // Resulting Data
    BYTE_BUFFER content_info;
    HTTP_HEADERS_HANDLE recv_header;
    uint32_t status_code;
    bool is_chunked;
} HTTP_INCOMING_DATA;

typedef struct HTTP_CODEC_INFO_TAG
{
    ON_HTTP_DATA_CALLBACK data_callback;
    void* user_ctx;

    bool trace_on;

    RESPONSE_MESSAGE_STATE recv_state;
    HTTP_INCOMING_DATA recv_data;
} HTTP_CODEC_INFO;

static void reduce_wasted_space(BYTE_BUFFER* recv_msg, size_t reduce_offset)
{
    for (size_t index = 0; index < recv_msg->payload_size; index++)
    {
        if (index < recv_msg->payload_size-reduce_offset)
        {
            recv_msg->payload[index] = recv_msg->payload[reduce_offset+index];
        }
        else
        {
            break;
            //recv_msg->payload[index] = '\0';
        }
    }
    recv_msg->payload_size -= reduce_offset;
}

static int string_compare_no_case(const char* str1, size_t str1_len, const char* str2, size_t str2_len)
{
    int result = 0;
    if (str1 != NULL && str2 == NULL)
    {
        result = 1;
    }
    else if (str1 == NULL && str2 != NULL)
    {
        result = -1;
    }
    else
    {
        size_t min_len = str1_len < str2_len ? str1_len : str2_len;
        for (size_t index = 0; index < min_len; index++)
        {
            result = tolower(*(str1+index)) - tolower(*(str2+index));
            if (result != 0)
            {
                break;
            }
        }
    }
    return result;
}

static int convert_char_to_hex(const unsigned char* hexText, size_t len)
{
    int result = 0;
    for (size_t index = 0; index < len; index++)
    {
        if (hexText[index] == ';')
        {
            break;
        }
        else
        {
            int accumulator = 0;
            if (hexText[index] >= 48 && hexText[index] <= 57)
            {
                accumulator = hexText[index] - 48;
            }
            else if (hexText[index] >= 65 && hexText[index] <= 70)
            {
                accumulator = hexText[index] - 55;
            }
            else if (hexText[index] >= 97 && hexText[index] <= 102)
            {
                accumulator = hexText[index] - 87;
            }
            if (index > 0)
            {
                result = result << 4;
            }
            result += accumulator;
        }
    }
    return result;
}

static int initialize_received_data(HTTP_INCOMING_DATA* recv_data, const unsigned char* buffer, size_t length)
{
    int result;
    if (recv_data->recv_header == NULL)
    {
        if ((recv_data->recv_header = http_header_create()) == NULL)
        {
            log_error("Failure allocating http receive header");
            result = __LINE__;
        }
        else
        {
            result = 0;
        }
    }
    else
    {
        http_header_clear(recv_data->recv_header);
        result = 0;
    }

    if (result == 0)
    {
        if (recv_data->recv_msg.payload != NULL)
        {
            free(recv_data->recv_msg.payload);
            memset(&recv_data->recv_msg, 0, sizeof(BYTE_BUFFER));
        }

        if (byte_buffer_construct(&recv_data->recv_msg, buffer, length) != 0)
        {
            log_error("Failure allocating recieve message buffer");
            result = __LINE__;
        }
        else
        {
            recv_data->buffer_offset = 0;
            recv_data->content_info.payload = NULL;
            recv_data->content_info.payload_size = 0;
            recv_data->buffer_length = length;
            result = 0;
        }
    }
    return result;
}

static PARSE_RESULT process_status_code_line(const unsigned char* content_data, size_t content_len, size_t* position, uint32_t* status_code)
{
    PARSE_RESULT result = result_success;
    int space_found = 0;
    char temp_status_code[4];
    const char* initial_space = NULL;
    for (size_t index = 0; index < content_len; index++)
    {
        if (content_data[index] == ' ')
        {
            if (space_found == 1)
            {
                memcpy(temp_status_code, initial_space, 3);
                temp_status_code[3] = '\0';
            }
            else if (space_found == 0)
            {
                initial_space = (const char*)(content_data+index+1);
            }
            space_found++;
        }
        // Done with the status line
        else if (content_data[index] == '\n')
        {
            /* code */
            *status_code = (uint32_t)atol(initial_space);
            if (index < content_len)
            {
                *position = index+1;
            }
            else
            {
                *position = index;
            }
            result = result_complete;
            break;
        }
    }
    return result;
}

static PARSE_RESULT process_header_line(HTTP_HEADERS_HANDLE recv_header, const unsigned char* content_data, size_t content_len, size_t* position, size_t* content_header_len, bool* is_chunked)
{
    PARSE_RESULT result = result_success;
    bool continue_processing = true;
    bool colon_encountered = false;
    bool crlf_encounted = false;

    const char* header_key = NULL;
    size_t header_key_len = 0;
    const unsigned char* target_pos = content_data;

    for (size_t index = 0; index < content_len && continue_processing; index++)
    {
        if (content_data[index] == ':' && !colon_encountered)
        {
            colon_encountered = true;
            size_t key_len = (&content_data[index])-target_pos;

            header_key = (const char*)target_pos;
            header_key_len = key_len;

            target_pos = content_data+index+1;
            crlf_encounted = false;
        }
        else if (content_data[index] == '\r')
        {
            if (header_key != NULL)
            {
                // Remove leading spaces
                while (*target_pos == 32) { target_pos++; }

                size_t value_len = (&content_data[index])-target_pos;
                const char* header_value = (const char*)target_pos;
                size_t header_value_len = value_len;

                if (http_header_add_partial(recv_header, header_key, header_key_len, header_value, header_value_len) != 0)
                {
                    log_error("Failure adding header value");
                    result = result_failure;
                    continue_processing = false;
                }
                else
                {
                    if (string_compare_no_case(header_key, header_key_len, HTTP_CONTENT_LEN, HTTP_CONTENT_LENGTH_LEN) == 0)
                    {
                        *is_chunked = false;
                        *content_header_len = atol(header_value);
                    }
                    else if (string_compare_no_case(header_key, header_key_len, HTTP_TRANSFER_ENCODING, HTTP_TRANSFER_ENCODING_LEN) == 0)
                    {
                        *is_chunked = true;
                        *content_header_len = 0;
                    }
                    if (index < content_len)
                    {
                        *position = index;
                    }
                    else
                    {
                        *position = index-1;
                    }
                }
                header_key = NULL;
            }
        }
        else if (content_data[index] == '\n')
        {
            if (crlf_encounted)
            {
                if (index < content_len)
                {
                    *position = index+1;
                }
                else
                {
                    *position = index;
                }
                result = result_complete;
                break;
            }
            else
            {
                colon_encountered = false;
                crlf_encounted = true;
                target_pos = content_data+index+1;
            }
        }
        else
        {
            crlf_encounted = false;
        }
    }
    return result;
}

static void on_http_bytes_recv(void* context, const unsigned char* buffer, size_t length)
{
    HTTP_CODEC_INFO* codec_info = (HTTP_CODEC_INFO*)context;
    if (codec_info != NULL && buffer != NULL && length > 0 && codec_info->recv_state != state_error)
    {
        PARSE_RESULT parse_res;

        if (codec_info->recv_state == state_initial || codec_info->recv_state == state_open)
        {
            if (initialize_received_data(&codec_info->recv_data, buffer, length) != 0)
            {
                codec_info->recv_state = state_error;
            }
            else
            {
                codec_info->recv_state = state_process_status_line;
            }
        }
        else
        {
            // Add the data to the end
            if (byte_buffer_construct(&codec_info->recv_data.recv_msg, buffer, length) != 0)
            {
                log_error("Failure allocating recieve message buffer");
                codec_info->recv_state = state_error;
            }
            else
            {
                codec_info->recv_data.buffer_length += length;
            }
        }

        if (codec_info->recv_state != state_error)
        {
            size_t buff_index = 0;
            if (codec_info->recv_state == state_process_status_line)
            {
                parse_res = process_status_code_line(codec_info->recv_data.recv_msg.payload+codec_info->recv_data.buffer_offset, codec_info->recv_data.buffer_length, &buff_index, &codec_info->recv_data.status_code);
                if (parse_res == result_complete && codec_info->recv_data.status_code > 0)
                {
                    // Shrink the buffer
                    codec_info->recv_data.buffer_offset += buff_index;
                    codec_info->recv_data.buffer_length -= buff_index;
                    codec_info->recv_state = state_process_headers;

                    // Reduce the buffer space
                    reduce_wasted_space(&codec_info->recv_data.recv_msg, codec_info->recv_data.buffer_offset);
                    codec_info->recv_data.buffer_offset = 0;
                }
                else if (parse_res == result_failure)
                {
                    log_error("Failure attempting to parse status line");
                    codec_info->recv_state = state_error;
                }
            }

            if (codec_info->recv_state == state_process_headers)
            {
                buff_index = 0;
                parse_res = process_header_line(codec_info->recv_data.recv_header, codec_info->recv_data.recv_msg.payload+codec_info->recv_data.buffer_offset, codec_info->recv_data.buffer_length, &buff_index, &codec_info->recv_data.content_info.payload_size, &codec_info->recv_data.is_chunked);
                if (parse_res == result_complete)
                {
                    if (codec_info->recv_data.content_info.payload_size == 0)
                    {
                        if (codec_info->recv_data.is_chunked)
                        {
                            codec_info->recv_state = state_process_chunked_body;
                        }
                        else
                        {
                            // Content len is 0 so we are finished with the body
                            codec_info->recv_state = state_send_user_callback;
                        }
                    }
                    else
                    {
                        codec_info->recv_state = state_process_body;
                    }

                    // If we're not sending the user infor
                    codec_info->recv_data.buffer_offset += buff_index;
                    codec_info->recv_data.buffer_length -= buff_index;

                    reduce_wasted_space(&codec_info->recv_data.recv_msg, codec_info->recv_data.buffer_offset);
                    codec_info->recv_data.buffer_offset = 0;
                }
                else if (parse_res == result_failure)
                {
                    log_error("Failure attempting to parse header line");
                    codec_info->recv_state = state_error;
                }
                else
                {
                    codec_info->recv_data.buffer_offset += buff_index;
                    codec_info->recv_data.buffer_length -= buff_index;
                }
            }

            if (codec_info->recv_state == state_process_body)
            {
                if (codec_info->recv_data.content_info.payload_size > 0)
                {
                    if ((codec_info->recv_data.content_info.payload_size == codec_info->recv_data.buffer_length) ||
                        (codec_info->recv_data.content_info.payload_size == (codec_info->recv_data.buffer_length - HTTP_END_TOKEN_LEN)))
                    {
                        codec_info->recv_data.content_info.payload = codec_info->recv_data.recv_msg.payload+codec_info->recv_data.buffer_offset;
                        codec_info->recv_state = state_send_user_callback;
                    }
                    else if (codec_info->recv_data.buffer_length > codec_info->recv_data.content_info.payload_size)
                    {
                        log_error("Failure attempting to parse header line");
                        codec_info->recv_state = state_error;
                    }
                }
            }

            if (codec_info->recv_state == state_process_chunked_body)
            {
                const unsigned char* iterator = codec_info->recv_data.recv_msg.payload+codec_info->recv_data.buffer_offset;
                const unsigned char* initial_pos = iterator;
                const unsigned char* begin = iterator;
                const unsigned char* end = iterator;
                BYTE_BUFFER chunk_msg = { 0 };
                chunk_msg.default_alloc = codec_info->recv_data.recv_msg.payload_size;

                while (iterator < (initial_pos + codec_info->recv_data.buffer_length))
                {
                    if (*iterator == '\r')
                    {
                        // Don't need anything
                        end = iterator;
                        iterator++;
                    }
                    else if (*iterator == '\n')
                    {
                        size_t data_length = 0;

                        size_t hex_len = end - begin;
                        if ((data_length = convert_char_to_hex(begin, hex_len)) == 0)
                        {
                            if (codec_info->recv_data.buffer_length - (iterator - initial_pos + 1) <= HTTP_END_TOKEN_LEN)
                            {
                                codec_info->recv_state = state_send_user_callback;
                            }
                            else
                            {
                                // Need to continue parsing
                                codec_info->recv_state = state_process_headers;
                            }
                            break;
                        }
                        else if ((data_length + HTTP_CRLF_LEN) < codec_info->recv_data.buffer_length - (iterator - initial_pos))
                        {
                            iterator += 1;
                            if (byte_buffer_construct(&chunk_msg, iterator, data_length) != 0)
                            {
                                log_error("Failure building buffer for chunked data");
                                codec_info->recv_state = state_error;
                                break;
                            }
                            else
                            {
                                if (iterator + (data_length + HTTP_CRLF_LEN) > initial_pos + codec_info->recv_data.buffer_length)
                                {
                                    log_error("Failure Invalid length specified");
                                    codec_info->recv_state = state_error;
                                    break;
                                }
                                else if (iterator + (data_length + HTTP_CRLF_LEN) == initial_pos + codec_info->recv_data.buffer_length)
                                {
                                    free(chunk_msg.payload);
                                    break;
                                }
                                else
                                {
                                    // Move the iterator beyond the data we read and the /r/n
                                    iterator += (data_length + HTTP_CRLF_LEN);
                                }

                                if (*iterator == '0' && (codec_info->recv_data.buffer_length - (iterator - initial_pos + 1) <= HTTP_END_TOKEN_LEN))
                                {
                                    if (codec_info->recv_data.buffer_length - (iterator - initial_pos + 1) <= HTTP_END_TOKEN_LEN)
                                    {
                                        free(codec_info->recv_data.recv_msg.payload);
                                        codec_info->recv_data.content_info.payload = codec_info->recv_data.recv_msg.payload = chunk_msg.payload;
                                        codec_info->recv_data.content_info.payload_size = codec_info->recv_data.recv_msg.payload_size = chunk_msg.payload_size;
                                        codec_info->recv_data.recv_msg.alloc_size = chunk_msg.alloc_size;
                                        codec_info->recv_state = state_send_user_callback;
                                    }
                                    else
                                    {
                                        // Need to continue parsing
                                        codec_info->recv_state = state_process_headers;
                                    }
                                    break;
                                }
                                else
                                {
                                }
                            }
                            begin = end = iterator;
                        }
                        else
                        {
                            break;
                        }
                    }
                    else
                    {
                        end = iterator;
                        iterator++;
                    }
                }
            }

            if (codec_info->recv_state == state_send_user_callback || codec_info->recv_state == state_error)
            {
                HTTP_RECV_DATA http_recv_data = {0};
                HTTP_CODEC_CB_RESULT operation_result = HTTP_CODEC_CB_RESULT_OK;

                if (codec_info->trace_on)
                {
                    log_trace("\r\nHTTP Status: %d\r\n", codec_info->recv_data.status_code);

                    size_t header_count = http_header_get_count(codec_info->recv_data.recv_header);
                    for (size_t index = 0; index < header_count; index++)
                    {
                        const char* name;
                        const char* value;
                        if (http_header_get_name_value_pair(codec_info->recv_data.recv_header, index, &name, &value) == 0)
                        {
                            log_trace("\r\n%s: %s\r\n", name, value);
                        }

                        // Trace body
                        log_trace("\r\n%.*s\r\n", (int)codec_info->recv_data.content_info.payload_size, codec_info->recv_data.content_info.payload);
                    }
                }

                http_recv_data.recv_header = codec_info->recv_data.recv_header;
                http_recv_data.status_code = codec_info->recv_data.status_code;
                http_recv_data.http_content.payload = codec_info->recv_data.content_info.payload;
                http_recv_data.http_content.payload_size = codec_info->recv_data.content_info.payload_size;

                codec_info->data_callback(codec_info->user_ctx, operation_result, &http_recv_data);
                codec_info->recv_state = state_parse_complete;
            }

            if (codec_info->recv_state == state_parse_complete )
            {
                http_header_destroy(codec_info->recv_data.recv_header);
                free(codec_info->recv_data.recv_msg.payload);

                memset(&codec_info->recv_data, 0, sizeof(HTTP_INCOMING_DATA));
            }
        }
        else
        {
            codec_info->data_callback(codec_info->user_ctx, HTTP_CODEC_CB_RESULT_ERROR, NULL);
            codec_info->recv_state = state_parse_complete;
        }
    }
}

static void deinit_data(HTTP_INCOMING_DATA* data_obj)
{
    http_header_destroy(data_obj->recv_header);
    data_obj->recv_header = NULL;
    free(data_obj->content_info.payload);
    data_obj->content_info.payload = NULL;
    free(data_obj->recv_msg.payload);
    data_obj->recv_msg.payload = NULL;
}

HTTP_CODEC_HANDLE http_codec_create(ON_HTTP_DATA_CALLBACK data_callback, void* user_ctx)
{
    HTTP_CODEC_INFO* result;
    if ((result = (HTTP_CODEC_INFO*)malloc(sizeof(HTTP_CODEC_INFO))) == NULL)
    {
        log_error("Failure allocating http client info");
    }
    else
    {
        memset(result, 0, sizeof(HTTP_CODEC_INFO));
        result->data_callback = data_callback;
        result->user_ctx = user_ctx;
    }
    return result;
}

void http_codec_destroy(HTTP_CODEC_HANDLE handle)
{
    if (handle != NULL)
    {
        deinit_data(&handle->recv_data);
        free(handle);
    }
}

int http_codec_reintialize(HTTP_CODEC_HANDLE handle)
{
    int result;
    if (handle == NULL)
    {
        log_error("Invalid argument specified handle: NULL");
        result = __LINE__;
    }
    else
    {
        handle->recv_state = state_initial;
        deinit_data(&handle->recv_data);
    }
    return result;
}

ON_BYTES_RECEIVED http_codec_get_recv_function(void)
{
    return on_http_bytes_recv;
}

int http_codec_set_trace(HTTP_CODEC_HANDLE handle, bool set_trace)
{
    int result;
    if (handle == NULL)
    {
        log_error("Invalid argument specified handle: NULL");
        result = __LINE__;
    }
    else
    {
        handle->trace_on = set_trace;
        result = 0;
    }
    return result;
}