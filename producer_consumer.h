
#define DRIVER_NAME "producer_consumer"
#define PDEBUG(fmt,args...) printk(KERN_DEBUG"%s:"fmt,__FUNCTION__, ##args)
#define PERR(fmt,args...) printk(KERN_ERR"%s:"fmt,__FUNCTION__,##args)
#define PINFO(fmt,args...) printk(KERN_INFO"%s:"fmt,__FUNCTION__, ##args)
#include<linux/init.h>
#include<linux/list.h>
#include<linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>  // for threads
#include <linux/debugfs.h>
