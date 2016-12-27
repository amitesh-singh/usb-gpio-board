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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/device.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/gpio.h>

#include "../firmware/common.h"

/*
 * usb_control_msg(struct usb_device *dev, unsigned int pipe,
 _ _u8 request, _ _u8 requesttype,
 _ _u16 value, _ _u16 index,
 void *data, _ _u16 size, int timeout);
 */
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Amitesh Singh <singh.amitesh@gmail.com>");
MODULE_DESCRIPTION("usb gpio-12 driver.");
MODULE_VERSION("0.1");

struct my_usb
{
   struct usb_device *udev;
   struct gpio_chip chip; //this is our GPIO chip
};

static void
_gpioa_set(struct gpio_chip *chip,
           unsigned offset, int value)
{
   struct my_usb *data = container_of(chip, struct my_usb, chip);
   static uint8_t gpio_val;

   gpio_val = value;
   usb_control_msg(data->udev,
                   usb_sndctrlpipe(data->udev, 0),
                   GPIO_WRITE, USB_TYPE_VENDOR | USB_DIR_OUT,
                   (offset + 1) | (gpio_val << 8), 0,
                   NULL, 0,
                   1000);
}

static int
_gpioa_get(struct gpio_chip *chip,
           unsigned offset)
{
   struct my_usb *data = container_of(chip, struct my_usb, chip);

   unsigned char replybuf[3];
   pktheader *pkt;

   printk(KERN_INFO "GPIO GET INFO: %d", offset);

   usb_control_msg(data->udev,
                   usb_sndctrlpipe(data->udev, 0),
                   GPIO_READ, USB_TYPE_VENDOR | USB_DIR_OUT,
                   (offset + 1), 0,
				   replybuf, 3,
                   1000);

   pkt = (pktheader *)replybuf;

   return pkt->gpio.val;
}

static int
_direction_output(struct gpio_chip *chip,
                  unsigned offset, int value)
{
   struct my_usb *data = container_of(chip, struct my_usb, chip);

   printk("Setting pin to OUTPUT");

   usb_control_msg(data->udev,
                   usb_sndctrlpipe(data->udev, 0),
                   GPIO_OUTPUT, USB_TYPE_VENDOR | USB_DIR_OUT,
                   (offset + 1), 0,
                   NULL, 0,
                   1000);

   return offset;
}

static int
_direction_input(struct gpio_chip *chip,
                 unsigned offset)
{
   struct my_usb *data = container_of(chip, struct my_usb, chip);

   printk("setting pin to INPUT");

   usb_control_msg(data->udev,
                   usb_sndctrlpipe(data->udev, 0),
                   GPIO_INPUT, USB_TYPE_VENDOR | USB_DIR_OUT,
                   (offset + 1), 0,
                   NULL, 0,
                   1000);

   return offset;
}

//static int
//_to_irq(struct gpio_chip *chip,
//        unsigned offset)
//{
//   printk("GPIO to IRQ: 2");
//   return 2;
//}

//called when a usb device is connected to PC
static int
my_usb_probe(struct usb_interface *interface,
             const struct usb_device_id *id)
{
   struct usb_device *udev = interface_to_usbdev(interface);
   struct usb_host_interface *iface_desc;
   //struct usb_endpoint_descriptor *endpoint;
   struct my_usb *data;
   //int i;

   printk(KERN_INFO "manufacturer: %s", udev->manufacturer);
   printk(KERN_INFO "product: %s", udev->product);

   iface_desc = interface->cur_altsetting;
   printk(KERN_INFO "GPIO-12 board %d probed: (%04X:%04X)",
          iface_desc->desc.bInterfaceNumber, id->idVendor, id->idProduct);
   printk(KERN_INFO "bNumEndpoints: %d", iface_desc->desc.bNumEndpoints);

   //There is no endpoint. we are using default control in and out for data transfer.
   //   for (i = 0; i < iface_desc->desc.bNumEndpoints; i++)
   //     {
   //        endpoint = &iface_desc->endpoint[i].desc;
   //
   //        printk(KERN_INFO "ED[%d]->bEndpointAddress: 0x%02X\n",
   //               i, endpoint->bEndpointAddress);
   //        printk(KERN_INFO "ED[%d]->bmAttributes: 0x%02X\n",
   //               i, endpoint->bmAttributes);
   //        printk(KERN_INFO "ED[%d]->wMaxPacketSize: 0x%04X (%d)\n",
   //               i, endpoint->wMaxPacketSize, endpoint->wMaxPacketSize);
   //     }

   data = kzalloc(sizeof(struct my_usb), GFP_KERNEL);
   if (data == NULL)
     {
        printk(KERN_ALERT "Failed to alloc data");
        return -ENODEV;
     }

   //increase ref count, make sure u call usb_put_dev() in disconnect()
   data->udev = usb_get_dev(udev);

   data->chip.label = "gpio-12";
   data->chip.dev = &data->udev->dev; // optional device providing the GPIOs
   data->chip.owner = THIS_MODULE; // helps prevent removal of modules exporting active GPIOs, so this is required for proper cleanup
   data->chip.base = -1;
   data->chip.ngpio = 12; /* 12 GPIO pins, PD0, PD1, PD3, PD5, PD6, PD7 (1 - 6);
   	   	   	   	   	   	   PB0, PB1, PB2, PB3, PB4, PB5 (7 - 12) */
   data->chip.can_sleep = false;

   data->chip.set = _gpioa_set;
   data->chip.get = _gpioa_get;

   data->chip.direction_input = _direction_input;
   data->chip.direction_output = _direction_output;

   //TODO: implement it later
   //data->chip.to_irq = _to_irq;

   if (gpiochip_add(&data->chip) < 0)
     printk(KERN_ALERT "Failed to add gpio chip");
   else
     printk(KERN_INFO "Able to add gpiochip: %s", data->chip.label);

   usb_set_intfdata(interface, data);

   printk(KERN_INFO "usb %s is connected", data->chip.label);

   //init the board
   usb_control_msg(data->udev,
                   usb_sndctrlpipe(data->udev, 0),
                   BOARD_INIT, USB_TYPE_VENDOR | USB_DIR_OUT,
                   0, 0,
                   NULL, 0,
                   1000);

   return 0;
}

//called when unplugging a USB device
static void
my_usb_disconnect(struct usb_interface *interface)
{
   struct my_usb *data;

   data = usb_get_intfdata(interface);

   gpiochip_remove(&data->chip);

   usb_set_intfdata(interface, NULL);

   //deref the count
   usb_put_dev(data->udev);

   kfree(data);

   printk(KERN_INFO "usb device is disconnected");
}

#define MY_USB_VENDOR_ID 0x16c0
#define MY_USB_PRODUCT_ID 0x03e8
static struct usb_device_id my_usb_table[] = {
       { USB_DEVICE(MY_USB_VENDOR_ID, MY_USB_PRODUCT_ID) },
       {},
};

MODULE_DEVICE_TABLE(usb, my_usb_table);

static struct usb_driver my_usb_driver = {
     .name = "usb_gpio12",
     .id_table = my_usb_table,
     .probe = my_usb_probe,
     .disconnect = my_usb_disconnect,
};

//we could use module_usb_driver(my_usb_driver); instead of 
// init and exit functions
//called on module loading
static int __init
_usb_init(void)
{
   int result;
   printk(KERN_INFO "usb driver is loaded");

   result = usb_register(&my_usb_driver);
   if (result)
     {
        printk(KERN_ALERT "device registeration failed!!");
     }
   else
     {
        printk(KERN_INFO "device registered");
     }

   return result;
}

//called on module unloading
static void __exit
_usb_exit(void)
{
   printk(KERN_INFO "usb driver is unloaded");
   usb_deregister(&my_usb_driver);
}

module_init(_usb_init);
module_exit(_usb_exit);
