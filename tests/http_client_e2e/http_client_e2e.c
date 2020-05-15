// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#else
#include <stdlib.h>
#include <stddef.h>
#endif

#include "ctest.h"
#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_prod.h"

#include "lib-util-c/buffer_alloc.h"
#include "lib-util-c/app_logging.h"
#include "lib-util-c/thread_mgr.h"
#include "lib-util-c/alarm_timer.h"

#include "http_client/http_client.h"
#include "http_client/http_headers.h"

static const char* HTTP_TEST_SERVER = "httpbin.org";
static const char* APP_JSON_HEADER = "application/json";

#define OPERATION_TIMEOUT_SEC       30

typedef enum CLIENT_E2E_STATE_TAG
{
    CLIENT_STATE_CONN,
    CLIENT_STATE_CONNECTING,
    CLIENT_STATE_SENDING,
    CLIENT_STATE_RECEIVING,
    CLIENT_STATE_RECEIVED,
    CLIENT_STATE_CLOSING,
    CLIENT_STATE_CLOSED,
    CLIENT_STATE_ERROR,
    CLIENT_STATE_COMPLETE
} CLIENT_E2E_STATE;

typedef struct CLIENT_E2E_DATA_TAG
{
    HTTP_CLIENT_HANDLE http_client;
    CLIENT_E2E_STATE client_state;
    CLIENT_E2E_STATE complete_op;
    bool test_complete;
    bool request_recv;
    BYTE_BUFFER recv_data;

    unsigned int expected_status_code;
    const char* expected_header;
    HTTP_CLIENT_RESULT expected_results;
    HTTP_CLIENT_RESULT error_result;
    ALARM_TIMER_INFO timer;
} CLIENT_E2E_DATA;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)
static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    CTEST_ASSERT_FAIL("umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
}

CTEST_BEGIN_TEST_SUITE(http_client_e2e)

CTEST_SUITE_INITIALIZE()
{
}

CTEST_SUITE_CLEANUP()
{
}

CTEST_FUNCTION_INITIALIZE()
{
}

CTEST_FUNCTION_CLEANUP()
{
}

static void on_close_complete(void* context)
{
    log_debug("on_close_complete called");
    CLIENT_E2E_DATA* e2e_data = (CLIENT_E2E_DATA*)context;
    CTEST_ASSERT_IS_NOT_NULL(e2e_data, "on_close_complete context NULL");

    e2e_data->complete_op = CLIENT_STATE_CLOSED;
}

static void on_open_complete(void* context, HTTP_CLIENT_RESULT open_result)
{
    log_debug("on_open_complete called");

    CLIENT_E2E_DATA* e2e_data = (CLIENT_E2E_DATA*)context;
    CTEST_ASSERT_IS_NOT_NULL(e2e_data, "on_open_complete context NULL");
    CTEST_ASSERT_ARE_EQUAL(int, HTTP_CLIENT_OK, open_result, "On open complete not valid result");

    e2e_data->complete_op = CLIENT_STATE_CONNECTING;
}

static void on_error(void* context, HTTP_CLIENT_RESULT error_result)
{
    log_debug("on_error called");

    CLIENT_E2E_DATA* e2e_data = (CLIENT_E2E_DATA*)context;
    CTEST_ASSERT_IS_NOT_NULL(e2e_data, "on_error context NULL");

    e2e_data->error_result = error_result;
    e2e_data->client_state = CLIENT_STATE_ERROR;
}

static void on_request_callback(void* context, HTTP_CLIENT_RESULT request_result, const unsigned char* content, size_t content_length, unsigned int status_code, HTTP_HEADERS_HANDLE response_headers)
{
    log_debug("on_request_callback called");
    CLIENT_E2E_DATA* e2e_data = (CLIENT_E2E_DATA*)context;
    CTEST_ASSERT_IS_NOT_NULL(e2e_data, "on_request_callback context NULL");

    e2e_data->request_recv = true;

    if (e2e_data->expected_header != NULL)
    {
        CTEST_ASSERT_ARE_EQUAL(char_ptr, e2e_data->expected_header, http_header_get_value(response_headers, "content-type"));
    }
    CTEST_ASSERT_ARE_EQUAL(int, e2e_data->expected_status_code, status_code, "Invalid status code from request");
    CTEST_ASSERT_ARE_EQUAL(int, e2e_data->expected_results, request_result, "Invalid results from request");
}

static void open_http_connection(CLIENT_E2E_DATA* e2e_data)
{
    e2e_data->http_client = http_client_create();
    CTEST_ASSERT_IS_NOT_NULL(e2e_data->http_client, "Failure creating socket");

    CTEST_ASSERT_ARE_EQUAL(int, 0, alarm_timer_init(&e2e_data->timer));
    CTEST_ASSERT_ARE_EQUAL(int, 0, alarm_timer_start(&e2e_data->timer, OPERATION_TIMEOUT_SEC));

    HTTP_ADDRESS http_address = {0};
    http_address.hostname = HTTP_TEST_SERVER;
    http_address.port = 80;
    CTEST_ASSERT_ARE_EQUAL(int, 0, http_client_open(e2e_data->http_client, &http_address, on_open_complete, e2e_data, on_error, e2e_data));
}

static void set_current_state(CLIENT_E2E_DATA* e2e_data, CLIENT_E2E_STATE new_state)
{
    e2e_data->client_state = new_state;
    alarm_timer_reset(&e2e_data->timer);
}

CTEST_FUNCTION(http_client_send_get_create_succeed)
{
    // arrange
    CLIENT_E2E_DATA e2e_data = {0};
    HTTP_HEADERS_HANDLE http_header = NULL;

    // act
    do
    {
        switch (e2e_data.client_state)
        {
            case CLIENT_STATE_CONN:
            {
                open_http_connection(&e2e_data);
                set_current_state(&e2e_data, CLIENT_STATE_CONNECTING);
                break;
            }
            case CLIENT_STATE_CONNECTING:
            {
                if (e2e_data.complete_op == CLIENT_STATE_CONNECTING)
                {
                    set_current_state(&e2e_data, CLIENT_STATE_SENDING);
                }
                break;
            }
            case CLIENT_STATE_SENDING:
                log_debug("Executing http request get");
                e2e_data.expected_status_code = 200;
                e2e_data.expected_results = HTTP_CLIENT_OK;
                CTEST_ASSERT_ARE_EQUAL(int, 0, http_client_execute_request(e2e_data.http_client, HTTP_CLIENT_REQUEST_GET, "/get", http_header, NULL, 0, on_request_callback, &e2e_data));
                set_current_state(&e2e_data, CLIENT_STATE_RECEIVING);
                break;
            case CLIENT_STATE_RECEIVING:
                if (e2e_data.request_recv)
                {
                    set_current_state(&e2e_data, CLIENT_STATE_RECEIVED);
                }
                break;
            case CLIENT_STATE_RECEIVED:
            {
                set_current_state(&e2e_data, CLIENT_STATE_CLOSING);
                break;
            }
            case CLIENT_STATE_CLOSING:
            {
                CTEST_ASSERT_ARE_EQUAL(int, 0, http_client_close(e2e_data.http_client, on_close_complete, &e2e_data));
                set_current_state(&e2e_data, CLIENT_STATE_CLOSED);
                break;
            }
            case CLIENT_STATE_CLOSED:
                if (e2e_data.complete_op == CLIENT_STATE_CLOSED)
                {
                    e2e_data.test_complete = true;
                    set_current_state(&e2e_data, CLIENT_STATE_COMPLETE);
                }
                break;
            case CLIENT_STATE_ERROR:
                CTEST_ASSERT_FAIL("Failure http client encountered %d", (int)e2e_data.error_result);
                break;
        }
        http_client_process_item(e2e_data.http_client);
        CTEST_ASSERT_IS_FALSE(alarm_timer_is_expired(&e2e_data.timer), "Failure http client has timed out on operation %d", (int)e2e_data.client_state);
        thread_mgr_sleep(5);
    } while (!e2e_data.test_complete);

    http_client_destroy(e2e_data.http_client);
}

CTEST_FUNCTION(http_client_send_delete_create_succeed)
{
    // arrange
    CLIENT_E2E_DATA e2e_data = {0};

    // act
    HTTP_HEADERS_HANDLE http_header = http_header_create();
    CTEST_ASSERT_IS_NOT_NULL(http_header, "Failed creating http header");
    CTEST_ASSERT_ARE_EQUAL(int, 0, http_header_add(http_header, "accept", APP_JSON_HEADER));

    do
    {
        switch (e2e_data.client_state)
        {
            case CLIENT_STATE_CONN:
            {
                open_http_connection(&e2e_data);
                set_current_state(&e2e_data, CLIENT_STATE_CONNECTING);
                break;
            }
            case CLIENT_STATE_CONNECTING:
            {
                if (e2e_data.complete_op == CLIENT_STATE_CONNECTING)
                {
                    set_current_state(&e2e_data, CLIENT_STATE_SENDING);
                }
                break;
            }
            case CLIENT_STATE_SENDING:
                log_debug("Executing http request delete");
                e2e_data.expected_status_code = 200;
                e2e_data.expected_results = HTTP_CLIENT_OK;
                CTEST_ASSERT_ARE_EQUAL(int, 0, http_client_execute_request(e2e_data.http_client, HTTP_CLIENT_REQUEST_DELETE, "/delete", http_header, NULL, 0, on_request_callback, &e2e_data));
                set_current_state(&e2e_data, CLIENT_STATE_RECEIVING);
                break;
            case CLIENT_STATE_RECEIVING:
                if (e2e_data.request_recv)
                {
                    set_current_state(&e2e_data, CLIENT_STATE_RECEIVED);
                }
                break;
            case CLIENT_STATE_RECEIVED:
            {
                set_current_state(&e2e_data, CLIENT_STATE_CLOSING);
                break;
            }
            case CLIENT_STATE_CLOSING:
            {
                CTEST_ASSERT_ARE_EQUAL(int, 0, http_client_close(e2e_data.http_client, on_close_complete, &e2e_data));
                set_current_state(&e2e_data, CLIENT_STATE_CLOSED);
                break;
            }
            case CLIENT_STATE_CLOSED:
                if (e2e_data.complete_op == CLIENT_STATE_CLOSED)
                {
                    e2e_data.test_complete = true;
                    set_current_state(&e2e_data, CLIENT_STATE_COMPLETE);
                }
                break;
            case CLIENT_STATE_ERROR:
                CTEST_ASSERT_FAIL("Failure http client encountered %d", (int)e2e_data.error_result);
                break;
        }
        http_client_process_item(e2e_data.http_client);

        CTEST_ASSERT_IS_FALSE(alarm_timer_is_expired(&e2e_data.timer), "Failure http client has timed out on operation %d", (int)e2e_data.client_state);
        thread_mgr_sleep(5);
    } while (!e2e_data.test_complete);

    http_client_destroy(e2e_data.http_client);
}

CTEST_FUNCTION(http_client_status_code_404_succeed)
{
    // arrange
    CLIENT_E2E_DATA e2e_data = {0};
    HTTP_HEADERS_HANDLE http_header = NULL;

    do
    {
        switch (e2e_data.client_state)
        {
            case CLIENT_STATE_CONN:
            {
                open_http_connection(&e2e_data);
                set_current_state(&e2e_data, CLIENT_STATE_CONNECTING);
                break;
            }
            case CLIENT_STATE_CONNECTING:
            {
                if (e2e_data.complete_op == CLIENT_STATE_CONNECTING)
                {
                    set_current_state(&e2e_data, CLIENT_STATE_SENDING);
                }
                break;
            }
            case CLIENT_STATE_SENDING:
                log_debug("Executing http request status put of 404");
                e2e_data.expected_status_code = 404;
                e2e_data.expected_results = HTTP_CLIENT_OK;
                CTEST_ASSERT_ARE_EQUAL(int, 0, http_client_execute_request(e2e_data.http_client, HTTP_CLIENT_REQUEST_PUT, "/status/404", http_header, NULL, 0, on_request_callback, &e2e_data));
                set_current_state(&e2e_data, CLIENT_STATE_RECEIVING);
                break;
            case CLIENT_STATE_RECEIVING:
                if (e2e_data.request_recv)
                {
                    set_current_state(&e2e_data, CLIENT_STATE_RECEIVED);
                }
                break;
            case CLIENT_STATE_RECEIVED:
            {
                set_current_state(&e2e_data, CLIENT_STATE_CLOSING);
                break;
            }
            case CLIENT_STATE_CLOSING:
            {
                CTEST_ASSERT_ARE_EQUAL(int, 0, http_client_close(e2e_data.http_client, on_close_complete, &e2e_data));
                set_current_state(&e2e_data, CLIENT_STATE_CLOSED);
                break;
            }
            case CLIENT_STATE_CLOSED:
                if (e2e_data.complete_op == CLIENT_STATE_CLOSED)
                {
                    e2e_data.test_complete = true;
                    set_current_state(&e2e_data, CLIENT_STATE_COMPLETE);
                }
                break;
            case CLIENT_STATE_ERROR:
                CTEST_ASSERT_FAIL("Failure http client encountered %d", (int)e2e_data.error_result);
                break;
        }
        http_client_process_item(e2e_data.http_client);
        CTEST_ASSERT_IS_FALSE(alarm_timer_is_expired(&e2e_data.timer), "Failure http client has timed out on operation %d", (int)e2e_data.client_state);
        thread_mgr_sleep(5);
    } while (!e2e_data.test_complete);

    http_client_destroy(e2e_data.http_client);
}

CTEST_END_TEST_SUITE(http_client_e2e)
