/* Implement some "firmware", i.e. assign meaning to some registers.
 *
 * Information taken from the flash_mtca_demo_sis8300.map file in libmap

#custom name                      size (words)  offset in bar size (bytes)  bar (only took 0 here)
WORD_FIRMWARE                     0x00000001    0x00000000    0x00000004    0x00000000
WORD_COMPILATION                  0x00000001    0x00000004    0x00000004    0x00000000
WORD_STATUS                       0x00000001    0x00000008    0x00000004    0x00000000
WORD_USER                         0x00000001    0x0000000C    0x00000004    0x00000000
WORD_CLK_CNT                      0x00000002    0x00000010    0x00000008    0x00000000
WORD_CLK_CNT_0                    0x00000001    0x00000010    0x00000004    0x00000000
WORD_CLK_CNT_1                    0x00000001    0x00000014    0x00000004    0x00000000
WORD_CLK_MUX                      0x00000004    0x00000020    0x00000010    0x00000000
WORD_CLK_MUX_0                    0x00000001    0x00000020    0x00000004    0x00000000
WORD_CLK_MUX_1                    0x00000001    0x00000024    0x00000004    0x00000000
WORD_CLK_MUX_2                    0x00000001    0x00000028    0x00000004    0x00000000
WORD_CLK_MUX_3                    0x00000001    0x0000002C    0x00000004    0x00000000
WORD_CLK_RST                      0x00000001    0x00000040    0x00000004    0x00000000
WORD_ADC_ENA                      0x00000001    0x00000044    0x00000004    0x00000000
#The broken register cannot be read or written, it always causes an I/O error for testing
BROKEN_REGISTER			  0x00000001    0x00000048    0x00000004    0x00000000
#Reading is possible, but writing causes an I/O error. Content is the offset.
BROKEN_WRITE			  0x00000001    0x0000004C    0x00000004    0x00000000

*/

#ifndef MTCADUMMY_FIRMWARE_H
#define MTCADUMMY_FIRMWARE_H

/* 
 * Put an extern "C" declaration when compiling with C++. Like this the structs can be used from the included
 * header files. Having this declatation in the header saves extern "C" declaration in all C++ files using
 * this header (avoid code duplication and frogetting the declaration).
 */
#ifdef __cplusplus
extern "C" {
#endif

#define MTCADUMMY_WORD_FIRMWARE       0x00000000
#define MTCADUMMY_WORD_COMPILATION    0x00000004
#define MTCADUMMY_WORD_STATUS         0x00000008
#define MTCADUMMY_WORD_USER           0x0000000C
#define MTCADUMMY_WORD_CLK_CNT        0x00000010
#define MTCADUMMY_WORD_CLK_CNT_0      0x00000010
#define MTCADUMMY_WORD_CLK_CNT_1      0x00000014
#define MTCADUMMY_WORD_CLK_MUX        0x00000020
#define MTCADUMMY_WORD_CLK_MUX_0      0x00000020
#define MTCADUMMY_WORD_CLK_MUX_1      0x00000024
#define MTCADUMMY_WORD_CLK_MUX_2      0x00000028
#define MTCADUMMY_WORD_CLK_MUX_3      0x0000002C
#define MTCADUMMY_WORD_DUMMY          0x0000003C
#define MTCADUMMY_WORD_CLK_RST        0x00000040
#define MTCADUMMY_WORD_ADC_ENA        0x00000044
#define MTCADUMMY_BROKEN_REGISTER     0x00000048
#define MTCADUMMY_BROKEN_WRITE        0x0000004C

#ifdef __KERNEL__
#include "mtcadummy.h"

/* the driver functions are only usefull in the kernel module */
void mtcadummy_initialiseSystemBar(u32 * barStartAddress);

/* do something when a register has been written. Returns -1 if the BROKEN_REGISTER
   is accessed to simulate I/O errors. Otherwise returns 0. */
int mtcadummy_performActionOnWrite( u32 offset, unsigned int barNumber,
				    unsigned int slotNumber );
/* do something before a register is written. This could be some action like 
   increasing a counter so not always the same value is returned.
   Currently only used to return -1 when accessing the BROKEN_REGISTER. */
int mtcadummy_performPreReadAction( u32 offset, unsigned int barNumber,
				    unsigned int slotNumber );
#endif /* __KERNEL__ */

#ifdef __cplusplus
} /* closing the extern "C" { */
#endif

#endif /*MTCADUMMY_FIRMWARE_H*/
