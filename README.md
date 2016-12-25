### Intro

This is a gpio over usb board based on VUSB stack for AVR atmega8 chip. It exposes 12 GPIO ports.

### how to flash mega8
`cd firmware`  
`avrdude -c usbasp -p m8 -U flash:w:main.hex`  

### TODOs
 - ~~write firmware~~
 - write driver
 - design schematic
 - Add support for https://github.com/walac/pyusb and libusb/C/Cpp  
