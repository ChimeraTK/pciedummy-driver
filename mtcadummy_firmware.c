#include "version.h"
#include "mtcadummy_firmware.h"

#define REG(mem, addr) *((mem) + (addr) / sizeof(u32))

void mtcadummy_initialiseSystemBar(u32* systemBarBaseAddress) {
  REG(systemBarBaseAddress, MTCADUMMY_WORD_FIRMWARE) = MTCADUMMY_DRV_VERSION_MAJ;
  REG(systemBarBaseAddress, MTCADUMMY_WORD_COMPILATION) = MTCADUMMY_DRV_VERSION_MIN;
  REG(systemBarBaseAddress, MTCADUMMY_WORD_STATUS) = 0; /*is this the value for ok? */
  REG(systemBarBaseAddress, MTCADUMMY_WORD_USER) = 0;   /*let the user do this*/
  REG(systemBarBaseAddress, MTCADUMMY_WORD_DUMMY) = MTCADUMMY_DMMY_AS_ASCII;
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
  REG(systemBarBaseAddress, MTCADUMMY_WORD_CLK_RST) = 1;
  /*
   REG(systemBarBaseAddress, MTCADUMMY_WORD_ADC_ENA) =
   REG(systemBarBaseAddress, MTCADUMMY_BROKEN_REGISTER) =
   */

  /* you can read back the address, but cannot write. */
  REG(systemBarBaseAddress, MTCADUMMY_BROKEN_WRITE) = MTCADUMMY_BROKEN_WRITE;
}


/* do something when a register has been written */
int mtcadummy_performActionOnWrite(u32 offset, unsigned int barNumber, unsigned int slotNumber) {
  u32* systemBarBaseAddress;
  u32* dmaBarBaseAddress;
  bool spiFail;

  if(barNumber != 0) {
    /*Currently only some actions are foreseen when writing to the system bar.
      Just retrun otherwise */
    return 0;
  }

  if (mutex_lock_interruptible(&controlMutex) != 0) {
    dbg_print("%s", "Failed to acquire control mutex while opening file...");
    return -ERESTARTSYS;
  }

  spiFail = controlData.spi_error;
  mutex_unlock(&controlMutex);

  systemBarBaseAddress = dummyPrivateData[slotNumber].systemBar;
  dmaBarBaseAddress = dummyPrivateData[slotNumber].dmaBar;

  switch(offset) {
    case MTCADUMMY_WORD_CLK_RST:
      if(REG(systemBarBaseAddress, MTCADUMMY_WORD_CLK_RST)) /* test for the value of clk_rst */
      {
        /* reset all clock counter and clock mux values to 0 */
        REG(systemBarBaseAddress, MTCADUMMY_WORD_CLK_CNT_0) = 0;
        REG(systemBarBaseAddress, MTCADUMMY_WORD_CLK_CNT_1) = 0;
        REG(systemBarBaseAddress, MTCADUMMY_WORD_CLK_MUX_0) = 0;
        REG(systemBarBaseAddress, MTCADUMMY_WORD_CLK_MUX_1) = 0;
        REG(systemBarBaseAddress, MTCADUMMY_WORD_CLK_MUX_2) = 0;
        REG(systemBarBaseAddress, MTCADUMMY_WORD_CLK_MUX_3) = 0;
      }
      break;
    case MTCADUMMY_WORD_ADC_ENA:
      if(REG(systemBarBaseAddress, MTCADUMMY_WORD_ADC_ENA)) {
        unsigned int sample;
        /* perform a dummy sampling */
        unsigned int nSamples = 25;

        for(sample = 0; sample < nSamples; ++sample) {
          dmaBarBaseAddress[sample] = sample * sample; /* just a parabola... */
        }

        /* set the clock count and reset the "clock reset" register */
        REG(systemBarBaseAddress, MTCADUMMY_WORD_CLK_CNT_0) = nSamples;
        REG(systemBarBaseAddress, MTCADUMMY_WORD_CLK_RST) = 0;
      }
      break;
      // set back the firmware word. This is read only
    case MTCADUMMY_WORD_FIRMWARE:
      REG(systemBarBaseAddress, MTCADUMMY_WORD_FIRMWARE) = MTCADUMMY_DRV_VERSION_MAJ;
      break;
      // set back the compilation word. This is read only
    case MTCADUMMY_WORD_COMPILATION:
      REG(systemBarBaseAddress, MTCADUMMY_WORD_COMPILATION) = MTCADUMMY_DRV_VERSION_MIN;
      break;
    case MTCADUMMY_WORD_DUMMY:
      REG(systemBarBaseAddress, MTCADUMMY_WORD_DUMMY) = MTCADUMMY_DMMY_AS_ASCII;
      break;
    case MTCADUMMY_BROKEN_REGISTER:
    case MTCADUMMY_BROKEN_WRITE:
      return -1;
    case MTCADUMMY_WORD_SPI_WRITE:
      if (REG(systemBarBaseAddress, MTCADUMMY_WORD_SPI_SYNC) == MTCADUMMY_SPI_SYNC_REQUESTED && !spiFail) {
        REG(systemBarBaseAddress, MTCADUMMY_WORD_SPI_READ) = REG(systemBarBaseAddress, MTCADUMMY_WORD_SPI_WRITE);
        REG(systemBarBaseAddress, MTCADUMMY_WORD_SPI_SYNC) = MTCADUMMY_SPI_SYNC_OK;
      } else {
        REG(systemBarBaseAddress, MTCADUMMY_WORD_SPI_SYNC) = MTCADUMMY_SPI_SYNC_ERROR;
      }
      break;
    default:
      /* default: do nothing */
      break;
  }
  return 0;
}

int mtcadummy_performPreReadAction(u32 offset, unsigned int barNumber, unsigned int slotNumber) {
  if(barNumber != 0) {
    /*Currently only some actions are foreseen when writing to the system bar.
      Just retrun otherwise */
    return 0;
  }

  switch(offset) {
    case MTCADUMMY_BROKEN_REGISTER:
      return -1;
      /* default: do nothing */
  }
  return 0;
}
