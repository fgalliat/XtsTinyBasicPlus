# XtsTinyBasicPlus

a fork of -TinyBasic for Arduino- by Xtase (fgalliat) @Nov 2016

It actually uses ~43.6KB of flash & 7K of RAM (statically allocated)<br/>
so It **WILL NOT run on Arduino INO boards** ...

(It's an early stage work, so it's dirty & not well documented)

* added support for :
   - ssd1306 LCD I2C Screen + 3 user btns (up/down/ok) **[usable inside the BASIC]**
   - SPI uSD Card reader
   - ESP8266 Wifi module
   - 2nd UART (two serial ports control)
   - Teensy ++2 module
   
   - Canon X-07 Serial protocol

![Dev Board](/images/devBoard.jpg)

* 3 user buttons & LCD control routines ( BTN() + LCCLS, LCPRINT &lt;line&gt;,&lt;expr&gt; ),<br/>
  You are able to create simple menus !!

* Newly created mainboard & started to build enclosure...
![Dev Board](/images/enclosure_plus_photo.jpg)

* It provides a regular (even if limited) BASIC shell, and can act as an HTTP Server to launch a 'web.bas' file<br/>
  that outputs directly in the browser.

```
10 FOR I =1 TO 5
20 PRINT "web ";: PRINT I
30 NEXT I
40 FILES
50 PRINT "Free Mem:";: PRINT FREE()
```

* It also have 2 serial ports (MASTER / SLAVE) that can be hot-swapped with 'CINV' command.<br/>
  note that 'CINIT' <baud_rate> does ONLY set the "Software" Serial port & that the "Hardware" Serial port (USB),<br/>
  is always @ 115200 bauds (you can change it in the code).<br/>

![Web + 2 serial ports control](/images/Web_2Serials.jpg)

* Since 11/28/2016 : Started an experimental way to produce 4 String variables

* Since 12/03/2016 : **Support now (w/ a few bugs anyway) Canon X-07 PRGs writing to/reading from uSDCard**<br/>
  See basic_source/loader.bas & X07_source/LOADSD.TXT & SV2SD.TXT
![Canon X-07 & the 'black box'](/images/X07_Teensy.jpg)

* previous authors of TinyBasic :
   - Mike Field <hamster@snap.net.nz>
   - Scott Lawrence <yorgle@gmail.com>
   - Brian O'Dell <megamemnon@megamemnon.com>
   
   - https://github.com/BleuLlama/TinyBasicPlus
