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

#include "../common.h"

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

   if(argc < 2)
     {
        printf("\nUsage:\n");
        printf("\tusbtest on gpiono\n");
        printf("\tusbtest off gpiono\n");
        printf("\tusbtest read gpiono\n");
        printf("\tusbtest reboot\n");
        printf("\tubstest spiinit\n");
        printf("\tusbtest spiend\n");
        printf("\tusbtest spidata data\n");
        printf("\tusbtest gpiotest no\n");
        printf("\tubstest adcread adc_gpio_no (0 - 5)\n\n");
        exit(1);
     }

   handle = usbOpenDevice(0x16C0, "ami", 0x03E8, "GPIO-12");

   if(handle == NULL)
     {
        fprintf(stderr, "Could not find USB device!\n");
        exit(1);
     }

   int gpio_number = 1;  //default gpio is PD0
   if (argv[2] != 0)
     {
        gpio_number = atoi(argv[2]);
     }

   if(strcmp(argv[1], "on") == 0)
     {
        //board init
        nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, 
                                 BOARD_INIT, 0, 0,
                                 buffer, 3, 1000);
        printf("bytes: %d\n", nBytes);

        nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
                                 GPIO_OUTPUT, gpio_number, 0,
                                 buffer, 3, 1000);
        printf("bytes: %d\n", nBytes);

        nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
                                 GPIO_WRITE, gpio_number | (1 << 8), 0 ,
                                 buffer, 3, 1000);
        printf("bytes: %d\n", nBytes);
     }
   else if(strcmp(argv[1], "off") == 0)
     {
        nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, 
                                 GPIO_WRITE, gpio_number | (0 << 8), 0,
                                 buffer, 10, 5000);
        printf("bytes: %d\n", nBytes);
     }
   else if (strcmp(argv[1], "read") == 0)
     {
        nBytes = usb_control_msg(handle,
                                 USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
                                 GPIO_READ, gpio_number, 0,  buffer, 3, 5000);
        printf("bytes: %d\n", nBytes);

        gpiopktheader *pkt = (gpiopktheader *) buffer;

        printf("gpio value: %d\n", pkt->gpio.data);

     }
   else if (strcmp(argv[1], "reboot") == 0)
     {
        nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
                                 BOARD_RESET, 0, 0,
                                 0, 0, 5000);
        printf("bytes: %d\n", nBytes);
        if (nBytes == -32)
          {
             printf("Board is rebooted....\n");
          }
     }

   else if(!strcmp(argv[1], "spiinit"))
     {
        nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
                                 SPI_INIT, 0, 0,
                                 buffer, 3, 5000);
        printf("bytes: %d\n", nBytes);
     }
   else if (!strcmp(argv[1], "spiend"))
     {
        nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
                                 SPI_END, 0, 0,
                                 buffer, 3, 5000);
        printf("bytes: %d\n", nBytes);
     }
   else if (!strcmp(argv[1], "spidata"))
     {
        char *data_str = argv[2];
        unsigned char spi_data = atoi(data_str);
        nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
                                 SPI_INIT, 0, 0,
                                 0, 0, 5000);
        printf("spi_init: bytes: %d\n", nBytes);
        nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
                                 SPI_DATA, 0 | (spi_data << 8), 0,
                                 buffer, 3, 5000);
        printf("spi_data: bytes: %d\n", nBytes);
        printf("output:- %x:%x:%x\n", buffer[0], buffer[1], buffer[2]);

        nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
                                 SPI_END, 0, 0,
                                 0, 0, 5000);
        printf("spi_end: bytes: %d\n", nBytes);
     }
   else if (!strcmp(argv[1], "spitest"))
     {
        char *data_str = argv[2];
        unsigned char spi_data = atoi(data_str);
        printf("spi data to be send %d\n", spi_data);
        nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
                                 SPI_INIT, 0, 0,
                                 buffer, 3, 5000);
        printf("spi_init: bytes: %d\n", nBytes);

        int i = 0;

        for (; i < 1000000; ++i)
          {
             printf("spi data to be send %d\n", spi_data);
             memset(buffer, 0, 3);
             nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
                                      SPI_DATA, 0 | (spi_data << 8), 0,
                                      buffer, 3, 5000);
             printf("spi_data: bytes: %d\n", nBytes);

             //uint8_t val = buffer[2];
             printf("output: %x: %x, %x\n", buffer[0], buffer[1], buffer[2]);
             memset(buffer, 0, 3);
             sleep(1);
             printf("spi data to be send 0\n");

             nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
                                      SPI_DATA, 0 << 8, 0,
                                      buffer, 3, 5000);
             printf("spi_data: bytes: %d\n", nBytes);

             printf("output - down: %x: %x, %x\n\n", buffer[0], buffer[1], buffer[2]);
             sleep(1);
          }
        nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
                                 SPI_END, 0, 0,
                                 buffer, 3, 5000);
        printf("spi_end: bytes: %d\n", nBytes);
     }
   else if (!strcmp(argv[1], "gpiotest"))
     {
        int i = 0;
        nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
                                 GPIO_OUTPUT, gpio_number, 0,
                                 buffer, 3, 1000);
        //got speed around 1Khz
        for (; i < 100000; ++i)
          {
             nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
                                      GPIO_WRITE, gpio_number | (1 << 8), 0,
                                      buffer, 3, 1000);
             //usleep(400000);
             nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
                                      GPIO_WRITE, gpio_number | (0 << 8), 0,
                                      buffer, 10, 5000);
             //usleep(400000);
          }
     }
   else if (!strcmp(argv[1], "adcread"))
     {
        nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
                                 ADC_INIT, 0, 0,
                                 buffer, 1, 1000);
        printf("adc_int: bytes: %d\n", nBytes);

        nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
                                 ADC_READ, gpio_number, 0,
                                 buffer, 5, 1000);
        printf("adc_read: bytes: %d\n", nBytes);

        adcpktheader *adc_info = (adcpktheader *)buffer;
        printf("ADC pin %d read value: %d\n", adc_info->gpio_no, adc_info->data);

        nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
                                 ADC_END, 0, 0,
                                 buffer, 1, 1000);
        printf("adc_end: bytes: %d\n", nBytes);

     }


   if(nBytes < 0)
     fprintf(stderr, "USB error: %s\n", usb_strerror());

   usb_close(handle);

   return 0;
}
