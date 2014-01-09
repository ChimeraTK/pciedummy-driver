#include <cstdio>
#include <iostream> 
#include <fcntl.h>

#define DEVICE_NAME "/dev/utcadummys0"

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

  // Wait for the user to press enter before closing the device.
  // This allows to read the /proc/utcadummy in the meantime.
  std::cout << "Press Enter to continue..."; getchar();

  // close the device
  close( fileDescriptor );

  return 0;
}
