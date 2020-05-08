// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

#include "http_client/http_client.h"

typedef struct SAMPLE_DATA_TAG
{
    int keep_running;
    int socket_open;
    int socket_closed;
    int send_complete;
} SAMPLE_DATA;

static const char* TEST_SEND_DATA = "This is a test message\n";

void on_open_complete(void* context, HTTP_CLIENT_RESULT open_result)
{
    SAMPLE_DATA* sample = (SAMPLE_DATA*)context;
    if (open_result != HTTP_CLIENT_OK)
    {
        sample->keep_running = 1;
        printf("Open failed\n");
    }
    else
    {
        sample->socket_open = 1;
        printf("Open complete called");
    }
}

static void on_close_complete(void* context)
{
    SAMPLE_DATA* sample = (SAMPLE_DATA*)context;
    sample->socket_closed = 1;
}

void on_request_callback(void* callback_ctx, HTTP_CLIENT_RESULT request_result, const unsigned char* content, size_t content_length, unsigned int status_code, HTTP_HEADERS_HANDLE response_headers)
{
    SAMPLE_DATA* sample = (SAMPLE_DATA*)callback_ctx;
    sample->socket_closed = 1;
}

void on_error(void* context, HTTP_CLIENT_RESULT error_result)
{
    printf("Error detected\n");
}

int main()
{
    SAMPLE_DATA data = {0};
    HTTP_CLIENT_HANDLE http_client;

    HTTP_ADDRESS http_address = {0};
    http_address.hostname = "httpbin.org";
    http_address.port = 80;

    if ((http_client = http_client_create()) == NULL)
    {
        printf("Failure creating http client");
    }
    else if (http_client_open(http_client, &http_address, on_open_complete, &data, on_error, NULL) != 0)
    {
        printf("Failure creating http client");
    }
    else
    {
        do
        {
            http_client_process_item(http_client);

            if (data.socket_open > 0)
            {
                if (data.send_complete == 0)
                {
                    HTTP_HEADERS_HANDLE http_header;

                    // Send socket

                    if (http_client_execute_request(http_client, HTTP_CLIENT_REQUEST_GET, "/", http_header, NULL, 0, on_request_callback, &data) != 0)
                    {
                        printf("Failure sending data to socket\n");
                    }
                }
                else if (data.send_complete >= 2)
                {
                    http_client_close(http_client, on_close_complete, &data);
                }
            }
            if (data.socket_closed > 0)
            {
                break;
            }
        } while (data.keep_running == 0);
        http_client_destroy(http_client);
    }
    return 0;
}