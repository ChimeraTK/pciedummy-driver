#include "mtcadummy_firmware.h"

void mtcadummy_initialiseSystemBar(u32 *systemBarBaseAddress) {
  *(systemBarBaseAddress + MTCADUMMY_WORD_FIRMWARE / sizeof(u32)) =
      MTCADUMMY_DRV_VERSION_MAJ;
  *(systemBarBaseAddress + MTCADUMMY_WORD_COMPILATION / sizeof(u32)) =
      MTCADUMMY_DRV_VERSION_MIN;
  *(systemBarBaseAddress + MTCADUMMY_WORD_STATUS / sizeof(u32)) =
      0; /*is this the value for ok? */
  *(systemBarBaseAddress + MTCADUMMY_WORD_USER / sizeof(u32)) =
      0; /*let the user do this*/
  *(systemBarBaseAddress + MTCADUMMY_WORD_DUMMY / sizeof(u32)) =
      MTCADUMMY_DMMY_AS_ASCII;
  /* ok, the rest will stay 0. As we zeroed everything during install we leave
   *it like this (systemBarBaseAddress + MTCADUMMY_WORD_CLK_CNT    /sizeof(u32)
   *) = (systemBarBaseAddress + MTCADUMMY_WORD_CLK_CNT_0  /sizeof(u32) ) =
   *(systemBarBaseAddress + MTCADUMMY_WORD_CLK_CNT_1  /sizeof(u32) ) =
   *(systemBarBaseAddress + MTCADUMMY_WORD_CLK_MUX    /sizeof(u32) ) =
   *(systemBarBaseAddress + MTCADUMMY_WORD_CLK_MUX_0  /sizeof(u32) ) =
   *(systemBarBaseAddress + MTCADUMMY_WORD_CLK_MUX_1  /sizeof(u32) ) =
   *(systemBarBaseAddress + MTCADUMMY_WORD_CLK_MUX_2  /sizeof(u32) ) =
   *(systemBarBaseAddress + MTCADUMMY_WORD_CLK_MUX_3  /sizeof(u32) ) =
   */

  /* We set the clock reset bit to 1 to indicate that all counters have been
   * zeroed */
  *(systemBarBaseAddress + MTCADUMMY_WORD_CLK_RST / sizeof(u32)) = 1;
  /*
   *(systemBarBaseAddress + MTCADUMMY_WORD_ADC_ENA    /sizeof(u32)) =
   *(systemBarBaseAddress +  MTCADUMMY_BROKEN_REGISTER  /sizeof(u32)) =
   */

  /* you can read back the address, but cannot write. */
  *(systemBarBaseAddress + MTCADUMMY_BROKEN_WRITE / sizeof(u32)) =
      MTCADUMMY_BROKEN_WRITE;
}

/* do something when a register has been written */
int mtcadummy_performActionOnWrite(u32 offset, unsigned int barNumber,
                                   unsigned int slotNumber) {
  u32 *systemBarBaseAddress;
  u32 *dmaBarBaseAddress;

  if (barNumber != 0) {
    /*Currently only some actions are foreseen when writing to the system bar.
      Just retrun otherwise */
    return 0;
  }

  systemBarBaseAddress = dummyPrivateData[slotNumber].systemBar;
  dmaBarBaseAddress = dummyPrivateData[slotNumber].dmaBar;

  switch (offset) {
  case MTCADUMMY_WORD_CLK_RST:
    if (*(systemBarBaseAddress +
          MTCADUMMY_WORD_CLK_RST /
              sizeof(u32))) /* test for the value of clk_rst */
    {
      /* reset all clock counter and clock mux values to 0 */
      *(systemBarBaseAddress + MTCADUMMY_WORD_CLK_CNT_0 / sizeof(u32)) = 0;
      *(systemBarBaseAddress + MTCADUMMY_WORD_CLK_CNT_1 / sizeof(u32)) = 0;
      *(systemBarBaseAddress + MTCADUMMY_WORD_CLK_MUX_0 / sizeof(u32)) = 0;
      *(systemBarBaseAddress + MTCADUMMY_WORD_CLK_MUX_1 / sizeof(u32)) = 0;
      *(systemBarBaseAddress + MTCADUMMY_WORD_CLK_MUX_2 / sizeof(u32)) = 0;
      *(systemBarBaseAddress + MTCADUMMY_WORD_CLK_MUX_3 / sizeof(u32)) = 0;
    }
    break;
  case MTCADUMMY_WORD_ADC_ENA:
    if (*(systemBarBaseAddress + MTCADUMMY_WORD_ADC_ENA / sizeof(u32))) {
      unsigned int sample;
      /* perform a dummy sampling */
      unsigned int nSamples = 25;

      for (sample = 0; sample < nSamples; ++sample) {
        dmaBarBaseAddress[sample] = sample * sample; /* just a parabola... */
      }

      /* set the clock count and reset the "clock reset" register */
      *(systemBarBaseAddress + MTCADUMMY_WORD_CLK_CNT_0 / sizeof(u32)) =
          nSamples;
      *(systemBarBaseAddress + MTCADUMMY_WORD_CLK_RST / sizeof(u32)) = 0;
    }
    break;
    // set back the firmware word. This is read only
  case MTCADUMMY_WORD_FIRMWARE:
    *(systemBarBaseAddress + MTCADUMMY_WORD_FIRMWARE / sizeof(u32)) =
        MTCADUMMY_DRV_VERSION_MAJ;
    break;
    // set back the compilation word. This is read only
  case MTCADUMMY_WORD_COMPILATION:
    *(systemBarBaseAddress + MTCADUMMY_WORD_COMPILATION / sizeof(u32)) =
        MTCADUMMY_DRV_VERSION_MIN;
    break;
  case MTCADUMMY_WORD_DUMMY:
    *(systemBarBaseAddress + MTCADUMMY_WORD_DUMMY / sizeof(u32)) =
        MTCADUMMY_DMMY_AS_ASCII;
    break;
  case MTCADUMMY_BROKEN_REGISTER:
  case MTCADUMMY_BROKEN_WRITE:
    return -1;
  case MTCADUMMY_WORD_SPI_WRITE:
    if (*(systemBarBaseAddress + MTCADUMMY_WORD_SPI_SYNC / sizeof(u32)) ==
        MTCADUMMY_SPI_SYNC_REQUESTED) {
      *(systemBarBaseAddress + MTCADUMMY_WORD_SPI_READ / sizeof(u32)) =
          *(systemBarBaseAddress + MTCADUMMY_WORD_SPI_WRITE / sizeof(u32));
      *(systemBarBaseAddress + MTCADUMMY_WORD_SPI_SYNC / sizeof(u32)) =
          MTCADUMMY_SPI_SYNC_OK;
    } else {
      *(systemBarBaseAddress + MTCADUMMY_WORD_SPI_SYNC / sizeof(u32)) =
          MTCADUMMY_SPI_SYNC_ERROR;
    }
    /* default: do nothing */
  }
  return 0;
}

int mtcadummy_performPreReadAction(u32 offset, unsigned int barNumber,
                                   unsigned int slotNumber) {
  if (barNumber != 0) {
    /*Currently only some actions are foreseen when writing to the system bar.
      Just retrun otherwise */
    return 0;
  }

  switch (offset) {
  case MTCADUMMY_BROKEN_REGISTER:
    return -1;
    /* default: do nothing */
  }
  return 0;
}
