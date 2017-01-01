/*
 *  GPIO-12 driver
 *
 *  (C) Amitesh Singh <singh.amitesh@gmail.com>, 2016
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <usb.h>
#define AVR_USB_FIRMWARE

#include "../../firmware/common.h"

// used to get descriptor strings for device identification 
static int usbGetDescriptorString(usb_dev_handle *dev, int index, int langid, 
                                  char *buf, int buflen)
{
   char buffer[256];
   int rval, i;

   // make standard request GET_DESCRIPTOR, type string and given index 
   // (e.g. dev->iProduct)
   rval = usb_control_msg(dev, 
                          USB_TYPE_STANDARD | USB_RECIP_DEVICE | USB_ENDPOINT_IN, 
                          USB_REQ_GET_DESCRIPTOR, (USB_DT_STRING << 8) + index, langid, 
                          buffer, sizeof(buffer), 1000);

   if(rval < 0) // error
     return rval;

   // rval should be bytes read, but buffer[0] contains the actual response size
   if((unsigned char)buffer[0] < rval)
     rval = (unsigned char)buffer[0]; // string is shorter than bytes read

   if(buffer[1] != USB_DT_STRING) // second byte is the data type
     return 0; // invalid return type

   // we're dealing with UTF-16LE here so actual chars is half of rval,
   // and index 0 doesn't count
   rval /= 2;

   // lossy conversion to ISO Latin1 
   for(i = 1; i < rval && i < buflen; i++)
     {
        if(buffer[2 * i + 1] == 0)
          buf[i-1] = buffer[2 * i];
        else
          buf[i-1] = '?'; // outside of ISO Latin1 range
     }
   buf[i-1] = 0;

   return i-1;
}

static usb_dev_handle * usbOpenDevice(int vendor, char *vendorName, 
                                      int product, char *productName)
{
   struct usb_bus *bus;
   struct usb_device *dev;
   char devVendor[256], devProduct[256];

   usb_dev_handle * handle = NULL;

   usb_init();
   usb_find_busses();
   usb_find_devices();

   for(bus=usb_get_busses(); bus; bus=bus->next)
     {
        for(dev=bus->devices; dev; dev=dev->next)
          {
             if(dev->descriptor.idVendor != vendor ||
                dev->descriptor.idProduct != product)
               continue;

             // we need to open the device in order to query strings 
             if(!(handle = usb_open(dev)))
               {
                  fprintf(stderr, "Warning: cannot open USB device: %s\n",
                          usb_strerror());
                  continue;
               }

             // get vendor name 
             if(usbGetDescriptorString(handle, dev->descriptor.iManufacturer, 0x0409, devVendor, sizeof(devVendor)) < 0)
               {
                  fprintf(stderr, 
                          "Warning: cannot query manufacturer for device: %s\n", 
                          usb_strerror());
                  usb_close(handle);
                  continue;
               }

             // get product name 
             if(usbGetDescriptorString(handle, dev->descriptor.iProduct, 
                                       0x0409, devProduct, sizeof(devVendor)) < 0)
               {
                  fprintf(stderr, 
                          "Warning: cannot query product for device: %s\n", 
                          usb_strerror());
                  usb_close(handle);
                  continue;
               }

             if(strcmp(devVendor, vendorName) == 0 && 
                strcmp(devProduct, productName) == 0)
               return handle;
             else
               usb_close(handle);
          }
     }

   return NULL;
}

int main(int argc, char **argv)
{
   usb_dev_handle *handle = NULL;
   int nBytes = 0;
   char buffer[256];

   if(argc < 4)
     {
        printf("Usage:\n\n");
        printf("on_off start_gpio_no end_gpio_no timeout\n\n\
               this will keep on-off start_gpio_no to end_gpio_no gpio pins \n\n where \n \
               - start_gpio_no: start gpio no (range: 1 - 12) \n \
               - end_gpio_no: end gpio no (range: 1 - 12) \n \
               - timeout: the gap between subsequent on/off of gpio in milliseconds\n");
        exit(1);
     }

   handle = usbOpenDevice(0x16C0, "ami", 0x03E8, "GPIO-12");

   if(handle == NULL)
     {
        fprintf(stderr, "Could not find USB device!\n");
        exit(1);
     }

   unsigned int sgpio_no, egpio_no, timeout;

   sgpio_no = atoi(argv[1]);
   egpio_no = atoi(argv[2]);
   timeout = atoi(argv[3]);

   //board init
   nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
                            BOARD_INIT, 0, 0, buffer, 3, 1000);
   int gpio_number = sgpio_no;
   for (; gpio_number <= egpio_no; ++gpio_number)
     {
        nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
                                 GPIO_OUTPUT, gpio_number, 0,
                                 buffer, 3, 1000);
     }

   while (1)
     {
        gpio_number = sgpio_no;
        for (; gpio_number <= egpio_no; ++gpio_number)
          {
             //switch on the gpio
             nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
                                      GPIO_WRITE, gpio_number | (1 << 8), 0 , buffer, 3, 1000);
             printf("Led: %d is ON\n", gpio_number);
             usleep(timeout * 1000);

             //switch off the gpio
             nBytes = usb_control_msg(handle,
                                      USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
                                      GPIO_WRITE, gpio_number | (0 << 8), 0 , buffer, 3, 1000);
             printf("Led: %d is OFF\n =========== \n\n", gpio_number);
             usleep(timeout * 1000);
          }
     }

   usb_close(handle);

   return 0;
}
