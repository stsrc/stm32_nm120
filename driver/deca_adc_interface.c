/*
 * Copyright (C) 2022 Konrad Gotfryd
 *
 * Basing on rtl8150.c
 *
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/usb.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/err.h>

static const char driver_name[] = "deca_adc_interface";

static const struct usb_device_id deca_adcintf_table[] = {
	{USB_DEVICE(0x1209, 0x4711)},
	{}
};

MODULE_DEVICE_TABLE(usb, deca_adcintf_table);

#define POOL_SIZE 4

struct deca_adcintf {
	struct usb_device *usbdev;
	struct urb *rx_urb, *tx_urb, *intr_urb;
	struct file_operations fops;
	struct device *device;
	uint8_t ready;
	uint16_t word_count;

        const char *nodename;
	umode_t mode;

	const char *name;
	dev_t dev;
};
struct deca_adcintf *deca_g = NULL;
#define INTBUFSIZE 8
#define DECA_BUF_SIZE 1024
static char *data = NULL;
static int alloc_all_urbs(struct deca_adcintf *dev)
{
	dev->rx_urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!dev->rx_urb)
		return 0;

	dev->tx_urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!dev->tx_urb) {
		usb_free_urb(dev->rx_urb);
		return 0;
	}

        dev->intr_urb = usb_alloc_urb(0, GFP_KERNEL);
        if (!dev->intr_urb) {
                usb_free_urb(dev->rx_urb);
                usb_free_urb(dev->tx_urb);
                return 0;
        }

	return 1;
}

static void free_all_urbs(struct deca_adcintf *dev)
{
        usb_free_urb(dev->rx_urb);
        usb_free_urb(dev->tx_urb);
        usb_free_urb(dev->intr_urb);
}

static void unlink_all_urbs(struct deca_adcintf *dev)
{
        usb_kill_urb(dev->rx_urb);
        usb_kill_urb(dev->tx_urb);
        usb_kill_urb(dev->intr_urb);
}

static void read_bulk_callback(struct urb *urb);
static void write_bulk_callback(struct urb *urb)
{
	struct deca_adcintf *deca = urb->context;
        usb_fill_bulk_urb(deca->rx_urb,
			  deca->usbdev,
		          usb_rcvbulkpipe(deca->usbdev, 2),
                          data,
			  DECA_BUF_SIZE,
			  read_bulk_callback,
			  deca);
        usb_submit_urb(deca->rx_urb, GFP_ATOMIC);
}

static void send_word_count(struct deca_adcintf* deca, int atomic)
{
	int res;
	int flag = atomic == 1 ? GFP_ATOMIC : GFP_KERNEL;

	usb_fill_bulk_urb(deca->tx_urb,
			  deca->usbdev,
			  usb_sndbulkpipe(deca->usbdev, 3),
                          &deca->word_count,
			  sizeof(deca->word_count),
			  write_bulk_callback,
			  deca);
        if ((res = usb_submit_urb(deca->tx_urb, flag))) {
                /* Can we get/handle EPIPE here? */
                if (res == -ENODEV) {
                        pr_info("%s():%d, no dev\n", __FUNCTION__, __LINE__);
		} else {
                        pr_info("%s():%d, failed tx_urb %d\n", __FUNCTION__,
				__LINE__, res);
                }
        }
}



static void read_bulk_callback(struct urb *urb)
{
	struct deca_adcintf *deca;

	deca = urb->context;
	if (!deca) {
		pr_err("%s():%d\n", __FUNCTION__, __LINE__);
		return;
	}

	if (urb->actual_length != deca->word_count * 2) {
		pr_err("actual_length == %d, expected length == %d\n",
		       urb->actual_length, deca->word_count * 2);
		return;
	}

	deca->ready = 1;

	return;
}

static ssize_t deca_read(struct file *file, char __user *buf, size_t len,
			 loff_t *ppos)
{
	struct deca_adcintf *deca = file->private_data;
	if (!deca) {
		return -1;
	} else if (!deca->ready) {
		return -1;
	} else if (len != 2 * deca->word_count) {
		return -1;
	} else if (copy_to_user(buf, data, 2 * deca->word_count)) {
		return -EFAULT;
	}
	deca->ready = 0;
	deca->word_count = 0;
	return len;
}

static ssize_t deca_write(struct file *file, const char __user *buf, size_t len,
			  loff_t *ppos)
{
	unsigned short word_count_to_send;
	struct deca_adcintf *deca = file->private_data;
	if (!deca) {
		return -1;
	}
	if (len != 2) {
		return -1;
	}
	if (get_user(word_count_to_send, buf))
		return -EFAULT;
	deca->ready = 0;
	deca->word_count = word_count_to_send;
	send_word_count(deca, 0);
	return len;
}

static int deca_open(struct inode *inode, struct file *file)
{
	file->private_data = deca_g;
	return 0;
}

static int deca_close(struct inode *inode, struct file *file)
{
	return 0;
}

static char *deca_devnode(const struct device *dev, umode_t *mode)
{
        const struct deca_adcintf *deca = dev_get_drvdata(dev);

        if (mode && deca->mode)
                *mode = deca->mode;
        if (deca->nodename)
                return kstrdup(deca->nodename, GFP_KERNEL);
        return NULL;
}

static const struct class deca_class = {
        .name           = "deca_adc",
        .devnode        = deca_devnode,
};

static const struct file_operations deca_fops = {
        .owner          = THIS_MODULE,
        .open           = deca_open,
        .release        = deca_close,
	.write		= deca_write,
	.read		= deca_read
};

#define DECA_MAJOR 300
static int deca_adcintf_probe(struct usb_interface *intf,
                              const struct usb_device_id *id)
{
	struct deca_adcintf *deca = kzalloc(sizeof(struct deca_adcintf),
					    GFP_KERNEL);
	struct usb_device *usbdev = interface_to_usbdev(intf);
	int rt;

	if (!deca) {
		return -ENOMEM;
	}

	deca->name = "deca_adc";
	deca->nodename = deca->name;
	deca->mode = 0644;
	deca_g = deca;

	data = kmalloc(DECA_BUF_SIZE, GFP_KERNEL);

	deca->usbdev = usbdev;

	if (!alloc_all_urbs(deca)) {
		dev_err(&intf->dev, "out of memory\n");
		return -ENOMEM;
	}
        usb_set_intfdata(intf, deca);
	rt = class_register(&deca_class);
	if (rt) {
		goto err;
	}
	rt = register_chrdev(DECA_MAJOR, deca->name, &deca_fops);
	if (rt) {
		goto err;
	}
        deca->dev = MKDEV(DECA_MAJOR, 1);
        deca->device = device_create(&deca_class, NULL, deca->dev, deca, "%s",
			             deca->name);
        if (IS_ERR(deca->device)) {
                rt = PTR_ERR(deca->device);
                goto err;
        }

	return 0;
err:
	unlink_all_urbs(deca);
	free_all_urbs(deca);
	kfree(deca);
	return rt;
}

static void deca_adcintf_disconnect(struct usb_interface *intf)
{
	struct deca_adcintf *deca = usb_get_intfdata(intf);
	pr_info("%s():%d\n", __FUNCTION__, __LINE__);
	usb_set_intfdata(intf, NULL);
	device_destroy(&deca_class, deca->dev);
	unregister_chrdev(DECA_MAJOR, deca->name);
	class_unregister(&deca_class);
	if (deca) {
		unlink_all_urbs(deca);
		free_all_urbs(deca);
	}
	kfree(deca);
	kfree(data);
}

static int deca_adcintf_suspend(struct usb_interface *intf,
				pm_message_t message)
{
	struct deca_adcintf *deca = usb_get_intfdata(intf);
	printk(KERN_ERR "%s():%d\n", __FUNCTION__, __LINE__);
	usb_kill_urb(deca->rx_urb);
	usb_kill_urb(deca->intr_urb);
	return 0;
}

static struct usb_driver deca_adcintf = {
	.name = driver_name,
	.id_table = deca_adcintf_table,
	.probe = deca_adcintf_probe,
	.disconnect = deca_adcintf_disconnect,
	.suspend = deca_adcintf_suspend
};

module_usb_driver(deca_adcintf);

MODULE_LICENSE("GPL");

