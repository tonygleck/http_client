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

#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_bool.h"

static void* my_mem_shim_malloc(size_t size)
{
    return malloc(size);
}

static void my_mem_shim_free(void* ptr)
{
    free(ptr);
}

#define ENABLE_MOCKS
#include "umock_c/umock_c_prod.h"
#include "lib-util-c/sys_debug_shim.h"
#include "lib-util-c/item_list.h"
#include "lib-util-c/crt_extensions.h"
#include "lib-util-c/crt_extensions.h"
#include "lib-util-c/buffer_alloc.h"
#include "http_client/http_headers.h"
#include "http_client/http_codec.h"
#include "patchcords/xio_client.h"
#undef ENABLE_MOCKS

#include "http_client/http_client.h"

#define ENABLE_MOCKS
MOCKABLE_FUNCTION(, void, test_on_request_callback, void*, callback_ctx, HTTP_CLIENT_RESULT, request_result, const unsigned char*, content, size_t, content_length, unsigned int, status_code,
    HTTP_HEADERS_HANDLE, response_headers);

//MOCKABLE_FUNCTION(, void, test_on_open_complete, void*, context, HTTP_CLIENT_RESULT, open_result);
//MOCKABLE_FUNCTION(, void, test_on_error, void*, context, HTTP_CLIENT_RESULT, error_result);
//MOCKABLE_FUNCTION(, void, test_on_bytes_recv, void*, context, const unsigned char*, buffer, size_t, size);
//MOCKABLE_FUNCTION(, void, test_on_send_complete, void*, context, IO_SEND_RESULT, send_result);
//MOCKABLE_FUNCTION(, void, test_on_close_complete, void*, context);
#undef ENABLE_MOCKS

static const char* TEST_RELATIVE_PATH = "/";

static const char* TEST_HEADER_HOSTNAME = "HOSTNAME";
static const char* TEST_HEADER_VALUE_1 = "TEST_HEADER_VALUE_1";
static const char* TEST_HEADER_NAME_1 = "TEST_HEADER_NAME_1";

static XIO_INSTANCE_HANDLE TEST_XIO_HANDLE = (XIO_INSTANCE_HANDLE)0x12345;
static HTTP_HEADERS_HANDLE TEST_HTTP_HEADER = (HTTP_HEADERS_HANDLE)0x67890;
static HTTP_CODEC_HANDLE TEST_CODEC_HEADER = (HTTP_CODEC_HANDLE)0x987654;

static unsigned char TEST_SEND_CONTENT[] = { 0x33, 0x34, 0x35 };
static size_t TEST_CONTENT_LENGTH = 3;

static void* g_add_copy_item;
static void* g_destroy_user_ctx;
static const void* g_list_added_item;
static ITEM_LIST_DESTROY_ITEM g_list_destroy_cb;
static ON_IO_OPEN_COMPLETE g_on_open_complete;
static void* g_open_user_ctx;
static ON_IO_CLOSE_COMPLETE g_on_io_close_complete;
static void* g_on_close_user_ctx;
static ITEM_LIST_HANDLE g_do_not_delete_items;
static ON_HTTP_DATA_CALLBACK g_data_callback;
static void* data_cb_user_ctx;
static BYTE_BUFFER g_buffer_data;

#ifdef __cplusplus
extern "C" {
#endif
    int string_buffer_construct_sprintf(STRING_BUFFER* buffer, const char* format, ...);

    int string_buffer_construct_sprintf(STRING_BUFFER* buffer, const char* format, ...)
    {
        (void)format;
        if (buffer->payload == NULL)
        {
            const char* test_string = "test_string";

            buffer->payload = my_mem_shim_malloc(strlen(test_string)+1);
            strcpy(buffer->payload, test_string);
        }
        return 0;
    }

    void test_on_open_complete(void* context, HTTP_CLIENT_RESULT open_result)
    {
    }

    void test_on_error(void* context, HTTP_CLIENT_RESULT error_result)
    {
    }

    void test_on_close_complete(void* context)
    {
    }

    void test_on_request_callback(void* callback_ctx, HTTP_CLIENT_RESULT request_result, const unsigned char* content, size_t content_length, unsigned int status_code,
    HTTP_HEADERS_HANDLE response_headers)
    {
    }

    static int my_byte_buffer_construct(BYTE_BUFFER* buffer, const unsigned char* payload, size_t length)
    {
        buffer->payload = my_mem_shim_malloc(1);
        buffer->payload_size = 1;
        return 0;
    }

    static ITEM_LIST_HANDLE my_item_list_create(ITEM_LIST_DESTROY_ITEM destroy_cb, void* user_ctx)
    {
        if (destroy_cb != NULL)
        {
            g_list_destroy_cb = destroy_cb;
            g_destroy_user_ctx = user_ctx;
        }
        return (ITEM_LIST_HANDLE)my_mem_shim_malloc(1);
    }

    static void my_item_list_destroy(ITEM_LIST_HANDLE handle)
    {
        if (g_list_added_item != NULL && handle != g_do_not_delete_items)
        {
            g_list_destroy_cb(g_destroy_user_ctx, (void*)g_list_added_item);
            g_list_added_item = NULL;
        }
        if (g_add_copy_item != NULL)
        {
            my_mem_shim_free(g_add_copy_item);
            g_add_copy_item = NULL;
        }
        my_mem_shim_free(handle);
    }

    static int my_item_list_add_item(ITEM_LIST_HANDLE handle, const void* item)
    {
        (void)handle;
        g_list_added_item = item;
        return 0;
    }

    static const void* my_item_list_get_front(ITEM_LIST_HANDLE handle)
    {
        return g_add_copy_item;
    }

    static int my_item_list_add_copy(ITEM_LIST_HANDLE handle, const void* item, size_t item_size)
    {
        g_do_not_delete_items = handle;
        g_add_copy_item = my_mem_shim_malloc(item_size);
        memcpy(g_add_copy_item, item, item_size);
        return 0;
    }

    static int my_item_list_remove_item(ITEM_LIST_HANDLE handle, size_t remove_index)
    {
        if (handle != g_do_not_delete_items)
        {
            g_list_destroy_cb(g_destroy_user_ctx, (void*)g_list_added_item);
            g_list_added_item = NULL;
        }
        else
        {
            if (g_add_copy_item != NULL)
            {
                my_mem_shim_free(g_add_copy_item);
                g_add_copy_item = NULL;
            }
        }
        return 0;
    }

    static const void* my_item_list_get_item(ITEM_LIST_HANDLE handle, size_t item_index)
    {
        (void)handle;
        return g_list_added_item;
    }

    static int my_clone_string(char** target, const char* source)
    {
        size_t len = strlen(source);
        *target = my_mem_shim_malloc(len+1);
        strcpy(*target, source);
        return 0;
    }

    static int my_xio_client_open(XIO_INSTANCE_HANDLE xio, const XIO_CLIENT_CALLBACK_INFO* client_callbacks)
    {
        g_on_open_complete = client_callbacks->on_io_open_complete;
        g_open_user_ctx = client_callbacks->on_io_open_complete_ctx;
        return 0;
    }

    static int my_xio_client_close(XIO_INSTANCE_HANDLE xio, ON_IO_CLOSE_COMPLETE on_io_close_complete, void* callback_context)
    {
        g_on_io_close_complete = on_io_close_complete;
        g_on_close_user_ctx = callback_context;
        return 0;
    }

    static HTTP_HEADERS_HANDLE my_http_header_create(void)
    {
        return (HTTP_HEADERS_HANDLE)my_mem_shim_malloc(1);
    }

    static HTTP_CODEC_HANDLE my_http_codec_create(ON_HTTP_DATA_CALLBACK data_callback, void* user_ctx)
    {
        g_data_callback = data_callback;
        data_cb_user_ctx = user_ctx;
        return (HTTP_CODEC_HANDLE)my_mem_shim_malloc(1);
    }

    static void my_http_codec_destroy(HTTP_CODEC_HANDLE handle)
    {
        my_mem_shim_free(handle);
    }
#ifdef __cplusplus
}
#endif

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)
static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    CTEST_ASSERT_FAIL("umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
}

CTEST_BEGIN_TEST_SUITE(http_client_ut)

CTEST_SUITE_INITIALIZE()
{
    umock_c_init(on_umock_c_error);

    CTEST_ASSERT_ARE_EQUAL(int, 0, umocktypes_bool_register_types());

    REGISTER_UMOCK_ALIAS_TYPE(ITEM_LIST_DESTROY_ITEM, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTP_HEADERS_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ITEM_LIST_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTP_CLIENT_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(XIO_INSTANCE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_IO_OPEN_COMPLETE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_IO_CLOSE_COMPLETE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_BYTES_RECEIVED, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_IO_ERROR, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_SEND_COMPLETE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_HTTP_DATA_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTP_CODEC_HANDLE, void*);

    REGISTER_GLOBAL_MOCK_HOOK(mem_shim_malloc, my_mem_shim_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mem_shim_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(mem_shim_free, my_mem_shim_free);

    REGISTER_GLOBAL_MOCK_HOOK(item_list_create, my_item_list_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(item_list_create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(item_list_destroy, my_item_list_destroy);
    //REGISTER_GLOBAL_MOCK_HOOK(item_destroy_callback, my_item_destroy_cb);
    REGISTER_GLOBAL_MOCK_HOOK(item_list_add_item, my_item_list_add_item);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(item_list_add_item, __LINE__);
    REGISTER_GLOBAL_MOCK_HOOK(item_list_add_copy, my_item_list_add_copy);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(item_list_add_copy, __LINE__);
    REGISTER_GLOBAL_MOCK_HOOK(item_list_get_front, my_item_list_get_front);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(item_list_get_front, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(item_list_item_count, 0);
    REGISTER_GLOBAL_MOCK_HOOK(item_list_get_item, my_item_list_get_item);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(item_list_get_item, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(item_list_clear, 0);
    REGISTER_GLOBAL_MOCK_HOOK(item_list_remove_item, my_item_list_remove_item);

    REGISTER_GLOBAL_MOCK_HOOK(clone_string, my_clone_string);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(clone_string, __LINE__);
    REGISTER_GLOBAL_MOCK_HOOK(byte_buffer_construct, my_byte_buffer_construct);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(byte_buffer_construct, __LINE__);

    REGISTER_GLOBAL_MOCK_HOOK(xio_client_open, my_xio_client_open);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(xio_client_open, __LINE__);
    REGISTER_GLOBAL_MOCK_HOOK(xio_client_close, my_xio_client_close);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(xio_client_close, __LINE__);
    REGISTER_GLOBAL_MOCK_RETURN(xio_client_send, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(xio_client_send, __LINE__);

    REGISTER_GLOBAL_MOCK_HOOK(http_header_create, my_http_header_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(http_header_create, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(http_header_get_name_value_pair, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(http_header_get_name_value_pair, __LINE__);

    REGISTER_GLOBAL_MOCK_HOOK(http_codec_create, my_http_codec_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(http_codec_create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(http_codec_destroy, my_http_codec_destroy);
    REGISTER_GLOBAL_MOCK_RETURN(http_codec_set_trace, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(http_codec_set_trace, __LINE__);
}

CTEST_SUITE_CLEANUP()
{
    umock_c_deinit();
}

CTEST_FUNCTION_INITIALIZE()
{
    umock_c_reset_all_calls();
    g_list_added_item = NULL;
    g_on_open_complete = NULL;
    g_open_user_ctx = NULL;
    g_on_io_close_complete = NULL;
    g_on_close_user_ctx = NULL;
    g_add_copy_item = NULL;
}

CTEST_FUNCTION_CLEANUP()
{
}

static void setup_http_client_create_mocks(void)
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(http_codec_create(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(item_list_create(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(item_list_create(IGNORED_ARG, IGNORED_ARG));
}

static void setup_http_client_execute_request_mocks(bool add_content)
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(clone_string(IGNORED_ARG, IGNORED_ARG));
    if (add_content)
    {
        STRICT_EXPECTED_CALL(byte_buffer_construct(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(xio_client_query_endpoint(IGNORED_ARG, IGNORED_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(http_header_get_count(IGNORED_ARG)).SetReturn(1).CallCannotFail();
    STRICT_EXPECTED_CALL(http_header_get_name_value_pair(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CopyOutArgumentBuffer(3, &TEST_HEADER_NAME_1, sizeof(TEST_HEADER_NAME_1))
        .CopyOutArgumentBuffer(4, &TEST_HEADER_VALUE_1, sizeof(TEST_HEADER_VALUE_1));
    STRICT_EXPECTED_CALL(item_list_add_copy(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(item_list_add_item(IGNORED_ARG, IGNORED_ARG));
}

static void setup_http_client_execute_request_hostname_header_mocks(bool add_content)
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(clone_string(IGNORED_ARG, IGNORED_ARG));
    if (add_content)
    {
        STRICT_EXPECTED_CALL(byte_buffer_construct(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(xio_client_query_endpoint(IGNORED_ARG, IGNORED_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(http_header_get_count(IGNORED_ARG)).SetReturn(1).CallCannotFail();
    STRICT_EXPECTED_CALL(http_header_get_name_value_pair(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CopyOutArgumentBuffer(3, &TEST_HEADER_HOSTNAME, sizeof(TEST_HEADER_HOSTNAME))
        .CopyOutArgumentBuffer(4, &TEST_HEADER_VALUE_1, sizeof(TEST_HEADER_VALUE_1));
    STRICT_EXPECTED_CALL(item_list_add_copy(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(item_list_add_item(IGNORED_ARG, IGNORED_ARG));
}

static void setup_http_client_process_item_mocks(bool add_content)
{
    STRICT_EXPECTED_CALL(xio_client_process_item(IGNORED_ARG));
    STRICT_EXPECTED_CALL(item_list_item_count(IGNORED_ARG)).SetReturn(1).CallCannotFail();
    STRICT_EXPECTED_CALL(item_list_get_item(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(xio_client_send(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    if (add_content)
    {
        STRICT_EXPECTED_CALL(xio_client_send(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(item_list_remove_item(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
}

CTEST_FUNCTION(http_client_create_succeed)
{
    // arrange
    setup_http_client_create_mocks();

    // act
    HTTP_CLIENT_HANDLE handle = http_client_create();

    // assert
    CTEST_ASSERT_IS_NOT_NULL(handle);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_client_destroy(handle);
}

CTEST_FUNCTION(http_client_create_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    CTEST_ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    setup_http_client_create_mocks();
    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            HTTP_CLIENT_HANDLE handle = http_client_create();

            // assert
            CTEST_ASSERT_IS_NULL(handle, "http_client_create failure %d/%d", (int)index, (int)count);
        }
    }
    // cleanup
    umock_c_negative_tests_deinit();
}

CTEST_FUNCTION(http_client_destroy_handle_NULL_succeed)
{
    // arrange

    // act
    http_client_destroy(NULL);

    // assert
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

CTEST_FUNCTION(http_client_destroy_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE handle = http_client_create();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(http_codec_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(item_list_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(item_list_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    http_client_destroy(handle);

    // assert
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

CTEST_FUNCTION(http_client_open_handle_NULL_fail)
{
    // arrange

    // act
    int result = http_client_open(NULL, TEST_XIO_HANDLE, test_on_open_complete, NULL, test_on_error, NULL);

    // assert
    CTEST_ASSERT_ARE_NOT_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

CTEST_FUNCTION(http_client_open_xio_handle_NULL_fail)
{
    // arrange
    HTTP_CLIENT_HANDLE handle = http_client_create();
    umock_c_reset_all_calls();

    // act
    int result = http_client_open(handle, NULL, test_on_open_complete, NULL, test_on_error, NULL);

    // assert
    CTEST_ASSERT_ARE_NOT_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_client_destroy(handle);
}

CTEST_FUNCTION(http_client_open_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE handle = http_client_create();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(http_codec_get_recv_function());
    STRICT_EXPECTED_CALL(xio_client_open(TEST_XIO_HANDLE, IGNORED_ARG));

    // act
    int result = http_client_open(handle, TEST_XIO_HANDLE, test_on_open_complete, NULL, test_on_error, NULL);

    // assert
    CTEST_ASSERT_ARE_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    (void)http_client_close(handle, test_on_close_complete, NULL);
    http_client_destroy(handle);
}

CTEST_FUNCTION(http_client_open_previously_open_fail)
{
    // arrange
    HTTP_CLIENT_HANDLE handle = http_client_create();
    int result = http_client_open(handle, TEST_XIO_HANDLE, test_on_open_complete, NULL, test_on_error, NULL);
    umock_c_reset_all_calls();

    // act
    result = http_client_open(handle, TEST_XIO_HANDLE, test_on_open_complete, NULL, test_on_error, NULL);

    // assert
    CTEST_ASSERT_ARE_NOT_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    (void)http_client_close(handle, test_on_close_complete, NULL);
    http_client_destroy(handle);
}

CTEST_FUNCTION(http_client_open_fail)
{
    // arrange
    HTTP_CLIENT_HANDLE handle = http_client_create();
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    CTEST_ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(xio_client_open(TEST_XIO_HANDLE, IGNORED_ARG));
    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            int result = http_client_open(handle, TEST_XIO_HANDLE, test_on_open_complete, NULL, test_on_error, NULL);

            // assert
            CTEST_ASSERT_ARE_NOT_EQUAL(int, 0, result, "http_client_create failure %d/%d", (int)index, (int)count);
        }
    }

    // cleanup
    http_client_destroy(handle);
    umock_c_negative_tests_deinit();
}

CTEST_FUNCTION(http_client_close_handle_NULL)
{
    // arrange

    // act
    int result = http_client_close(NULL, test_on_close_complete, NULL);

    // assert
    CTEST_ASSERT_ARE_NOT_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

CTEST_FUNCTION(http_client_close_state_open_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE handle = http_client_create();
    (void)http_client_open(handle, TEST_XIO_HANDLE, test_on_open_complete, NULL, test_on_error, NULL);
    g_on_open_complete(g_open_user_ctx, IO_OPEN_OK);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(xio_client_close(TEST_XIO_HANDLE, IGNORED_ARG, IGNORED_ARG));

    // act
    int result = http_client_close(handle, test_on_close_complete, NULL);

    // assert
    CTEST_ASSERT_ARE_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_client_destroy(handle);
}

CTEST_FUNCTION(http_client_close_state_opening_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE handle = http_client_create();
    (void)http_client_open(handle, TEST_XIO_HANDLE, test_on_open_complete, NULL, test_on_error, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(xio_client_close(TEST_XIO_HANDLE, IGNORED_ARG, IGNORED_ARG));

    // act
    int result = http_client_close(handle, test_on_close_complete, NULL);

    // assert
    CTEST_ASSERT_ARE_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_client_destroy(handle);
}

CTEST_FUNCTION(http_client_close_on_error_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE handle = http_client_create();
    (void)http_client_open(handle, TEST_XIO_HANDLE, test_on_open_complete, NULL, test_on_error, NULL);
    STRICT_EXPECTED_CALL(xio_client_close(TEST_XIO_HANDLE, IGNORED_ARG, IGNORED_ARG)).SetReturn(__LINE__);
    http_client_close(handle, test_on_close_complete, NULL);
    umock_c_reset_all_calls();

    // act
    int result = http_client_close(handle, test_on_close_complete, NULL);

    // assert
    CTEST_ASSERT_ARE_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_client_destroy(handle);
}

CTEST_FUNCTION(http_client_execute_request_handle_NULL_fail)
{
    // arrange

    // act
    int result = http_client_execute_request(NULL, HTTP_CLIENT_REQUEST_POST, TEST_RELATIVE_PATH, TEST_HTTP_HEADER, NULL, 0, test_on_request_callback, NULL);

    // assert
    CTEST_ASSERT_ARE_NOT_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

CTEST_FUNCTION(http_client_execute_request_NULL_content_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE handle = http_client_create();
    (void)http_client_open(handle, TEST_XIO_HANDLE, test_on_open_complete, NULL, test_on_error, NULL);
    umock_c_reset_all_calls();

    setup_http_client_execute_request_mocks(false);

    // act
    int result = http_client_execute_request(handle, HTTP_CLIENT_REQUEST_POST, TEST_RELATIVE_PATH, TEST_HTTP_HEADER, NULL, 0, test_on_request_callback, NULL);

    // assert
    CTEST_ASSERT_ARE_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    (void)http_client_close(handle, test_on_close_complete, NULL);
    http_client_destroy(handle);
}

CTEST_FUNCTION(http_client_execute_request_hostname_header_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE handle = http_client_create();
    (void)http_client_open(handle, TEST_XIO_HANDLE, test_on_open_complete, NULL, test_on_error, NULL);
    umock_c_reset_all_calls();

    setup_http_client_execute_request_hostname_header_mocks(false);

    // act
    int result = http_client_execute_request(handle, HTTP_CLIENT_REQUEST_POST, TEST_RELATIVE_PATH, TEST_HTTP_HEADER, NULL, 0, test_on_request_callback, NULL);

    // assert
    CTEST_ASSERT_ARE_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    (void)http_client_close(handle, test_on_close_complete, NULL);
    http_client_destroy(handle);
}

CTEST_FUNCTION(http_client_execute_request_content_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE handle = http_client_create();
    (void)http_client_open(handle, TEST_XIO_HANDLE, test_on_open_complete, NULL, test_on_error, NULL);
    umock_c_reset_all_calls();

    setup_http_client_execute_request_mocks(true);

    // act
    int result = http_client_execute_request(handle, HTTP_CLIENT_REQUEST_POST, TEST_RELATIVE_PATH, TEST_HTTP_HEADER, TEST_SEND_CONTENT, TEST_CONTENT_LENGTH, test_on_request_callback, NULL);

    // assert
    CTEST_ASSERT_ARE_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    (void)http_client_close(handle, test_on_close_complete, NULL);
    http_client_destroy(handle);
}

CTEST_FUNCTION(http_client_execute_request_content_patch_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE handle = http_client_create();
    (void)http_client_open(handle, TEST_XIO_HANDLE, test_on_open_complete, NULL, test_on_error, NULL);
    umock_c_reset_all_calls();

    setup_http_client_execute_request_mocks(true);

    // act
    int result = http_client_execute_request(handle, HTTP_CLIENT_REQUEST_PATCH, TEST_RELATIVE_PATH, TEST_HTTP_HEADER, TEST_SEND_CONTENT, TEST_CONTENT_LENGTH, test_on_request_callback, NULL);

    // assert
    CTEST_ASSERT_ARE_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    (void)http_client_close(handle, test_on_close_complete, NULL);
    http_client_destroy(handle);
}

CTEST_FUNCTION(http_client_execute_request_content_delete_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE handle = http_client_create();
    (void)http_client_open(handle, TEST_XIO_HANDLE, test_on_open_complete, NULL, test_on_error, NULL);
    umock_c_reset_all_calls();

    setup_http_client_execute_request_mocks(true);

    // act
    int result = http_client_execute_request(handle, HTTP_CLIENT_REQUEST_DELETE, TEST_RELATIVE_PATH, TEST_HTTP_HEADER, TEST_SEND_CONTENT, TEST_CONTENT_LENGTH, test_on_request_callback, NULL);

    // assert
    CTEST_ASSERT_ARE_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    (void)http_client_close(handle, test_on_close_complete, NULL);
    http_client_destroy(handle);
}

CTEST_FUNCTION(http_client_execute_request_fail)
{
    // arrange
    HTTP_CLIENT_HANDLE handle = http_client_create();
    (void)http_client_open(handle, TEST_XIO_HANDLE, test_on_open_complete, NULL, test_on_error, NULL);
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    CTEST_ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    setup_http_client_execute_request_mocks(false);
    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            int result = http_client_execute_request(handle, HTTP_CLIENT_REQUEST_POST, TEST_RELATIVE_PATH, TEST_HTTP_HEADER, NULL, 0, test_on_request_callback, NULL);

            // assert
            CTEST_ASSERT_ARE_NOT_EQUAL(int, 0, result, "http_client_create failure %d/%d", (int)index, (int)count);
        }
    }
    // cleanup
    umock_c_negative_tests_deinit();

    (void)http_client_close(handle, test_on_close_complete, NULL);
    http_client_destroy(handle);
}

CTEST_FUNCTION(http_client_execute_request_w_content_fail)
{
    // arrange
    HTTP_CLIENT_HANDLE handle = http_client_create();
    (void)http_client_open(handle, TEST_XIO_HANDLE, test_on_open_complete, NULL, test_on_error, NULL);
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    CTEST_ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    setup_http_client_execute_request_mocks(true);
    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            int result = http_client_execute_request(handle, HTTP_CLIENT_REQUEST_POST, TEST_RELATIVE_PATH, TEST_HTTP_HEADER, TEST_SEND_CONTENT, TEST_CONTENT_LENGTH, test_on_request_callback, NULL);

            // assert
            CTEST_ASSERT_ARE_NOT_EQUAL(int, 0, result, "http_client_create failure %d/%d", (int)index, (int)count);
        }
    }
    // cleanup
    umock_c_negative_tests_deinit();

    (void)http_client_close(handle, test_on_close_complete, NULL);
    http_client_destroy(handle);
}

CTEST_FUNCTION(http_client_process_item_opening_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE handle = http_client_create();
    (void)http_client_open(handle, TEST_XIO_HANDLE, test_on_open_complete, NULL, test_on_error, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(xio_client_process_item(IGNORED_ARG));

    // act
    http_client_process_item(handle);

    // assert
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    (void)http_client_close(handle, test_on_close_complete, NULL);
    http_client_destroy(handle);
}

CTEST_FUNCTION(http_client_process_item_handle_succeed)
{
    // arrange

    // act
    http_client_process_item(NULL);

    // assert
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

CTEST_FUNCTION(http_client_process_item_no_items_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE handle = http_client_create();
    (void)http_client_open(handle, TEST_XIO_HANDLE, test_on_open_complete, NULL, test_on_error, NULL);
    g_on_open_complete(g_open_user_ctx, IO_OPEN_OK);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(xio_client_process_item(IGNORED_ARG));
    STRICT_EXPECTED_CALL(item_list_item_count(IGNORED_ARG));

    // act
    http_client_process_item(handle);

    // assert
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    (void)http_client_close(handle, test_on_close_complete, NULL);
    http_client_destroy(handle);
}

CTEST_FUNCTION(http_client_process_item_open_post_fail)
{
    // arrange
    HTTP_CLIENT_HANDLE handle = http_client_create();
    (void)http_client_open(handle, TEST_XIO_HANDLE, test_on_open_complete, NULL, test_on_error, NULL);
    g_on_open_complete(g_open_user_ctx, IO_OPEN_OK);
    int result = http_client_execute_request(handle, HTTP_CLIENT_REQUEST_POST, TEST_RELATIVE_PATH, TEST_HTTP_HEADER, TEST_SEND_CONTENT, TEST_CONTENT_LENGTH, test_on_request_callback, NULL);
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    CTEST_ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    setup_http_client_process_item_mocks(true);

    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            // act
            http_client_process_item(handle);

            // assert
        }
    }

    // cleanup
    (void)http_client_close(handle, test_on_close_complete, NULL);
    http_client_destroy(handle);
    umock_c_negative_tests_deinit();
}

CTEST_FUNCTION(http_client_process_item_open_post_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE handle = http_client_create();
    (void)http_client_open(handle, TEST_XIO_HANDLE, test_on_open_complete, NULL, test_on_error, NULL);
    g_on_open_complete(g_open_user_ctx, IO_OPEN_OK);
    int result = http_client_execute_request(handle, HTTP_CLIENT_REQUEST_POST, TEST_RELATIVE_PATH, TEST_HTTP_HEADER, TEST_SEND_CONTENT, TEST_CONTENT_LENGTH, test_on_request_callback, NULL);
    umock_c_reset_all_calls();

    setup_http_client_process_item_mocks(true);

    // act
    http_client_process_item(handle);

    // assert
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    (void)http_client_close(handle, test_on_close_complete, NULL);
    http_client_destroy(handle);
}

CTEST_FUNCTION(http_client_process_item_open_get_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE handle = http_client_create();
    (void)http_client_open(handle, TEST_XIO_HANDLE, test_on_open_complete, NULL, test_on_error, NULL);
    g_on_open_complete(g_open_user_ctx, IO_OPEN_OK);
    int result = http_client_execute_request(handle, HTTP_CLIENT_REQUEST_GET, TEST_RELATIVE_PATH, TEST_HTTP_HEADER, NULL, 0, test_on_request_callback, NULL);
    umock_c_reset_all_calls();

    setup_http_client_process_item_mocks(false);

    // act
    http_client_process_item(handle);

    // assert
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    (void)http_client_close(handle, test_on_close_complete, NULL);
    http_client_destroy(handle);
}

CTEST_FUNCTION(http_client_process_item_open_option_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE handle = http_client_create();
    (void)http_client_open(handle, TEST_XIO_HANDLE, test_on_open_complete, NULL, test_on_error, NULL);
    g_on_open_complete(g_open_user_ctx, IO_OPEN_OK);
    int result = http_client_execute_request(handle, HTTP_CLIENT_REQUEST_OPTIONS, TEST_RELATIVE_PATH, TEST_HTTP_HEADER, NULL, 0, test_on_request_callback, NULL);
    umock_c_reset_all_calls();

    setup_http_client_process_item_mocks(false);

    // act
    http_client_process_item(handle);

    // assert
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    (void)http_client_close(handle, test_on_close_complete, NULL);
    http_client_destroy(handle);
}

CTEST_FUNCTION(http_client_process_item_open_put_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE handle = http_client_create();
    (void)http_client_open(handle, TEST_XIO_HANDLE, test_on_open_complete, NULL, test_on_error, NULL);
    g_on_open_complete(g_open_user_ctx, IO_OPEN_OK);
    int result = http_client_execute_request(handle, HTTP_CLIENT_REQUEST_PUT, TEST_RELATIVE_PATH, TEST_HTTP_HEADER, NULL, 0, test_on_request_callback, NULL);
    umock_c_reset_all_calls();

    setup_http_client_process_item_mocks(false);

    // act
    http_client_process_item(handle);

    // assert
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    (void)http_client_close(handle, test_on_close_complete, NULL);
    http_client_destroy(handle);
}

CTEST_FUNCTION(http_client_process_item_open_get_item_fail_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE handle = http_client_create();
    (void)http_client_open(handle, TEST_XIO_HANDLE, test_on_open_complete, NULL, test_on_error, NULL);
    g_on_open_complete(g_open_user_ctx, IO_OPEN_OK);
    int result = http_client_execute_request(handle, HTTP_CLIENT_REQUEST_POST, TEST_RELATIVE_PATH, TEST_HTTP_HEADER, NULL, 0, test_on_request_callback, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(xio_client_process_item(IGNORED_ARG));
    STRICT_EXPECTED_CALL(item_list_item_count(IGNORED_ARG)).SetReturn(1);
    STRICT_EXPECTED_CALL(item_list_get_item(IGNORED_ARG, IGNORED_ARG)).SetReturn(NULL);

    // act
    http_client_process_item(handle);
    http_client_process_item(handle);

    // assert
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    (void)http_client_close(handle, test_on_close_complete, NULL);
    http_client_destroy(handle);
}

CTEST_FUNCTION(http_client_process_item_not_connected_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE handle = http_client_create();
    umock_c_reset_all_calls();

    // act
    http_client_process_item(handle);

    // assert
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_client_destroy(handle);
}

CTEST_FUNCTION(http_client_set_trace_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE handle = http_client_create();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(http_codec_set_trace(IGNORED_ARG, true));

    // act
    int result = http_client_set_trace(handle, true);

    // assert
    CTEST_ASSERT_ARE_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_client_destroy(handle);
}

CTEST_FUNCTION(http_client_set_trace_fail)
{
    // arrange
    HTTP_CLIENT_HANDLE handle = http_client_create();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(http_codec_set_trace(IGNORED_ARG, true)).SetReturn(__LINE__);

    // act
    int result = http_client_set_trace(handle, true);

    // assert
    CTEST_ASSERT_ARE_NOT_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_client_destroy(handle);
}

CTEST_FUNCTION(http_client_set_trace_handle_NULL_fail)
{
    // arrange

    // act
    int result = http_client_set_trace(NULL, true);

    // assert
    CTEST_ASSERT_ARE_NOT_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

CTEST_FUNCTION(on_codec_recv_callback_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE handle = http_client_create();
    (void)http_client_open(handle, TEST_XIO_HANDLE, test_on_open_complete, NULL, test_on_error, NULL);
    g_on_open_complete(g_open_user_ctx, IO_OPEN_OK);
    int result = http_client_execute_request(handle, HTTP_CLIENT_REQUEST_GET, TEST_RELATIVE_PATH, TEST_HTTP_HEADER, NULL, 0, test_on_request_callback, NULL);
    umock_c_reset_all_calls();

    HTTP_RECV_DATA recv_data;
    recv_data.status_code = 201;
    recv_data.recv_header = TEST_HTTP_HEADER;
    recv_data.http_content = g_buffer_data;

    STRICT_EXPECTED_CALL(item_list_get_front(IGNORED_ARG)).SetReturn(g_add_copy_item);
    //STRICT_EXPECTED_CALL(test_on_request_callback(IGNORED_ARG, HTTP_CLIENT_OK, IGNORED_ARG, IGNORED_ARG, recv_data.status_code, recv_data.recv_header));

    // act
    g_data_callback(data_cb_user_ctx, HTTP_CODEC_CB_RESULT_OK, &recv_data);

    // assert
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    (void)http_client_close(handle, test_on_close_complete, NULL);
    http_client_destroy(handle);
}

CTEST_FUNCTION(on_codec_recv_callback_get_front_NULL_fail)
{
    // arrange
    HTTP_CLIENT_HANDLE handle = http_client_create();
    (void)http_client_open(handle, TEST_XIO_HANDLE, test_on_open_complete, NULL, test_on_error, NULL);
    g_on_open_complete(g_open_user_ctx, IO_OPEN_OK);
    int result = http_client_execute_request(handle, HTTP_CLIENT_REQUEST_GET, TEST_RELATIVE_PATH, TEST_HTTP_HEADER, NULL, 0, test_on_request_callback, NULL);
    umock_c_reset_all_calls();

    HTTP_RECV_DATA recv_data;
    recv_data.status_code = 201;
    recv_data.recv_header = TEST_HTTP_HEADER;
    recv_data.http_content = g_buffer_data;

    STRICT_EXPECTED_CALL(item_list_get_front(IGNORED_ARG)).SetReturn(NULL);

    // act
    g_data_callback(data_cb_user_ctx, HTTP_CODEC_CB_RESULT_OK, &recv_data);

    // assert
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    (void)http_client_close(handle, test_on_close_complete, NULL);
    http_client_destroy(handle);
}

CTEST_END_TEST_SUITE(http_client_ut)
