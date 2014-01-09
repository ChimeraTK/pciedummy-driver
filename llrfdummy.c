#include "llrfdummy.h"
/*#include "llrfdrv_io.h"  already done in llrfdummy.h*/

/* do we need ALL of these? */
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <linux/moduleparam.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <linux/workqueue.h>
#include <asm/delay.h> 

MODULE_AUTHOR("Martin Killenberg");
MODULE_DESCRIPTION("LLRF board dummy driver");
MODULE_VERSION("0.0.0");
MODULE_LICENSE("Dual BSD/GPL");

/* 0 means automatic? */
int llrfDummyMinorNr = 0;
int llrfDummyMajorNr = 0;

struct class* llrfDummyClass;

struct file_operations llrfDummyFileOps;
/* static struct pci_driver pci_llrfdrv; no pci access */

/* this is the part I want to avoid, or is it not?*/
llrfDummyData dummyPrivateData[LLRFDUMMY_NR_DEVS + 1]; /*why +1?*/

/* initialise the device when loading the driver */
static int __init llrfDummy_init_module(void) {
    int status;
    dev_t dev;
    int i, j;
    int deviceNumber;

    dbg_print("%s\n", "MODULE INIT");
    status = alloc_chrdev_region(&dev, llrfDummyMinorNr, LLRFDUMMY_NR_DEVS, LLRFDUMMY_NAME);
    if (status) {
        dbg_print("Error in alloc_chrdev_region: %d\n", status);
        goto err_alloc_chrdev_region;
    }
    llrfDummyMajorNr = MAJOR(dev);
    dbg_print("DEV MAJOR NR: %d\n", llrfDummyMajorNr);
    llrfDummyClass = class_create(THIS_MODULE, LLRFDUMMY_NAME);
    if (IS_ERR(llrfDummyClass)) {
        dbg_print("Error in class_create: %ld\n", PTR_ERR(llrfDummyClass));
        goto err_class_create;
    }

    memset(dummyPrivateData, 0, sizeof (dummyPrivateData));
    for (i = 0; i < LLRFDUMMY_NR_DEVS + 1; i++) {
        deviceNumber = MKDEV(llrfDummyMajorNr, llrfDummyMinorNr + i);
        cdev_init(&dummyPrivateData[i].cdev, &llrfDummyFileOps);
        dummyPrivateData[i].cdev.owner = THIS_MODULE;
        dummyPrivateData[i].cdev.ops = &llrfDummyFileOps;
        status = cdev_add(&dummyPrivateData[i].cdev, deviceNumber, 1);
        if (status) {
            dbg_print("Error in cdev_add: %d\n", status);
            for (j = 0; j < i; j++) {
                cdev_del(&dummyPrivateData[j].cdev);
            }
            goto err_cdev_add;
        }
        mutex_init(&dummyPrivateData[i].devMutex);
        dummyPrivateData[i].inUse = 0;
    }

    dbg_print("%s\n", "MODULE INIT DONE");
    return 0;

err_cdev_add:
    class_destroy(llrfDummyClass);
err_class_create:
    unregister_chrdev_region(MKDEV(llrfDummyMajorNr, llrfDummyMinorNr), LLRFDUMMY_NR_DEVS);
err_alloc_chrdev_region:
    return -1;

}
