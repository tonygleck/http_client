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
#undef ENABLE_MOCKS

#include "http_client/http_headers.h"

static const char* TEST_HEADER_NAME_1 = "TEST_HEADER_NAME_1";
static const char* TEST_HEADER_VALUE_1 = "TEST_HEADER_VALUE_1";
static const char* TEST_HEADER_NAME_2 = "TEST_HEADER_NAME_2";
static const char* TEST_HEADER_VALUE_2 = "TEST_HEADER_VALUE_2";
static const char* TEST_HEADER_NAME_3 = "TEST_HEADER_NAME_3";
static const char* TEST_HEADER_VALUE_3 = "TEST_HEADER_VALUE_3";

static size_t PARTIAL_TEST_HEADER_LEN = 16;
static size_t PARTIAL_TEST_HEADER_VAL_LEN = 17;

static ITEM_LIST_DESTROY_ITEM g_list_destroy_cb;
static void* g_destroy_user_ctx;
static const void* g_list_added_item;

#ifdef __cplusplus
extern "C" {
#endif

    static ITEM_LIST_HANDLE my_item_list_create(ITEM_LIST_DESTROY_ITEM destroy_cb, void* user_ctx)
    {
        g_list_destroy_cb = destroy_cb;
        g_destroy_user_ctx = user_ctx;
        return my_mem_shim_malloc(1);
    }

    static void my_item_list_destroy(ITEM_LIST_HANDLE handle)
    {
        if (g_list_added_item != NULL)
        {
            g_list_destroy_cb(g_destroy_user_ctx, (void*)g_list_added_item);
        }
        my_mem_shim_free(handle);
    }

    static int my_item_list_add_item(ITEM_LIST_HANDLE handle, const void* item)
    {
        (void)handle;
        g_list_added_item = item;
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

    static int my_clone_string_with_size(char** target, const char* source, size_t len)
    {
        *target = my_mem_shim_malloc(len+1);
        strncpy(*target, source, len);
        return 0;
    }

#ifdef __cplusplus
}
#endif

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)
static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    CTEST_ASSERT_FAIL("umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
}

CTEST_BEGIN_TEST_SUITE(http_headers_ut)

CTEST_SUITE_INITIALIZE()
{
    umock_c_init(on_umock_c_error);

    REGISTER_UMOCK_ALIAS_TYPE(ITEM_LIST_DESTROY_ITEM, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTP_HEADERS_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ITEM_LIST_HANDLE, void*);

    REGISTER_GLOBAL_MOCK_HOOK(mem_shim_malloc, my_mem_shim_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mem_shim_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(mem_shim_free, my_mem_shim_free);

    REGISTER_GLOBAL_MOCK_HOOK(item_list_create, my_item_list_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(item_list_create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(item_list_destroy, my_item_list_destroy);
    //REGISTER_GLOBAL_MOCK_HOOK(item_destroy_callback, my_item_destroy_cb);
    REGISTER_GLOBAL_MOCK_HOOK(item_list_add_item, my_item_list_add_item);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(item_list_add_item, __LINE__);
    REGISTER_GLOBAL_MOCK_RETURN(item_list_item_count, 0);
    REGISTER_GLOBAL_MOCK_HOOK(item_list_get_item, my_item_list_get_item);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(item_list_get_item, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(item_list_clear, 0);

    REGISTER_GLOBAL_MOCK_HOOK(clone_string, my_clone_string);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(clone_string, __LINE__);
    REGISTER_GLOBAL_MOCK_HOOK(clone_string_with_size, my_clone_string_with_size);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(clone_string_with_size, __LINE__);
}

CTEST_SUITE_CLEANUP()
{
    umock_c_deinit();
}

CTEST_FUNCTION_INITIALIZE()
{
    umock_c_reset_all_calls();
    g_list_added_item = NULL;
}

CTEST_FUNCTION_CLEANUP()
{
}

static void http_header_create_mocks(void)
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(item_list_create(IGNORED_ARG, IGNORED_ARG));
}

static void setup_http_header_add_mocks(void)
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(clone_string(IGNORED_ARG, TEST_HEADER_NAME_1));
    STRICT_EXPECTED_CALL(clone_string(IGNORED_ARG, TEST_HEADER_VALUE_1));
    STRICT_EXPECTED_CALL(item_list_add_item(IGNORED_ARG, IGNORED_ARG));
}

static void setup_http_header_add_partial_mocks(void)
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(clone_string_with_size(IGNORED_ARG, TEST_HEADER_NAME_1, PARTIAL_TEST_HEADER_LEN));
    STRICT_EXPECTED_CALL(clone_string_with_size(IGNORED_ARG, TEST_HEADER_VALUE_1, PARTIAL_TEST_HEADER_VAL_LEN));
    STRICT_EXPECTED_CALL(item_list_add_item(IGNORED_ARG, IGNORED_ARG));
}

CTEST_FUNCTION(http_header_create_succeed)
{
    // arrange
    http_header_create_mocks();

    // act
    HTTP_HEADERS_HANDLE handle = http_header_create();

    // assert
    CTEST_ASSERT_IS_NOT_NULL(handle);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_header_destroy(handle);
}

CTEST_FUNCTION(http_header_create_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    CTEST_ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    http_header_create_mocks();
    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            HTTP_HEADERS_HANDLE handle = http_header_create();

            // assert
            CTEST_ASSERT_IS_NULL(handle, "http_header_create failure %d/%d", (int)index, (int)count);
        }
    }
    // cleanup
    umock_c_negative_tests_deinit();
}

CTEST_FUNCTION(http_header_destroy_succeed)
{
    // arrange
    HTTP_HEADERS_HANDLE handle = http_header_create();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(item_list_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    http_header_destroy(handle);

    // assert
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

CTEST_FUNCTION(http_header_destroy_handle_NULL_succeed)
{
    // arrange

    // act
    http_header_destroy(NULL);

    // assert
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

CTEST_FUNCTION(http_header_add_handle_NULL_fail)
{
    // arrange

    // act
    int result = http_header_add(NULL, TEST_HEADER_NAME_1, TEST_HEADER_VALUE_1);

    // assert
    CTEST_ASSERT_ARE_NOT_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

CTEST_FUNCTION(http_header_add_name_NULL_fail)
{
    // arrange
    HTTP_HEADERS_HANDLE handle = http_header_create();
    umock_c_reset_all_calls();

    // act
    int result = http_header_add(handle, NULL, TEST_HEADER_VALUE_1);

    // assert
    CTEST_ASSERT_ARE_NOT_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_header_destroy(handle);
}

CTEST_FUNCTION(http_header_add_value_NULL_fail)
{
    // arrange
    HTTP_HEADERS_HANDLE handle = http_header_create();
    umock_c_reset_all_calls();

    // act
    int result = http_header_add(handle, TEST_HEADER_NAME_1, NULL);

    // assert
    CTEST_ASSERT_ARE_NOT_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_header_destroy(handle);
}

CTEST_FUNCTION(http_header_add_fail)
{
    // arrange
    HTTP_HEADERS_HANDLE handle = http_header_create();
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    CTEST_ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    setup_http_header_add_mocks();
    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            int result = http_header_add(handle, TEST_HEADER_NAME_1, TEST_HEADER_VALUE_1);

            // assert
            CTEST_ASSERT_ARE_NOT_EQUAL(int, 0, result);
        }
    }
    // cleanup
    umock_c_negative_tests_deinit();
    http_header_destroy(handle);
}

CTEST_FUNCTION(http_header_add_succeed)
{
    // arrange
    HTTP_HEADERS_HANDLE handle = http_header_create();
    umock_c_reset_all_calls();

    setup_http_header_add_mocks();

    // act
    int result = http_header_add(handle, TEST_HEADER_NAME_1, TEST_HEADER_VALUE_1);

    // assert
    CTEST_ASSERT_ARE_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_header_destroy(handle);
}

CTEST_FUNCTION(http_header_add_partial_handle_NULL_fail)
{
    // arrange

    // act
    int result = http_header_add_partial(NULL, TEST_HEADER_NAME_1, PARTIAL_TEST_HEADER_LEN, TEST_HEADER_VALUE_1, PARTIAL_TEST_HEADER_VAL_LEN);

    // assert
    CTEST_ASSERT_ARE_NOT_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

CTEST_FUNCTION(http_header_add_partial_succeed)
{
    // arrange
    HTTP_HEADERS_HANDLE handle = http_header_create();
    umock_c_reset_all_calls();

    setup_http_header_add_partial_mocks();

    // act
    int result = http_header_add_partial(handle, TEST_HEADER_NAME_1, PARTIAL_TEST_HEADER_LEN, TEST_HEADER_VALUE_1, PARTIAL_TEST_HEADER_VAL_LEN);

    // assert
    CTEST_ASSERT_ARE_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_header_destroy(handle);
}

CTEST_FUNCTION(http_header_add_partial_fail)
{
    // arrange
    HTTP_HEADERS_HANDLE handle = http_header_create();
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    CTEST_ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    setup_http_header_add_partial_mocks();
    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            int result = http_header_add_partial(handle, TEST_HEADER_NAME_1, PARTIAL_TEST_HEADER_LEN, TEST_HEADER_VALUE_1, PARTIAL_TEST_HEADER_VAL_LEN);

            // assert
            CTEST_ASSERT_ARE_NOT_EQUAL(int, 0, result, "http_header_add_partial failure%d/%d", (int)index, (int)count);
        }
    }

    // cleanup
    http_header_destroy(handle);
    umock_c_negative_tests_deinit();
}

CTEST_FUNCTION(http_header_remove_no_items_succeed)
{
    // arrange
    HTTP_HEADERS_HANDLE handle = http_header_create();
    int result = http_header_add(handle, TEST_HEADER_NAME_1, TEST_HEADER_VALUE_1);
    CTEST_ASSERT_ARE_EQUAL(int, 0, result);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(item_list_item_count(IGNORED_ARG));

    // act
    result = http_header_remove(handle, TEST_HEADER_NAME_1);

    // assert
    CTEST_ASSERT_ARE_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_header_destroy(handle);
}

CTEST_FUNCTION(http_header_remove_items_handle_NULL_fail)
{
    // arrange

    // act
    int result = http_header_remove(NULL, TEST_HEADER_NAME_1);

    // assert
    CTEST_ASSERT_ARE_NOT_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

CTEST_FUNCTION(http_header_remove_items_succeed)
{
    // arrange
    HTTP_HEADERS_HANDLE handle = http_header_create();
    int result = http_header_add(handle, TEST_HEADER_NAME_1, TEST_HEADER_VALUE_1);
    CTEST_ASSERT_ARE_EQUAL(int, 0, result);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(item_list_item_count(IGNORED_ARG)).SetReturn(1);
    STRICT_EXPECTED_CALL(item_list_get_item(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(item_list_remove_item(IGNORED_ARG, 0));

    // act
    result = http_header_remove(handle, TEST_HEADER_NAME_1);

    // assert
    CTEST_ASSERT_ARE_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_header_destroy(handle);
}

CTEST_FUNCTION(http_header_get_value_handle_NULL_fail)
{
    // arrange

    // act
    const char* result = http_header_get_value(NULL, TEST_HEADER_NAME_1);

    // assert
    CTEST_ASSERT_IS_NULL(result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

CTEST_FUNCTION(http_header_get_value_name_NULL_fail)
{
    // arrange
    HTTP_HEADERS_HANDLE handle = http_header_create();
    umock_c_reset_all_calls();

    // act
    const char* result = http_header_get_value(handle, NULL);

    // assert
    CTEST_ASSERT_IS_NULL(result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_header_destroy(handle);
}

CTEST_FUNCTION(http_header_get_value_succeed)
{
    // arrange
    HTTP_HEADERS_HANDLE handle = http_header_create();
    CTEST_ASSERT_ARE_EQUAL(int, 0, http_header_add(handle, TEST_HEADER_NAME_1, TEST_HEADER_VALUE_1));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(item_list_item_count(IGNORED_ARG)).SetReturn(1);
    STRICT_EXPECTED_CALL(item_list_get_item(IGNORED_ARG, 0));

    // act
    const char* result = http_header_get_value(handle, TEST_HEADER_NAME_1);

    // assert
    CTEST_ASSERT_ARE_EQUAL(char_ptr, TEST_HEADER_VALUE_1, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_header_destroy(handle);
}

CTEST_FUNCTION(http_header_get_value_2_loop_succeed)
{
    // arrange
    HTTP_HEADERS_HANDLE handle = http_header_create();
    CTEST_ASSERT_ARE_EQUAL(int, 0, http_header_add(handle, TEST_HEADER_NAME_1, TEST_HEADER_VALUE_1));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(item_list_item_count(IGNORED_ARG)).SetReturn(2);
    STRICT_EXPECTED_CALL(item_list_get_item(IGNORED_ARG, 0)).SetReturn(NULL);
    STRICT_EXPECTED_CALL(item_list_get_item(IGNORED_ARG, 1));

    // act
    const char* result = http_header_get_value(handle, TEST_HEADER_NAME_1);

    // assert
    CTEST_ASSERT_ARE_EQUAL(char_ptr, TEST_HEADER_VALUE_1, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_header_destroy(handle);
}

CTEST_FUNCTION(http_header_get_count_handle_NULL_fail)
{
    // arrange
    HTTP_HEADERS_HANDLE handle = http_header_create();
    umock_c_reset_all_calls();

    // act
    size_t result = http_header_get_count(NULL);

    // assert
    CTEST_ASSERT_ARE_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_header_destroy(handle);
}

CTEST_FUNCTION(http_header_get_count_succeed)
{
    // arrange
    HTTP_HEADERS_HANDLE handle = http_header_create();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(item_list_item_count(IGNORED_ARG)).SetReturn(1);

    // act
    size_t result = http_header_get_count(handle);

    // assert
    CTEST_ASSERT_ARE_EQUAL(int, 1, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_header_destroy(handle);
}

CTEST_FUNCTION(http_header_get_name_value_pair_handle_NULL_fail)
{
    // arrange

    // act
    const char* name;
    const char* value;
    int result = http_header_get_name_value_pair(NULL, 0, &name, &value);

    // assert
    CTEST_ASSERT_ARE_NOT_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

CTEST_FUNCTION(http_header_get_name_value_pair_name_NULL_fail)
{
    // arrange
    HTTP_HEADERS_HANDLE handle = http_header_create();
    umock_c_reset_all_calls();

    // act
    const char* value;
    int result = http_header_get_name_value_pair(handle, 0, NULL, &value);

    // assert
    CTEST_ASSERT_ARE_NOT_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_header_destroy(handle);
}

CTEST_FUNCTION(http_header_get_name_value_pair_value_NULL_fail)
{
    // arrange
    HTTP_HEADERS_HANDLE handle = http_header_create();
    CTEST_ASSERT_ARE_EQUAL(int, 0, http_header_add(handle, TEST_HEADER_NAME_1, TEST_HEADER_VALUE_1));
    umock_c_reset_all_calls();

    // act
    const char* name;
    int result = http_header_get_name_value_pair(handle, 0, &name, NULL);

    // assert
    CTEST_ASSERT_ARE_NOT_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_header_destroy(handle);
}

CTEST_FUNCTION(http_header_get_name_value_pair_succeed)
{
    // arrange
    HTTP_HEADERS_HANDLE handle = http_header_create();
    CTEST_ASSERT_ARE_EQUAL(int, 0, http_header_add(handle, TEST_HEADER_NAME_1, TEST_HEADER_VALUE_1));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(item_list_get_item(IGNORED_ARG, 0));

    // act
    const char* name;
    const char* value;
    int result = http_header_get_name_value_pair(handle, 0, &name, &value);

    // assert
    CTEST_ASSERT_ARE_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, TEST_HEADER_NAME_1, name);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, TEST_HEADER_VALUE_1, value);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_header_destroy(handle);
}

CTEST_FUNCTION(http_header_get_name_value_pair_get_item_fail)
{
    // arrange
    HTTP_HEADERS_HANDLE handle = http_header_create();
    CTEST_ASSERT_ARE_EQUAL(int, 0, http_header_add(handle, TEST_HEADER_NAME_1, TEST_HEADER_VALUE_1));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(item_list_get_item(IGNORED_ARG, 0)).SetReturn(NULL);

    // act
    const char* name;
    const char* value;
    int result = http_header_get_name_value_pair(handle, 0, &name, &value);

    // assert
    CTEST_ASSERT_ARE_NOT_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_header_destroy(handle);
}

CTEST_FUNCTION(http_header_clear_handle_NULL_fail)
{
    // arrange

    // act
    int result = http_header_clear(NULL);

    // assert
    CTEST_ASSERT_ARE_NOT_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

CTEST_FUNCTION(http_header_clear_succeed)
{
    // arrange
    HTTP_HEADERS_HANDLE handle = http_header_create();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(item_list_clear(IGNORED_ARG));

    // act
    int result = http_header_clear(handle);

    // assert
    CTEST_ASSERT_ARE_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_header_destroy(handle);
}

CTEST_END_TEST_SUITE(http_headers_ut)
