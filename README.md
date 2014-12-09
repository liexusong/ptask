ptask
=====
ptask is a coroutine for php extension

How to use?
-----------
```php
<?php

function handler($arg)
{
	for ($i = 0; $i < 1000; $i++) {
		echo $arg, ": ", $i, "\n";
		ptask_yield();
	}
}


ptask_create("handler", "handler1");
ptask_create("handler", "handler2");

ptask_run();

```
