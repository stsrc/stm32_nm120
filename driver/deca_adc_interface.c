/*
 * Copyright (C) 2022 Konrad Gotfryd
 *
 * Basing on rtl8150.c
 *
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/usb.h>

#include <linux/jiffies.h>

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
};

#define INTBUFSIZE 8
#define DECA_BUF_SIZE 4
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

static unsigned int *to_send = NULL;

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

static void send_next_unsigned(struct deca_adcintf* deca, int atomic)
{
	int res;
	int flag = atomic == 1 ? GFP_ATOMIC : GFP_KERNEL;

	usb_fill_bulk_urb(deca->tx_urb,
			  deca->usbdev,
			  usb_sndbulkpipe(deca->usbdev, 3),
                          to_send,
			  sizeof(unsigned int),
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
	*to_send = *to_send + 1;
}



static void read_bulk_callback(struct urb *urb)
{
	struct deca_adcintf *deca;
	//int status = urb->status;
	unsigned int received;

	deca = urb->context;
	if (!deca) {
		printk(KERN_ERR "%s():%d\n", __FUNCTION__, __LINE__);
		return;
	}

	if (urb->actual_length != 4) {
		printk(KERN_ERR "%s():%d\n", __FUNCTION__, __LINE__);
		return;
	}

	memcpy((char *)&received, data, DECA_BUF_SIZE);
	if ((received != (*to_send) - 1) && (received != 0)) {
		pr_info("%s():%d, fail! received %u, sent %u\n",
			__FUNCTION__,
			__LINE__,
			received,
			(*to_send) - 1);
	}

	send_next_unsigned(deca, 1);

	return;
}

static int deca_adcintf_probe(struct usb_interface *intf,
                              const struct usb_device_id *id)
{
	struct deca_adcintf *deca = kmalloc(sizeof(struct deca_adcintf), GFP_KERNEL);
	struct usb_device *usbdev = interface_to_usbdev(intf);

	data = kmalloc(sizeof(unsigned int), GFP_KERNEL);
	to_send = kmalloc(sizeof(unsigned int), GFP_KERNEL);

	deca->usbdev = usbdev;

	printk(KERN_INFO "%s():%d\n", __FUNCTION__, __LINE__);

	if (!alloc_all_urbs(deca)) {
		dev_err(&intf->dev, "out of memory\n");
		return -ENOMEM;
	}

        usb_set_intfdata(intf, deca);

	send_next_unsigned(deca, 0);

	return 0;
}

static void deca_adcintf_disconnect(struct usb_interface *intf)
{
	struct deca_adcintf *deca = usb_get_intfdata(intf);
	printk(KERN_ERR "%s():%d\n", __FUNCTION__, __LINE__);
	usb_set_intfdata(intf, NULL);
	if (deca) {
		unlink_all_urbs(deca);
		free_all_urbs(deca);
	}
	kfree(deca);
	kfree(to_send);
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

