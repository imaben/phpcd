#ifndef __RPC_SERVER_H__
#define __RPC_SERVER_H__

#include <php.h>

BEGIN_EXTERN_C()

extern PHPAPI zend_class_entry *phpcd_rpc_server_ce;

END_EXTERN_C()

void init_rpc_server_class();

#endif
