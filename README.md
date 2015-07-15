# zyconf
持久化配置




###  Install zyconf 

```
$/path/to/phpize
$./configure --with-php-config=/path/to/php-config/
$make && make install
```

###  zyconf Api
```
mixed zyconf::get(String filename, String  key)
boolean zyconf::has(String filename, String  key)

```
