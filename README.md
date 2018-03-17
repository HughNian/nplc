# Nplc - php extension local cache

## Logo :trollface:
![logo](https://raw.githubusercontent.com/HughNian/nplc/master/logo.jpg)

```php
`____``_____`````````__``````````
|_```\|_```_|```````[``|`````````
``|```\`|`|``_`.--.``|`|``.---.``
``|`|\`\|`|`[`'/'`\`\|`|`/`/'`\]`
`_|`|_\```|_`|`\__/`||`|`|`\__.``
|_____|\____||`;.__/[___]'.___.'`
````````````[__|`````````````````
```

## 扩展信息页面
![phpinfo](https://raw.githubusercontent.com/HughNian/nplc/master/phpinfo.jpg)

目前测试通过php7

## Methods

### Nplc::__construct
```
   Nplc::__construct([string $prefix = ""])
```
```php
<?php
   $nplc = new Nplc();
?>
```

### Nplc::set
```
   Nplc::set($key, $value[, $tv]) //$tv 选填项超时时间
   Nplc::set(array $kvs[, $tv])   //$tv 选填项超时时间
```
   Store a value into Nplc cache. 
```php
<?php
$nplc = new Nplc();
$nplc->set("name", "niansong");
$nplc->set(
    array(
        "name" => "niansong",
        "age" => 20,
        )
    );
?>
```

### Nplc::get
```
   Nplc::get(string $key)
```
```php
<?php
$nplc = new nplc();
$nplc->set("name", "niansong");
$nplc->set(
    array(
        "name" => "niansong",
        "age" => 20,
        )
    );
$nplc->get("name");
?>
```

### Nplc::delete
```
   Nplc::delete(array|string $keys)
```
### Nplc::info
```
   Nplc::info(void)
```
```php
Array
(
    [storage_size] => 67108864
    [node_nums] => 64
    [keys_nums] => 7
    [fail_nums] => 0
    [miss_nums] => 5
    [recycles_nums] => 0
)
```
