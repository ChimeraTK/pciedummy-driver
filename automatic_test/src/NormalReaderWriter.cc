#include "NormalReaderWriter.h"

#include "pcieuni_io_compat.h"
#include <fcntl.h>
#include <unistd.h>

#include <cstdio>
#include <iostream>

// sorry, this is rather C-Style

NormalReaderWriter::NormalReaderWriter(std::string const &deviceFileName)
    : ReaderWriter(deviceFileName) {}

int32_t NormalReaderWriter::readSingle(uint32_t offset, uint32_t bar) {
  int32_t returnValue;
  readArea(offset, bar, 1, &returnValue);
  return returnValue;
}

void NormalReaderWriter::readArea(uint32_t offset, uint32_t bar,
                                  uint32_t nWords, int32_t *readBuffer) {
  if (bar > 5) {
    throw DeviceIOException("Bar number is too large.");
  }

  loff_t virtualOffset = PCIEUNI_BAR_OFFSETS[bar] + offset;

  if (pread(_fileDescriptor, readBuffer, nWords * sizeof(int32_t),
            virtualOffset) != nWords * sizeof(int32_t)) {
    throw DeviceIOException("Error reading from device");
  }
}

void NormalReaderWriter::writeSingle(uint32_t offset, uint32_t bar,
                                     int32_t value) {
  writeArea(offset, bar, 1, &value);
}
void NormalReaderWriter::writeArea(uint32_t offset, uint32_t bar,
                                   uint32_t nWords,
                                   int32_t const *writeBuffer) {
  if (bar > 5) {
    throw DeviceIOException("Bar number is too large.");
  }

  loff_t virtualOffset = PCIEUNI_BAR_OFFSETS[bar] + offset;

  int writeStatus = pwrite(_fileDescriptor, writeBuffer,
                           nWords * sizeof(int32_t), virtualOffset);
  //  if ( pwrite(_fileDescriptor, writeBuffer, nWords*sizeof(int32_t),
  //  virtualOffset)
  //       != nWords*sizeof(int32_t) ){
  if (writeStatus != nWords * sizeof(int32_t)) {
    throw DeviceIOException("Error writing to device");
  }
}
