/* For debugging the content of the registers and the "dma" region can be read in human readable
   for from a proc file. The code is put into a separate file in order not to clutter the 
   regular driver code.
 */

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/version.h>

#include "mtcadummy.h"
#include "mtcadummy_proc.h"

/*extern mtcaDummyData dummyPrivateData[MTCADUMMY_NR_DEVS];*/

/*
 * Here are our sequence iteration methods.  Our "position" is
 * simply the device number.
 */
static void *mtcadummy_seq_start(struct seq_file *s, loff_t *pos)
{
        if (*pos >= MTCADUMMY_NR_DEVS)
                return NULL;   /* No more to read */
        return dummyPrivateData + *pos;
}

static void *mtcadummy_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
        (*pos)++;
        if (*pos >= MTCADUMMY_NR_DEVS)
                return NULL;
        return dummyPrivateData + *pos;
}

static void mtcadummy_seq_stop(struct seq_file *s, void *v)
{
        /* Actually, there's nothing to do here */
}

/* Print the output of the system bar line by line.
   Print the output of the 
 */
static int mtcadummy_seq_show(struct seq_file *s, void *v)
{
        mtcaDummyData *dummyDeviceData = (mtcaDummyData *) v;
	unsigned int reg, index;
	int inUse;

	/* Only print something for devices which have been opened.*/
	/* Check whether the devide is open. If not just return (no error) */
	inUse = atomic_read(&dummyDeviceData->inUse);
	if (inUse == 0)
	{
	  seq_printf(s, "Device %i not in use.\n\n", dummyDeviceData->slotNr);
	  return 0;
	}

	if (mutex_lock_interruptible(&dummyDeviceData->devMutex)) {
	  dbg_print("mutex_lock_interruptible %s\n",
		    "- locking attempt was interrupted by a signal when preparing proc file");
	  return -ERESTARTSYS;
	}

        seq_printf(s, "Device %i System Bar:\n", dummyDeviceData->slotNr);
	seq_printf(s, "Register Content_hex Content_dec:\n");
	for (reg = 0; reg < MTCADUMMY_N_REGISTERS  ; ++reg)
	{
	  seq_printf(s, "0x%08X\t0x%08X\t%d\n",reg, *(dummyDeviceData->systemBar + reg),
		     *(dummyDeviceData->systemBar + reg));
	}

        seq_printf(s, "\nDevice %i DMA Bar:\n", dummyDeviceData->slotNr);
        seq_printf(s, "Offset Content[offset]  Content[offset+0x4]  Content[offset+0x8] Content[offset+0xC]:\n");
	for (index = 0; index <= MTCADUMMY_DMA_SIZE/sizeof(u32)-4; index+=4)
	{
	  seq_printf(s, "0x%08lX\t0x%08X\t0x%08X\t0x%08X\t0x%08X\n",
		     index*sizeof(u32), dummyDeviceData->dmaBar[index],dummyDeviceData->dmaBar[index+1],
		     dummyDeviceData->dmaBar[index+2],dummyDeviceData->dmaBar[index+3]);
	}
	seq_printf(s, "\n");

        mutex_unlock(&dummyDeviceData->devMutex);
        return 0;
}
        
/*
 * Tie the sequence operators up.
 */
static struct seq_operations mtcadummy_seq_operations = {
        .start = mtcadummy_seq_start,
        .next  = mtcadummy_seq_next,
        .stop  = mtcadummy_seq_stop,
        .show  = mtcadummy_seq_show
};

/*
 * Now to implement the /proc file we need only make an open
 * method which sets up the sequence operators.
 */
static int mtcadummy_proc_open(struct inode *inode, struct file *file)
{
        return seq_open(file, &mtcadummy_seq_operations);
}

/*
 * Create a set of file operations for the proc file.
 */
static struct file_operations mtcadummy_proc_opperations = {
        .owner   = THIS_MODULE,
        .open    = mtcadummy_proc_open,
        .read    = seq_read,
        .llseek  = seq_lseek,
        .release = seq_release
};
        

/*
 * Actually create (and remove) the /proc file(s). They cannot be static because they are used in the other object.
 */

void mtcadummy_create_proc(void)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
        struct proc_dir_entry *entry;
        entry = create_proc_entry("mtcadummy", 0, NULL);
        if (entry)
                entry->proc_fops = &mtcadummy_proc_opperations;
#else
	proc_create("mtcadummy", 0, NULL, &mtcadummy_proc_opperations);
#endif
}

void mtcadummy_remove_proc(void)
{
        /* no problem if it was not registered */
        remove_proc_entry("mtcadummy", NULL);
}
