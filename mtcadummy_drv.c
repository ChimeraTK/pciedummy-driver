#include "mtcadummy.h"
#include "mtcadummy_firmware.h"
#include "mtcadummy_proc.h"
#include "mtcadummy_rw_no_struct.h"
/*#include "mtcadrv_io.h"  already done in mtcadummy.h*/

/* do we need ALL of these? */
#include <linux/device.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/version.h>
/*#include <linux/workqueue.h>*/
#include <asm/delay.h>
#include <linux/slab.h> /* kmalloc() */

#include "llrfdrv_io_compat.h"
#include "pcieuni_io_compat.h"

MODULE_AUTHOR("Martin Killenberg");
MODULE_DESCRIPTION("MTCA board dummy driver");
MODULE_VERSION(MTCADUMMY_MODULE_VERSION);
MODULE_LICENSE("Dual BSD/GPL");

/* 0 means automatic? */
int mtcaDummyMinorNr = 0;
int mtcaDummyMajorNr = 0;

struct class* mtcaDummyClass;

struct file_operations mtcaDummyFileOps;
struct file_operations llrfDummyFileOps;
struct file_operations noIoctlDummyFileOps;
struct file_operations pcieuniDummyFileOps;

/* this is the part I want to avoid, or is it not avoidable?*/
mtcaDummyData dummyPrivateData[MTCADUMMY_NR_DEVS];

/* initialise the device when loading the driver */
static int __init mtcaDummy_init_module(void) {
  int status;
  dev_t dev;
  int i, j;

  dbg_print("%s\n", "MODULE INIT");
  status = alloc_chrdev_region(&dev, mtcaDummyMinorNr, MTCADUMMY_NR_DEVS, MTCADUMMY_NAME);
  if(status) {
    dbg_print("Error in alloc_chrdev_region: %d\n", status);
    goto err_alloc_chrdev_region;
  }
  mtcaDummyMajorNr = MAJOR(dev);
  dbg_print("DEV MAJOR NR: %d\n", mtcaDummyMajorNr);
  mtcaDummyClass = class_create(THIS_MODULE, MTCADUMMY_NAME);
  if(IS_ERR(mtcaDummyClass)) {
    dbg_print("Error in class_create: %ld\n", PTR_ERR(mtcaDummyClass));
    goto err_class_create;
  }

  memset(dummyPrivateData, 0, sizeof(dummyPrivateData));
  for(i = 0; i < MTCADUMMY_NR_DEVS; i++) {
    dev_t deviceNumber = MKDEV(mtcaDummyMajorNr, mtcaDummyMinorNr + i);
    const char* deviceNameWithPlaceholder = NULL;

    /* before we initialise the character device we have to initialise all the
     * local variables and the mutex. These have to be in place before the
     * device file becomes available. */

    /* try to allocate memory, this might fail */
    dummyPrivateData[i].systemBar = kmalloc(MTCADUMMY_N_REGISTERS * sizeof(u32), GFP_KERNEL);
    if(dummyPrivateData[i].systemBar == NULL) {
      goto err_allocate_systemBar;
    }
    memset(dummyPrivateData[i].systemBar, 0, MTCADUMMY_N_REGISTERS * sizeof(u32));
    dummyPrivateData[i].dmaBar = kmalloc(MTCADUMMY_DMA_SIZE, GFP_KERNEL);
    if(dummyPrivateData[i].dmaBar == NULL) {
      /* free the already allocated memory */
      kfree(dummyPrivateData[i].systemBar);
      goto err_allocate_dmaBar;
    }
    memset(dummyPrivateData[i].dmaBar, 0, MTCADUMMY_DMA_SIZE);

    /* before initialising diver object run the "firmware" initialisation, i.e.
     * set the allocated registers*/
    mtcadummy_initialiseSystemBar(dummyPrivateData[i].systemBar);

    mutex_init(&dummyPrivateData[i].devMutex);
    atomic_set(&dummyPrivateData[i].inUse, 0);
    dummyPrivateData[i].slotNr = i;
    dummyPrivateData[i].minor = mtcaDummyMinorNr + i;
    dummyPrivateData[i].major = mtcaDummyMajorNr;

    /* small "hack" to have different ioct for one device */
    switch(i) {
      case 4:
        cdev_init(&dummyPrivateData[i].cdev, &llrfDummyFileOps);
        dummyPrivateData[i].cdev.ops = &llrfDummyFileOps;
        deviceNameWithPlaceholder = LLRFDUMMY_NAME "s%d";
        break;
      case 5:
        cdev_init(&dummyPrivateData[i].cdev, &noIoctlDummyFileOps);
        dummyPrivateData[i].cdev.ops = &noIoctlDummyFileOps;
        deviceNameWithPlaceholder = NOIOCTLDUMMY_NAME "s%d";
        break;
      case 6:
        cdev_init(&dummyPrivateData[i].cdev, &pcieuniDummyFileOps);
        dummyPrivateData[i].cdev.ops = &pcieuniDummyFileOps;
        deviceNameWithPlaceholder = PCIEUNIDUMMY_NAME "s%d";
        break;
      default:
        cdev_init(&dummyPrivateData[i].cdev, &mtcaDummyFileOps);
        dummyPrivateData[i].cdev.ops = &mtcaDummyFileOps;
        deviceNameWithPlaceholder = MTCADUMMY_NAME "s%d";
    }

    dummyPrivateData[i].cdev.owner = THIS_MODULE;
    status = cdev_add(&dummyPrivateData[i].cdev, deviceNumber, 1);
    if(status) {
      dbg_print("Error in cdev_add: %d\n", status);
      goto err_cdev_init;
    }

    if(device_create(mtcaDummyClass, NULL, deviceNumber, NULL, deviceNameWithPlaceholder, i) == NULL) {
      dbg_print("%s\n", "Error in device_create");
      goto err_device_create;
    }
  }

  mtcadummy_create_proc();

  dbg_print("%s\n", "MODULE INIT DONE");
  return 0;

  /* undo the steps for device i. As i is still at the same position where it
     was when we jumped out of the loop, we can use it here. */
err_device_create:
  cdev_del(&dummyPrivateData[i].cdev);
err_cdev_init:
  mutex_destroy(&dummyPrivateData[i].devMutex);
  kfree(dummyPrivateData[i].systemBar);
err_allocate_dmaBar:
  kfree(dummyPrivateData[i].systemBar);
err_allocate_systemBar:
  /* Unroll all already registered device files and their mutexes and memory. */
  /* As i still contains the position where the loop stopped we can use it here.
   */
  for(j = 0; j < i; j++) {
    device_destroy(mtcaDummyClass, MKDEV(mtcaDummyMajorNr, mtcaDummyMinorNr + j));
    cdev_del(&dummyPrivateData[j].cdev);
    mutex_destroy(&dummyPrivateData[j].devMutex);
    kfree(dummyPrivateData[j].systemBar);
    kfree(dummyPrivateData[j].dmaBar);
  }
  class_destroy(mtcaDummyClass);
err_class_create:
  unregister_chrdev_region(MKDEV(mtcaDummyMajorNr, mtcaDummyMinorNr), MTCADUMMY_NR_DEVS);
err_alloc_chrdev_region:
  return -1;
}

static void __exit mtcaDummy_cleanup_module(void) {
  int i = 0;
  dev_t deviceNumber;
  dbg_print("%s\n", "MODULE CLEANUP");

  mtcadummy_remove_proc();

  deviceNumber = MKDEV(mtcaDummyMajorNr, mtcaDummyMinorNr);
  for(i = 0; i < MTCADUMMY_NR_DEVS; i++) {
    device_destroy(mtcaDummyClass, MKDEV(mtcaDummyMajorNr, mtcaDummyMinorNr + i));
    cdev_del(&dummyPrivateData[i].cdev);
    mutex_destroy(&dummyPrivateData[i].devMutex);
    kfree(dummyPrivateData[i].systemBar);
    kfree(dummyPrivateData[i].dmaBar);
  }
  class_destroy(mtcaDummyClass);
  unregister_chrdev_region(deviceNumber, MTCADUMMY_NR_DEVS);
}

static int mtcaDummy_open(struct inode* inode, struct file* filp) {
  mtcaDummyData* privData;
  privData = container_of(inode->i_cdev, mtcaDummyData, cdev);
  filp->private_data = privData;

  // count up inUse
  atomic_inc(&privData->inUse);

  dbg_print("DEVICE IN SLOT: %d OPENED BY: %s PID: %d\n", privData->slotNr, current->comm, current->pid);
  return 0;
}

static int mtcaDummy_release(struct inode* inode, struct file* filp) {
  mtcaDummyData* privData;
  privData = filp->private_data;

  // count down inUse
  atomic_dec(&privData->inUse);

  dbg_print("DEVICE IN SLOT: %d CLOSED BY: %s PID: %d\n", privData->slotNr, current->comm, current->pid);
  return 0;
}

static ssize_t mtcaDummy_read(struct file* filp, char __user* buf, size_t count, loff_t* f_pos) {
  mtcaDummyData* privData;
  device_rw readReqInfo;
  u32* barBaseAddress;
  size_t barSize;

  privData = filp->private_data;

  if(mutex_lock_interruptible(&privData->devMutex)) {
    dbg_print("mutex_lock_interruptible %s\n", "- locking attempt was interrupted by a signal");
    return -ERESTARTSYS;
  }
  if(count < sizeof(readReqInfo)) {
    dbg_print("Number of data to read is less then required structure size. "
              "Minimum is %ld: requested size is %ld\n",
        sizeof(readReqInfo), count);
    mutex_unlock(&privData->devMutex);
    return -EFAULT;
  }
  if(copy_from_user(&readReqInfo, buf, sizeof(readReqInfo))) {
    dbg_print("copy_from_user - %s\n", "Cannot copy data from user");
    mutex_unlock(&privData->devMutex);
    return -EFAULT;
  }

  if(readReqInfo.mode_rw == RW_INFO) {
    readReqInfo.offset_rw = MTCADUMMY_DRV_VERSION_MIN;
    readReqInfo.data_rw = MTCADUMMY_DRV_VERSION_MAJ;
    readReqInfo.barx_rw = privData->slotNr;
    readReqInfo.mode_rw = MTCADUMMY_DRV_VERSION_MAJ + 1;
    readReqInfo.size_rw = MTCADUMMY_DRV_VERSION_MIN + 1;
    if(copy_to_user(buf, &readReqInfo, sizeof(device_rw))) {
      mutex_unlock(&privData->devMutex);
      return -EFAULT;
    }
    mutex_unlock(&privData->devMutex);
    return sizeof(device_rw);
  }
  else if(readReqInfo.mode_rw == RW_D32) {
    switch(readReqInfo.barx_rw) {
      case 0:
        barBaseAddress = privData->systemBar;
        barSize = MTCADUMMY_N_REGISTERS * 4;
        break;
      case 2:
        barBaseAddress = privData->dmaBar;
        barSize = MTCADUMMY_DMA_SIZE;
        break;
      default:
        dbg_print("Incorrect bar number %d, only 0 and 2 are supported\n", readReqInfo.barx_rw);
        mutex_unlock(&privData->devMutex);
        return -EFAULT;
    }
    if((readReqInfo.offset_rw % 4) || (readReqInfo.offset_rw >= barSize)) {
      dbg_print("Incorrect data offset %d\n", readReqInfo.offset_rw);
      mutex_unlock(&privData->devMutex);
      return -EFAULT;
    }

    if(mtcadummy_performPreReadAction(readReqInfo.offset_rw, readReqInfo.barx_rw, privData->slotNr)) {
      dbg_print("Simulating read access to bad register at offset %d, %s\n", readReqInfo.offset_rw,
          "intentinally causing an I/O error");
      mutex_unlock(&privData->devMutex);
      return -EIO;
    }

    readReqInfo.data_rw = barBaseAddress[readReqInfo.offset_rw / 4];

    if(copy_to_user(buf, &readReqInfo, sizeof(device_rw))) {
      mutex_unlock(&privData->devMutex);
      return -EFAULT;
    }
    mutex_unlock(&privData->devMutex);
    return sizeof(readReqInfo);
  }
  else if(readReqInfo.mode_rw == RW_D8 || readReqInfo.mode_rw == RW_D16) {
    dbg_print("Reading of RW_D8 or RW_D16 %s\n", "not supported");
    mutex_unlock(&privData->devMutex);
    return -EFAULT;
  }
  else if(readReqInfo.mode_rw == RW_DMA) {
    u32 dma_size;
    u32 dma_offset;

    dma_size = readReqInfo.size_rw;
    dma_offset = readReqInfo.offset_rw;

    if(dma_size == 0) {
      mutex_unlock(&privData->devMutex);
      return 0;
    }

    if((dma_size + dma_offset) > MTCADUMMY_DMA_SIZE) {
      dbg_print("%s\n", "Reqested dma transfer is too large");
      mutex_unlock(&privData->devMutex);
      return -EFAULT;
    }

    if(copy_to_user(buf, (void*)(privData->dmaBar + (dma_offset / 4)), dma_size)) {
      dbg_print("%s\n", "Error in copy_to_user");
      mutex_unlock(&privData->devMutex);
      return -EFAULT;
    }
    mutex_unlock(&privData->devMutex);
    return dma_size;
  }
  else {
    dbg_print("Command %d not supported\n", readReqInfo.mode_rw);
    mutex_unlock(&privData->devMutex);
    return -EFAULT;
  }
  mutex_unlock(&privData->devMutex);
  return -EFAULT;
}

static ssize_t mtcaDummy_write(struct file* filp, const char __user* buf, size_t count, loff_t* f_pos) {
  mtcaDummyData* privData;
  device_rw writeReqInfo;
  u32* barBaseAddress;
  size_t barSize;

  privData = filp->private_data;
  if(mutex_lock_interruptible(&privData->devMutex)) {
    dbg_print("mutex_lock_interruptible %s\n", "- locking attempt was interrupted by a signal");
    return -ERESTARTSYS;
  }
  if(count < sizeof(writeReqInfo)) {
    dbg_print("Number of data to write is less then required structure size. "
              "Minimum is %ld: requested size is %ld\n",
        sizeof(writeReqInfo), count);
    mutex_unlock(&privData->devMutex);
    return -EFAULT;
  }
  if(copy_from_user(&writeReqInfo, buf, sizeof(writeReqInfo))) {
    dbg_print("copy_from_user - %s\n", "Cannot copy data from user");
    mutex_unlock(&privData->devMutex);
    return -EFAULT;
  }

  if(writeReqInfo.mode_rw == RW_D32) {
    switch(writeReqInfo.barx_rw) {
      case 0:
        barBaseAddress = privData->systemBar;
        barSize = MTCADUMMY_N_REGISTERS * 4;
        break;
      case 2:
        barBaseAddress = privData->dmaBar;
        barSize = MTCADUMMY_DMA_SIZE;
        break;
      default:
        dbg_print("Incorrect bar number %d, only 0 and 2 are supported\n", writeReqInfo.barx_rw);
        mutex_unlock(&privData->devMutex);
        return -EFAULT;
    }
    if((writeReqInfo.offset_rw % 4) || (writeReqInfo.offset_rw >= barSize)) {
      dbg_print("Incorrect data offset %d\n", writeReqInfo.offset_rw);
      mutex_unlock(&privData->devMutex);
      return -EFAULT;
    }
    barBaseAddress[writeReqInfo.offset_rw / 4] = writeReqInfo.data_rw;
    // invoce the simulation of the "firmware"
    if(mtcadummy_performActionOnWrite(writeReqInfo.offset_rw, writeReqInfo.barx_rw, privData->slotNr)) {
      dbg_print("Simulating write access to bad register at offset %d, %s.\n", writeReqInfo.offset_rw,
          "intentinally causing an I/O error");
      mutex_unlock(&privData->devMutex);
      return -EIO;
    }

    /* not mapped memory, no need to wait here:        wmb(); */
    mutex_unlock(&privData->devMutex);
    return sizeof(writeReqInfo);
  }
  else if(writeReqInfo.mode_rw == RW_D8 || writeReqInfo.mode_rw == RW_D16) {
    dbg_print("Writing of RW_D8 or RW_D16 %s\n", "not supported");
    mutex_unlock(&privData->devMutex);
    return -EFAULT;
  }
  else if(writeReqInfo.mode_rw == RW_DMA) {
    dbg_print("DMA write %s\n", "not supported");
    mutex_unlock(&privData->devMutex);
    return -EFAULT;
  }
  else {
    dbg_print("Command %d not supported\n", writeReqInfo.mode_rw);
    mutex_unlock(&privData->devMutex);
    return -EFAULT;
  }
  mutex_unlock(&privData->devMutex);
  return -EFAULT;
}

static long mtcaDummy_ioctl(struct file* filp, unsigned int cmd, unsigned long arg) {
  mtcaDummyData* privateData;
  int status = 0;
  device_ioctrl_data dataStruct;
  device_ioctrl_dma dmaStruct;
  long returnValue = 0;

  if(_IOC_TYPE(cmd) != PCIEDOOCS_IOC) {
    dbg_print("Incorrect ioctl command %d\n", cmd);
    return -ENOTTY;
  }

  /* there are two ranges of ioct commands: 'normal' and ioclt */
  if(((_IOC_NR(cmd) < PCIEDOOCS_IOC_MINNR) || (_IOC_NR(cmd) > PCIEDOOCS_IOC_MAXNR)) &&
      ((_IOC_NR(cmd) < PCIEDOOCS_IOC_DMA_MINNR) || (_IOC_NR(cmd) > PCIEDOOCS_IOC_DMA_MAXNR))) {
    dbg_print("Incorrect ioctl command %d\n", cmd);
    return -ENOTTY;
  }

  if(_IOC_DIR(cmd) & _IOC_READ)
    status = !access_ok(VERIFY_WRITE, (void __user*)arg, _IOC_SIZE(cmd));
  else if(_IOC_DIR(cmd) & _IOC_WRITE)
    status = !access_ok(VERIFY_READ, (void __user*)arg, _IOC_SIZE(cmd));
  if(status) {
    dbg_print("Incorrect ioctl command %d\n", cmd);
    return -EFAULT;
  }

  privateData = filp->private_data;
  if(mutex_lock_interruptible(&privateData->devMutex)) {
    dbg_print("mutex_lock_interruptible %s\n", "- locking attempt was interrupted by a signal");
    return -ERESTARTSYS;
  }

  switch(cmd) {
    case PCIEDEV_PHYSICAL_SLOT:
      if(copy_from_user(&dataStruct, (device_ioctrl_data*)arg, sizeof(device_ioctrl_data))) {
        mutex_unlock(&privateData->devMutex);
        return -EFAULT;
      }
      dataStruct.data = privateData->slotNr;
      if(copy_to_user((device_ioctrl_data*)arg, &dataStruct, sizeof(device_ioctrl_data))) {
        returnValue = -EFAULT;
        // mutex will be unlocked directly after the switch
      }
      break;
    case PCIEDEV_DRIVER_VERSION:
    case PCIEDEV_FIRMWARE_VERSION:
      /* dummy driver and firmware version are identical */
      if(copy_from_user(&dataStruct, (device_ioctrl_data*)arg, sizeof(device_ioctrl_data))) {
        mutex_unlock(&privateData->devMutex);
        return -EFAULT;
      }
      dataStruct.data = MTCADUMMY_DRV_VERSION_MAJ;
      dataStruct.offset = MTCADUMMY_DRV_VERSION_MIN;
      if(copy_to_user((device_ioctrl_data*)arg, &dataStruct, sizeof(device_ioctrl_data))) {
        returnValue = -EFAULT;
        // mutex will be unlocked directly after the switch
      }
      break;
    case PCIEDEV_READ_DMA:
      // like the current struct-read implementation we ignore the bar information
      // (variable 'pattern')
      if(copy_from_user(&dmaStruct, (device_ioctrl_dma*)arg, sizeof(device_ioctrl_dma))) {
        mutex_unlock(&privateData->devMutex);
        return -EFAULT;
      }
      if(dmaStruct.dma_offset + dmaStruct.dma_size > MTCADUMMY_DMA_SIZE) {
        mutex_unlock(&privateData->devMutex);
        return -EFAULT;
      }
      // check that the offset is a multiple of 4
      if(dmaStruct.dma_offset % 4 != 0) {
        mutex_unlock(&privateData->devMutex);
        return -EFAULT;
      }
      // as there is no real dma we also ignore the control register variable
      // ('cmd')

      // adding to the dmaBAR, which is a pointer, adds multiples of the word
      // size, so the values added has to be in words, not bytes
      if(copy_to_user((u32*)arg, privateData->dmaBar + (dmaStruct.dma_offset / 4), dmaStruct.dma_size)) {
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
 * to be removed. It's not worth the efford to make the constants variables
 * which can be changed for llrf and pciedev, just to safe some lines of
 * duplicity. This is a temporary solution!
 */
static long llrfDummy_ioctl(struct file* filp, unsigned int cmd, unsigned long arg) {
  mtcaDummyData* privateData;
  int status = 0;
  device_ioctrl_data dataStruct;
  long returnValue = 0;

  if(_IOC_TYPE(cmd) != LLRFDRV_IOC) {
    dbg_print("Incorrect ioctl command %d\n", cmd);
    return -ENOTTY;
  }

  if(((_IOC_NR(cmd) < LLRFDRV_IOC_MINNR) || (_IOC_NR(cmd) > LLRFDRV_IOC_MAXNR))) {
    dbg_print("Incorrect ioctl command %d\n", cmd);
    return -ENOTTY;
  }

  if(_IOC_DIR(cmd) & _IOC_READ)
    status = !access_ok(VERIFY_WRITE, (void __user*)arg, _IOC_SIZE(cmd));
  else if(_IOC_DIR(cmd) & _IOC_WRITE)
    status = !access_ok(VERIFY_READ, (void __user*)arg, _IOC_SIZE(cmd));
  if(status) {
    dbg_print("Incorrect ioctl command %d\n", cmd);
    return -EFAULT;
  }

  privateData = filp->private_data;
  if(mutex_lock_interruptible(&privateData->devMutex)) {
    dbg_print("mutex_lock_interruptible %s\n", "- locking attempt was interrupted by a signal");
    return -ERESTARTSYS;
  }

  switch(cmd) {
    case LLRFDRV_PHYSICAL_SLOT:
      if(copy_from_user(&dataStruct, (device_ioctrl_data*)arg, sizeof(device_ioctrl_data))) {
        mutex_unlock(&privateData->devMutex);
        return -EFAULT;
      }
      dataStruct.data = privateData->slotNr;
      if(copy_to_user((device_ioctrl_data*)arg, &dataStruct, sizeof(device_ioctrl_data))) {
        returnValue = -EFAULT;
        // mutex will be unlocked directly after the switch
      }
      break;
    case LLRFDRV_DRIVER_VERSION:
    case LLRFDRV_FIRMWARE_VERSION:
      /* dummy driver and firmware version are identical */
      if(copy_from_user(&dataStruct, (device_ioctrl_data*)arg, sizeof(device_ioctrl_data))) {
        mutex_unlock(&privateData->devMutex);
        return -EFAULT;
      }
      dataStruct.data = MTCADUMMY_DRV_VERSION_MAJ;
      dataStruct.offset = MTCADUMMY_DRV_VERSION_MIN;
      if(copy_to_user((device_ioctrl_data*)arg, &dataStruct, sizeof(device_ioctrl_data))) {
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

/* Just another copy of the ioctl sequence. It really is time to phase out the
 * old drivers.
 */
static long pcieuniDummy_ioctl(struct file* filp, unsigned int cmd, unsigned long arg) {
  mtcaDummyData* privateData;
  int status = 0;
  device_ioctrl_data dataStruct;
  device_ioctrl_dma dmaStruct;
  long returnValue = 0;

  if(_IOC_TYPE(cmd) != PCIEUNI_IOC) {
    dbg_print("Incorrect ioctl command %d\n", cmd);
    return -ENOTTY;
  }

  /* there are two ranges of ioct commands: 'normal' and ioclt */
  if(((_IOC_NR(cmd) < PCIEUNI_IOC_MINNR) || (_IOC_NR(cmd) > PCIEUNI_IOC_MAXNR)) &&
      ((_IOC_NR(cmd) < PCIEUNI_IOC_DMA_MINNR) || (_IOC_NR(cmd) > PCIEUNI_IOC_DMA_MAXNR))) {
    dbg_print("Incorrect ioctl command %d\n", cmd);
    return -ENOTTY;
  }

  if(_IOC_DIR(cmd) & _IOC_READ)
    status = !access_ok(VERIFY_WRITE, (void __user*)arg, _IOC_SIZE(cmd));
  else if(_IOC_DIR(cmd) & _IOC_WRITE)
    status = !access_ok(VERIFY_READ, (void __user*)arg, _IOC_SIZE(cmd));
  if(status) {
    dbg_print("Incorrect ioctl command %d\n", cmd);
    return -EFAULT;
  }

  privateData = filp->private_data;
  if(mutex_lock_interruptible(&privateData->devMutex)) {
    dbg_print("mutex_lock_interruptible %s\n", "- locking attempt was interrupted by a signal");
    return -ERESTARTSYS;
  }

  switch(cmd) {
    case PCIEUNI_PHYSICAL_SLOT:
      if(copy_from_user(&dataStruct, (device_ioctrl_data*)arg, sizeof(device_ioctrl_data))) {
        mutex_unlock(&privateData->devMutex);
        return -EFAULT;
      }
      dataStruct.data = privateData->slotNr;
      if(copy_to_user((device_ioctrl_data*)arg, &dataStruct, sizeof(device_ioctrl_data))) {
        returnValue = -EFAULT;
        // mutex will be unlocked directly after the switch
      }
      break;
    case PCIEUNI_DRIVER_VERSION:
    case PCIEUNI_FIRMWARE_VERSION:
      /* dummy driver and firmware version are identical */
      if(copy_from_user(&dataStruct, (device_ioctrl_data*)arg, sizeof(device_ioctrl_data))) {
        mutex_unlock(&privateData->devMutex);
        return -EFAULT;
      }
      dataStruct.data = MTCADUMMY_DRV_VERSION_MAJ;
      dataStruct.offset = MTCADUMMY_DRV_VERSION_MIN;
      if(copy_to_user((device_ioctrl_data*)arg, &dataStruct, sizeof(device_ioctrl_data))) {
        returnValue = -EFAULT;
        // mutex will be unlocked directly after the switch
      }
      break;
    case PCIEUNI_READ_DMA:
      // like the current struct-read implementation we ignore the bar information
      // (variable 'pattern')
      if(copy_from_user(&dmaStruct, (device_ioctrl_dma*)arg, sizeof(device_ioctrl_dma))) {
        mutex_unlock(&privateData->devMutex);
        return -EFAULT;
      }
      if(dmaStruct.dma_offset + dmaStruct.dma_size > MTCADUMMY_DMA_SIZE) {
        mutex_unlock(&privateData->devMutex);
        return -EFAULT;
      }
      // check that the offset is a multiple of 4
      if(dmaStruct.dma_offset % 4 != 0) {
        mutex_unlock(&privateData->devMutex);
        return -EFAULT;
      }
      // as there is no real dma we also ignore the control register variable
      // ('cmd')

      // adding to the dmaBAR, which is a pointer, adds multiples of the word
      // size, so the values added has to be in words, not bytes
      if(copy_to_user((u32*)arg, privateData->dmaBar + (dmaStruct.dma_offset / 4), dmaStruct.dma_size)) {
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

struct file_operations mtcaDummyFileOps = {
    .owner = THIS_MODULE,
    .read = mtcaDummy_read,
    .write = mtcaDummy_write,
    .unlocked_ioctl = mtcaDummy_ioctl,
    .open = mtcaDummy_open,
    .release = mtcaDummy_release,
};

struct file_operations llrfDummyFileOps = {
    .owner = THIS_MODULE,
    .read = mtcaDummy_read,
    .write = mtcaDummy_write,
    .unlocked_ioctl = llrfDummy_ioctl,
    .open = mtcaDummy_open,
    .release = mtcaDummy_release,
};

struct file_operations noIocltDummyFileOps = {
    .owner = THIS_MODULE,
    .read = mtcaDummy_read,
    .write = mtcaDummy_write,
    .open = mtcaDummy_open,
    .release = mtcaDummy_release,
};

struct file_operations pcieuniDummyFileOps = {
    .owner = THIS_MODULE,
    .read = mtcaDummy_read_no_struct,
    .write = mtcaDummy_write_no_struct,
    .unlocked_ioctl = pcieuniDummy_ioctl,
    .open = mtcaDummy_open,
    .release = mtcaDummy_release,
};

module_init(mtcaDummy_init_module);
module_exit(mtcaDummy_cleanup_module);
