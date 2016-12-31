#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>

#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>
#include <linux/spi/spi_gpio.h>


static struct spi_gpio_platform_data _gpio12_bitbang_platform_data =
{
   .sck = 424,
   .mosi = 425,
   .miso = 426, //not using miso
   .num_chipselect = 1,
};

static void
_gpio12_release(struct device *pdev)
{
}

static struct platform_device _gpio12_spi_bitbang_platform_device =
{
   .name = "spi-gpio",

   .id = 0,
   .dev = {
        .platform_data = &_gpio12_bitbang_platform_data,
        .release = _gpio12_release,
   },
};

static int __init
_gpio12_bitbang_spi_init(void)
{
   return platform_device_register(&_gpio12_spi_bitbang_platform_device);
}

static void __exit
_gpio12_bitbang_spi_exit(void)
{
   platform_device_unregister(&_gpio12_spi_bitbang_platform_device);
}

module_init(_gpio12_bitbang_spi_init);
module_exit(_gpio12_bitbang_spi_exit);

MODULE_LICENSE("GPL v2");
MODULE_VERSION("0.1");
MODULE_DESCRIPTION("GPIO-12 SPI bitbang driver");
MODULE_AUTHOR("Amitesh Singh <singh.amitesh@gmail.com>");
