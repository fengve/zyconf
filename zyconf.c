/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2013 The PHP Group                                |
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

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_zyconf.h"
#include "main/php_scandir.h"

ZEND_DECLARE_MODULE_GLOBALS(zyconf)

static int le_zyconf;
static int zyconf_update_interval = 15;//15s
zend_class_entry *zyconf_ce;
HashTable *zyconf_file_name;
HashTable *parsed_ini_files;

typedef struct _zyconf_filenode {
	char *filename;
	time_t mtime;
} zyconf_filenode;


#define PALLOC_HASHTABLE(ht)   do {                         \
    (ht) = (HashTable*)pemalloc(sizeof(HashTable), 1);		\
    if ((ht) == NULL) {                                     \
        zend_error(E_ERROR, "Cannot allocate HashTable");	\
    }														\
															\
} while(0)

 //使用持久化内存
#define ZYCONF_MAKE_STD_ZVAL(zv)	do{				\
	(zv) = (zval *) pemalloc(sizeof(zval), 1);		\
	INIT_PZVAL(zv); 								\
} while (0)

				  
ZEND_BEGIN_ARG_INFO_EX(php_zyconf_get_arginfo, 0, 0, 2)
	ZEND_ARG_INFO(0, project)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(php_zyconf_has_arginfo, 0, 0, 2)
	ZEND_ARG_INFO(0, project)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()



const zend_function_entry zyconf_methods[]={
	PHP_ME(zyconf, get, php_zyconf_get_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(zyconf, has, php_zyconf_has_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
    PHP_FE_END
};

/* {{{ zyconf_module_entry
 */
zend_module_entry zyconf_module_entry = {
	STANDARD_MODULE_HEADER,
	"zyconf",
	NULL,
	PHP_MINIT(zyconf),
	PHP_MSHUTDOWN(zyconf),
	PHP_RINIT(zyconf),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(zyconf),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(zyconf),
	ZYCONF_VERSION, 
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_ZYCONF
ZEND_GET_MODULE(zyconf)
#endif


PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("zyconf.directory", "", PHP_INI_ALL, OnUpdateString, directory, zend_zyconf_globals, zyconf_globals)
PHP_INI_END()


static void php_zyconf_init_globals(zend_zyconf_globals *zyconf_globals)
{
	zyconf_globals->directory = NULL;
}


PHP_MINIT_FUNCTION(zyconf)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, "zyconf", zyconf_methods);
	zyconf_ce = zend_register_internal_class(&ce TSRMLS_CC);

    zend_declare_class_constant_string(zyconf_ce, "version", strlen("version"), ZYCONF_VERSION TSRMLS_CC);

	REGISTER_INI_ENTRIES();	

	const char *dirname;
	size_t dirlen;
	if ((dirname = ZYCONF_G(directory)) && (dirlen = strlen(dirname))){
		zval *configs;
		int ndir;
		struct dirent **namelist;
		char *p, ini_file[MAXPATHLEN];

		if ((ndir = php_scandir(dirname, &namelist, 0, php_alphasort)) > 0) {			  
			int i;
			struct stat sb;	 
			zend_file_handle fh = {0};

		
			PALLOC_HASHTABLE(zyconf_file_name);
			zend_hash_init(zyconf_file_name, ndir-2, NULL, NULL, 1);
			
			PALLOC_HASHTABLE(parsed_ini_files);
			zend_hash_init(parsed_ini_files, ndir-2, NULL, NULL, 1);
			
			
			for(i=0; i<ndir; i++){
				if (!(p = strrchr(namelist[i]->d_name, '.')) || strcmp(p, ".ini")) {
					free(namelist[i]);
					continue;
				}
				snprintf(ini_file, MAXPATHLEN, "%s%c%s", dirname, DEFAULT_SLASH, namelist[i]->d_name);
				
				if(VCWD_STAT(ini_file, &sb) == 0){
					if (S_ISREG(sb.st_mode)){
						zyconf_filenode *node;
                        if ((fh.handle.fp = VCWD_FOPEN(ini_file, "r"))){
							fh.filename = ini_file;
                            fh.type = ZEND_HANDLE_FP;
							
							//MAKE_STD_ZVAL(configs);
							//ZVAL_NULL(configs);
							//array_init(configs);
							
							ZYCONF_MAKE_STD_ZVAL(configs);   
							php_zyconf_hash_init(configs, 8);


                            if (zend_parse_ini_file(&fh, 0, 0, php_zyconf_ini_parser_cb, configs TSRMLS_CC) == FAILURE 
                                    || ZYCONF_G(parse_err)) {
                            
                                if (!ZYCONF_G(parse_err)) {
                                    php_error(E_WARNING, "Parsing '%s' failed", ini_file);
                                }

								zval_ptr_dtor(&configs);
                                ZYCONF_G(parse_err) = 0;
                                free(namelist[i]);
                                continue;
                            }

							zend_symtable_update(zyconf_file_name, namelist[i]->d_name, strlen(namelist[i]->d_name)+1, (void *)&configs, sizeof(zval *), NULL);

							node = pemalloc(sizeof(zyconf_filenode), 1);
							node->filename =  pestrndup(namelist[i]->d_name, strlen(namelist[i]->d_name), 1);
							node->mtime = sb.st_mtime;

							
							zend_symtable_update(parsed_ini_files, node->filename, strlen(namelist[i]->d_name)+1, (void *)&node, sizeof(zyconf_filenode *), NULL);
							/*
							zval **ppzval,**ppzval2;
							zend_symtable_find(zyconf_file_name, namelist[i]->d_name, strlen(namelist[i]->d_name)+1, (void **)&ppzval);
							
							php_printf(" %s, %d, %p, %p\n", namelist[i]->d_name, strlen(namelist[i]->d_name)+1, ppzval, Z_ARRVAL_PP(ppzval));

							zend_symtable_find(Z_ARRVAL_P(configs), "cps_rest_cps", strlen("cps_rest_cps")+1, (void **)&ppzval2);
							php_printf(" %d, %s, %p\n", Z_STRLEN_PP(ppzval2), Z_STRVAL_PP(ppzval2), &Z_STRVAL_PP(ppzval2));
					       

							zend_symtable_find(Z_ARRVAL_PP(ppzval), "cps_rest_cps", strlen("cps_rest_cps")+1, (void **)&ppzval2);
							php_printf(" %p, %s ,%d\n", Z_STRVAL_PP(ppzval2), Z_STRVAL_PP(ppzval2), Z_STRLEN_PP(ppzval2));
							*/
						}else{
							php_error(E_ERROR, " Cloud not open %s ", ini_file);
						}
					}
				}else{
					php_error(E_ERROR, " Cloud not stat %s ", ini_file);
				}
				
			}
			
			ZYCONF_G(last_check) = time(NULL);
			
		}else{
			php_error(E_ERROR, " Cloud not opendir %s ", dirname);
		}
	}else{
		php_error(E_ERROR, " In php.ini zyconf.directory cat not empty ");
	}
	
	return SUCCESS;
}


PHP_MSHUTDOWN_FUNCTION(zyconf)
{
	UNREGISTER_INI_ENTRIES();
	if(zyconf_file_name){
		php_printf("\nPHP_MSHUTDOWN_FUNCTION\n");
		//php_zyconf_hash_destroy(zyconf_file_name);
	} 
	php_printf("\nPHP_MSHUTDOWN_FUNCTION222222\n");
	return SUCCESS;
}


PHP_RINIT_FUNCTION(zyconf)
{
	time_t curr_time = time(NULL);
	//在 $_SERVER 中获取 REQUEST_TIME
	//zval **request_time;
	//zend_hash_find(HASH_OF(PG(http_globals)[TRACK_VARS_SERVER]), ZEND_STRS("REQUEST_TIME"), (void **)&request_time);
	
	//不到更新时间
	if(curr_time - ZYCONF_G(last_check) < zyconf_update_interval){
		return SUCCESS;
	}
	
	ZYCONF_G(last_check) = curr_time;
	
	const char *dirname;
	size_t dirlen;
	
	if ((dirname = ZYCONF_G(directory)) && (dirlen = strlen(dirname))){
		zval *configs;
		int ndir;
		struct dirent **namelist;
		char *p, ini_file[MAXPATHLEN];
		zyconf_filenode **node, *node_new;

		if ((ndir = php_scandir(dirname, &namelist, 0, php_alphasort)) > 0) {
			int i;
			struct stat sb;	 
			zend_file_handle fh = {0};
			
			for(i=0; i<ndir; i++){
				if (!(p = strrchr(namelist[i]->d_name, '.')) || strcmp(p, ".ini")) {
					free(namelist[i]);
					continue;
				}
				snprintf(ini_file, MAXPATHLEN, "%s%c%s", dirname, DEFAULT_SLASH, namelist[i]->d_name);
			
				if(VCWD_STAT(ini_file, &sb) == 0){
					if (S_ISREG(sb.st_mode)){
						
						zend_symtable_find(parsed_ini_files, namelist[i]->d_name, strlen(namelist[i]->d_name)+1, (void **)&node);
						if((**node).mtime == sb.st_mtime){
							free(namelist[i]);
							continue;
						}
						
						if ((fh.handle.fp = VCWD_FOPEN(ini_file, "r"))){
							fh.filename = ini_file;
							fh.type = ZEND_HANDLE_FP;
							
							ZYCONF_MAKE_STD_ZVAL(configs);   
							php_zyconf_hash_init(configs, 8);
							
							if (zend_parse_ini_file(&fh, 0, 0, php_zyconf_ini_parser_cb, configs TSRMLS_CC) == FAILURE 
									|| ZYCONF_G(parse_err)) {
							
								if (!ZYCONF_G(parse_err)) {
									php_error(E_WARNING, "Parsing '%s' failed", ini_file);
								}

								ZYCONF_G(parse_err) = 0;
								free(namelist[i]);
								continue;
							}

							zend_symtable_update(zyconf_file_name, namelist[i]->d_name, strlen(namelist[i]->d_name)+1, (void *)&configs, sizeof(zval *), NULL);

							if(node != NULL){
								free(*node);
							}
							
							node_new = pemalloc(sizeof(zyconf_filenode), 1);
							node_new->filename =  pestrndup(namelist[i]->d_name, strlen(namelist[i]->d_name), 1);
							node_new->mtime = sb.st_mtime;

							zend_symtable_update(parsed_ini_files, node_new->filename, strlen(namelist[i]->d_name)+1, (void *)&node_new, sizeof(zyconf_filenode *), NULL);
							
						}
					}
				}
			}
		}
	}
	
	
	
	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(zyconf)
{
	return SUCCESS;
}



PHP_MINFO_FUNCTION(zyconf)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "zyconf support", "enabled");
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}

static void php_zyconf_ini_parser_cb(zval *key, zval *value, zval *index, int callback_type, zval *arr ) {

    zval *element;
    switch (callback_type) {
        case ZEND_INI_PARSER_ENTRY:
            {
                char *skey, *seg, *ptr;
                zval **ppzval, *dst;

                if (!value) {
                    break;
                }

                dst = arr;
                skey = estrndup(Z_STRVAL_P(key), Z_STRLEN_P(key));
                if ((seg = php_strtok_r(skey, ".", &ptr))) {
                    do {
                        char *real_key = seg;
                        seg = php_strtok_r(NULL, ".", &ptr);
                        if (zend_symtable_find(Z_ARRVAL_P(dst), real_key, strlen(real_key) + 1, (void **) &ppzval) == FAILURE) {
                            if (seg) {
                                zval *tmp;
                                ZYCONF_MAKE_STD_ZVAL(tmp);   
								php_zyconf_hash_init(tmp, 8);
                                zend_symtable_update(Z_ARRVAL_P(dst), real_key, strlen(real_key) + 1, (void **)&tmp, sizeof(zval *), (void **)&ppzval);
                            } else {
                                
								ZYCONF_MAKE_STD_ZVAL(element);
                                //ZVAL_ZVAL(element, value, 1, 0);
								php_zyconf_zval_persistent(value, element);

                                zend_symtable_update(Z_ARRVAL_P(dst), real_key, strlen(real_key) + 1, (void **)&element, sizeof(zval *), NULL);
                                break;
                            }
                        } else {
                            SEPARATE_ZVAL(ppzval);
                            if (IS_ARRAY != Z_TYPE_PP(ppzval)) {
                                if (seg) {
                                    zval *tmp;
                                    ZYCONF_MAKE_STD_ZVAL(tmp);   
                                    //array_init(tmp);
									php_zyconf_hash_init(tmp, 8);
                                    zend_symtable_update(Z_ARRVAL_P(dst), real_key, strlen(real_key) + 1, (void **)&tmp, sizeof(zval *), (void **)&ppzval);
                                } else {
                                    ZYCONF_MAKE_STD_ZVAL(element);
                                    //ZVAL_ZVAL(element, value, 1, 0);
									php_zyconf_zval_persistent(value, element);
                                    zend_symtable_update(Z_ARRVAL_P(dst), real_key, strlen(real_key) + 1, (void **)&element, sizeof(zval *), NULL);
                                }
                            } 
                        }
                        dst = *ppzval;
                    } while (seg);
                }
                efree(skey);
            }
            break;

        case ZEND_INI_PARSER_POP_ENTRY:
            {
                zval *hash, **find_hash, *dst;

                if (!value) {
                    break;
                }

                if (!(Z_STRLEN_P(key) > 1 && Z_STRVAL_P(key)[0] == '0')
                        && is_numeric_string(Z_STRVAL_P(key), Z_STRLEN_P(key), NULL, NULL, 0) == IS_LONG) {
                    ulong skey = (ulong)zend_atol(Z_STRVAL_P(key), Z_STRLEN_P(key));
                    if (zend_hash_index_find(Z_ARRVAL_P(arr), skey, (void **) &find_hash) == FAILURE) {
                        ZYCONF_MAKE_STD_ZVAL(hash);
                        //array_init(hash);
						php_zyconf_hash_init(hash, 8);
                        zend_hash_index_update(Z_ARRVAL_P(arr), skey, &hash, sizeof(zval *), NULL);
                    } else {
                        hash = *find_hash;
                    }
                } else {
                    char *seg, *ptr;
                    char *skey = estrndup(Z_STRVAL_P(key), Z_STRLEN_P(key));

                    dst = arr;
                    if ((seg = php_strtok_r(skey, ".", &ptr))) {
                        while (seg) {
                            if (zend_symtable_find(Z_ARRVAL_P(dst), seg, strlen(seg) + 1, (void **) &find_hash) == FAILURE) {
                                ZYCONF_MAKE_STD_ZVAL(hash);
                                //array_init(hash);
								php_zyconf_hash_init(hash, 8);
                                zend_symtable_update(Z_ARRVAL_P(dst), 
                                        seg, strlen(seg) + 1, (void **)&hash, sizeof(zval *), (void **)&find_hash);
                            }
                            dst = *find_hash;
                            seg = php_strtok_r(NULL, ".", &ptr);
                        }
                        hash = dst;
                    } else {
                        if (zend_symtable_find(Z_ARRVAL_P(dst), seg, strlen(seg) + 1, (void **)&find_hash) == FAILURE) {
                            ZYCONF_MAKE_STD_ZVAL(hash);
                            //array_init(hash);
							php_zyconf_hash_init(hash, 8);
                            zend_symtable_update(Z_ARRVAL_P(dst), seg, strlen(seg) + 1, (void **)&hash, sizeof(zval *), NULL);
                        } else {
                            hash = *find_hash;
                        }
                    }
                    efree(skey);
                }

                if (Z_TYPE_P(hash) != IS_ARRAY) {
                    zval_dtor(hash);
                    INIT_PZVAL(hash);
                    array_init(hash);
                }

                ZYCONF_MAKE_STD_ZVAL(element);
                //ZVAL_ZVAL(element, value, 1, 0);
				php_zyconf_zval_persistent(value, element);

                if (index && Z_STRLEN_P(index) > 0) {
                    add_assoc_zval_ex(hash, Z_STRVAL_P(index), Z_STRLEN_P(index) + 1, element);
                } else {
                    add_next_index_zval(hash, element);
                }
            }
            break;

        case ZEND_INI_PARSER_SECTION:
            break;
    }
}

static void php_zyconf_zval_persistent(zval *zv, zval *rv){
	switch (Z_TYPE_P(zv)) {
		case IS_CONSTANT:
		case IS_STRING:
			{

				Z_TYPE_P(rv) = IS_STRING;
				Z_STRLEN_P(rv) = Z_STRLEN_P(zv);
				//持久化内存
				Z_STRVAL_P(rv) = pestrndup(Z_STRVAL_P(zv), Z_STRLEN_P(zv), 1);
				
			}
			break;
		case IS_ARRAY:
			{
				php_zyconf_hash_init(rv, zend_hash_num_elements(Z_ARRVAL_P(zv)));
				zend_hash_copy(Z_ARRVAL_P(rv), Z_ARRVAL_P(zv), NULL, NULL, sizeof(void *));
			}
			break;
		case IS_RESOURCE:
		case IS_OBJECT:
		case IS_BOOL:
		case IS_LONG:
		case IS_NULL:
			 assert(0);
			break;
	}
}


static void php_zyconf_hash_destroy(HashTable *ht){
    Bucket *p, *q;
	zval **element;

    p = ht->pListHead;
    while (p != NULL) {
        q = p;
        p = p->pListNext;

		element = (zval **) q->pData;

		switch(Z_TYPE_PP(element)){
			case IS_STRING:
				pefree(Z_STRVAL_PP(element), 1);
				break;
			case IS_ARRAY:
				   php_zyconf_hash_destroy(Z_ARRVAL_PP(element));
				break;
		}

        if (ht->pDestructor) {
            ht->pDestructor(q->pData);
        }
        
        pefree(q, ht->persistent);
    }


    if (ht->nTableMask) {
        pefree(ht->arBuckets, ht->persistent);
    }

}

PHPAPI zval * php_zyconf_get(char *file_name, int file_name_len, char *name, int name_len) { 
	zval **ppconf, **ppzval; 
	int result;
	char file_name_ini[200];
	HashTable *target;
	
	if(zyconf_file_name){

		do{
		//获得文件下所有的

			strcpy(file_name_ini, file_name);
			strcat(file_name_ini, ".ini");
			result = zend_symtable_find(zyconf_file_name, file_name_ini, strlen(file_name_ini)+1, (void **)&ppconf);
			
			if(result==FAILURE){
				break;
			}
		    target = Z_ARRVAL_PP(ppconf);
			
			char *entry, *seg, *pptr;
			entry = estrndup(name, name_len);
			if((seg = php_strtok_r(entry, ".", &pptr))){
				while (seg) {
					if (zend_hash_find(target, seg, strlen(seg) + 1, (void **) &ppzval) == FAILURE) {
						efree(entry);
						return NULL;
					}

					target = Z_ARRVAL_PP(ppzval);
					seg = php_strtok_r(NULL, ".", &pptr);
				}
			}else{
				result = zend_symtable_find(target, seg, strlen(seg) + 1, (void **)&ppzval);
				/*
				php_printf("%p, %s, %d\n", target, name, name_len+1);
				php_printf("%p, %s ,%d\n", Z_STRVAL_PP(ppzval), Z_STRVAL_PP(ppzval), Z_STRLEN_PP(ppzval));
				*/
			}
			efree(entry);
			if(result == SUCCESS)  return *ppzval; 

		}while(0);
	}
	return NULL;
}

PHPAPI int php_zyconf_has(char *file_name, int file_name_len, char *name, int name_len) {
	 if (php_zyconf_get(file_name, file_name_len, name, name_len)) {
        return 1;
    }
	return 0;
}

static inline void php_zyconf_hash_init(zval *zv, size_t size) /* {{{ */ {
    HashTable *ht;
    PALLOC_HASHTABLE(ht);
    zend_hash_init(ht, size, NULL, NULL, 1);
   
    Z_ARRVAL_P(zv) = ht; 
    Z_TYPE_P(zv) = IS_ARRAY;
	INIT_PZVAL(zv); 
} 
  
PHP_METHOD(zyconf, get){

    char *name;
    int name_len;
	char *file_name;
    int file_name_len;
    zval *reval;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &file_name, &file_name_len, &name, &name_len) == FAILURE){
        return;
    }

    reval = php_zyconf_get(file_name, file_name_len, name, name_len);
    if(reval){
    	RETURN_ZVAL(reval, 1, 0);
    }
    RETURN_NULL();
}

PHP_METHOD(zyconf, has){

    char *name;
    int name_len;
	char *file_name;
    int file_name_len;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &file_name, &file_name_len, &name, &name_len) == FAILURE){
        return;
    }

    RETURN_BOOL(php_zyconf_has(file_name, file_name_len, name, name_len));
}

