/*
+----------------------------------------------------------------------+
| PHP Version 7                                                        |
+----------------------------------------------------------------------+
| Copyright (c) 1997-2016 The PHP Group                                |
+----------------------------------------------------------------------+
| This source file is subject to version 3.01 of the PHP license,      |
| that is bundled with this package in the file LICENSE, and is        |
| available through the world-wide-web at the following url:           |
| http://www.php.net/license/3_01.txt                                  |
| If you did not receive a copy of the PHP license and are unable to   |
| obtain it through the world-wide-web, please send a note to          |
| license@php.net so we can mail you a copy immediately.               |
+----------------------------------------------------------------------+
| Author: niansong                                                     |
+----------------------------------------------------------------------+
`____``_____`````````__``````````
|_```\|_```_|```````[``|`````````
``|```\`|`|``_`.--.``|`|``.---.``
``|`|\`\|`|`[`'/'`\`\|`|`/`/'`\]`
`_|`|_\```|_`|`\__/`||`|`|`\__.``
|_____|\____||`;.__/[___]'.___.'`
````````````[__|`````````````````
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "zend_smart_str.h"
#include "SAPI.h"

#include "php_nplc.h"
#include "storage/npl_cache.h"
#include "storage/fastlz.h"
#include "serializer/nplc_serializer.h"

zend_class_entry *nplc_class_ce;

/**
 * If you declare any globals in php_nplc.h uncomment this:
 */
ZEND_DECLARE_MODULE_GLOBALS(nplc)

/** {{{ ARG_INFO
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_nplc_constructor, 0, 0, 0)
	ZEND_ARG_INFO(0, prefix)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_nplc_setter, 0, 0, 2)
	ZEND_ARG_INFO(0, keys)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_nplc_getter, 0, 0, 1)
	ZEND_ARG_INFO(0, key)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_nplc_delete, 0, 0, 1)
	ZEND_ARG_INFO(0, key)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_nplc_void, 0, 0, 0)
ZEND_END_ARG_INFO()
/* }}} */

/* True global resources - no need for thread safety here */
static int le_nplc;

/* {{{ PHP_INI
 */
static PHP_INI_MH(OnChangeValsMemoryLimit) /* {{{ */ {
	if (new_value) {
		NPLC_G(storage_size) = zend_atol(ZSTR_VAL(new_value), ZSTR_LEN(new_value));
	}

	return SUCCESS;
}

PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("nplc.enable", "1", PHP_INI_SYSTEM, OnUpdateBool, enable, zend_nplc_globals, nplc_globals)
	STD_PHP_INI_ENTRY("nplc.storage_size", "64M", PHP_INI_SYSTEM, OnChangeValsMemoryLimit, storage_size, zend_nplc_globals, nplc_globals)
	STD_PHP_INI_ENTRY("nplc.enable_cli", "0", PHP_INI_SYSTEM, OnUpdateBool, enable_cli, zend_nplc_globals, nplc_globals)
PHP_INI_END()
/* }}} */

#if 0
/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_nplc_compiled(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(confirm_nplc_compiled)
{
	char *arg = NULL;
	size_t arg_len, len;
	zend_string *strg;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &arg, &arg_len) == FAILURE) {
		return;
	}

	strg = strpprintf(0, "Congratulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.", "nplc", arg);

	RETURN_STR(strg);
}
/* }}} */

/* {{{ nplc_functions[]
 *
 * Every user visible function must have an entry in nplc_functions[].
 */
const zend_function_entry nplc_functions[] = {
	PHP_FE(confirm_nplc_compiled,	NULL)		/* For testing, remove later. */
	PHP_FE_END	/* Must be the last line in nplc_functions[] */
};
/* }}} */
#endif

static int
nplc_add(zend_string *prefix, zend_string *key, zval *value)
{
    int ret = 0;
    unsigned int flag = Z_TYPE_P(value);
    char *msg;
    zend_string *prefix_key;

    if((ZSTR_LEN(key) + prefix->len) > KEY_MAX_LEN){
    	php_error_docref(NULL, E_WARNING, "Key%s can not be longer than %d bytes",
    					prefix->len ? "(include prefix)" : "", KEY_MAX_LEN);
    	return ret;
    }

    if(prefix->len){
        prefix_key = strpprintf(KEY_MAX_LEN, "%s%s", ZSTR_VAL(prefix), ZSTR_VAL(key));
        key = prefix_key;
    }

    switch(Z_TYPE_P(value)){
    	case IS_NULL:
    	case IS_TRUE:
    	case IS_FALSE:
    		ret = npl_update_data(ZSTR_VAL(key), ZSTR_LEN(key), Z_STRVAL_P(value), Z_STRLEN_P(value), flag, &msg);
    		break;
    	case IS_LONG:
    		ret = npl_update_data(ZSTR_VAL(key), ZSTR_LEN(key), (char *)&(Z_LVAL_P(value)), sizeof(long), flag, &msg);
    		break;
    	case IS_DOUBLE:
    		ret = npl_update_data(ZSTR_VAL(key), ZSTR_LEN(key), (char *)&Z_DVAL_P(value), sizeof(double), flag, &msg);
    		break;
    	case IS_STRING:
    	case IS_CONSTANT:
    		{
    			if(Z_STRLEN_P(value) > NODE_MAX_DATA_SIZE){
    				int compressed_len;
    				char *compressed;

    				compressed = emalloc(Z_STRLEN_P(value) * 1.05);
    				compressed_len = fastlz_compress(Z_STRVAL_P(value), Z_STRLEN_P(value), compressed);
    				if(!compressed_len || compressed_len > Z_STRLEN_P(value)){
    					php_error_docref(NULL, E_WARNING, "Compressed failed");
    					efree(compressed);
    					if(prefix->len){
    						zend_string_release(prefix_key);
    					}
    					return ret;
    				}
    				if(compressed_len > NODE_MAX_DATA_SIZE){
    					php_error_docref(NULL, E_WARNING, "Value is too large (%d) can't be stored", Z_STRLEN_P(value));
    					efree(compressed);
    					if(prefix->len){
    						zend_string_release(prefix_key);
    					}
    					return ret;
    				}

    				flag |= NPLC_ENTRY_COMPRESSED;
    				flag |= (Z_STRLEN_P(value) << NPLC_ENTRY_ORIG_LEN_SHIT);
    				ret = npl_update_data(ZSTR_VAL(key), ZSTR_LEN(key), compressed, compressed_len, flag, &msg);
    				efree(compressed);
    			} else {
					ret = npl_update_data(ZSTR_VAL(key), ZSTR_LEN(key), Z_STRVAL_P(value), Z_STRLEN_P(value), flag, &msg);
    			}
    		}
    		break;
    	case IS_ARRAY:
#ifdef IS_CONSTANT_ARRAY
		case IS_CONSTANT_ARRAY:
#endif
		case IS_OBJECT:
			{
				smart_str buf = {0};
				if (nplc_serializer_php_pack(value, &buf, &msg))
				{
					if(buf.s->len > NODE_MAX_DATA_SIZE){
						int compressed_len;
						char *compressed;

						compressed = emalloc(buf.s->len * 1.05);
						compressed_len = fastlz_compress(ZSTR_VAL(buf.s), ZSTR_LEN(buf.s), compressed);
						if(!compressed_len || compressed_len > buf.s->len){
							php_error_docref(NULL, E_WARNING, "Compression failed");
							efree(compressed);
							if (prefix->len) {
								zend_string_release(prefix_key);
							}
							return ret;
						}

						if(compressed_len > NODE_MAX_DATA_SIZE){
							php_error_docref(NULL, E_WARNING, "Value is too large (%d) can't be stored", Z_STRLEN_P(value));
							efree(compressed);
							if(prefix->len){
								zend_string_release(prefix_key);
							}
							return ret;
						}

						flag |= NPLC_ENTRY_COMPRESSED;
						flag |= (buf.s->len << NPLC_ENTRY_ORIG_LEN_SHIT);
						ret = npl_update_data(ZSTR_VAL(key), ZSTR_LEN(key), compressed, compressed_len, flag, &msg);
						efree(compressed);
					} else {
						ret = npl_update_data(ZSTR_VAL(key), ZSTR_LEN(key), ZSTR_VAL(buf.s), ZSTR_LEN(buf.s), flag, &msg);
					}
					smart_str_free(&buf);
				} else {
					php_error_docref(NULL, E_WARNING, "Serialization failed");
					smart_str_free(&buf);
				}
			}
			break;
		case IS_RESOURCE:
			php_error_docref(NULL, E_WARNING, "Type 'IS_RESOURCE' cannot be stored");
			break;
		default:
			php_error_docref(NULL, E_WARNING, "Unsupported valued type to be stored");
			break;

    }

    if(prefix->len) {
		zend_string_release(prefix_key);
	}

    return ret;
}

static int
nplc_add_multi(zend_string *prefix, zval *kvs)
{
	HashTable *ht = Z_ARRVAL_P(kvs);
	zend_string *key;
	zend_ulong idx;
	zval *value;

	ZEND_HASH_FOREACH_KEY_VAL(ht, idx, key, value) {
			uint32_t should_free = 0;
			if (!key) {
				key = strpprintf(0, "%lu", idx);
				should_free = 1;
			}

			if (nplc_add(prefix, key, value)) {
				if (should_free) {
					zend_string_release(key);
				}
				continue;
			} else {
				if (should_free) {
					zend_string_release(key);
				}
				return 0;
			}
	}
	ZEND_HASH_FOREACH_END();

	return 1;
}

static zval*
nplc_get(zend_string *prefix, zend_string *key, zval *rv)
{
	uint32_t flag, size = 0;
	char *data, *msg;
	zend_string *prefix_key;

	if((ZSTR_LEN(key) + prefix->len) > KEY_MAX_LEN){
		php_error_docref(NULL, E_WARNING, "Key%s can not be longer than %d bytes",
						prefix->len ? "(include prefix)" : "", KEY_MAX_LEN);
		return NULL;
	}

	if (prefix->len) {
		prefix_key = strpprintf(KEY_MAX_LEN, "%s%s", ZSTR_VAL(prefix), ZSTR_VAL(key));
		key = prefix_key;
	}

	data = npl_find_data(ZSTR_VAL(key), ZSTR_LEN(key), &size, &flag);

	switch(flag & NPLC_ENTRY_TYPE_MASK){
		case IS_NULL:
			if (size == sizeof(int)) {
				ZVAL_NULL(rv);
			}
			efree(data);
			break;
		case IS_TRUE:
			if (size == sizeof(int)) {
				ZVAL_TRUE(rv);
			}
			efree(data);
			break;
		case IS_FALSE:
			if (size == sizeof(int)) {
				ZVAL_FALSE(rv);
			}
			efree(data);
			break;
		case IS_LONG:
			if (size == sizeof(long)) {
				ZVAL_LONG(rv, *(long*)data);
			}
			efree(data);
			break;
		case IS_DOUBLE:
			if (size == sizeof(double)) {
				ZVAL_DOUBLE(rv, *(double*)data);
			}
			efree(data);
			break;
		case IS_STRING:
		case IS_CONSTANT:
			{
				if ((flag & NPLC_ENTRY_COMPRESSED)) {
					size_t orig_len = ((uint32_t)flag >> NPLC_ENTRY_ORIG_LEN_SHIT);
					char *origin = emalloc(orig_len + 1);
					uint32_t length;
					length = fastlz_decompress(data, size, origin, orig_len);
					if (!length) {
						php_error_docref(NULL, E_WARNING, "Decompression failed");
						efree(data);
						efree(origin);
						if (prefix->len) {
							zend_string_release(prefix_key);
						}
						return NULL;
					}
					ZVAL_STRINGL(rv, origin, length);
					efree(origin);
					efree(data);
				} else {
					ZVAL_STRINGL(rv, data, size);
					efree(data);
				}
			}
			break;
		case IS_ARRAY:
#ifdef IS_CONSTANT_ARRAY
		case IS_CONSTANT_ARRAY:
#endif
		case IS_OBJECT:
			{
				if ((flag & NPLC_ENTRY_COMPRESSED)) {
					size_t length, orig_len = ((uint32_t)flag >> NPLC_ENTRY_ORIG_LEN_SHIT);
					char *origin = emalloc(orig_len + 1);
					length = fastlz_decompress(data, size, origin, orig_len);
					if (!length) {
						php_error_docref(NULL, E_WARNING, "Decompression failed");
						efree(data);
						efree(origin);
						if (prefix->len) {
							zend_string_release(prefix_key);
						}
						return NULL;
					}
					efree(data);
					data = origin;
					size = length;
				}

				rv = nplc_serializer_php_unpack(data, size, &msg, rv);
				if (!rv) {
					php_error_docref(NULL, E_WARNING, "Unserialization failed");
				}
				efree(data);
			}
			break;
		default:
			php_error_docref(NULL, E_WARNING, "Unexpected valued type '%d'", flag);
			rv = NULL;
			break;
	}

	if (prefix->len) {
		zend_string_release(prefix_key);
	}

	return rv;
}

static zval *
nplc_get_multi(zend_string *prefix, zval *keys, zval *rv)
{
	zval *value;
	HashTable *ht = Z_ARRVAL_P(keys);

	array_init(rv);

	ZEND_HASH_FOREACH_VAL(ht, value) {
		zval *v, tmp_rv;

		ZVAL_UNDEF(&tmp_rv);

		switch (Z_TYPE_P(value)) {
			case IS_STRING:
				if ((v = nplc_get(prefix, Z_STR_P(value), &tmp_rv)) && !Z_ISUNDEF(tmp_rv)) {
					zend_symtable_update(Z_ARRVAL_P(rv), Z_STR_P(value), v);
				} else {
					ZVAL_FALSE(&tmp_rv);
					zend_symtable_update(Z_ARRVAL_P(rv), Z_STR_P(value), &tmp_rv);
				}
				continue;
			default:
				{
					zend_string *key = zval_get_string(value);
					if ((v = nplc_get(prefix, key, &tmp_rv)) && !Z_ISUNDEF(tmp_rv)) {
						zend_symtable_update(Z_ARRVAL_P(rv), key, v);
					} else {
						ZVAL_FALSE(&tmp_rv);
						zend_symtable_update(Z_ARRVAL_P(rv), key, &tmp_rv);
					}
					zend_string_release(key);
				}
				continue;
		}
	} ZEND_HASH_FOREACH_END();

	return rv;
}

void
nplc_delete(char *prefix, uint32_t prefix_len, char *key, uint32_t len)
{
	char buf[KEY_MAX_LEN];

	if ((len + prefix_len) > KEY_MAX_LEN) {
		php_error_docref(NULL, E_WARNING, "Key%s can not be longer than %d bytes",
				prefix_len ? "(include prefix)" : "", KEY_MAX_LEN);
		return;
	}

	if (prefix_len) {
		len = snprintf(buf, sizeof(buf), "%s%s", prefix, key);
		key = (char *)buf;
	}

	npl_delete_data(key, len);
}

void
nplc_delete_multi(char *prefix, uint32_t prefix_len, zval *keys)
{
	zval *value;
	HashTable *ht = Z_ARRVAL_P(keys);

	ZEND_HASH_FOREACH_VAL(ht, value) {
		switch (Z_TYPE_P(value)) {
			case IS_STRING:
				nplc_delete(prefix, prefix_len, Z_STRVAL_P(value), Z_STRLEN_P(value));
				continue;
			default:
				{
					zval copy;
					zend_make_printable_zval(value, &copy);
					nplc_delete(prefix, prefix_len, Z_STRVAL(copy), Z_STRLEN(copy));
					zval_dtor(&copy);
				}
				continue;
		}
	} ZEND_HASH_FOREACH_END();

	return;
}

/** {{{ proto public Nplc::__construct([string $prefix]) */
PHP_METHOD(nplc, __construct) {
	zend_string *prefix = NULL;

	if (!NPLC_G(enable)) {
		return;
	}

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "|S", &prefix) == FAILURE) {
		return;
	}

	if (!prefix) {
		return;
	}

	zend_update_property_str(nplc_class_ce, getThis(), ZEND_STRL(NPLC_CLASS_PROPERTY_PREFIX), prefix);

}
/* }}} */

/** {{{ proto public Nplc::set(mixed $keys, mixed $value)
*/
PHP_METHOD(nplc, set) {
    long ttl = 0;
	zval rv, *keys, *prefix, *value = NULL;
	uint32_t ret;

	if (!NPLC_G(enable)) {
		RETURN_FALSE;
	}

	switch (ZEND_NUM_ARGS()) {
		case 1:
			if (zend_parse_parameters(ZEND_NUM_ARGS(), "a", &keys) == FAILURE) {
				return;
			}
			break;
		case 2:
			if (zend_parse_parameters(ZEND_NUM_ARGS(), "zz", &keys, &value) == FAILURE) {
				return;
			}
			if (Z_TYPE_P(keys) == IS_ARRAY) {
				value = NULL;
			}
			break;
		default:
			WRONG_PARAM_COUNT;
	}

	prefix = zend_read_property(nplc_class_ce, getThis(), ZEND_STRL(NPLC_CLASS_PROPERTY_PREFIX), 0, &rv);

	if (Z_TYPE_P(keys) == IS_ARRAY) {
		ret = nplc_add_multi(Z_STR_P(prefix), keys);
	} else if (Z_TYPE_P(keys) == IS_STRING) {
		ret = nplc_add(Z_STR_P(prefix), Z_STR_P(keys), value);
	} else {
		zval copy;
		zend_make_printable_zval(keys, &copy);
		ret = nplc_add(Z_STR_P(prefix), Z_STR(copy), value);
		zval_dtor(&copy);
	}

	RETURN_BOOL(ret);
}
/* }}} */


/** {{{ proto public Nplc::get(mixed $keys)
*/
PHP_METHOD(nplc, get) {
	zval rv, *prefix, *ret, *keys;

	if(!NPLC_G(enable)){
		RETURN_FALSE;
	}

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &keys) == FAILURE) {
		return;
	}

	prefix = zend_read_property(nplc_class_ce, getThis(), ZEND_STRL(NPLC_CLASS_PROPERTY_PREFIX), 0, &rv);

	if (Z_TYPE_P(keys) == IS_ARRAY) {
		ret = nplc_get_multi(Z_STR_P(prefix), keys, return_value);
	} else if (Z_TYPE_P(keys) == IS_STRING) {
		ret = nplc_get(Z_STR_P(prefix), Z_STR_P(keys), return_value);
	} else {
		zval copy;
		zend_make_printable_zval(keys, &copy);
		ret = nplc_get(Z_STR_P(prefix), Z_STR(copy), return_value);
		zval_dtor(&copy);
	}

	if (ret == NULL) {
		RETURN_FALSE;
	}
}
/* }}} */

/** {{{ proto public Nplc::delete(mixed $keys)
*/
PHP_METHOD(nplc, delete) {
	zval *keys, rv, *prefix;
	char *sprefix = NULL;
	uint32_t prefix_len;

	if (!NPLC_G(enable)) {
		RETURN_FALSE;
	}

   /**
    * todo 延迟删除
 	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z|l", &keys, &time) == FAILURE) {
		return;
	}*/

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &keys) == FAILURE) {
		return;
	}

	prefix  = zend_read_property(nplc_class_ce, getThis(), ZEND_STRL(NPLC_CLASS_PROPERTY_PREFIX), 0, &rv);
	sprefix = Z_STRVAL_P(prefix);
	prefix_len = Z_STRLEN_P(prefix);

	if (Z_TYPE_P(keys) == IS_ARRAY) {
		nplc_delete_multi(sprefix, prefix_len, keys);
	} else if (Z_TYPE_P(keys) == IS_STRING) {
		nplc_delete(sprefix, prefix_len, Z_STRVAL_P(keys), Z_STRLEN_P(keys));
	} else {
		zval copy;
		zend_make_printable_zval(keys, &copy);
		nplc_delete(sprefix, prefix_len, Z_STRVAL(copy), Z_STRLEN(copy));
		zval_dtor(&copy);
	}

	RETURN_TRUE;
}
/* }}} */

/** {{{ proto public Nplc::info(void)
*/
PHP_METHOD(nplc, info) {
	npl_cache_info *info;

	if (!NPLC_G(enable)) {
		RETURN_FALSE;
	}

	info = npl_info();

	array_init(return_value);
	add_assoc_long(return_value, "storage_size", info->storage_size);
	add_assoc_long(return_value, "node_nums", info->node_nums);
	add_assoc_long(return_value, "keys_nums", info->keys_nums);
	add_assoc_long(return_value, "fail_nums", info->fail_nums);
	add_assoc_long(return_value, "miss_nums", info->miss_nums);
	add_assoc_long(return_value, "recycles_nums", info->recycles_nums);

	npl_info_free(info);
}
/* }}} */

/** {{{ nplc_methods */
zend_function_entry nplc_methods[] = {
	PHP_ME(nplc, __construct, arginfo_nplc_constructor, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	//PHP_ME(nplc, add, arginfo_nplc_add, ZEND_ACC_PUBLIC)
	PHP_ME(nplc, set, arginfo_nplc_setter, ZEND_ACC_PUBLIC)
	//PHP_ME(nplc, __set, arginfo_nplc_setter, ZEND_ACC_PUBLIC)
	PHP_ME(nplc, get, arginfo_nplc_getter, ZEND_ACC_PUBLIC)
	//PHP_ME(nplc, __get, arginfo_nplc_getter, ZEND_ACC_PUBLIC)
	PHP_ME(nplc, delete, arginfo_nplc_delete, ZEND_ACC_PUBLIC)
	//PHP_ME(nplc, flush, arginfo_nplc_void, ZEND_ACC_PUBLIC)
	PHP_ME(nplc, info, arginfo_nplc_void, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ php_nplc_init_globals */
/* Uncomment this function if you have INI entries */
static void php_nplc_init_globals(zend_nplc_globals *nplc_globals)
{
	nplc_globals->enable = 1;
	nplc_globals->enable_cli = 0;
	nplc_globals->storage_size = 64 * 1024 * 1024;
	//nplc_globals->global_value = 0;
	//nplc_globals->global_string = NULL;
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(nplc)
{
	/* If you have INI entries, uncomment these lines */
	REGISTER_INI_ENTRIES();

	zend_class_entry ce;
	unsigned long s_size;

	if(!NPLC_G(enable_cli) && !strcmp(sapi_module.name, "cli")) {
		NPLC_G(enable) = 0;
	}

	if (NPLC_G(enable)) {
		s_size = NPLC_G(storage_size);
		if (!npl_start(s_size)) {
			php_error(E_ERROR, "Shared memory allocator startup failed %s", strerror(errno));
			return FAILURE;
		}
	}

	REGISTER_STRINGL_CONSTANT("NPLC_VERSION", PHP_NPLC_VERSION, sizeof(PHP_NPLC_VERSION) - 1, CONST_PERSISTENT | CONST_CS);

	INIT_CLASS_ENTRY(ce, "Nplc", nplc_methods);
	nplc_class_ce = zend_register_internal_class(&ce);
	zend_declare_property_stringl(nplc_class_ce, ZEND_STRS(NPLC_CLASS_PROPERTY_PREFIX) - 1, "", 0, ZEND_ACC_PROTECTED);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(nplc)
{
	/* uncomment this line if you have INI entries */
	UNREGISTER_INI_ENTRIES();
	if (NPLC_G(enable)) {
		npl_shutdown();
	}

	return SUCCESS;
}
/* }}} */

#if 0
/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(nplc)
{
#if defined(COMPILE_DL_NPLC) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(nplc)
{
	return SUCCESS;
}
/* }}} */
#endif

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(nplc)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "nplc support", NPLC_LOGO_IMG"enabled");
	php_info_print_table_row(2, "Version", PHP_NPLC_VERSION);
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini */
	DISPLAY_INI_ENTRIES();
}
/* }}} */

/* {{{ nplc_module_entry
 */
zend_module_entry nplc_module_entry = {
	STANDARD_MODULE_HEADER,
	"nplc",
	NULL, //nplc_functions,
	PHP_MINIT(nplc),
	PHP_MSHUTDOWN(nplc),
	NULL,//PHP_RINIT(nplc),		/* Replace with NULL if there's nothing to do at request start */
	NULL,//PHP_RSHUTDOWN(nplc),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(nplc),
	PHP_NPLC_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_NPLC
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(nplc)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
