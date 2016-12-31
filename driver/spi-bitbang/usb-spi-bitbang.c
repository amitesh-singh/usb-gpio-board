/*
 *  GPIO-12 spi bitbang driver
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
#include <linux/spinlock.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>


#include "../../firmware/common.h"

/*
 * usb_control_msg(struct usb_device *dev, unsigned int pipe,
 _ _u8 request, _ _u8 requesttype,
 _ _u16 value, _ _u16 index,
 void *data, _ _u16 size, int timeout);
 */
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Amitesh Singh <singh.amitesh@gmail.com>");
MODULE_DESCRIPTION("usb spi driver.");
MODULE_VERSION("0.1");

typedef struct _usb_spi_info
{
   struct usb_device *udev;
   struct spi_bitbang bitbang;
   int mosi, rst, clk, cs;
   struct gpio_desc *mosi_desc, *rst_desc, *clk_desc;
   spinlock_t lock;
} usb_spi_info;

//static int
//_to_irq(struct gpio_chip *chip,
//        unsigned offset)
//   struct my_usb *data = container_of(chip, struct my_usb, chip);
//{
//   printk("GPIO to IRQ: 2");
//   return 2;
//}

static void _spi_chipselect(struct spi_device *spi, int value)
{
}

static int
_spi_setuptransfer(struct spi_device *spi, struct spi_transfer *t)
{
   return 0;
}

static int
_spi_master_setup(struct spi_device *spi)
{
	int ret;

   usb_spi_info *data = spi_master_get_devdata(spi->master);

   data->mosi_desc = gpio_to_desc(data->mosi);
   data->rst_desc = gpio_to_desc(data->rst);
   data->clk_desc = gpio_to_desc(data->clk);
   ret = gpio_request(data->mosi, "sysfs");
   ret = gpio_request(data->clk, "sysfs");
   ret = gpio_request(data->rst, "sysfs");
}

static void
_spi_master_cleanup(struct spi_device *spi)
{
	usb_spi_info *data = spi_master_get_devdata(spi->master);

	gpiod_unexport(data->mosi_desc);
	gpiod_unexport(data->clk_desc);
	gpiod_unexport(data->rst_desc);

	gpio_free(data->mosi);
	gpio_free(data->clk);
	gpio_free(data->rst);
}

static void
_spibitbang_chipselect(struct spi_device *spi, int is_on)
{

}

static int
_spibitbang_setuptransfer(struct spi_device *spi, struct spi_transfer *t)
{
	return 0;
}

static int
my_usb_probe(struct usb_interface *interface,
             const struct usb_device_id *id)
{
   struct usb_device *udev = interface_to_usbdev(interface);
   struct usb_host_interface *iface_desc;
   usb_spi_info *data;
   unsigned char replybuf[3];
   int ret;

   printk(KERN_INFO "manufacturer: %s", udev->manufacturer);
   printk(KERN_INFO "product: %s", udev->product);

   iface_desc = interface->cur_altsetting;
   printk(KERN_INFO "GPIO-12 board %d probed: (%04X:%04X)",
          iface_desc->desc.bInterfaceNumber, id->idVendor, id->idProduct);
   printk(KERN_INFO "bNumEndpoints: %d", iface_desc->desc.bNumEndpoints);

   struct spi_master *master;

   master = spi_alloc_master(&udev->dev, sizeof(usb_spi_info));
   if (!master)
     return -ENOMEM;
   data = spi_master_get_devdata(master);
   /*
   data = kzalloc(sizeof(struct usb_spi_info), GFP_KERNEL);
   if (data == NULL)
     {
        printk(KERN_ALERT "Failed to alloc data");
        return -ENODEV;
     }
     */

   //increase ref count, make sure u call usb_put_dev() in disconnect()
   data->udev = usb_get_dev(udev);
   //TODO: take input from user at module loading time
   data->rst = 424;
   data->mosi = 425;
   data->clk = 426;
   data->cs = 427; // i am not sure if we can support it.


   data->bitbang.master = master;
   data->bitbang.chipselect = _spibitbang_chipselect;
   data->bitbang.setup_transfer = _spibitbang_setuptransfer;

   //master->max_speed_hz = ;
   //master->min_speed_hz = ;
   master->bus_num = -1; //let kernel decide
   master->num_chipselect = 1;
   master->mode_bits = SPI_CPOL | SPI_CPHA | SPI_CS_HIGH | SPI_LSB_FIRST;
   master->flags = 0;
   master->setup = _spi_master_setup;
   master->cleanup = _spi_master_cleanup;

   ret = spi_bitbang_start(&data->bitbang);
   if (ret)
     {
        dev_err("failed to start spi bitbang\n");
        return -ENODEV;
     }

   usb_set_intfdata(interface, data);

   spin_lock_init(&data->lock);

   //init the board
   spin_lock(&data->lock);
   ret = usb_control_msg(data->udev,
                         usb_sndctrlpipe(data->udev, 0),
                         BOARD_INIT, USB_TYPE_VENDOR | USB_DIR_OUT,
                         0, 0,
                         replybuf, 3,
                         1000);
   spin_unlock(&data->lock);

   if (ret != sizeof(pktheader) - 2)
     return -EREMOTEIO;

   return 0;
}

static void
my_usb_disconnect(struct usb_interface *interface)
{
   usb_spi_info *data;

   data = usb_get_intfdata(interface);
   spi_bitbang_stop(&data->bitbang);
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
     .name = "usb_spi",
     .id_table = my_usb_table,
     .probe = my_usb_probe,
     .disconnect = my_usb_disconnect,
};

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
