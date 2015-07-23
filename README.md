# zyconf
持久化配置

php version >= 5.3   ， Non Thread Safe


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

;普通数组
memcache_ip[]="127.0.0.1:11211"
memcache_ip[]="127.0.0.1:10000"

;带有索引的数组
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

test.php
```
var_dump(zyconf::get('test', 'struct'));
var_dump(zyconf::get('test', 'struct.name'));
var_dump(zyconf::get('test', 'memcache_ip'));

___________________________________________________________________
array(5) {
  ["name"]=>
  string(7) "fw"
  ["age"]=>
  string(2) "18"
  ["gender"]=>
  string(1) "m"
  ["lang"]=>
  array(7) {
    ["php"]=>
    string(1) "1"
    ["c"]=>
    string(1) "1"
    ["cpp"]=>
    string(1) "1"
    ["js"]=>
    string(1) "1"
    ["java"]=>
    string(0) ""
    ["python"]=>
    string(0) ""
    ["ruby"]=>
    string(0) ""
  }
  ["lanng"]=>
  array(1) {
    ["perl"]=>
    string(1) "0"
  }
}



string(7) "fw"



array(2) {
  [0]=>
  string(15) "127.0.0.1:11211"
  [1]=>
  string(15) "127.0.0.1:10000"
}
```

