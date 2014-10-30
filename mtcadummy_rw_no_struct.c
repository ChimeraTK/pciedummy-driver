#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <linux/moduleparam.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/device.h>

#include "mtcadummy.h"
#include "mtcadummy_firmware.h"

ssize_t mtcaDummy_write_no_struct(struct file *filp, const char __user *buf, 
				  size_t count, loff_t *f_pos)
{
  mtcaDummyData *deviceData;
  unsigned int bar;
  unsigned long offset; /* offset inside the bar */
  unsigned long barSizeInBytes;
  u32 * barStart, * barEnd;
  unsigned int nBytesToTransfer;

  deviceData = filp->private_data;
  
  /* ### sanity and input checks ### */

  /* check the input data. Only 32 bit reads are supported */
  if ( ((unsigned long)*f_pos)%4 ){
    printk("%s\n", "Incorrect position, has the be a multiple of 4");
    return -EFAULT;
  }
  if ( count%4 ){
    printk("%s\n", "Incorrect size, has the be a multiple of 4");
    return -EFAULT;
  }

  /* Before locking the mutex check if the request is valid (do not write after the end of the bar). */
  /* Do not access the registers, only check the pointer values without locking the mutex! */
  
  /* determine the bar from the f_pos */
  bar = (*f_pos >> 60) & 0x7;
  /* mask out the bar position from the offset */
  offset = *f_pos & 0x0FFFFFFFFFFFFFFFL;
  
  /* get the bar's start and end address */
  /* it is safe to access deviceData->systemBar without the mutex because it
     is never written anywhere */
  switch (bar){
  case 0:
    barStart = deviceData->systemBar;
    barSizeInBytes = MTCADUMMY_N_REGISTERS*4;
    break;
  case 2:
    barStart = deviceData->systemBar;
    barSizeInBytes = MTCADUMMY_N_REGISTERS*4;
    break;
  case 1:
  case 3:
  case 4:
  case 5:
    barStart = 0;
    barSizeInBytes = 0;
    
  default:
    printk("MTCADUMMY_WRITE_NO_STRUCT: Invalid bar number %d\n", bar);
    return -EFAULT;
  }
  /* When adding to a pointer, the + operator expects number of items, not the size in bytes */
  barEnd = barStart + barSizeInBytes/sizeof(u32);

  /* check that writing does not start after the end of the bar */
  if ( offset > barSizeInBytes ){
    printk("%s\n", "Cannot start writing after the end of the bar.");
    return -EFAULT;
  }

  /* limit the number of transferred by to the end of the bar. */
  /* The second line is safe because we checked before that offset <= barSizeInBytes */
  nBytesToTransfer = ( barSizeInBytes < offset + count ?
		       barSizeInBytes - offset  :
		       count);

  /* now we really want to access, so we need the mutex */
  if (mutex_lock_interruptible(&deviceData->devMutex)) {
    printk("mutex_lock_interruptible %s\n", "- locking attempt was interrupted by a signal");
    return -ERESTARTSYS;
  }
  
  /* not very efficient, but currently the pre read action is per word */
  { unsigned long offsetInLoop ;
    for (offsetInLoop = offset; offsetInLoop < barSizeInBytes; ++offsetInLoop){
      if (mtcadummy_performPreReadAction( offsetInLoop,
					  bar,
					  deviceData->slotNr )){
	dbg_print("Simulating read access to bad register at offset %lu, %s\n", 
		  offsetInLoop,
		  "intentinally causing an I/O error");
	mutex_unlock(&deviceData->devMutex);
	return -EIO;	  
      }      
    }/*for offsetInLoop*/
  }/*scope of offsetInLoop*/
  
  /* adding to the barStart pointer has to be in words, not in bytes */
  if (copy_from_user((void*)(barStart + (offset/sizeof(u32))), buf, nBytesToTransfer)){
    dbg_print("%s\n", "Error in copy_from_user");
    mutex_unlock(&deviceData->devMutex);
    return -EFAULT;
  }

  mutex_unlock(&deviceData->devMutex);
  return nBytesToTransfer;
}
