#include "rpc_server.h"
#include "zend.h"
#include "zend_API.h"

PHPAPI zend_class_entry *phpcd_rpc_server_ce;

#define TYPE_REQUEST 0x00
#define TYPE_RESPONSE 0x01
#define TYPE_NOTIFICATION 0x02

ZEND_METHOD(RpcServer, onMessage)
{
    zval *messages = NULL;
    zval *type;
    HashTable *ht;
    if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_THROW, ZEND_NUM_ARGS(), "a", &messages) == SUCCESS) {
        ht = Z_ARRVAL_P(messages);
        type = zend_hash_get_current_data(ht);
        switch (Z_LVAL_P(type)) {
            case TYPE_REQUEST:
                break;
            case TYPE_RESPONSE:
                break;
            case TYPE_NOTIFICATION:
                break;
        }
    }
}

ZEND_METHOD(RpcServer, onRequest)
{

#define MOVE_HASH_WITH_CHECK(ht) \
    if (FAILURE == zend_hash_move_forward(ht)) { \
        RETURN_FALSE; \
    }

    zval *messages = NULL;
    zval *type, *msg_id, *method, *params;
    HashTable *ht;
    zval *result, *error;
    if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_THROW, ZEND_NUM_ARGS(), 'a', &messages) == FAILURE) {
        RETURN_FALSE;
    }
    ht = Z_ARRVAL_P(messages);
    type = zend_hash_get_current_data(ht);

    MOVE_HASH_WITH_CHECK(ht);
    msg_id = zend_hash_get_current_data(ht);

    MOVE_HASH_WITH_CHECK(ht);
    method = zend_hash_get_current_data(ht);

    MOVE_HASH_WITH_CHECK(ht);
    params = zend_hash_get_current_data(ht);

    zend_update_property(Z_OBJCE_P(getThis()), getThis(), "msg_id", strlen("msg_id") -1, msg_id);

    zend_bool method_exists;
    method_exists = zend_hash_exists(&Z_OBJCE_P(getThis())->function_table, Z_STR_P(method));
    if (method_exists) {
        // call this->doRequest
    } else {
        ZVAL_NEW_STR(error, "method not exists");
    }
}

ZEND_METHOD(RpcServer, onResponse)
{
}

ZEND_METHOD(RpcServer, onNotification)
{
}

#define REGISTER_RPC_SERVER_CLASS_CONST_LONG(const_name, value) \
    zend_declare_class_constant_long(phpcd_rpc_server_ce, const_name, sizeof(const_name)-1, (zend_long)value);

ZEND_BEGIN_ARG_INFO(arginfo_rpc_server_onMessage, 0)
    ZEND_ARG_ARRAY_INFO(0, "messages", 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_rpc_server_onRequest, 0)
    ZEND_ARG_ARRAY_INFO(0, "messages", 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_rpc_server_onResponse, 0)
    ZEND_ARG_ARRAY_INFO(0, "messages", 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_rpc_server_onNotification, 0)
    ZEND_ARG_ARRAY_INFO(0, "messages", 0)
ZEND_END_ARG_INFO()

static const zend_function_entry rpc_server_functions[] = {
    ZEND_ME(RpcServer, onMessage, arginfo_rpc_server_onMessage, ZEND_ACC_PUBLIC)
    ZEND_ME(RpcServer, onRequest, arginfo_rpc_server_onRequest, ZEND_ACC_PUBLIC)
    ZEND_ME(RpcServer, onResponse, arginfo_rpc_server_onResponse, ZEND_ACC_PUBLIC)
    ZEND_ME(RpcServer, onNotification, arginfo_rpc_server_onNotification, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

void init_rpc_server_class()
{
    zend_class_entry _rpc_server;
    INIT_CLASS_ENTRY(_rpc_server, "RpcServer", rpc_server_functions);
    phpcd_rpc_server_ce = zend_register_internal_class(&_rpc_server);

    REGISTER_RPC_SERVER_CLASS_CONST_LONG("TYPE_REQUEST", TYPE_REQUEST)
    REGISTER_RPC_SERVER_CLASS_CONST_LONG("TYPE_RESPONSE", TYPE_RESPONSE)
    REGISTER_RPC_SERVER_CLASS_CONST_LONG("TYPE_NOTIFICATION", TYPE_NOTIFICATION)

    zend_declare_property_long(phpcd_rpc_server_ce, "msg_id", sizeof("msg_id") - 1, 0, ZEND_ACC_PRIVATE);
    zend_declare_property_long(phpcd_rpc_server_ce, "current_msg_id", sizeof("current_msg_id") - 1, 0, ZEND_ACC_PRIVATE);
    zend_declare_property_null(phpcd_rpc_server_ce, "request_callback", sizeof("request_callback") - 1, ZEND_ACC_PRIVATE);
    zend_declare_property_null(phpcd_rpc_server_ce, "log_file", sizeof("log_file") - 1, ZEND_ACC_PRIVATE);
}
