
#ifndef PHP_ZYCONF_H
#define PHP_ZYCONF_H

extern zend_module_entry zyconf_module_entry;
#define phpext_zyconf_ptr &zyconf_module_entry

#ifdef PHP_WIN32
#	define PHP_ZYCONF_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_ZYCONF_API __attribute__ ((visibility("default")))
#else
#	define PHP_ZYCONF_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_MINIT_FUNCTION(zyconf);
PHP_MSHUTDOWN_FUNCTION(zyconf);
PHP_RINIT_FUNCTION(zyconf);
PHP_RSHUTDOWN_FUNCTION(zyconf);
PHP_MINFO_FUNCTION(zyconf);

PHP_METHOD(zyconf, get);
PHP_METHOD(zyconf, has);
#define ZYCONF_VERSION  "1.0.0"


ZEND_BEGIN_MODULE_GLOBALS(zyconf)
	char *directory;
	int parse_err;
	time_t last_check;
ZEND_END_MODULE_GLOBALS(zyconf)


BEGIN_EXTERN_C() 
	PHPAPI int php_zyconf_has(char *file_name, int file_name_len, char *name, int name_len);
	PHPAPI zval * php_zyconf_get(char *file_name, int file_name_len, char *name, int name_len);
	static void php_zyconf_ini_parser_cb(zval *key, zval *value, zval *index, int callback_type, zval *arg);
	static inline void php_zyconf_hash_init(zval *zv, size_t size);
	static void php_zyconf_zval_persistent(zval *zv, zval *rv);
	static void php_zyconf_hash_destroy(HashTable *ht);
END_EXTERN_C()



#ifdef ZTS
#define ZYCONF_G(v) TSRMG(zyconf_globals_id, zend_zyconf_globals *, v)
#else
#define ZYCONF_G(v) (zyconf_globals.v)
#endif

#endif 
