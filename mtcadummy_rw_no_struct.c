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

/** A struct which contains information about the transfer which is to be performed.
 *  Used to get data out of a common calculate and check function for read and write.
 *  Not all variables are needed later, but they are kept in case they are needed in future
 *  because they have to be calculated anyway for the checks.
 */
typedef struct _transfer_information{
  unsigned int bar;
  unsigned long offset; /* offset inside the bar */
  unsigned long barSizeInBytes;
  u32 * barStart, * barEnd;
  unsigned int nBytesToTransfer;
} transfer_information;


/** Checks the input and calculates the transfer information.
 *  Returns a negative value in case of error, 0 upon success.
 */
int checkAndCalculateTransferInformation( mtcaDummyData const * deviceData,
					  size_t count, loff_t virtualOffset,
					  transfer_information * transferInformation ){

  /* check the input data. Only 32 bit reads are supported */
  if ( virtualOffset%4 ){
    printk("%s\n", "Incorrect position, has the be a multiple of 4");
    return -EFAULT;
  }
  if ( count%4 ){
    printk("%s\n", "Incorrect size, has the be a multiple of 4");
    return -EFAULT;
  }

  /*  printk("mtcadummy::checkAndCalculateTransferInformation: count %zx , virtualOffsets %Lx\n", count, virtualOffset );
   */

  /* Before locking the mutex check if the request is valid (do not write after the end of the bar). */
  /* Do not access the registers, only check the pointer values without locking the mutex! */
  
  /* determine the bar from the f_pos */
  transferInformation->bar = (virtualOffset >> 60) & 0x7;
  /* mask out the bar position from the offset */
  transferInformation->offset = virtualOffset & 0x0FFFFFFFFFFFFFFFL;
  
  /*  printk("mtcadummy::checkAndCalculateTransferInformation: bar %x, offset %lx\n",
	    transferInformation->bar,
	    transferInformation->offset);  
  */

  /* get the bar's start and end address */
  /* FIXME: organise the information as arrays, not as individual variables, and you might get rid of this block */
  switch (transferInformation->bar){
  case 0:
    transferInformation->barStart = deviceData->systemBar;
    transferInformation->barSizeInBytes = MTCADUMMY_N_REGISTERS*4;
    break;
  case 2:
    transferInformation->barStart = deviceData->dmaBar;
    transferInformation->barSizeInBytes = MTCADUMMY_DMA_SIZE;
    break;
  case 1:
  case 3:
  case 4:
  case 5:
    transferInformation->barStart = 0;
    transferInformation->barSizeInBytes = 0;
    
  default:
    printk("MTCADUMMY_WRITE_NO_STRUCT: Invalid bar number %d\n", 
	   transferInformation->bar);
    return -EFAULT;
  }  

  if (!transferInformation->barStart){
    printk("BAR %d not implemented in this device",  transferInformation->bar);
    return -EFAULT;
  }

  /* When adding to a pointer, the + operator expects number of items, not the size in bytes */
  transferInformation->barEnd = transferInformation->barStart + 
    transferInformation->barSizeInBytes/sizeof(u32);

  /*
  printk("mtcadummy::checkAndCalculateTransferInformation: barStart %p, barSize %lx, barEnd %p\n",
	    transferInformation->barStart, transferInformation->barSizeInBytes,
	    transferInformation->barEnd);  
  */

  /* check that writing does not start after the end of the bar */
  if ( transferInformation->offset > transferInformation->barSizeInBytes ){
    printk("%s\n", "Cannot start writing after the end of the bar.");
    return -EFAULT;
  }

  /* limit the number of transferred by to the end of the bar. */
  /* The second line is safe because we checked before that offset <= barSizeInBytes */
  transferInformation->nBytesToTransfer = 
    ( (transferInformation->barSizeInBytes < transferInformation->offset + count) ?
      transferInformation->barSizeInBytes - transferInformation->offset  :
      count);

  /*
  printk("mtcadummy::checkAndCalculateTransferInformation:  nBytesToTransfer %x, count %lx",
	 transferInformation->nBytesToTransfer, count);
  */

  return 0;
}

ssize_t mtcaDummy_read_no_struct(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
  mtcaDummyData *deviceData;
  transfer_information transferInformation;
  int transferInfoError;

  deviceData = filp->private_data;
  
  /* The checkAndCalculateTransferInformation is only accessing static data in the 
     deviceData struct. No need to hold the mutex.
     FIXME: Is this correct? What if the device goes offline in the mean time?
  */
  transferInfoError = 
    checkAndCalculateTransferInformation( deviceData, count, *f_pos, &transferInformation);
  
  if (transferInfoError){
    return transferInfoError;
  }

  /* now we really want to access, so we need the mutex */
  if (mutex_lock_interruptible(&deviceData->devMutex)) {
    printk("mutex_lock_interruptible %s\n", "- locking attempt was interrupted by a signal");
    return -ERESTARTSYS;
  }
  
  printk("mtcaDummy_read_no_struct: starting copy from user");  
  

  /* not very efficient, but currently the post-write action is per word */
  { unsigned int i;
    for (i = 0; i < transferInformation.nBytesToTransfer/sizeof(int32_t); ++i){

      /* offset in loop is the additional offset due to the writing process */
      unsigned int offsetInLoop = i*sizeof(int32_t);

      /* invoke the simulation of the "firmware" */
      if (mtcadummy_performPreReadAction( transferInformation.offset + offsetInLoop,
					  transferInformation.bar,
					  deviceData->slotNr )){
	dbg_print("Simulating read access to bad register at offset %lx, %s\n", 
		  transferInformation.offset + offsetInLoop,
		  "intentinally causing an I/O error");
	mutex_unlock(&deviceData->devMutex);
	return -EIO;	  
      }
   
      /* buf is a pointer to char, to here we add the offset in bytes */
      if (copy_to_user(buf + offsetInLoop,
		       transferInformation.barStart + 
		       transferInformation.offset/sizeof(uint32_t) +i, 
		       sizeof(int32_t))){
	dbg_print("%s\n", "Error in copy_to_user");
	mutex_unlock(&deviceData->devMutex);
	return -EFAULT;
      }

      /* update the f_pos pointer after a successful read */
      *f_pos += sizeof(u32);
    }/*for offsetInLoop*/
  }/*scope of i*/

  printk("mtcaDummy_read_no_struct: copy to user finished");  

  mutex_unlock(&deviceData->devMutex);
  return transferInformation.nBytesToTransfer;
}

ssize_t mtcaDummy_write_no_struct(struct file *filp, const char __user *buf, 
				  size_t count, loff_t *f_pos)
{
  mtcaDummyData *deviceData;
  transfer_information transferInformation;
  int transferInfoError;

  deviceData = filp->private_data;
  
  /* The checkAndCalculateTransferInformation is only accessing static data in the 
     deviceData struct. No need to hold the mutex.
     FIXME: Is this correct? What if the device goes offline in the mean time?
  */
  transferInfoError = 
    checkAndCalculateTransferInformation( deviceData, count, *f_pos, &transferInformation);
  
  if (transferInfoError){
    return transferInfoError;
  }

  /* now we really want to access, so we need the mutex */
  if (mutex_lock_interruptible(&deviceData->devMutex)) {
    printk("mutex_lock_interruptible %s\n", "- locking attempt was interrupted by a signal");
    return -ERESTARTSYS;
  }
  

  /*well, this obviously is for the read section, keep here to cpoy over */
  /*  { unsigned long offsetInLoop ;
    for (offsetInLoop = offset; offsetInLoop < barSizeInBytes; 
	 offsetInLoop+=sizeof(int32_t)){
      if (mtcadummy_performPreReadAction( offsetInLoop,
					  bar,
					  deviceData->slotNr )){
	dbg_print("Simulating read access to bad register at offset %lu, %s\n", 
		  offsetInLoop,
		  "intentinally causing an I/O error");
	mutex_unlock(&deviceData->devMutex);
	return -EIO;	  
      }      
    }
  }
*/

  printk("mtcaDummy_write_no_struct: starting copy from user");  
  

  /* not very efficient, but currently the post-write action is per word */
  { unsigned int i;
    for (i = 0; i < transferInformation.nBytesToTransfer/sizeof(int32_t); ++i){
      /* offset in loop is the additional offset due to the writing process */
      unsigned int offsetInLoop = i*sizeof(int32_t);
      /* adding to the barStart pointer has to be in words, not in bytes */
      if (copy_from_user((void*)(transferInformation.barStart + 
				 transferInformation.offset/sizeof(uint32_t) +i), 
			 /* buf is a pointer to char, to here we add the offset in bytes */
			   buf + offsetInLoop, sizeof(int32_t))){
	dbg_print("%s\n", "Error in copy_from_user");
	mutex_unlock(&deviceData->devMutex);
	return -EFAULT;
      }

      /* invoke the simulation of the "firmware" */
      if (mtcadummy_performActionOnWrite( transferInformation.offset + offsetInLoop,
					  transferInformation.bar,
					  deviceData->slotNr )){
	dbg_print("Simulating write access to bad register at offset %ld, %s.\n",
		  transferInformation.offset + offsetInLoop,
		  "intentinally causing an I/O error");
	mutex_unlock(&deviceData->devMutex);
	return -EIO;	  
      }
      
      /* update the f_pos pointer after a successful read */
      *f_pos += sizeof(u32);
    }/*for offsetInLoop*/
  }/*scope of i*/

  printk("mtcaDummy_write_no_struct: copy to user finished");  

  mutex_unlock(&deviceData->devMutex);
  
  return transferInformation.nBytesToTransfer;
}
