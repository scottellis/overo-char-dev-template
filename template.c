/*
  Char device driver template.

  Do a global replace of 'template' with your driver name.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/slab.h>

#define USER_BUFF_SIZE 128


struct template_dev {
	dev_t devt;
	struct cdev cdev;
	struct semaphore sem;
	struct class *class;
	char *user_buff;
};

static struct template_dev template_dev;


static ssize_t template_write(struct file *filp, const char __user *buff,
		size_t count, loff_t *f_pos)
{
	ssize_t status;
	size_t len = USER_BUFF_SIZE - 1;

	if (count == 0)
		return 0;

	if (down_interruptible(&template_dev.sem))
		return -ERESTARTSYS;

	if (len > count)
		len = count;
	
	memset(template_dev.user_buff, 0, USER_BUFF_SIZE);
	
	if (copy_from_user(template_dev.user_buff, buff, len)) {
		status = -EFAULT;
		goto template_write_done;
	}

	/* do something with the user data */

	status = len;
	*f_pos += len;

template_write_done:

	up(&template_dev.sem);

	return status;
}

static ssize_t template_read(struct file *filp, char __user *buff, 
				size_t count, loff_t *offp)
{
	ssize_t status;
	size_t len;

	/* 
	Generic user progs like cat will continue calling until we 
	return zero. So if *offp != 0, we know this is at least the
	second call.
	*/
	if (*offp > 0)
		return 0;

	if (down_interruptible(&template_dev.sem)) 
		return -ERESTARTSYS;

	strcpy(template_dev.user_buff, "template driver data goes here\n");

	len = strlen(template_dev.user_buff);

	if (len > count)
		len = count;

	if (copy_to_user(buff, template_dev.user_buff, len)) {
		status = -EFAULT;
		goto template_read_done;
	}

	*offp += len;
	status = len;

template_read_done:
			
	up(&template_dev.sem);
	
	return status;	
}

static int template_open(struct inode *inode, struct file *filp)
{	
	int status = 0;

	if (down_interruptible(&template_dev.sem)) 
		return -ERESTARTSYS;
	
	if (!template_dev.user_buff) {
		template_dev.user_buff = kmalloc(USER_BUFF_SIZE, GFP_KERNEL);

		if (!template_dev.user_buff) {
			printk(KERN_ALERT "template_open: user_buff alloc failed\n");
			status = -ENOMEM;
		}
	}

	up(&template_dev.sem);

	return status;
}

static const struct file_operations template_fops = {
	.owner = THIS_MODULE,
	.open =	template_open,	
	.read =	template_read,
	.write = template_write,
};

static int __init template_init_cdev(void)
{
	int error;

	template_dev.devt = MKDEV(0, 0);

	error = alloc_chrdev_region(&template_dev.devt, 0, 1, "template");
	if (error) {
		printk(KERN_ALERT "alloc_chrdev_region() failed: %d\n", error);
		return error;
	}

	cdev_init(&template_dev.cdev, &template_fops);
	template_dev.cdev.owner = THIS_MODULE;

	error = cdev_add(&template_dev.cdev, template_dev.devt, 1);
	if (error) {
		printk(KERN_ALERT "cdev_add() failed: %d\n", error);
		unregister_chrdev_region(template_dev.devt, 1);
		return error;
	}	

	return 0;
}

static int __init template_init_class(void)
{
	struct device *device;

	template_dev.class = class_create(THIS_MODULE, "template");

	if (IS_ERR(template_dev.class)) {
		printk(KERN_ALERT "class_create(template) failed\n");
		return PTR_ERR(template_dev.class);
	}

	device = device_create(template_dev.class, NULL, template_dev.devt, NULL, 
							"template");

	if (IS_ERR(device)) {
		class_destroy(template_dev.class);
		return PTR_ERR(device);
	}

	return 0;
}

static int __init template_init(void)
{
	printk(KERN_INFO "template_init()\n");

	memset(&template_dev, 0, sizeof(struct template_dev));

	sema_init(&template_dev.sem, 1);

	if (template_init_cdev())
		goto init_fail_1;

	if (template_init_class())
		goto init_fail_2;

	return 0;

init_fail_2:
	cdev_del(&template_dev.cdev);
	unregister_chrdev_region(template_dev.devt, 1);

init_fail_1:

	return -1;
}
module_init(template_init);

static void __exit template_exit(void)
{
	printk(KERN_INFO "template_exit()\n");

	device_destroy(template_dev.class, template_dev.devt);
  	class_destroy(template_dev.class);

	cdev_del(&template_dev.cdev);
	unregister_chrdev_region(template_dev.devt, 1);

	if (template_dev.user_buff)
		kfree(template_dev.user_buff);
}
module_exit(template_exit);


MODULE_AUTHOR("Scott Ellis");
MODULE_DESCRIPTION("template driver");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION("0.1");

