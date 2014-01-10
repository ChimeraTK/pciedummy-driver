#include "utcadummy.h"
#include "utcadummy_proc.h"
#include "utcadummy_firmware.h"
/*#include "utcadrv_io.h"  already done in utcadummy.h*/

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

#include "llrfdrv_io_compat.h"

MODULE_AUTHOR("Martin Killenberg");
MODULE_DESCRIPTION("UTCA board dummy driver");
MODULE_VERSION("0.5.0");
MODULE_LICENSE("Dual BSD/GPL");

/* 0 means automatic? */
int utcaDummyMinorNr = 0;
int utcaDummyMajorNr = 0;

struct class* utcaDummyClass;

struct file_operations utcaDummyFileOps;
struct file_operations llrfDummyFileOps;
/* static struct pci_driver pci_utcadrv; no pci access */

/* this is the part I want to avoid, or is it not?*/
utcaDummyData dummyPrivateData[UTCADUMMY_NR_DEVS];

/* initialise the device when loading the driver */
static int __init utcaDummy_init_module(void) {
    int status;
    dev_t dev;
    int i, j;

    dbg_print("%s\n", "MODULE INIT");
    status = alloc_chrdev_region(&dev, utcaDummyMinorNr, UTCADUMMY_NR_DEVS, UTCADUMMY_NAME);
    if (status) {
        dbg_print("Error in alloc_chrdev_region: %d\n", status);
        goto err_alloc_chrdev_region;
    }
    utcaDummyMajorNr = MAJOR(dev);
    dbg_print("DEV MAJOR NR: %d\n", utcaDummyMajorNr);
    utcaDummyClass = class_create(THIS_MODULE, UTCADUMMY_NAME);
    if (IS_ERR(utcaDummyClass)) {
        dbg_print("Error in class_create: %ld\n", PTR_ERR(utcaDummyClass));
        goto err_class_create;
    }

    memset(dummyPrivateData, 0, sizeof (dummyPrivateData));
    for (i = 0; i < UTCADUMMY_NR_DEVS; i++) {
        dev_t deviceNumber = MKDEV(utcaDummyMajorNr, utcaDummyMinorNr + i);
	
	/* before we initialise the character device we have to initialise all the local variables and
	 * the mutex. These have to be in place before the device file becomes available. */
	
	/* try to allocate memory, this might fail */
	dummyPrivateData[i].systemBar = kmalloc( UTCADUMMY_N_REGISTERS * sizeof(u32), GFP_KERNEL);
	if ( dummyPrivateData[i].systemBar == NULL)
	{
	  goto err_allocate_systemBar;
	}
	memset(dummyPrivateData[i].systemBar, 0, UTCADUMMY_N_REGISTERS * sizeof(u32));
	dummyPrivateData[i].dmaBar = kmalloc(  UTCADUMMY_DMA_SIZE, GFP_KERNEL);
	if ( dummyPrivateData[i].dmaBar == NULL)
	{
	  /* free the already allocated memory */
	  kfree(  dummyPrivateData[i].systemBar );
	  goto err_allocate_dmaBar;
	}
	memset(dummyPrivateData[i].dmaBar, 0, UTCADUMMY_DMA_SIZE);

	/* before initialising diver object run the "firmware" initialisation, i.e. set the allocated registers*/
	utcadummy_initialiseSystemBar(dummyPrivateData[i].systemBar);

        mutex_init(&dummyPrivateData[i].devMutex);
	atomic_set(&dummyPrivateData[i].inUse,0);
	dummyPrivateData[i].slotNr = i;
	dummyPrivateData[i].minor =  utcaDummyMinorNr + i;
	dummyPrivateData[i].major = utcaDummyMajorNr;

	/* small "hack" to have different ioct for one device */
	if (i == 4){
	  cdev_init(&dummyPrivateData[i].cdev, &llrfDummyFileOps);
	  dummyPrivateData[i].cdev.ops = &llrfDummyFileOps;
	}
	else{
	  cdev_init(&dummyPrivateData[i].cdev, &utcaDummyFileOps);
	  dummyPrivateData[i].cdev.ops = &utcaDummyFileOps;
	}

        dummyPrivateData[i].cdev.owner = THIS_MODULE;
        status = cdev_add(&dummyPrivateData[i].cdev, deviceNumber, 1);
        if (status) {
            dbg_print("Error in cdev_add: %d\n", status);
            goto err_cdev_init;
        }

	if (device_create(utcaDummyClass, NULL, deviceNumber, NULL, 
			  (i==4?LLRFDUMMY_NAME"s%d":UTCADUMMY_NAME"s%d"), i) == NULL) {
	    dbg_print("%s\n", "Error in device_create");
	    goto err_device_create;
	}

    }

    utcadummy_create_proc();

    dbg_print("%s\n", "MODULE INIT DONE");
    return 0;

    /* undo the steps for device i. As i is still at the same position where it was when we jumped 
       out of the loop, we can use it here. */
err_device_create:
    cdev_del(&dummyPrivateData[j].cdev);
err_cdev_init:
    mutex_destroy(&dummyPrivateData[i].devMutex);
    kfree(dummyPrivateData[i].systemBar);
err_allocate_dmaBar:
    kfree(dummyPrivateData[i].systemBar);
err_allocate_systemBar:
    /* Unroll all already registered device files and their mutexes and memory. */
    /* As i still contains the position where the loop stopped we can use it here. */
    for (j = 0; j < i; j++) {      
      device_destroy(utcaDummyClass, MKDEV(utcaDummyMajorNr, utcaDummyMinorNr + j));
      cdev_del(&dummyPrivateData[j].cdev);
      mutex_destroy(&dummyPrivateData[j].devMutex);
      kfree(dummyPrivateData[j].systemBar);
      kfree(dummyPrivateData[j].dmaBar);
    }
    class_destroy(utcaDummyClass);
err_class_create:
    unregister_chrdev_region(MKDEV(utcaDummyMajorNr, utcaDummyMinorNr), UTCADUMMY_NR_DEVS);
err_alloc_chrdev_region:
    return -1;

}

static void __exit utcaDummy_cleanup_module(void) {
    int i = 0;
    dev_t deviceNumber;
    dbg_print("%s\n", "MODULE CLEANUP");

    utcadummy_remove_proc();

    deviceNumber = MKDEV(utcaDummyMajorNr, utcaDummyMinorNr);
    for (i = 0; i < UTCADUMMY_NR_DEVS; i++) {
        device_destroy(utcaDummyClass, MKDEV(utcaDummyMajorNr, utcaDummyMinorNr + i));
        cdev_del(&dummyPrivateData[i].cdev);
        mutex_destroy(&dummyPrivateData[i].devMutex);
	kfree(dummyPrivateData[i].systemBar);	
	kfree(dummyPrivateData[i].dmaBar);
    }
    class_destroy(utcaDummyClass);
    unregister_chrdev_region(deviceNumber, UTCADUMMY_NR_DEVS);
}

static int utcaDummy_open(struct inode *inode, struct file *filp) {
    utcaDummyData *privData;
    privData = container_of(inode->i_cdev, utcaDummyData, cdev);
    filp->private_data = privData;

    // count up inUse
    atomic_inc(&privData->inUse);

    dbg_print("DEVICE IN SLOT: %d OPENED BY: %s PID: %d\n", privData->slotNr, current->comm, current->pid);
    return 0;
}

static int utcaDummy_release(struct inode *inode, struct file *filp) {
    utcaDummyData *privData;
    privData = filp->private_data;

    // count down inUse
    atomic_dec(&privData->inUse);

    dbg_print("DEVICE IN SLOT: %d CLOSED BY: %s PID: %d\n", privData->slotNr, current->comm, current->pid);
    return 0;
}

static ssize_t utcaDummy_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
    utcaDummyData *privData;
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
        readReqInfo.offset_rw = UTCADUMMY_DRV_VERSION_MIN;
        readReqInfo.data_rw = UTCADUMMY_DRV_VERSION_MAJ;
        readReqInfo.barx_rw = privData->slotNr;
        readReqInfo.mode_rw = UTCADUMMY_DRV_VERSION_MAJ + 1;
        readReqInfo.size_rw = UTCADUMMY_DRV_VERSION_MIN + 1;
        if (copy_to_user(buf, &readReqInfo, sizeof (device_rw))) {
            mutex_unlock(&privData->devMutex);
            return -EFAULT;
        }
        mutex_unlock(&privData->devMutex);
        return sizeof (device_rw);
    } else if (readReqInfo.mode_rw == RW_D32) {
        switch( readReqInfo.barx_rw ){
	case 0:
	    barBaseAddress = privData->systemBar;
	    barSize = UTCADUMMY_N_REGISTERS*4;
	    break;
	case 2:
	    barBaseAddress = privData->dmaBar;
	    barSize = UTCADUMMY_DMA_SIZE;
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
	
	if ( (dma_size + dma_offset) > UTCADUMMY_DMA_SIZE) {
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


static ssize_t utcaDummy_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
    utcaDummyData *privData;
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
	    barBaseAddress = privData->systemBar;
	    barSize = UTCADUMMY_N_REGISTERS*4;
	    break;
	case 2:
	    barBaseAddress = privData->dmaBar;
	    barSize = UTCADUMMY_DMA_SIZE;
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
	// invoce the simulation of the "firmware"
	utcadummy_performActionOnWrite( writeReqInfo.offset_rw,  writeReqInfo.barx_rw );
		
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



static long utcaDummy_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
      utcaDummyData                 *privateData;
      int                         status = 0;
      device_ioctrl_data          dataStruct;
      device_ioctrl_dma           dmaStruct;
      long returnValue = 0;

    if (_IOC_TYPE(cmd) != PCIEDOOCS_IOC) {
        dbg_print("Incorrect ioctl command %d\n", cmd);
        return -ENOTTY;
    }

    /* there are two ranges of ioct commands: 'normal' and ioclt */
    if ( ( (_IOC_NR(cmd) < PCIEDOOCS_IOC_MINNR) || 
	   (_IOC_NR(cmd) > PCIEDOOCS_IOC_MAXNR) )  && 
	 ( (_IOC_NR(cmd) < PCIEDOOCS_IOC_DMA_MINNR) || 
	   (_IOC_NR(cmd) > PCIEDOOCS_IOC_DMA_MAXNR) ) ) {
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

    privateData = filp->private_data;
    if (mutex_lock_interruptible(&privateData->devMutex)) {
        dbg_print("mutex_lock_interruptible %s\n", "- locking attempt was interrupted by a signal");
        return -ERESTARTSYS;
    }

    switch (cmd) {
        case PCIEDEV_PHYSICAL_SLOT:
            if (copy_from_user(&dataStruct, (device_ioctrl_data*) arg, sizeof(device_ioctrl_data))) {
                mutex_unlock(&privateData->devMutex);
                return -EFAULT;
            }            
            dataStruct.data = privateData->slotNr;            
            if (copy_to_user((device_ioctrl_data*) arg, &dataStruct, sizeof(device_ioctrl_data))) {
                returnValue = -EFAULT;
		// mutex will be unlocked directly after the switch
            }
            break;
        case PCIEDEV_DRIVER_VERSION:
        case PCIEDEV_FIRMWARE_VERSION:
	  /* dummy driver and firmware version are identical */
            if (copy_from_user(&dataStruct, (device_ioctrl_data*) arg, sizeof(device_ioctrl_data))) {
                mutex_unlock(&privateData->devMutex);
                return -EFAULT;
            }            
            dataStruct.data = UTCADUMMY_DRV_VERSION_MAJ;
            dataStruct.offset = UTCADUMMY_DRV_VERSION_MIN;
            if (copy_to_user((device_ioctrl_data*) arg, &dataStruct, sizeof(device_ioctrl_data))) {
                returnValue = -EFAULT;
		// mutex will be unlocked directly after the switch
            }
            break;
        case PCIEDEV_READ_DMA:
	  // like the current struct-read implementation we ignore the bar information (variable 'pattern')
            if (copy_from_user(&dmaStruct, (device_ioctrl_dma*) arg, sizeof(device_ioctrl_dma))) {
                mutex_unlock(&privateData->devMutex);
                return -EFAULT;
            }            
	    if( dmaStruct.dma_offset + dmaStruct.dma_size > UTCADUMMY_DMA_SIZE ) {
                mutex_unlock(&privateData->devMutex);
                return -EFAULT;
	    }
	    // as there is no real dma we also ignore the control register variable ('cmd')

            if (copy_to_user((u32*) arg, privateData->dmaBar, dmaStruct.dma_size)) {
                returnValue = -EFAULT;
		// mutex will be unlocked directly after the switch
            }
            break;
        default:
	  returnValue = -ENOTTY;
    }
    mutex_unlock(&privateData->devMutex);
    return returnValue;
}

/* An alternatice ioctl section for the llrfdummy device.
 * 
 * Unfortunately this is a copy and paste hack for backward compatibility and
 * to be removed. It's not worth the efford to make the constants variables which
 * can be changed for llrf and pciedev, just to safe some lines of duplicity.
 * This is a temporary solution!
 */
static long llrfDummy_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
      utcaDummyData                 *privateData;
      int                         status = 0;
      device_ioctrl_data          dataStruct;
      long returnValue = 0;

    if (_IOC_TYPE(cmd) != LLRFDRV_IOC) {
        dbg_print("Incorrect ioctl command %d\n", cmd);
        return -ENOTTY;
    }

    if ( ( (_IOC_NR(cmd) < LLRFDRV_IOC_MINNR) || 
	   (_IOC_NR(cmd) > LLRFDRV_IOC_MAXNR) ) ) {
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

    privateData = filp->private_data;
    if (mutex_lock_interruptible(&privateData->devMutex)) {
        dbg_print("mutex_lock_interruptible %s\n", "- locking attempt was interrupted by a signal");
        return -ERESTARTSYS;
    }

    switch (cmd) {
        case LLRFDRV_PHYSICAL_SLOT:
            if (copy_from_user(&dataStruct, (device_ioctrl_data*) arg, sizeof(device_ioctrl_data))) {
                mutex_unlock(&privateData->devMutex);
                return -EFAULT;
            }            
            dataStruct.data = privateData->slotNr;            
            if (copy_to_user((device_ioctrl_data*) arg, &dataStruct, sizeof(device_ioctrl_data))) {
                returnValue = -EFAULT;
		// mutex will be unlocked directly after the switch
            }
            break;
        case LLRFDRV_DRIVER_VERSION:
        case LLRFDRV_FIRMWARE_VERSION:
	  /* dummy driver and firmware version are identical */
            if (copy_from_user(&dataStruct, (device_ioctrl_data*) arg, sizeof(device_ioctrl_data))) {
                mutex_unlock(&privateData->devMutex);
                return -EFAULT;
            }            
            dataStruct.data = UTCADUMMY_DRV_VERSION_MAJ;
            dataStruct.offset = UTCADUMMY_DRV_VERSION_MIN;
            if (copy_to_user((device_ioctrl_data*) arg, &dataStruct, sizeof(device_ioctrl_data))) {
                returnValue = -EFAULT;
		// mutex will be unlocked directly after the switch
            }
            break;
        default:
	  returnValue = -ENOTTY;
    }
    mutex_unlock(&privateData->devMutex);
    return returnValue;
}


struct file_operations utcaDummyFileOps = {
    .owner = THIS_MODULE,
    .read = utcaDummy_read,
    .write = utcaDummy_write,
    .unlocked_ioctl = utcaDummy_ioctl,
    .open = utcaDummy_open,
    .release = utcaDummy_release,
};

struct file_operations llrfDummyFileOps = {
    .owner = THIS_MODULE,
    .read = utcaDummy_read,
    .write = utcaDummy_write,
    .unlocked_ioctl = llrfDummy_ioctl,
    .open = utcaDummy_open,
    .release = utcaDummy_release,
};

module_init(utcaDummy_init_module);
module_exit(utcaDummy_cleanup_module);
