#ifndef NPLC_SERIALIZER_H
#define NPLC_SERIALIZER_H

int nplc_serializer_php_pack(zval *pzval, smart_str *buf, char **msg);
zval *nplc_serializer_php_unpack(char *content, size_t len, char **msg, zval *rv);

#endif
