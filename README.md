#Nplc - NianSong's php local cache

##Logo :trollface:
![logo](https://raw.githubusercontent.com/HughNian/nplc/master/logo.jpg)

## 扩展信息页面
![phpinfo](https://raw.githubusercontent.com/HughNian/nplc/master/phpinfo.jpg)

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
   Nplc::set($key, $value[, $ttl])
   Nplc::set(array $kvs[, $ttl])
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
