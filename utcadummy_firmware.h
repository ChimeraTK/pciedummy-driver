/* Implement some "firmware", i.e. assign meaning to some registers.
 *
 * Information taken from the flash_utca_demo_sis8300.map file in libmap

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

*/

#ifndef UTCADUMMY_FIRMWARE_H
#define UTCADUMMY_FIRMWARE_H

/* 
 * Put an extern "C" declaration when compiling with C++. Like this the structs can be used from the included
 * header files. Having this declatation in the header saves extern "C" declaration in all C++ files using
 * this header (avoid code duplication and frogetting the declaration).
 */
#ifdef __cplusplus
extern "C" {
#endif

#define UTCADUMMY_WORD_FIRMWARE       0x00000000
#define UTCADUMMY_WORD_COMPILATION    0x00000004
#define UTCADUMMY_WORD_STATUS         0x00000008
#define UTCADUMMY_WORD_USER           0x0000000C
#define UTCADUMMY_WORD_CLK_CNT        0x00000010
#define UTCADUMMY_WORD_CLK_CNT_0      0x00000010
#define UTCADUMMY_WORD_CLK_CNT_1      0x00000014
#define UTCADUMMY_WORD_CLK_MUX        0x00000020
#define UTCADUMMY_WORD_CLK_MUX_0      0x00000020
#define UTCADUMMY_WORD_CLK_MUX_1      0x00000024
#define UTCADUMMY_WORD_CLK_MUX_2      0x00000028
#define UTCADUMMY_WORD_CLK_MUX_3      0x0000002C
#define UTCADUMMY_WORD_CLK_RST        0x00000040
#define UTCADUMMY_WORD_ADC_ENA        0x00000044

#ifdef __KERNEL__
#include "utcadummy.h"

/* the driver functions are only usefull in the kernel module */
void utcadummy_initialiseSystemBar(u32 * barStartAddress);

/* do something when a register has been written */
void utcadummy_performActionOnWrite( u32 offset, unsigned int barNumber );
#endif /* __KERNEL__ */

#ifdef __cplusplus
} /* closing the extern "C" { */
#endif

#endif /*UTCADUMMY_FIRMWARE_H*/
