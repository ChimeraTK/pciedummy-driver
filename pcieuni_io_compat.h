#ifndef _PCIEUNI_IO_COMPAT_H
#define _PCIEUNI_IO_COMPAT_H

/** Information about the start addresses of the bars in user space.
 *  @fixme Should this be a fixed size array so addressing with the bar number is 
 *  easier? What about a range check to avoid buffer overruns in that case?
 */
typedef struct _pcieuni_bar_start{
  loff_t addressBar0; /** Start address of bar 0*/
  loff_t addressBar1; /** Start address of bar 1*/
  loff_t addressBar2; /** Start address of bar 2*/
  loff_t addressBar3; /** Start address of bar 3*/
  loff_t addressBar4; /** Start address of bar 4*/
  loff_t addressBar5; /** Start address of bar 5*/
} pcieuni_bar_start;

/** Information about the bar sizes. It is retrieved via IOCTL.
 */
typedef struct _pcieuni_ioctl_bar_sizes{
  size_t sizeBar0; /** Size of bar 0*/
  loff_t sizeBar1; /** Size of bar 1*/
  loff_t sizeBar2; /** Size of bar 2*/
  loff_t sizeBar3; /** Size of bar 3*/
  loff_t sizeBar4; /** Size of bar 4*/
  loff_t sizeBar5; /** Size of bar 5*/
} pcieuni_ioctl_bar_sizes;

/* Use 'U' like pcieUni as magic number */
#define PCIEUNI_IOC                               'U'
/* relative to the new IOC we keep the same ioctls */
#define PCIEUNI_PHYSICAL_SLOT           _IOWR(PCIEUNI_IOC, 60, int)
#define PCIEUNI_DRIVER_VERSION          _IOWR(PCIEUNI_IOC, 61, int)
#define PCIEUNI_FIRMWARE_VERSION        _IOWR(PCIEUNI_IOC, 62, int)
#define PCIEUNI_GET_DMA_TIME            _IOWR(PCIEUNI_IOC, 70, int)
#define PCIEUNI_WRITE_DMA               _IOWR(PCIEUNI_IOC, 71, int)
#define PCIEUNI_READ_DMA                _IOWR(PCIEUNI_IOC, 72, int)
#define PCIEUNI_SET_IRQ                 _IOWR(PCIEUNI_IOC, 73, int)
#define PCIEUNI_IOC_MINNR  60
#define PCIEUNI_IOC_MAXNR 63
#define PCIEUNI_IOC_DMA_MINNR  70
#define PCIEUNI_IOC_DMA_MAXNR  74

#endif /* _PCIEUNI_IO_COMPAT_H */
