### Firmware

This driver is for avr(atmega16a) based on vusb.

`$ cd ../usb-led/firmware`   
`$ make hex`  
upload main.hex file into your vusb based usb device.  
`$ avrdude -c usbasp -p atmega16a -U flash:w:main.hex`  

### Driver

`$make`  
`$sudo insmod usb-gpio.ko`  
` $cd /sys/class/gpio/ `  
`$ gpiochip **N**` where **N** is the value allocated by kernel  
since base = -1  
`$ sudo chmod 666 export unexport`  
`$ echo **N** > export`  
To Allocate next gpio (depending upon gpio_chip.ngpio value),  
` $ echo **N+1 > export`
 a folder named gpioN must be created  
`$ cd gpio**N**`  
`$ cat value`  
`$ echo 1 > value`  
`$ echo 0 > value`  

Before unloading the module, do $ echo **N** > unexport  
gpio**N** gets deleted by doing so.  
To unload the module, `$sudo rmmod usb_gpio`  
