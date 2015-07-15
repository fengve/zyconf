# zyconf
持久化配置




###  Install zyconf 

```
$/path/to/phpize
$./configure --with-php-config=/path/to/php-config/
$make && make install
```
###  php.ini
```
//指定ini文件所在的位置
zyconf.directory=/tmp/zyconf
```
###  zyconf Api
```
mixed zyconf::get(String filename, String  key)
boolean zyconf::has(String filename, String  key)

```

###  Example

在/tmp/zyconf目录创建test.ini

test.ini
```
id=3256
ip="127.0.0.1"

memcache_ip[]="127.0.0.1:11211"
memcache_ip[]="127.0.0.1:10000"



;
struct.name=fw
struct.age=18
struct.gender=m

;test boolean
struct.lang.php=yes
struct.lang.c=true
struct.lang.cpp=1
struct.lang.js=on

struct.lang.java=no
struct.lang.python=false
struct.lanng.perl=0
struct.lang.ruby=off


```
