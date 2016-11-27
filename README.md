# XtsTinyBasicPlus

a fork of TinyBasic for Arduino by Xtase (fgalliat) @Nov 2016

It actually uses ~42KB of flash & 7K of RAM (statically allocated)<br/>
so It **WILL NOT run on Arduino INO boards** ...

(It's an early stage work, so it's dirty & not well documented)

* added support for :
   - ssd1306 LCD I2C Screen + 3 user btns (up/down/ok) **[new]**
   - SPI uSD Card reader
   - ESP8266 Wifi module
   - 2nd UART (two serial ports control)
   - Teensy ++2 module

![Dev Board](/images/devBoard.jpg)

* With it's 3 new user buttons & LCD control routines ( BTN() + LCCLS, LCPRINT &lt;line&gt;,&lt;expr&gt; ),<br/>
  You are now able to create simple menus !!

* Newly created mainboard & started to build enclosure...
![Dev Board](/images/enclosure_plus_photo.jpg)

* It provides a regular BASIC shell, and can act as an HTTP Server to launch a 'web.bas' file<br/>
  that outputs directly in the browser.

```
10 FOR I =1 TO 5
20 PRINT "web ";: PRINT I
30 NEXT I
40 FILES
50 PRINT "Free Mem:";: PRINT FREE()
```

![Web autorun script](/images/autorunWeb.jpg)

* It also have 2 serial ports (MASTER / SLAVE) that can be hot-swapped with 'CINV' command.<br/>
  note that 'CINIT' <baud_rate> does ONLY set the "Software" Serial port & that the "Hardware" Serial port (USB),<br/>
  is always @ 115200 bauds (you can change it in the code).<br/>

![Web + 2 serial ports control](/images/Web_2Serials.jpg)



* previous authors of TinyBasic :
   - Mike Field <hamster@snap.net.nz>
   - Scott Lawrence <yorgle@gmail.com>
   - Brian O'Dell <megamemnon@megamemnon.com>
   
   - https://github.com/BleuLlama/TinyBasicPlus