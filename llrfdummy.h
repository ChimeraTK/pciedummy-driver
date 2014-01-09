/* A dummy driver which does not access the PCI bus but features the same
 * interface as the llrfuni driver. All operations are performed in 
 * memory.
 * The idea is to have a device to test against with defined behaviour
 * without the need to install hardware.
 * This allows the implementation and execution of unit tests / automated tests
 * on any computer.
 */

#ifndef LLRF_DUMMY_DRV_H
#define LLRF_DUMMY_DRV_H

/* FIXME: Where to put the 'extern C' declaration for C++?
 * Here: +simplifies C++, -is not plain C
 * In each C++ include: +unnecessary code duplication, -keeps the C code clean
 */

/* Include the io definitions for the regular driver. The interface on the
 * user side should be the same.
 */
#include <linux/cdev.h>
#include <linux/module.h>

#include "llrfdrv_io.h"

#define LLRFDUMMY_NR_DEVS       4 /*create 4 devices*/
#define LLRFDUMMY_DRV_VERSION_MAJ 0 /*dummy driver major version*/
#define LLRFDUMMY_DRV_VERSION_MIN 0 /*dummy driver minor version*/

#define LLRFDUMMY_VENDOR_ID               0x10EE
#define LLRFDUMMY_DEVICE_ID               0x0038

#define LLRFDUMMY_NAME                    "llrfdummy"
#define LLRFDUMMY_DBG_MSG_DEV_NAME        "LLRFDUMMY"

#define __DEBUG_MODE__

typedef struct _llrfDummyData {    
    char                inUse;
    struct cdev         cdev; /* character device struct */
    struct mutex        devMutex; /* The mutex for concurrent access */
  /*    u32                 slotNr;
    struct pci_dev      *dev;
    int                 waitFlag;
    wait_queue_head_t   waitDMA;*/
    int                 minor;/*I think we need major and minor*/
    int                 major;
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
} llrfDummyData;


#ifdef __DEBUG_MODE__
#define dbg_print(format, ...)  do {\
                                    printk("%s: [%s] " format, LLRFDUMMY_DBG_MSG_DEV_NAME, __FUNCTION__,__VA_ARGS__); \
                                    } while (0);
#else
#define dbg_print(...)
#endif


#endif /* LLRF_DUMMY_DRV_H */
