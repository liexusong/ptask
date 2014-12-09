/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2008 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id: header,v 1.16.2.1.2.1.2.1 2008/02/07 19:39:50 iliaa Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_ptask.h"
#include <task.h>

/* If you declare any globals in php_ptask.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(ptask)
*/

#define  PTASK_STACK_SIZE  32768
#define  PTASK_NET_TCP     TCP
#define  PTASK_NET_UDP     UDP

/* True global resources - no need for thread safety here */
static int le_ptask;

/* {{{ ptask_functions[]
 *
 * Every user visible function must have an entry in ptask_functions[].
 */
const zend_function_entry ptask_functions[] = {
    /* Base functions */
    PHP_FE(ptask_create, NULL)
    PHP_FE(ptask_run,    NULL)
    PHP_FE(ptask_yield,  NULL)
    PHP_FE(ptask_exit,   NULL)
    /* network functions */
    PHP_FE(ptask_net_listen, NULL)
    PHP_FE(ptask_net_accept, NULL)
    PHP_FE(ptask_net_recv,   NULL)
    PHP_FE(ptask_net_send,   NULL)
    {NULL, NULL, NULL}    /* Must be the last line in ptask_functions[] */
};
/* }}} */

/* {{{ ptask_module_entry
 */
zend_module_entry ptask_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    "ptask",
    ptask_functions,
    PHP_MINIT(ptask),
    PHP_MSHUTDOWN(ptask),
    PHP_RINIT(ptask),        /* Replace with NULL if there's nothing to do at request start */
    PHP_RSHUTDOWN(ptask),    /* Replace with NULL if there's nothing to do at request end */
    PHP_MINFO(ptask),
#if ZEND_MODULE_API_NO >= 20010901
    "0.1", /* Replace with version number for your extension */
#endif
    STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_PTASK
ZEND_GET_MODULE(ptask)
#endif

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("ptask.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_ptask_globals, ptask_globals)
    STD_PHP_INI_ENTRY("ptask.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_ptask_globals, ptask_globals)
PHP_INI_END()
*/
/* }}} */

/* {{{ php_ptask_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_ptask_init_globals(zend_ptask_globals *ptask_globals)
{
    ptask_globals->global_value = 0;
    ptask_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(ptask)
{
    /* If you have INI entries, uncomment these lines 
    REGISTER_INI_ENTRIES();
    */
    REGISTER_LONG_CONSTANT("PTASK_NET_TCP", PTASK_NET_TCP, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("PTASK_NET_UDP", PTASK_NET_UDP, CONST_CS | CONST_PERSISTENT);

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(ptask)
{
    /* uncomment this line if you have INI entries
    UNREGISTER_INI_ENTRIES();
    */
    return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(ptask)
{
    return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(ptask)
{
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(ptask)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "ptask support", "enabled");
    php_info_print_table_end();

    /* Remove comments if you have entries in php.ini
    DISPLAY_INI_ENTRIES();
    */
}
/* }}} */

/*
 * Base functions
 */

struct ptask_ctx {
    char *func;
    zval *arg;
};

void ptask_thread_fn(void *arg)
{
    struct ptask_ctx *ctx = arg;
    zval fname, retval;
    zval *params[1];

    ZVAL_STRING(&fname, ctx->func, 0); /* create new function name zval */

    params[0] = ctx->arg;

    call_user_function(EG(function_table), NULL, &fname,
        &retval, 1, params TSRMLS_CC);

    /* free all memory */
    Z_DELREF_PP(&(ctx->arg));
    if (Z_REFCOUNT_PP(&(ctx->arg)) == 0) {
        zval_dtor(ctx->arg);
    }
    efree(ctx->func);
    efree(ctx);
}

/* {{{ bool ptask_create(string $func, zval $arg) */
PHP_FUNCTION(ptask_create)
{
    char  *fname;
    int    flen;
    zval  *arg;
    struct ptask_ctx *ctx;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz",
        &fname, &flen, &arg) == FAILURE)
    {
        RETURN_FALSE;
    }

    /* create task context */
    ctx = emalloc(sizeof(struct ptask_ctx));

    /* copy function name */
    ctx->func = emalloc(flen + 1);
    memcpy(ctx->func, fname, flen);
    ctx->func[flen] = '\0';

    ctx->arg = arg;

    Z_ADDREF_PP(&arg); /* arg->refcount++ */

    taskcreate(ptask_thread_fn, (void *)ctx, PTASK_STACK_SIZE);

    RETURN_TRUE;
}
/* }}} */

/* {{{ */
PHP_FUNCTION(ptask_yield)
{
    taskyield();
}
/* }}} */

/* {{{ */
PHP_FUNCTION(ptask_run)
{
    taskscheduler();
}
/* }}} */

/* {{{ */
PHP_FUNCTION(ptask_exit)
{
    int val;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &val) == FAILURE) {
        RETURN_FALSE;
    }

    taskexit(val);
    
    RETURN_TRUE; /* never */
}
/* }}} */


/*
 * Network functions
 */

/* {{{ long ptask_net_listen(int $tcp, char $host, int $port) */
PHP_FUNCTION(ptask_net_listen)
{
    int istcp = PTASK_NET_TCP;
    char *host;
    int hlen;
    int port;
    int sock;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lsl",
        &istcp, &host, &hlen, &port) == FAILURE)
    {
        RETURN_FALSE;
    }

    sock = netannounce(istcp, host, port);
    if (sock == -1) {
        RETURN_FALSE;
    }

    RETURN_LONG(sock);
}
/* }}} */

/* {{{ long ptask_net_accept(int $sock) */
PHP_FUNCTION(ptask_net_accept)
{
    int lsock, csock;
    char ip[16];
    int port;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &lsock) == FAILURE
        || lsock < 0)
    {
        RETURN_FALSE;
    }

    csock = netaccept(lsock, ip, &port);
    if (csock == -1) {
        RETURN_FALSE;
    }

    RETURN_LONG(csock);
}
/* }}} */

/* {{{ string ptask_net_recv(int $sock, int $size) */
PHP_FUNCTION(ptask_net_recv)
{
    int sock;
    int size, nbytes;
    char *buf;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ll",
        &sock, &size) == FAILURE || sock < 0 || size <= 0)
    {
        RETURN_FALSE;
    }

    buf = emalloc(size + 1);

    nbytes = fdread(sock, buf, size);

    buf[nbytes] = '\0';

    RETURN_STRINGL(buf, nbytes, 0);
}
/* }}} */

/* {{{ int ptask_net_send(int $sock, string $buf) */
PHP_FUNCTION(ptask_net_send)
{
    int sock;
    char *buf;
    int blen, nbytes;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ls",
        &sock, &buf, &blen) == FAILURE || sock < 0)
    {
        RETURN_FALSE;
    }

    nbytes = fdwrite(sock, buf, blen);

    RETURN_LONG(nbytes);
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
