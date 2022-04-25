/* For debugging the content of the registers and the "dma" region can be read
   in human readable for from a proc file. The code is put into a separate file
   in order not to clutter the regular driver code.
 */

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#include "mtcadummy.h"
#include "mtcadummy_proc.h"

/*
 * Here are our sequence iteration methods.  Our "position" is
 * simply the device number.
 */
static void* mtcadummy_seq_start(struct seq_file* s, loff_t* pos) {
  if(*pos >= MTCADUMMY_NR_DEVS) return NULL; /* No more to read */
  return ((struct mtcaDummyControl *) s->private)->data + *pos;
}

static void* mtcadummy_seq_next(struct seq_file* s, void* v, loff_t* pos) {
  (*pos)++;
  if(*pos >= MTCADUMMY_NR_DEVS) return NULL;
  return ((struct mtcaDummyControl *) s->private)->data + *pos;
}

static void mtcadummy_seq_stop(struct seq_file* s, void* v) {
  /* Actually, there's nothing to do here */
}

/* Print the output of the system bar line by line.
   Print the output of the
 */
static int mtcadummy_seq_show(struct seq_file* s, void* v) {
  mtcaDummyData* dummyDeviceData = (mtcaDummyData*)v;
  unsigned int reg, index;
  int inUse;

  /* Only print something for devices which have been opened.*/
  /* Check whether the devide is open. If not just return (no error) */
  inUse = atomic_read(&dummyDeviceData->inUse);
  if(inUse == 0) {
    seq_printf(s, "Device %i not in use.\n\n", dummyDeviceData->slotNr);
    return 0;
  }

  if(mutex_lock_interruptible(&dummyDeviceData->devMutex)) {
    dbg_print("mutex_lock_interruptible %s\n",
        "- locking attempt was interrupted by a signal when preparing "
        "proc file");
    return -ERESTARTSYS;
  }

  seq_printf(s, "Device %i System Bar:\n", dummyDeviceData->slotNr);
  seq_printf(s, "Register Content_hex Content_dec:\n");
  for(reg = 0; reg < MTCADUMMY_N_REGISTERS; ++reg) {
    seq_printf(
        s, "0x%08X\t0x%08X\t%d\n", reg, *(dummyDeviceData->systemBar + reg), *(dummyDeviceData->systemBar + reg));
  }

  seq_printf(s, "\nDevice %i DMA Bar:\n", dummyDeviceData->slotNr);
  seq_printf(s,
      "Offset Content[offset]  Content[offset+0x4]  "
      "Content[offset+0x8] Content[offset+0xC]:\n");
  for(index = 0; index <= MTCADUMMY_DMA_SIZE / sizeof(u32) - 4; index += 4) {
    seq_printf(s, "0x%08lX\t0x%08X\t0x%08X\t0x%08X\t0x%08X\n", index * sizeof(u32), dummyDeviceData->dmaBar[index],
        dummyDeviceData->dmaBar[index + 1], dummyDeviceData->dmaBar[index + 2], dummyDeviceData->dmaBar[index + 3]);
  }
  seq_printf(s, "\n");

  mutex_unlock(&dummyDeviceData->devMutex);
  return 0;
}

/*
 * Tie the sequence operators up.
 */
static struct seq_operations mtcadummy_seq_operations = {.start = mtcadummy_seq_start,
    .next = mtcadummy_seq_next,
    .stop = mtcadummy_seq_stop,
    .show = mtcadummy_seq_show};

/*
 * Now to implement the /proc file we need only make an open
 * method which sets up the sequence operators.
 */
static int mtcadummy_proc_open(struct inode* inode, struct file* file) {
  int ret = seq_open(file, &mtcadummy_seq_operations);
  if (!ret) {
    // copied from single_open
    ((struct seq_file*)file->private_data)->private = PDE_DATA(inode);
  }

  return ret;
}

static ssize_t mtcadummy_proc_write(struct file* file, const char __user* ubuf, size_t count, loff_t* ppos) {
  struct mtcaDummyControl* d = PDE_DATA(file_inode(file));

  size_t len;
  char cmd[255];
  char* iter = cmd;
  char* token = NULL;
  int value = 0;
  int open = -1, spi = -1, read = -1, write = -1;

  dbg_print("%s", "got write\n");

  len = min(count, sizeof(cmd) - 1);
  if (copy_from_user(cmd, ubuf, len))
    return -EFAULT;

  cmd[len] = '\0';

  // Collect all parameters, lock the control mutex once we are done
  // to modify all in one go
  while ((token = strsep(&iter, " ")) != NULL) {
    if (sscanf(token, "open:%i", &value) == 1) {
      open = value;
    } else if (sscanf(token, "read:%i", &value) == 1) {
      read = value;
    } else if (sscanf(token, "write:%i", &value) == 1) {
      write = value;
    } else if (sscanf(token, "spi:%i", &value) == 1) {
      spi = value;
    }
    dbg_print("Got token %s\n", token);
  }

  if (mutex_lock_interruptible(&controlMutex) != 0) {
    pr_warn("Unable to lock control mutex");
    return -EWOULDBLOCK;
  }

  // Fill control data - or copy back if not set, so we can print it below
  if (open > -1)
    d->open_error = open;
  else
    open = d->open_error;

  if (read > -1)
    d->read_error = read;
  else
    read = d->read_error;

  if (write > -1)
    d->write_error = write;
  else
    write = d->write_error;

  if (spi > -1)
    d->spi_error = spi;
  else
    spi = d->spi_error;

  mutex_unlock(&controlMutex);

  dbg_print("module parameters changed: open:%d read:%d write:%d spi:%d\n", open, read, write, spi);

  return count;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,6,0)
/*
 * Create a set of file operations for the proc file.
 */
static struct file_operations mtcadummy_proc_opperations = {
    .owner = THIS_MODULE,
    .open = mtcadummy_proc_open,
    .read = seq_read,
    .write = mtcadummy_proc_write,
    .llseek = seq_lseek,
    .release = seq_release};
#else
static struct proc_ops mtcadummy_proc_opperations = {
    .proc_open = mtcadummy_proc_open,
    .proc_read = seq_read,
    .proc_write = mtcadummy_proc_write,
    .proc_lseek = seq_lseek,
    .proc_release = seq_release
};
#endif

/*
 * Actually create (and remove) the /proc file(s). They cannot be static because
 * they are used in the other object.
 */

void mtcadummy_create_proc(void *data) {
  proc_create_data("mtcadummy", 0666, NULL, &mtcadummy_proc_opperations, data);
}

void mtcadummy_remove_proc(void) {
  /* no problem if it was not registered */
  remove_proc_entry("mtcadummy", NULL);
}
