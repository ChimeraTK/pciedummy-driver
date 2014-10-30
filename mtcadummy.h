/* A dummy driver which does not access the PCI bus but features the same
 * interface as the pciedev driver. All operations are performed in 
 * memory.
 * The idea is to have a device to test against with defined behaviour
 * without the need to install hardware.
 * This allows the implementation and execution of unit tests / automated tests
 * on any computer.
 *
 * There are six devices created: 4 mtcadummy drivers, compatible to pciedev (slots 0--3),
 * one llrfdummy device (slot 4) which is compatible with llrfuni, and pciedevdummy
 * (slot 5) which is compatible with pcieuni
 *
 * For debugging purposes there is a proc file which dumps the register content and the dma area 
 * in human readable form.
 */

#ifndef MTCA_DUMMY_DRV_H
#define MTCA_DUMMY_DRV_H

/* 
 * Put an extern "C" declaration when compiling with C++. Like this the structs can be used from the included
 * header files. Having this declatation in the header saves extern "C" declaration in all C++ files using
 * this header (avoid code duplication and forgetting the declaration).
 */
#ifdef __cplusplus
extern "C" {
#endif

/* Include the io definitions for the regular driver. The interface on the
 * user side should be the same.
 */
#include <linux/cdev.h>
#include <linux/module.h>
#include <asm/atomic.h> 

#include "pciedev_io.h"

#define MTCADUMMY_NR_DEVS       7 /*create 7 devices*/

  /* DO NOT FORGET TO ADAPT BOTH DRIVER AND MODULE VERSION! */
#define MTCADUMMY_DRV_VERSION_MAJ 0 /*dummy driver major version*/
#define MTCADUMMY_DRV_VERSION_MIN 9 /*dummy driver minor version*/
#define MTCADUMMY_MODULE_VERSION "0.9.0"

//#define MTCADUMMY_VENDOR_ID               0x10EE
//#define MTCADUMMY_DEVICE_ID               0x0038

#define MTCADUMMY_NAME                    "mtcadummy"
#define LLRFDUMMY_NAME                    "llrfdummy"
#define NOIOCTLDUMMY_NAME                 "noioctldummy"
#define PCIEUNIDUMMY_NAME                 "pcieunidummy"
#define MTCADUMMY_DBG_MSG_DEV_NAME        "MTCADUMMY"

#define MTCADUMMY_DMMY_AS_ASCII 0x444D4D59

#define __DEBUG_MODE__

/* some defines for the dummy test. We only provide two bars:
 * bar 0 consits of "registers", each 32 bit word being a register
 * bar 2 is a "dma" bar where arbitrary data is located There is no real DMA as there is no hardware.
 * it's just called that for consistency
 * Idea: bar 2 should be accessed by ioctl, bar 0 is accessed by read/write.
 */

#define MTCADUMMY_N_REGISTERS 32 /* 32 registers, just as a starting point */
#define MTCADUMMY_DMA_SIZE 4096  /* 4 kilo bytes (= 1k 32bit words) */

typedef struct _mtcaDummyData {    
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
} mtcaDummyData;

/* defined in the mtcadrummy_drv.c file, but needed in all of the .c files */
extern mtcaDummyData dummyPrivateData[MTCADUMMY_NR_DEVS];

#ifdef __DEBUG_MODE__
#define dbg_print(format, ...)  do {\
                                    printk("%s: [%s] " format, MTCADUMMY_DBG_MSG_DEV_NAME, __FUNCTION__,__VA_ARGS__); \
                                    } while (0);
#else
#define dbg_print(...)
#endif

#ifdef __cplusplus
} /* closing the extern "C" { */
#endif

#endif /* MTCA_DUMMY_DRV_H */
