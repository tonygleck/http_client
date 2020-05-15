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

static void* my_mem_shim_realloc(void* ptr, size_t size)
{
    return realloc(ptr, size);
}

static void my_mem_shim_free(void* ptr)
{
    free(ptr);
}

#define ENABLE_MOCKS
#include "umock_c/umock_c_prod.h"
#include "lib-util-c/sys_debug_shim.h"
#include "lib-util-c/buffer_alloc.h"
#include "http_client/http_headers.h"
#undef ENABLE_MOCKS

#include "http_client/http_codec.h"

#define ENABLE_MOCKS
MOCKABLE_FUNCTION(, void, test_on_data_recv_callback, void*, callback_ctx, HTTP_CODEC_CB_RESULT, result, const HTTP_RECV_DATA*, http_recv_data);
#undef ENABLE_MOCKS

typedef struct HTTP_CODEC_VALIDATE_TAG
{
    const char* content;
    uint32_t status_code;
} HTTP_CODEC_VALIDATE;

static const char* TEST_HTTP_EXAMPLE[] =
{
    "HTTP/1.1 200 OK\r\nDate: Mon, 23 May 2005 22:38:34 GMT\r\nContent-Type: tex",
    "t/html; charset=UTF-8\r\nContent-Encoding: UTF-8\r\ncontent-leng",
    "th: 118\r\nLast-Modified: Wed, 08 Jan 2003 23:11:55 GMT\r\nServer: Apache/1.3.3.7 (Unix)(Red-Hat/Linux)\r\n",
    "ETag: \"3f80f-1b6-3e1cb03b\"\r\nAccept-",
    "Ranges: bytes\r\nConnection: close\r\n\r\n<html><head><title>An Example Page</title>",
    "</head><body>Hello World, this is a very simple HTML document.</body></html>\r\n\r\n"
};
static const char* TEST_HTTP_EXAMPLE_BODY = "<html><head><title>An Example Page</title></head><body>Hello World, this is a very simple HTML document.</body></html>";

static const char* TEST_HTTP_EXAMPLE_2[] =
{
    "HTTP/1.1 200 OK\r\nDate: Tue, 08 May 2018 22:41:08 GMT\r\nX-Content-Type-Options: nosniff\r\n",
    "Expires: 0\r\nCache-Control: no-cache,no-store,must-revalidate\r\nX-Hudson-Theme: default\r\nContent-Type: text/html;charset=utf-8\r\n",
    "X-Hudson: 1.395\r\nX-Jenkins: 2.89.4\r\nX-Jenkins-Session: 5eb4cc0a\r\nX-Hudson-CLI-Port: 50000\r\nX-Jenkins-CLI-Port: 50000\r\n",
    "X-Jenkins-CLI2-Port: 50000\r\nX-Frame-Options: sameorigin\r\nContent-Length: 69\r\nServer: Jetty(9.4.z-SNAPSHOT)\r\n\r\n",
    "<!DOCTYPE html><html><body><footer><div></div></footer></body></html>"
};
static const char* TEST_HTTP_EXAMPLE_2_BODY = "<!DOCTYPE html><html><body><footer><div></div></footer></body></html>";

static const char* TEST_HTTP_NO_CONTENT_EXAMPLE[] =
{
    "HTTP/1.1 204 No Content\r\nContent-Length:",
    "0\r\nServer: Microsoft-HTTPAPI/2.0\r\nDate: Wed",
    ", 25 May 2016 00:07:51 GMT\r\n\r\n"
};

static const char* TEST_HTTP_CHUNK_EXAMPLE_IRL_2[] =
{
    "H",
    "TTP/1.1 200 OK",
    "\r\nDate: Thu, 11 May 2017 21:52:38 GMT\r\nContent-Typ",
    "e: application/json; charset=utf-8\r\nTransfer-Encoding: chunked\r\n",
    "\r\n",
    "a",
    "5\r\n",
    "{\"enrollmentGroupId\":\"RIoT_Device\",\"attestation\":{\"x509\":{}},\"etag\":\"\\\"14009e7f-0000-0000-0000-5914dd230000\\\"\",\"generationId\":\"9d546a85-5e8a-44c3-8946-c8ce4f54e5b6\"}\r\n",
    "0\r\n\r\n"
};
static const char* TEST_HTTP_CHUNK_EXAMPLE_IRL_2_BODY = "{\"enrollmentGroupId\":\"RIoT_Device\",\"attestation\":{\"x509\":{}},\"etag\":\"\\\"14009e7f-0000-0000-0000-5914dd230000\\\"\",\"generationId\":\"9d546a85-5e8a-44c3-8946-c8ce4f54e5b6\"}";

static const char* TEST_HTTP_CHUNK_EXAMPLE_IRL_3[] =
{
    "HTTP/1.1 200 OK\r\nDate: Tue, 09 May 2017 00:04:30 GMT\r\nContent-Type: application/json; charset=utf-8\r\nTransfer-Encoding: chunked\r\n\r\n29d\r\n",
    "{\"registrationId\":\"AAaVpgGlHlHY0zcAitlUv%2bkcvkVsd6bZ5XrcEm%2bzEdw%3d\",\"deviceId\":\"AFake_do_dev\",\"attestation\":{\"tpm\":{\"endorsementKey\":\"AToaAQALAqMqsqAqgqGqZqSqs/gqkqyqRqXqJq1q1q4qUqtq8qHqGqMqaqoABgCAAEMAEAgAAAAAAAEAzKJy5Ar/TZrRNU3jAq1knxz/6QkDAD3SIs8yf87D9560p4ikwbwE63RD1mghFnenLUexfAikMOqCZBOQ0Hcn6rVQMqhO8vTCg+0eKkb2KoqP7OwlZoLx5ZTsOav2fpX09PRtnlGV/E2u6Ih9lNDYlYcFyM3zJ7UKWBSkLx9Api3xfCUtzN4rhvbJVepzGxrrBvzR8b4QP4UVUTcO1Ptsr3LnXAw8xcI1c64vsFIdALcLZrEgJhEB9CCG9wSuBnr9SwRF7c+hVYrX2ffn+JGjUyexra7MjDTPDEREMwERmskmjHxXo8kKxbxwClBnMa+B4hFCwMfjtmvBITfe+YnHSw==\"}},\"etag\":\"\\\"0900ae54-0000-0000-0000-5911078f0000\\\"\",\"generationId\":\"1e9523ce-732d-4920-96eb-abf9e2e27b75\"}\r\n0\r\n\r\n"
};
#define CHUNK_IRL_LENGTH_3_1      669

static const char* TEST_HTTP_CHUNK_EXAMPLE_IRL[] =
{
    "HTTP/1.1 200 OK\r\nDate: Thu, 04 May 2017 18:03:35 GMT\r\n",
    "Content-Type: application/json; charset=utf-8\r\nTransfer-Encoding: ",
    "chunked\r\n\r\n1A\r\nABCDEFGHIJKLMNOPQRSTUVWXYZ\r\n10\r\n",
    "1234567890123456\r",
    "\n100\r\n1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF\r\n0\r\n\r\n"
};
#define CHUNK_IRL_LENGTH_1      26
#define CHUNK_IRL_LENGTH_2      16
#define CHUNK_IRL_LENGTH_3      256

/*static const char* TEST_HTTP_CHUNK_EXAMPLE_IRL_2[] =
{
    "HTTP/1.1 401 Unauthorized\r\nDate: Tue, 09 May 2017 18:28:44 GMT\r\nContent-Type: application/json; charset=utf-8\r\nTransfer-Encoding: chunked\r\n\r\n400\r\n{\"message\":\"Authorization required, resend request using supplied key\",\"authenticationKey\":\"ADQAIIvHKlrqd0xE1r7NQXDB/IrVsCBXBjvQxhkR+DLUngtYEL36OLX3Id8pN2N4pIGDj8FDAQA6P1J6aDVixNB5TMiO5D9Kx7tvsqjju31eoSTIfIn37Ud2tm2lvv3FeAzauG40YEUgEymyLgzw74I0rIJYhWhSzKgacFx7q05TCh0NvaZ8X28cRPMlhhmsYzfNzGnNMDQgj/5n5XZkvznkf7MLh+USrFS6Q0mkZrUWrMl2iHuYn7HUIxRaEmb5Srk4YIRyXbdUwKP9fiUPPIk984ufKxy7kbrod9xFV7PDL2eX9Hb12XWDxIUP2V2IIgUviyCbXKsuMrM3v0jgD88VJfSpjQcvxBN/lm7Q3IUjgsgaDCmod98XH/7Rj/yBiv3nTK93gi91bayAim3+1MbvD7UfQALtAI4AIOeBYtB+qk+2vbILTHVw8Rv6e+jgnkiZhQyxoahFBQ5EbEbEZQgwo9voYrYLuLbPf4GU2yl0Hqvlu5MC5gfhzSAFF26TsPT1AyPbWTsDwG3EKceHJGNe1WgzmL3TMB2wcSw3d/rI/qORtw6EiCE6REVav5i8rnAWmVB7G2Pap/TW+j54terM0CbsS+0ZAQBAHTflOCaVGPpKP/PwKdzOhMKh/FVW0MfNtJoN1bvP+eYEgbOnAPX7nG7bum2UktgDiZx7rx75tOFUQ6HlUgnQmwrr+CAdGQK+hOu9KjUgzAbOdnCtGNBE5W2ufEpByCp3g7kBBh6duJN5B69xP+FtdMZ/l2FbLeYPjIKt3C5d55rlsyfuiTZw414u4DQngSdAvjFUs1UH62CBJcX+gJLQ6B3w0vnE9Mih1mrnuVwoz5e6JWaacG70IZu3SWj0SQbAapK3GxkGxQAXU/WtVHpgNDxrkiLG1yQiqISkXCMVeqoZVolcwF5fSwgzbppMlLY+\r\n1c1\r\nsDGuAwU8xqNVyn6nPcaXADAACAALAAQEQAAAAAUACwAg/yMycoNJTM2h1DGjguQccuxhy+pHvv4XNTdZYizhwX4AvrHKBGZ78zk2Wlk+7R3OS6mY2S92u9wjDU4N4PMWS4CdT5sxXH7dkYp7jbvhN+8GiYmsoPePQLg30Eqv9wf9DJaHBbJOKHTyKQbMiavospzvOxQj/46KRRxgaHadzAwwjaF9Z8VqxqjhgruNXzySjdmRyDXliAqPgGbEPl1cj1zLjegUCch+8PYTUnukiF7SR5tVAYeOO/W2L9qyYBtsLpOWHPIRXj0AvnC/eZuxaWAif9R/ulcC41nH9S8feWA=\",\"unencryptedAuthenticationKey\":\"x/xWHUmzcm5NAY6vGZwMZ3vl7ylPfnGHcQN9RGkf1HM=\",\"keyName\":\"registration\"}\r\n0\r\n\r\n"
};
#define CHUNK_IRL_2_LENGTH_1      1024
#define CHUNK_IRL_2_LENGTH_2      449*/

static const char* TEST_HTTP_CHUNK_EXAMPLE[] =
{
    "HTTP/1.1 200 OK\r\nDate: Thu, 04 May 2017 18:03:35 GMT\r\n",
    "Content-Type: application/json; charset=utf-8\r\nTransfer-Encoding: ",
    "chunked\r\n\r\n12;this is junk\r\n1234567890ABCDEFGH\r\n9\r\nIJKLMNOPQ\r\n0\r\n\r\n"
};
static const char* TEST_HTTP_CHUNK_EXAMPLE_BODY = "1234567890ABCDEFGHIJKLMNOPQ";

static const char* TEST_HTTP_CHUNK_SPLIT_EXAMPLE[] =
{
    "HTTP/1.1 200 OK\r\nDate: Thu, 04 May 2017 18:03:35 GMT\r\n",
    "Content-Type: application/json; charset=utf-8\r\nTransfer-Encoding: ",
    "chunked\r\n\r\n12\r\n1234567890ABCDEF",
    "GH\r\n0f\r\nIJKLMNOPQRSTUVW\r\n0\r\n\r\n"
};
static const char* TEST_HTTP_CHUNK_SPLIT_EXAMPLE_BODY = "1234567890ABCDEFGHIJKLMNOPQRSTUVW";

static const char* TEST_SMALL_HTTP_EXAMPLE = "HTTP/1.1 200 OK\r\nDate: Mon, 23 May 2005 22:38:34 GMT\r\nAccept-Ranges: data\r\nContent-Type: text/html; charset=UTF-8\r\ncontent-length: 118\r\n\r\n<html><head><title>An Example Page</title></head><body>Hello World, this is a very simple HTML document.</body></html>\r\n\r\n";

static const char* TEST_HTTP_BODY = "<html><head><title>An Example Page</title></head><body>Hello World, this is a very simple HTML document.</body></html>\r\n\r\n";

static HTTP_HEADERS_HANDLE TEST_HTTP_HEADER = (HTTP_HEADERS_HANDLE)0x67890;

#ifdef __cplusplus
extern "C" {
#endif
    int string_buffer_construct_sprintf(STRING_BUFFER* buffer, const char* format, ...);

    int string_buffer_construct_sprintf(STRING_BUFFER* buffer, const char* format, ...)
    {
        (void)format;
        if (buffer->payload == NULL)
        {
            buffer->payload = my_mem_shim_malloc(1);
        }
        return 0;
    }

    void test_on_data_recv_callback(void* callback_ctx, HTTP_CODEC_CB_RESULT result, const HTTP_RECV_DATA* http_recv_data)
    {
        HTTP_CODEC_VALIDATE* validate = (HTTP_CODEC_VALIDATE*)callback_ctx;
        CTEST_ASSERT_IS_NOT_NULL(validate);
        if (result == HTTP_CODEC_CB_RESULT_OK)
        {
            if (validate->content == NULL)
            {
                CTEST_ASSERT_IS_NULL(http_recv_data->http_content.payload);
            }
            else
            {
                CTEST_ASSERT_ARE_EQUAL(int, 0, memcmp(http_recv_data->http_content.payload, validate->content, http_recv_data->http_content.payload_size));
            }
            CTEST_ASSERT_ARE_EQUAL(uint32_t, validate->status_code, http_recv_data->status_code);
        }
        else
        {
            CTEST_ASSERT_IS_NULL(http_recv_data);
        }
    }

    static int my_byte_buffer_construct(BYTE_BUFFER* buffer, const unsigned char* payload, size_t length)
    {
        if (buffer->payload == NULL)
        {
            buffer->payload = my_mem_shim_malloc(length);
            memcpy(buffer->payload, payload, length);
            buffer->payload_size = length;
        }
        else
        {
            buffer->payload = my_mem_shim_realloc(buffer->payload, buffer->payload_size+length);
            memcpy(buffer->payload+buffer->payload_size, payload, length);
            buffer->payload_size += length;
        }
        return 0;
    }


    static int my_clone_string(char** target, const char* source)
    {
        size_t len = strlen(source);
        *target = my_mem_shim_malloc(len+1);
        strcpy(*target, source);
        return 0;
    }

    static HTTP_HEADERS_HANDLE my_http_header_create(void)
    {
        return (HTTP_HEADERS_HANDLE)my_mem_shim_malloc(1);
    }

    static void my_http_header_destroy(HTTP_HEADERS_HANDLE handle)
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

CTEST_BEGIN_TEST_SUITE(http_codec_ut)

CTEST_SUITE_INITIALIZE()
{
    umock_c_init(on_umock_c_error);

    REGISTER_UMOCK_ALIAS_TYPE(HTTP_HEADERS_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PATCH_INSTANCE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_IO_OPEN_COMPLETE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_IO_CLOSE_COMPLETE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_BYTES_RECEIVED, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_IO_ERROR, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_SEND_COMPLETE, void*);

    REGISTER_GLOBAL_MOCK_HOOK(mem_shim_malloc, my_mem_shim_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mem_shim_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(mem_shim_free, my_mem_shim_free);

    REGISTER_GLOBAL_MOCK_HOOK(byte_buffer_construct, my_byte_buffer_construct);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(byte_buffer_construct, __LINE__);

    REGISTER_GLOBAL_MOCK_HOOK(http_header_create, my_http_header_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(http_header_create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(http_header_destroy, my_http_header_destroy);

    REGISTER_GLOBAL_MOCK_RETURN(http_header_get_name_value_pair, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(http_header_get_name_value_pair, __LINE__);
}

CTEST_SUITE_CLEANUP()
{
    umock_c_deinit();
}

CTEST_FUNCTION_INITIALIZE()
{
    umock_c_reset_all_calls();

}

CTEST_FUNCTION_CLEANUP()
{
}

static void setup_deinit_data_mocks(void)
{
    STRICT_EXPECTED_CALL(http_header_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
}

static void setup_http_header_item(const char* header_name)
{
    STRICT_EXPECTED_CALL(http_header_add_partial(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
}

CTEST_FUNCTION(http_codec_create_succeed)
{
    // arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

    // act
    HTTP_CODEC_HANDLE handle = http_codec_create(test_on_data_recv_callback, NULL);

    // assert
    CTEST_ASSERT_IS_NOT_NULL(handle);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_codec_destroy(handle);
}

CTEST_FUNCTION(http_codec_create_fail)
{
    // arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG)).SetReturn(NULL);

    // act
    HTTP_CODEC_HANDLE handle = http_codec_create(test_on_data_recv_callback, NULL);

    // assert
    CTEST_ASSERT_IS_NULL(handle);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

CTEST_FUNCTION(http_codec_destroy_handle_NULL_succeed)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    http_codec_destroy(NULL);

    // assert
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

CTEST_FUNCTION(http_codec_destroy_succeed)
{
    // arrange
    HTTP_CODEC_HANDLE handle = http_codec_create(test_on_data_recv_callback, NULL);
    umock_c_reset_all_calls();

    setup_deinit_data_mocks();
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    http_codec_destroy(handle);

    // assert
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

CTEST_FUNCTION(http_codec_reintialize_fail)
{
    // arrange

    // act
    http_codec_reintialize(NULL);

    // assert
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

CTEST_FUNCTION(http_codec_reintialize_succeed)
{
    // arrange
    HTTP_CODEC_HANDLE handle = http_codec_create(test_on_data_recv_callback, NULL);
    umock_c_reset_all_calls();

    setup_deinit_data_mocks();

    // act
    http_codec_reintialize(handle);

    // assert
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_codec_destroy(handle);
}

CTEST_FUNCTION(http_codec_get_recv_function_succeed)
{
    // arrange
    HTTP_CODEC_HANDLE handle = http_codec_create(test_on_data_recv_callback, NULL);
    umock_c_reset_all_calls();

    // act
    ON_BYTES_RECEIVED bytes_recv = http_codec_get_recv_function();

    // assert
    CTEST_ASSERT_IS_NOT_NULL(bytes_recv);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_codec_destroy(handle);
}

CTEST_FUNCTION(on_http_bytes_recv_succeed)
{
    // arrange
    HTTP_CODEC_VALIDATE validate = {TEST_HTTP_EXAMPLE_BODY, 200};

    HTTP_CODEC_HANDLE handle = http_codec_create(test_on_data_recv_callback, &validate);
    ON_BYTES_RECEIVED on_bytes_recv = http_codec_get_recv_function();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(http_header_create());
    STRICT_EXPECTED_CALL(byte_buffer_construct(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    STRICT_EXPECTED_CALL(http_header_add_partial(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    STRICT_EXPECTED_CALL(byte_buffer_construct(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(http_header_add_partial(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(http_header_add_partial(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    //setup_http_header_item("content-type");
    //setup_http_header_item("content-encoding");
    STRICT_EXPECTED_CALL(byte_buffer_construct(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    STRICT_EXPECTED_CALL(http_header_add_partial(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(http_header_add_partial(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(http_header_add_partial(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(byte_buffer_construct(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(http_header_add_partial(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(byte_buffer_construct(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(http_header_add_partial(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(http_header_add_partial(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(byte_buffer_construct(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(http_header_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    size_t count = sizeof(TEST_HTTP_EXAMPLE)/sizeof(TEST_HTTP_EXAMPLE[0]);
    for (size_t index = 0; index < count; index++)
    {
        const char* test_value = TEST_HTTP_EXAMPLE[index];
        size_t test_len = strlen(test_value);
        on_bytes_recv(handle, (const unsigned char*)test_value, test_len);
    }

    // assert
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_codec_destroy(handle);
}

CTEST_FUNCTION(on_http_bytes_recv_small_example_succeed)
{
    // arrange
    HTTP_CODEC_VALIDATE validate = {TEST_HTTP_EXAMPLE_BODY, 200};
    HTTP_CODEC_HANDLE handle = http_codec_create(test_on_data_recv_callback, (void*)&validate);
    ON_BYTES_RECEIVED on_bytes_recv = http_codec_get_recv_function();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(http_header_create());
    STRICT_EXPECTED_CALL(byte_buffer_construct(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    setup_http_header_item("Date");
    setup_http_header_item("Accept-Ranges");
    setup_http_header_item("Content-Type");
    setup_http_header_item("content-length");
    STRICT_EXPECTED_CALL(http_header_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    const char* test_value = TEST_SMALL_HTTP_EXAMPLE;
    size_t test_len = strlen(test_value);
    on_bytes_recv(handle, (const unsigned char*)test_value, test_len);

    // assert
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_codec_destroy(handle);
}

CTEST_FUNCTION(on_http_bytes_recv_small_example_fail)
{
    // arrange
    HTTP_CODEC_VALIDATE validate = {TEST_HTTP_EXAMPLE_BODY, 200};
    HTTP_CODEC_HANDLE handle = http_codec_create(test_on_data_recv_callback, (void*)&validate);
    ON_BYTES_RECEIVED on_bytes_recv = http_codec_get_recv_function();
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    CTEST_ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(http_header_create());
    STRICT_EXPECTED_CALL(byte_buffer_construct(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    setup_http_header_item("Date");
    setup_http_header_item("Accept-Ranges");
    setup_http_header_item("Content-Type");
    setup_http_header_item("content-length");
    STRICT_EXPECTED_CALL(http_header_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            const char* test_value = TEST_SMALL_HTTP_EXAMPLE;
            size_t test_len = strlen(test_value);
            on_bytes_recv(handle, (const unsigned char*)test_value, test_len);

            // assert

            (void)http_codec_reintialize(handle);
        }
    }
    // cleanup
    http_codec_destroy(handle);
    umock_c_negative_tests_deinit();
}

CTEST_FUNCTION(on_http_bytes_recv_example_2_succeed)
{
    // arrange
    HTTP_CODEC_VALIDATE validate = {TEST_HTTP_EXAMPLE_2_BODY, 200};
    HTTP_CODEC_HANDLE handle = http_codec_create(test_on_data_recv_callback, (void*)&validate);
    ON_BYTES_RECEIVED on_bytes_recv = http_codec_get_recv_function();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(http_header_create());
    STRICT_EXPECTED_CALL(byte_buffer_construct(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    setup_http_header_item("Date");
    setup_http_header_item("X-Content-Type-Options");
    STRICT_EXPECTED_CALL(byte_buffer_construct(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    setup_http_header_item("Expires");
    setup_http_header_item("Cache-Control");
    setup_http_header_item("X-Hudson-Theme");
    setup_http_header_item("Content-Type");
    STRICT_EXPECTED_CALL(byte_buffer_construct(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    setup_http_header_item("X-Hudson");
    setup_http_header_item("X-Jenkins");
    setup_http_header_item("X-Jenkins-Session");
    setup_http_header_item("X-Hudson-CLI-Port");
    setup_http_header_item("X-Jenkins-CLI-Port");
    STRICT_EXPECTED_CALL(byte_buffer_construct(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    setup_http_header_item("X-Jenkins-CLI2-Port");
    setup_http_header_item("X-Frame-Options");
    setup_http_header_item("content-length");
    setup_http_header_item("Server");
    STRICT_EXPECTED_CALL(byte_buffer_construct(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(http_header_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    size_t count = sizeof(TEST_HTTP_EXAMPLE_2)/sizeof(TEST_HTTP_EXAMPLE_2[0]);
    for (size_t index = 0; index < count; index++)
    {
        const char* test_value = TEST_HTTP_EXAMPLE_2[index];
        size_t test_len = strlen(test_value);
        on_bytes_recv(handle, (const unsigned char*)test_value, test_len);
    }

    // assert
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_codec_destroy(handle);
}

CTEST_FUNCTION(on_http_bytes_recv_no_content_succeed)
{
    // arrange
    HTTP_CODEC_VALIDATE validate = {NULL, 204};
    HTTP_CODEC_HANDLE handle = http_codec_create(test_on_data_recv_callback, &validate);
    ON_BYTES_RECEIVED on_bytes_recv = http_codec_get_recv_function();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(http_header_create());
    STRICT_EXPECTED_CALL(byte_buffer_construct(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(byte_buffer_construct(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    setup_http_header_item("content-length");
    setup_http_header_item("Server");
    STRICT_EXPECTED_CALL(byte_buffer_construct(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    setup_http_header_item("date");
    STRICT_EXPECTED_CALL(http_header_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    size_t count = sizeof(TEST_HTTP_NO_CONTENT_EXAMPLE)/sizeof(TEST_HTTP_NO_CONTENT_EXAMPLE[0]);
    for (size_t index = 0; index < count; index++)
    {
        const char* test_value = TEST_HTTP_NO_CONTENT_EXAMPLE[index];
        size_t test_len = strlen(test_value);
        on_bytes_recv(handle, (const unsigned char*)test_value, test_len);
    }

    // assert
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_codec_destroy(handle);
}

CTEST_FUNCTION(on_http_bytes_recv_chunked_content_succeed)
{
    // arrange
    HTTP_CODEC_VALIDATE validate = {TEST_HTTP_CHUNK_EXAMPLE_BODY, 200};
    HTTP_CODEC_HANDLE handle = http_codec_create(test_on_data_recv_callback, &validate);
    ON_BYTES_RECEIVED on_bytes_recv = http_codec_get_recv_function();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(http_header_create());
    STRICT_EXPECTED_CALL(byte_buffer_construct(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    setup_http_header_item("date");
    STRICT_EXPECTED_CALL(byte_buffer_construct(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    setup_http_header_item("content-length");
    STRICT_EXPECTED_CALL(byte_buffer_construct(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    setup_http_header_item("Transfer-Encoding");

    STRICT_EXPECTED_CALL(byte_buffer_construct(IGNORED_ARG, IGNORED_ARG, 18));
    STRICT_EXPECTED_CALL(byte_buffer_construct(IGNORED_ARG, IGNORED_ARG, 9));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(http_header_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    size_t count = sizeof(TEST_HTTP_CHUNK_EXAMPLE)/sizeof(TEST_HTTP_CHUNK_EXAMPLE[0]);
    for (size_t index = 0; index < count; index++)
    {
        const char* test_value = TEST_HTTP_CHUNK_EXAMPLE[index];
        size_t test_len = strlen(test_value);
        on_bytes_recv(handle, (const unsigned char*)test_value, test_len);
    }

    // assert
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_codec_destroy(handle);
}

CTEST_FUNCTION(on_http_bytes_recv_chunked_IRL_content_succeed)
{
    // arrange
    HTTP_CODEC_VALIDATE validate = {TEST_HTTP_CHUNK_EXAMPLE_IRL_2_BODY, 200};
    HTTP_CODEC_HANDLE handle = http_codec_create(test_on_data_recv_callback, &validate);
    ON_BYTES_RECEIVED on_bytes_recv = http_codec_get_recv_function();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(http_header_create());
    STRICT_EXPECTED_CALL(byte_buffer_construct(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(byte_buffer_construct(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(byte_buffer_construct(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    setup_http_header_item("date");
    STRICT_EXPECTED_CALL(byte_buffer_construct(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    setup_http_header_item("Content-Type");
    setup_http_header_item("Transfer-Encoding");
    STRICT_EXPECTED_CALL(byte_buffer_construct(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(byte_buffer_construct(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(byte_buffer_construct(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(byte_buffer_construct(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(byte_buffer_construct(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(byte_buffer_construct(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(byte_buffer_construct(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(http_header_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    size_t count = sizeof(TEST_HTTP_CHUNK_EXAMPLE_IRL_2)/sizeof(TEST_HTTP_CHUNK_EXAMPLE_IRL_2[0]);
    for (size_t index = 0; index < count; index++)
    {
        const char* test_value = TEST_HTTP_CHUNK_EXAMPLE_IRL_2[index];
        size_t test_len = strlen(test_value);
        on_bytes_recv(handle, (const unsigned char*)test_value, test_len);
    }

    // assert
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_codec_destroy(handle);
}

CTEST_FUNCTION(http_codec_set_trace_handle_NULL_fail)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    int result = http_codec_set_trace(NULL, true);

    // assert
    CTEST_ASSERT_ARE_NOT_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

CTEST_FUNCTION(http_codec_set_trace_succeed)
{
    // arrange
    HTTP_CODEC_VALIDATE validate = {TEST_HTTP_CHUNK_EXAMPLE_IRL_2_BODY, 200};
    HTTP_CODEC_HANDLE handle = http_codec_create(test_on_data_recv_callback, &validate);
    umock_c_reset_all_calls();

    // act
    int result = http_codec_set_trace(handle, true);

    // assert
    CTEST_ASSERT_ARE_EQUAL(int, 0, result);
    CTEST_ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    http_codec_destroy(handle);
}

CTEST_END_TEST_SUITE(http_codec_ut)
