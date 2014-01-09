#include "utcadummy_firmware.h"

void utcadummy_initialiseSystemBar(u32 * systemBarBaseAddress)
{
  *(systemBarBaseAddress + UTCADUMMY_WORD_FIRMWARE   /sizeof(u32) ) = UTCADUMMY_DRV_VERSION_MAJ;
  *(systemBarBaseAddress + UTCADUMMY_WORD_COMPILATION/sizeof(u32) ) = UTCADUMMY_DRV_VERSION_MIN;
  *(systemBarBaseAddress + UTCADUMMY_WORD_STATUS     /sizeof(u32) ) = 0; /*is this the value for ok? */
  *(systemBarBaseAddress + UTCADUMMY_WORD_USER       /sizeof(u32) ) = 0; /*let the user do this*/ 
  /* ok, the rest will stay 0. As we zeroed everything during install we leave it like this
   *(systemBarBaseAddress + UTCADUMMY_WORD_CLK_CNT    /sizeof(u32) ) =  
   *(systemBarBaseAddress + UTCADUMMY_WORD_CLK_CNT_0  /sizeof(u32) ) =  
   *(systemBarBaseAddress + UTCADUMMY_WORD_CLK_CNT_1  /sizeof(u32) ) =  
   *(systemBarBaseAddress + UTCADUMMY_WORD_CLK_MUX    /sizeof(u32) ) =  
   *(systemBarBaseAddress + UTCADUMMY_WORD_CLK_MUX_0  /sizeof(u32) ) =  
   *(systemBarBaseAddress + UTCADUMMY_WORD_CLK_MUX_1  /sizeof(u32) ) =  
   *(systemBarBaseAddress + UTCADUMMY_WORD_CLK_MUX_2  /sizeof(u32) ) =  
   *(systemBarBaseAddress + UTCADUMMY_WORD_CLK_MUX_3  /sizeof(u32) ) =  
   */

  /* We set the clock reset bit to 1 to indicate that all counters have been zeroed */
  *(systemBarBaseAddress + UTCADUMMY_WORD_CLK_RST    /sizeof(u32)) = 1;
  /*
  systemBarBaseAddress + UTCADUMMY_WORD_ADC_ENA    /sizeof(u32) =  
  */
 
}

/* do something when a register has been written */
void  utcadummy_performActionOnWrite( u32 offset, unsigned int barNumber )
{
  if (barNumber != 0)
  {
    /*Currently only some actions are foreseen when writing to the system bar.
      Just retrun otherwise */
    return;
  }

  u32 * systemBarBaseAddress;
  u32 * dmaBarBaseAddress;

  systemBarBaseAddress = dummyPrivateData->systemBar;
  dmaBarBaseAddress = dummyPrivateData->dmaBar;

  switch (offset)
  {
    case UTCADUMMY_WORD_CLK_RST:
      if ( *(systemBarBaseAddress + UTCADUMMY_WORD_CLK_RST /sizeof(u32)) ) /* test for the value of clk_rst */
      {
	/* reset all clock counter and clock mux values to 0 */
	*(systemBarBaseAddress + UTCADUMMY_WORD_CLK_CNT_0  /sizeof(u32)) = 0;
	*(systemBarBaseAddress + UTCADUMMY_WORD_CLK_CNT_1  /sizeof(u32)) = 0; 
	*(systemBarBaseAddress + UTCADUMMY_WORD_CLK_MUX_0  /sizeof(u32)) = 0;  
	*(systemBarBaseAddress + UTCADUMMY_WORD_CLK_MUX_1  /sizeof(u32)) = 0;  
	*(systemBarBaseAddress + UTCADUMMY_WORD_CLK_MUX_2  /sizeof(u32)) = 0;  
	*(systemBarBaseAddress + UTCADUMMY_WORD_CLK_MUX_3  /sizeof(u32)) = 0;  
      }
      break;
    case UTCADUMMY_WORD_ADC_ENA:
      if ( *(systemBarBaseAddress + UTCADUMMY_WORD_ADC_ENA /sizeof(u32)) ) 
      {
	unsigned int sample;
	/* perform a dummy sampling */
	unsigned int nSamples = 25;
	
	for(sample = 0; sample < nSamples; ++sample)
	{
	  dmaBarBaseAddress[sample] = sample*sample; /* just a parabola... */
	}

	/* set the clock count and reset the "clock reset" register */
	*(systemBarBaseAddress + UTCADUMMY_WORD_CLK_CNT_0  /sizeof(u32)) = nSamples;
	*(systemBarBaseAddress + UTCADUMMY_WORD_CLK_RST /sizeof(u32)) = 0;
      }
      break;
      /* default: do nothing */
  }
}
