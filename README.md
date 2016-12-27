### Intro

This is a gpio over usb board based on VUSB stack for AVR atmega8 chip. It exposes 12 GPIO ports.

### how to flash mega8
`cd firmware`  
`avrdude -c usbasp -p m8 -U flash:w:main.hex`  

### How to compile driver
`cd driver`  
`make`  
### TODOs
 - ~~write firmware~~
 - ~~write basic gpio driver~~
 - Add support of locking in gpio driver.
 - Add support of spi bitbang driver.
 - Add support of spi hw in firmware.
 - Add support of spi hw driver.
 - Add support of i2c in firmware.
 - Add support of i2c driver.
 - Add adc support in firmware.
 - Add adc driver support.
 - design schematic.
 - Add support for https://github.com/walac/pyusb and libusb/C/Cpp  

## Links
 - how to make nice pinouts: http://www.pighixxx.com/test/?s=made+a+pinout
 http://www.pighixxx.com/test/2016/06/how-its-made-a-pinout-part-1/

