#include "NormalReaderWriter.h"

#include <unistd.h>
#include <fcntl.h>
#include "pciedev_io.h"

#include <iostream>
#include <cstdio>

// sorry, this is rather C-Style

NormalReaderWriter::NormalReaderWriter(std::string const & deviceFileName)
  : ReaderWriter(deviceFileName){
}

/*
// the real implementation without struct 
int32_t NormalReaderWriter::readSingle(uint32_t offset, uint32_t bar){
  int32_t returnValue;
  readArea( offset, bar, 1, &returnValue );
  return returnValue;
}
*/

// use the "struct" implementation until it is implemented in the driver
int32_t NormalReaderWriter::readSingle(uint32_t offset, uint32_t bar){
  device_rw readInstruction;
  readInstruction.offset_rw = offset;
  readInstruction.data_rw = 0;
  readInstruction.mode_rw = RW_D32;
  readInstruction.barx_rw = bar;
  readInstruction.size_rw = 0;
  readInstruction.rsrvd_rw  = 0;

  if ( read(_fileDescriptor, &readInstruction, sizeof(device_rw)) 
       != sizeof(device_rw) )
    {     
      throw DeviceIOException("Error reading from device");
    }

  return readInstruction.data_rw;
}

/* draft for the real implementation 
void NormalReaderWriter::readArea(uint32_t offset, uint32_t bar, uint32_t nWords,
				  int32_t * readBuffer){
  // fixme: replace the impl. hack with the correct constants from the header
  loff_t virtualOffset = (static_cast<loff_t>(bar & 0x7) << 60) + offset ;
  std::cout << std::hex << "virtual Offset 0x" << virtualOffset << std::endl;

  if ( read(_fileDescriptor, readBuffer, nWords*sizeof(int32_t), virtualOffset) 
       != nWords*sizeof(int32_t) ){
      throw DeviceIOException("Error reading from device");
  }
}
*/

// use the struct implementation until it is implemented in the driver
void NormalReaderWriter::readArea(uint32_t offset, uint32_t bar, uint32_t nWords,
	      int32_t * readBuffer){
  for (uint32_t i = 0; i < nWords; ++i){
    readBuffer[i] = readSingle(offset +i*sizeof(int32_t), bar);
  }
}

void NormalReaderWriter::writeSingle(uint32_t offset, uint32_t bar, int32_t value){
  writeArea( offset, bar, 1, &value );
}
void NormalReaderWriter::writeArea(uint32_t offset, uint32_t bar, uint32_t nWords,
	       int32_t const * writeBuffer){
  // fixme: replace the impl. hack with the correct constants from the header
  loff_t virtualOffset = (static_cast<loff_t>(bar & 0x7) << 60) + offset ;

  int writeStatus = pwrite(_fileDescriptor, writeBuffer, nWords*sizeof(int32_t),
			   virtualOffset );
//  if ( pwrite(_fileDescriptor, writeBuffer, nWords*sizeof(int32_t), virtualOffset) 
//       != nWords*sizeof(int32_t) ){
  if ( writeStatus != nWords*sizeof(int32_t) ){
    throw DeviceIOException("Error writing to device");
  }
}

