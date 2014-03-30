#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h> // required for various structures related to files liked fops. 
#include <linux/semaphore.h>
#include <linux/cdev.h> 
#include <linux/version.h>
#define IOC_MAGIC '0xFE'

#define IOCTL_SETEKEY _IO(IOC_MAGIC,0)