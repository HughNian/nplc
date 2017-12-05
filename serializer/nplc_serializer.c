#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if !ENABLE_MSGPACK

#include "php.h"
#include "ext/standard/php_var.h" /* for serialize */
#include "zend_smart_str.h"
#include "nplc_serializer.h"

int
nplc_serializer_php_pack(zval *pzval, smart_str *buf, char **msg) /* {{{ */ {
	php_serialize_data_t var_hash;

	PHP_VAR_SERIALIZE_INIT(var_hash);
	php_var_serialize(buf, pzval, &var_hash);
	PHP_VAR_SERIALIZE_DESTROY(var_hash);

	return 1;
} /* }}} */

zval *
nplc_serializer_php_unpack(char *content, size_t len, char **msg, zval *rv) /* {{{ */ {
	const unsigned char *p;
	php_unserialize_data_t var_hash;
	p = (const unsigned char*)content;

	ZVAL_FALSE(rv);
	PHP_VAR_UNSERIALIZE_INIT(var_hash);
	if (!php_var_unserialize(rv, &p, p + len,  &var_hash)) {
		zval_ptr_dtor(rv);
		PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
		spprintf(msg, 0, "unpack error at offset %ld of %ld bytes", (long)((char*)p - content), len);
		return NULL;
	}
	PHP_VAR_UNSERIALIZE_DESTROY(var_hash);

	return rv;
} /* }}} */

#endif
