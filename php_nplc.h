/* $Id$ */

#ifndef PHP_NPLC_H
#define PHP_NPLC_H

extern zend_module_entry nplc_module_entry;
#define phpext_nplc_ptr &nplc_module_entry

#ifdef PHP_WIN32
#define PHP_NPLC_API __declspec(dllexport)
#else
#define PHP_NPLC_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#define PHP_NPLC_VERSION "0.1.0"
#define NPLC_CLASS_PROPERTY_PREFIX "_nplc_prefix"
#define NPLC_ENTRY_COMPRESSED     0x0020
#define NPLC_ENTRY_TYPE_MASK      0x1f
#define NPLC_ENTRY_ORIG_LEN_SHIT  6
#define NPLC_ENTRY_MAX_ORIG_LEN   ((1U << ((sizeof(int)*8 - NPLC_ENTRY_ORIG_LEN_SHIT))) - 1)

ZEND_BEGIN_MODULE_GLOBALS(nplc)
	zend_bool  enable;
    zend_bool  enable_cli;
    size_t     storage_size;
ZEND_END_MODULE_GLOBALS(nplc)

PHP_MINIT_FUNCTION(nplc);
PHP_MSHUTDOWN_FUNCTION(nplc);
PHP_RINIT_FUNCTION(nplc);
PHP_RSHUTDOWN_FUNCTION(nplc);
PHP_MINFO_FUNCTION(nplc);

#ifdef ZTS
#define NPLC_G(v) TSRMG(nplc_globals_id, zend_nplc_globals *, v)
extern int nplc_globals_id;
#else
#define NPLC_G(v) (nplc_globals.v)
extern zend_nplc_globals nplc_globals;
#endif

#endif	/* PHP_NPLC_H */
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
