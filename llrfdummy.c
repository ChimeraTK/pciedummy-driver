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
/*#include <linux/workqueue.h>*/
#include <linux/slab.h> /* kmalloc() */
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
	
	/* before we initialise the character device we have to initialise all the local variables and
	 * the mutex. These have to be in place before the device file becomes available. */
	
	/* try to allocate memory, this might fail */
	dummyPrivateData[i].registerBar = kmalloc( LLRFDUMMY_N_REGISTERS * sizeof(u32), GFP_KERNEL);
	if ( dummyPrivateData[i].registerBar == NULL)
	{
	  goto err_device_create;
	}
	memset(dummyPrivateData[i].registerBar, 0, LLRFDUMMY_N_REGISTERS * sizeof(u32));
	dummyPrivateData[i].dmaBar = kmalloc(  LLRFDUMMY_DMA_SIZE, GFP_KERNEL);
	if ( dummyPrivateData[i].dmaBar == NULL)
	{
	  /* free the already allocated memory */
	  kfree(  dummyPrivateData[i].registerBar );
	  goto err_device_create;
	}
	memset(dummyPrivateData[i].dmaBar, 0, LLRFDUMMY_DMA_SIZE);

        mutex_init(&dummyPrivateData[i].devMutex);
	atomic_set(&dummyPrivateData[i].inUse,0);
	dummyPrivateData[i].slotNr = i;
	dummyPrivateData[i].minor =  llrfDummyMinorNr + i;
	dummyPrivateData[i].major = llrfDummyMajorNr;

        cdev_init(&dummyPrivateData[i].cdev, &llrfDummyFileOps);
        dummyPrivateData[i].cdev.owner = THIS_MODULE;
        dummyPrivateData[i].cdev.ops = &llrfDummyFileOps;
        status = cdev_add(&dummyPrivateData[i].cdev, deviceNumber, 1);
        if (status) {
            dbg_print("Error in cdev_add: %d\n", status);
	    /* release this mutex and free the memory*/
	    mutex_destroy(&dummyPrivateData[i].devMutex);
	    kfree(dummyPrivateData[i].registerBar);
	    kfree(dummyPrivateData[i].dmaBar);
	    
            goto err_device_create;
        }
    }

    dbg_print("%s\n", "MODULE INIT DONE");
    return 0;

err_device_create:
    /* Unroll all already registered device files and their mutexes and memory. */
    /* As i still contains the position where the loop stopped we can use it here. */
    for (j = 0; j < i; j++) {      
      cdev_del(&dummyPrivateData[j].cdev);
      mutex_destroy(&dummyPrivateData[j].devMutex);
      kfree(dummyPrivateData[j].registerBar);
      kfree(dummyPrivateData[j].dmaBar);
    }
    class_destroy(llrfDummyClass);
err_class_create:
    unregister_chrdev_region(MKDEV(llrfDummyMajorNr, llrfDummyMinorNr), LLRFDUMMY_NR_DEVS);
err_alloc_chrdev_region:
    return -1;

}

static void __exit llrfDummy_cleanup_module(void) {
    int i = 0;
    dev_t deviceNumber;
    dbg_print("%s\n", "MODULE CLEANUP");

    deviceNumber = MKDEV(llrfDummyMinorNr, llrfDummyMinorNr);
    for (i = 0; i < LLRFDUMMY_NR_DEVS + 1; i++) {
        cdev_del(&dummyPrivateData[i].cdev);
        mutex_destroy(&dummyPrivateData[i].devMutex);
	kfree(dummyPrivateData[i].registerBar);	
	kfree(dummyPrivateData[i].dmaBar);
    }
    class_destroy(llrfDummyClass);
    unregister_chrdev_region(deviceNumber, LLRFDUMMY_NR_DEVS);
}

static int llrfDummy_open(struct inode *inode, struct file *filp) {
    llrfDummyData *privData;
    privData = container_of(inode->i_cdev, llrfDummyData, cdev);
    filp->private_data = privData;

    // count up inUse
    atomic_inc(&privData->inUse);

    dbg_print("DEVICE IN SLOT: %d OPENED BY: %s PID: %d\n", privData->slotNr, current->comm, current->pid);
    return 0;
}

static int llrfDummy_release(struct inode *inode, struct file *filp) {
    llrfDummyData *privData;
    privData = filp->private_data;

    // count down inUse
    atomic_dec(&privData->inUse);

    dbg_print("DEVICE IN SLOT: %d CLOSED BY: %s PID: %d\n", privData->slotNr, current->comm, current->pid);
    return 0;
}

static ssize_t llrfDummy_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
    llrfDummyData *privData;
    device_rw readReqInfo;
    u32 * barBaseAddress;
    size_t barSize;

    privData = filp->private_data;


    if (mutex_lock_interruptible(&privData->devMutex)) {
        dbg_print("mutex_lock_interruptible %s\n", "- locking attempt was interrupted by a signal");
        return -ERESTARTSYS;
    }
    if (count < sizeof (readReqInfo)) {
        dbg_print("Number of data to read is less then required structure size. Minimum is %ld: requested size is %ld\n", sizeof (readReqInfo), count);
        mutex_unlock(&privData->devMutex);
        return -EFAULT;
    }
    if (copy_from_user(&readReqInfo, buf, sizeof (readReqInfo))) {
        dbg_print("copy_from_user - %s\n", "Cannot copy data from user");
        mutex_unlock(&privData->devMutex);
        return -EFAULT;
    }

    if (readReqInfo.mode_rw == RW_INFO) {
        readReqInfo.offset_rw = LLRFDUMMY_DRV_VERSION_MIN;
        readReqInfo.data_rw = LLRFDUMMY_DRV_VERSION_MAJ;
        readReqInfo.barx_rw = privData->slotNr;
        readReqInfo.mode_rw = LLRFDUMMY_DRV_VERSION_MAJ + 1;
        readReqInfo.size_rw = LLRFDUMMY_DRV_VERSION_MIN + 1;
        if (copy_to_user(buf, &readReqInfo, sizeof (device_rw))) {
            mutex_unlock(&privData->devMutex);
            return -EFAULT;
        }
        mutex_unlock(&privData->devMutex);
        return sizeof (device_rw);
    } else if (readReqInfo.mode_rw == RW_D32) {
        switch( readReqInfo.barx_rw ){
	case 0:
	    barBaseAddress = privData->registerBar;
	    barSize = LLRFDUMMY_N_REGISTERS*4;
	    break;
	case 2:
	    barBaseAddress = privData->dmaBar;
	    barSize = LLRFDUMMY_DMA_SIZE;
	    break;
	default:
            dbg_print("Incorrect bar number %d, only 0 and 2 are supported\n", readReqInfo.barx_rw);
            mutex_unlock(&privData->devMutex);
            return -EFAULT;
        }
        if ((readReqInfo.offset_rw % 4) || (readReqInfo.offset_rw >= barSize)) {
            dbg_print("Incorrect data offset %d\n", readReqInfo.offset_rw);
            mutex_unlock(&privData->devMutex);
            return -EFAULT;
        }

        readReqInfo.data_rw = barBaseAddress[readReqInfo.offset_rw/4];

        if (copy_to_user(buf, &readReqInfo, sizeof (device_rw))) {
            mutex_unlock(&privData->devMutex);
            return -EFAULT;
        }
        mutex_unlock(&privData->devMutex);
        return sizeof (readReqInfo);
    } else if (readReqInfo.mode_rw == RW_D8 || readReqInfo.mode_rw == RW_D16) {
        dbg_print("Reading of RW_D8 or RW_D16 %s\n", "not supported");
        mutex_unlock(&privData->devMutex);
        return -EFAULT;
    } else if (readReqInfo.mode_rw == RW_DMA) {
        u32 dma_size;
        u32 dma_offset;

        dma_size = readReqInfo.size_rw;
        dma_offset = readReqInfo.offset_rw;

        if (dma_size == 0) {
            mutex_unlock(&privData->devMutex);
            return 0;
        }
	
	if ( (dma_size + dma_offset) > LLRFDUMMY_DMA_SIZE) {
            dbg_print("%s\n", "Reqested dma transfer is too large");
            mutex_unlock(&privData->devMutex);
            return -EFAULT;
        }
    
        if (copy_to_user(buf,  (void*)(privData->dmaBar + (dma_offset/4)), dma_size)) {
                dbg_print("%s\n", "Error in copy_to_user");
                mutex_unlock(&privData->devMutex);
                return -EFAULT;
	}
        mutex_unlock(&privData->devMutex);
        return dma_size;
    } else {
        dbg_print("Command %d not supported\n", readReqInfo.mode_rw);
        mutex_unlock(&privData->devMutex);
        return -EFAULT;
    }
    mutex_unlock(&privData->devMutex);
    return -EFAULT;
}


static ssize_t llrfDummy_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
    llrfDummyData *privData;
    device_rw writeReqInfo;
    u32 * barBaseAddress;
    size_t barSize;

    privData = filp->private_data;
    if (mutex_lock_interruptible(&privData->devMutex)) {
        dbg_print("mutex_lock_interruptible %s\n", "- locking attempt was interrupted by a signal");
        return -ERESTARTSYS;
    }
    if (count < sizeof (writeReqInfo)) {
        dbg_print("Number of data to write is less then required structure size. Minimum is %ld: requested size is %ld\n", sizeof (writeReqInfo), count);
        mutex_unlock(&privData->devMutex);
        return -EFAULT;
    }
    if (copy_from_user(&writeReqInfo, buf, sizeof (writeReqInfo))) {
        dbg_print("copy_from_user - %s\n", "Cannot copy data from user");
        mutex_unlock(&privData->devMutex);
        return -EFAULT;
    }

    if (writeReqInfo.mode_rw == RW_D32) {
        switch( writeReqInfo.barx_rw ){
	case 0:
	    barBaseAddress = privData->registerBar;
	    barSize = LLRFDUMMY_N_REGISTERS*4;
	    break;
	case 2:
	    barBaseAddress = privData->dmaBar;
	    barSize = LLRFDUMMY_DMA_SIZE;
	    break;
	default:
            dbg_print("Incorrect bar number %d, only 0 and 2 are supported\n", writeReqInfo.barx_rw);
            mutex_unlock(&privData->devMutex);
            return -EFAULT;
        }
        if ((writeReqInfo.offset_rw % 4) || (writeReqInfo.offset_rw >= barSize)) {
            dbg_print("Incorrect data offset %d\n", writeReqInfo.offset_rw);
            mutex_unlock(&privData->devMutex);
            return -EFAULT;
        }
        barBaseAddress[writeReqInfo.offset_rw/4] = writeReqInfo.data_rw;
	
	/* not mapped memory, no need to wait here:        wmb(); */
        mutex_unlock(&privData->devMutex);
        return sizeof (writeReqInfo);
    } else if (writeReqInfo.mode_rw == RW_D8 || writeReqInfo.mode_rw == RW_D16) {
        dbg_print("Writing of RW_D8 or RW_D16 %s\n", "not supported");
        mutex_unlock(&privData->devMutex);
        return -EFAULT;
    } else if (writeReqInfo.mode_rw == RW_DMA) {
        dbg_print("DMA write %s\n", "not supported");
        mutex_unlock(&privData->devMutex);
        return -EFAULT;
    } else {
        dbg_print("Command %d not supported\n", writeReqInfo.mode_rw);
        mutex_unlock(&privData->devMutex);
        return -EFAULT;
    }
    mutex_unlock(&privData->devMutex);
    return -EFAULT;
}


/*
#if LINUX_VERSION_CODE < 0x20613 
static int llrfDummy_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg) {
#else
static long llrfDummy_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
#endif
    llrfDummyData                 *privData;
    int                         status = 0;
    device_ioctrl_data          data;

    if (_IOC_TYPE(cmd) != LLRFDUMMY_IOC) {
        dbg_print("Incorrect ioctl command %d\n", cmd);
        return -ENOTTY;
    }
    if (_IOC_NR(cmd) > LLRFDUMMY_IOC_MAXNR) {
        dbg_print("Incorrect ioctl command %d\n", cmd);
        return -ENOTTY;
    }
    if (_IOC_NR(cmd) < LLRFDUMMY_IOC_MINNR) {
        dbg_print("Incorrect ioctl command %d\n", cmd);
        return -ENOTTY;
    }
    if (_IOC_DIR(cmd) & _IOC_READ)
        status = !access_ok(VERIFY_WRITE, (void __user *) arg, _IOC_SIZE(cmd));
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
        status = !access_ok(VERIFY_READ, (void __user *) arg, _IOC_SIZE(cmd));
    if (status) {
        dbg_print("Incorrect ioctl command %d\n", cmd);
        return -EFAULT;
    }

    privData = filp->private_data;
    if (mutex_lock_interruptible(&privData->devMutex)) {
        dbg_print("mutex_lock_interruptible %s\n", "- locking attempt was interrupted by a signal");
        return -ERESTARTSYS;
    }

    switch (cmd) {
        case LLRFDUMMY_PHYSICAL_SLOT:
            if (copy_from_user(&data, (device_ioctrl_data*) arg, sizeof(device_ioctrl_data))) {
                mutex_unlock(&privData->devMutex);
                return -EFAULT;
            }            
            data.data = privData->slotNr;            
            if (copy_to_user((device_ioctrl_data*) arg, &data, sizeof(device_ioctrl_data))) {
                mutex_unlock(&privData->devMutex);
                return -EFAULT;
            }
            break;
        case LLRFDUMMY_DRIVER_VERSION:
            data.data = LLRFDEV_DUMMY_VERSION_MAJ;
            data.offset = LLRFDEV_DUMMY_VERSION_MIN;
            if (copy_to_user((device_ioctrl_data*) arg, &data, sizeof(device_ioctrl_data))) {
                mutex_unlock(&privData->devMutex);
                return -EFAULT;
            }
            break;
        case LLRFDUMMY_FIRMWARE_VERSION:
            data.data = LLRFDEV_DUMMY_VERSION_MAJ + 1;
            data.offset = LLRFDEV_DUMMY_VERSION_MIN + 1;
            if (copy_to_user((device_ioctrl_data*) arg, &data, sizeof(device_ioctrl_data))) {
                mutex_unlock(&privData->devMutex);
                return -EFAULT;
            }
            break;
        default:
            return -ENOTTY;
    }
    mutex_unlock(&privData->devMutex);
    return 0;
}
*/

struct file_operations llrfDummyFileOps = {
    .owner = THIS_MODULE,
    .read = llrfDummy_read,
    .write = llrfDummy_write,
    /*
#if LINUX_VERSION_CODE < 0x20613     
    .ioctl = llrfdrv_ioctl,    
#else    
    .unlocked_ioctl = llrfdrv_ioctl,
#endif
    */    
    .open = llrfDummy_open,
    .release = llrfDummy_release,
};

module_init(llrfDummy_init_module);
module_exit(llrfDummy_cleanup_module);
