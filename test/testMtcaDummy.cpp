#include <cstdio>
#include <iostream> 
#include <unistd.h>
#include <fcntl.h>
#include <exception> 

//FIXME: the io file with the struct should not have a hard coded path.
#include "../pciedev_io.h"

//FIXME: The register information should come from the mapping file, not from the kernel module's source
// (the generic driver does not have this info, only the dummy has it)
#include "../mtcadummy_firmware.h"

#define DEVICE_NAME "/dev/mtcadummys0"

// a class to throw which basically is a std::exception, 
// but as it's derived it allows selective catching
class DeviceIOException: public std::exception
{
};

void pause(std::string message)
{
  std::cout << message <<std::endl;
  std::cout << "Press Enter to continue..."; getchar();
  std::cout << std::endl;
}

int main()
{
  // open the device
  int fileDescriptor = open(DEVICE_NAME, O_RDWR);

  if (fileDescriptor < 0)
  {
    std::cout << "Could not open file " <<DEVICE_NAME 
	      << ". Check that the udev rules are installed and the "
	      << "kernel module is loaded!" << std::endl;
    return 1;
  }

  // Wait for the user to press enter before continuing.
  // This allows to read the /proc/mtcadummy in the meantime.
  pause(std::string("Device file is opened. check /proc/mtcadummy for the status")+
	" of the bar contents." );

  // Turn on the daq. The "firmware" should reset the CLK_RST register to 0,
  // write the number of time samples to CLK_CNT and set CLF_CNT samples 
  // in the dma bar to index^2
  device_rw readWriteInstruction;
  readWriteInstruction.offset_rw = MTCADUMMY_WORD_ADC_ENA;
  readWriteInstruction.data_rw = 0x1;
  readWriteInstruction.mode_rw = RW_D32;
  readWriteInstruction.barx_rw = 0;
  readWriteInstruction.size_rw = sizeof(device_rw);
  readWriteInstruction.rsrvd_rw  = 0;

  try{ 
    if ( write (fileDescriptor, &readWriteInstruction, sizeof(device_rw)) != sizeof(device_rw) )
      {     
	std::cout << "Error writing to device" << std::endl;
	throw DeviceIOException();
      }

    pause("started DAQ.");

    // reset the clock counter. Except for the register to write the write instructions stay the same
    readWriteInstruction.offset_rw = MTCADUMMY_WORD_CLK_RST;
    if ( write (fileDescriptor, &readWriteInstruction, sizeof(device_rw)) != sizeof(device_rw) )
    {     
      std::cout << "Error writing to device" << std::endl;
      throw DeviceIOException();
    }
    
    pause("reset the clock counter.");

    // turn off the daq. This should just set the corresponding word to 0
    readWriteInstruction.offset_rw = MTCADUMMY_WORD_ADC_ENA;
    readWriteInstruction.data_rw = 0x1;  
    if ( write (fileDescriptor, &readWriteInstruction, sizeof(device_rw)) != sizeof(device_rw) )
    {     
      std::cout << "Error writing to device" << std::endl;
      throw DeviceIOException();
    }
    pause("restarted the daq.");
  
    // turn off the daq. This should just set the corresponding word to 0
    readWriteInstruction.offset_rw = MTCADUMMY_WORD_ADC_ENA;
    readWriteInstruction.data_rw = 0x0;  
    if ( write (fileDescriptor, &readWriteInstruction, sizeof(device_rw)) != sizeof(device_rw) )
    {     
      std::cout << "Error writing to device" << std::endl;
      throw DeviceIOException();
    }
    pause("Turned off the daq.");

  }catch( DeviceIOException & /*variable name not needed. a std::exception does not provide much info*/)
  {
    std::cout << "Quitting execution due to IO exception when working with the device" << std::endl;
  }
  

  // close the device
  close( fileDescriptor );

  return 0;
}
