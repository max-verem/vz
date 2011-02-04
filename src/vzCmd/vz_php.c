#include <string.h>

#include <php.h>
#include <ext/standard/basic_functions.h>

#include "vz_cmd_send.h"

#define LOG(TYPE,...)  php_error_docref(NULL TSRMLS_CC, TYPE, "vz_php: " __VA_ARGS__);

ZEND_FUNCTION(vz_send);
zend_function_entry vz_php_functions[] =
{
    ZEND_FE(vz_send, NULL)
    {NULL, NULL, NULL}
};
zend_module_entry vz_php_module_entry =
{
    STANDARD_MODULE_HEADER,
    "VZ Send Command interface module",
    vz_php_functions,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NO_VERSION_YET,
    STANDARD_MODULE_PROPERTIES
};

ZEND_GET_MODULE(vz_php)

ZEND_FUNCTION(vz_send)
{
    int r;
    char* host_data;
    long host_len;
    zval *cmd_array;
    HashTable *cmd_hash;
    HashPosition cmd_hash_ptr;
    char** cmd_list_data;
    int cmd_list_len;

    /* check arguments count */
    if(2 != ZEND_NUM_ARGS())
        WRONG_PARAM_COUNT;

    /* parse arguments */
    r = zend_parse_parameters
    (
        ZEND_NUM_ARGS() TSRMLS_CC, "sa",
        &host_data, &host_len, &cmd_array
    );

    /* check parse result */
    if(r == FAILURE)
    {
        LOG(E_WARNING, "zend_parse_parameters('sa') failed");
        RETURN_NULL();
    };

    cmd_hash = Z_ARRVAL_P(cmd_array);
    r = zend_hash_num_elements(cmd_hash);

    /* check for parameters */
    if(!r)
    {
        LOG(E_WARNING, "cmd array is empty");
        RETURN_LONG(-1);
    };

    cmd_list_data = (char**)malloc(sizeof(char*));
    cmd_list_data[0] = NULL;
    cmd_list_len = 0;

    zend_hash_internal_pointer_reset_ex(cmd_hash, &cmd_hash_ptr);

    for(;;)
    {
        zval ** data;

        r = zend_hash_get_current_data_ex(cmd_hash, (void**) &data, &cmd_hash_ptr);

        if(SUCCESS != r) break;

        if (IS_STRING == Z_TYPE_PP(data))
        {
            cmd_list_data = (char**)realloc(cmd_list_data, sizeof(char*) * (cmd_list_len + 1));
            cmd_list_data[cmd_list_len] = strndup(Z_STRVAL_PP(data), Z_STRLEN_PP(data));
            cmd_list_len++;
        };

        zend_hash_move_forward_ex(cmd_hash, &cmd_hash_ptr);
    };

    if(cmd_list_len)
    {
        char* host = strndup(host_data, host_len);

        r = vz_cmd_send_strlist_udp(host, cmd_list_data, cmd_list_len, NULL);

        free(host);
    }
    else
        r = -2;

    RETURN_LONG(r);
}
