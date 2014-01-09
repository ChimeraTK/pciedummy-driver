/* A dummy driver which does not access the PCI bus but features the same
 * interface as the llrfuni driver. All operations are performed in 
 * memory.
 * The idea is to have a device to test against with defined behaviour
 * without the need to install hardware.
 * This allows the implementation and execution of unit tests / automated tests
 * on any computer.
 *
 * For debugging purposes there is a proc file which dumps the register content and the dma area 
 * in human readable form.
 */

#ifndef UTCA_DUMMY_DRV_H
#define UTCA_DUMMY_DRV_H

/* FIXME: Where to put the 'extern C' declaration for C++?
 * Here: +simplifies C++, -is not plain C
 * In each C++ include: +unnecessary code duplication, -keeps the C code clean
 */

/* Include the io definitions for the regular driver. The interface on the
 * user side should be the same.
 */
#include <linux/cdev.h>
#include <linux/module.h>
#include <asm/atomic.h> 

#include "utcadrv_io.h"

#define UTCADUMMY_NR_DEVS       4 /*create 4 devices*/
#define UTCADUMMY_DRV_VERSION_MAJ 0 /*dummy driver major version*/
#define UTCADUMMY_DRV_VERSION_MIN 1 /*dummy driver minor version*/

//#define UTCADUMMY_VENDOR_ID               0x10EE
//#define UTCADUMMY_DEVICE_ID               0x0038

#define UTCADUMMY_NAME                    "utcadummy"
#define UTCADUMMY_DBG_MSG_DEV_NAME        "UTCADUMMY"

#define __DEBUG_MODE__

/* some defines for the dummy test. We only provide two bars:
 * bar 0 consits of "registers", each 32 bit word being a register
 * bar 2 is a "dma" bar where arbitrary data is located There is no real DMA as there is no hardware.
 * it's just called that for consistency
 * Idea: bar 0 should be accessed by ioctl, bar 2 is accessed by read/write.
 * Currently everything is done read/write, as in the original llrfuni.
 */

#define UTCADUMMY_N_REGISTERS 32 /* 32 registers, just as a starting point */
#define UTCADUMMY_DMA_SIZE 4096  /* 4 kilo bytes (= 1k 32bit words) */

typedef struct _utcaDummyData {    
    atomic_t            inUse; /* count how many times the device has been opened in a thread-safe way. */
    struct cdev         cdev; /* character device struct */
    struct mutex        devMutex; /* The mutex for concurrent access */
    u32                 slotNr;
    /*struct pci_dev      *dev;
    int                 waitFlag;
    wait_queue_head_t   waitDMA;*/
    int                 minor;/*I think we need major and minor. Read only, but no mutex required.*/
    int                 major;

    u32 *               systemBar; /* only access when holding the mutex! */
    u32 *               dmaBar; /* Just called dma. Only access when holding the mutex! */
  /* not sure, but I think from memory there is no need for a buffer.
     it's in memory anyway.
    void*               pWriteBuf;
	u32                 pWriteBufOrder;*/
    
  /* this is in the ddrfdev.h. I try to avoid explicit bar handling like this.
     Maybe it's needed. I don't see clearly yet.
    struct {                
        u32             resStart;
        u32             resEnd;
        u32             resFlag;
        void __iomem    *baseAddress;
        u32             memSize;
    } barInfo[MAX_BAR_NR];
   */
} utcaDummyData;

/* defined in the utcadrummy_drv.c file, but needed in all of the .c files */
extern utcaDummyData dummyPrivateData[UTCADUMMY_NR_DEVS];

#ifdef __DEBUG_MODE__
#define dbg_print(format, ...)  do {\
                                    printk("%s: [%s] " format, UTCADUMMY_DBG_MSG_DEV_NAME, __FUNCTION__,__VA_ARGS__); \
                                    } while (0);
#else
#define dbg_print(...)
#endif


#endif /* UTCA_DUMMY_DRV_H */
