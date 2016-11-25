# XtsTinyBasicPlus

a fork of TinyBasic for Arduino by Xtase (fgalliat) @Nov 2016

It actually uses 40KB of flash & 7K of RAM (statically allocated)
so It **WILL NOT run on Arduino INO boards** ...

(It's an early stage work, so it's dirty & not well documented)

* added support for :
   - ssd1306 LCD I2C Screen
   - SPI uSD Card reader
   - ESP8266 Wifi module
   - Teensy ++2 module

![Dev Board](/images/devBoard.jpg)

* It provides a regular BASIC shell, and can act as an HTTP Server to launch a 'web.bas' file
  that outputs directly in the browser.

```
10 FOR I =1 TO 5
20 PRINT "web ";: PRINT I
30 NEXT I
40 FILES
50 PRINT "Free Mem:";: PRINT FREE()
```

![Web autorun script](/images/autorunWeb.jpg)

* previous authors of TinyBasic :
   - Mike Field <hamster@snap.net.nz>
   - Scott Lawrence <yorgle@gmail.com>
   - Brian O'Dell <megamemnon@megamemnon.com>