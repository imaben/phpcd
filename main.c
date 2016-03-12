#include <sapi/embed/php_embed.h>
#include "zend_interfaces.h"
#include "main.h"
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>

#ifdef ZTS
    void ***tsrm_ls;
#endif

static void startup_php(void)
{
    int argc = 1;
    char *argv[2] = { "phpcd", NULL };
    php_embed_init(argc, argv);
    init_rpc_server_class();
}

static void shutdown_php(void)
{
    //php_embed_shutdown(TSRMLS_C);
}

static void load_autoload_file(char *filename)
{
    zend_first_try {
        char *include_script;
        spprintf(&include_script, 0, "include '%s';", filename);
        zend_eval_string(include_script, NULL, filename TSRMLS_CC);
        efree(include_script);
    } zend_end_try();
}

static void test()
{
    zend_first_try {
        char *test_code;
        spprintf(&test_code, 0, "$r = new ReflectionClass(new RpcServer);%s%s%s%s%s",
                "var_dump($r->getConstants());",
                "var_dump($r->getProperties());",
                "var_dump($r->getMethods());",
                "$rpc = new RpcServer;",
                "$rpc->onMessage([1, 2, 3]);"
        );
        zend_eval_string(test_code, NULL, "ss" TSRMLS_CC);
        efree(test_code);
    } zend_end_try();
}

int main(int argc, char *argv[])
{
    startup_php();
    char *autoload_file = "/home/maben/angejia/vendor/autoload.php";
    load_autoload_file(autoload_file);
    test();
    shutdown_php();
    return 0;
}
