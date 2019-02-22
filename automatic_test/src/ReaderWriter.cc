#include "ReaderWriter.h"
#include <fcntl.h>
#include <sstream>
#include <unistd.h>

ReaderWriter::ReaderWriter(std::string const& deviceFileName) {
  _fileDescriptor = open(deviceFileName.c_str(), O_RDWR);

  if(_fileDescriptor <= 0) {
    std::stringstream errorMessage;
    errorMessage << "Could not open file " << deviceFileName << ". Check that the udev rules are installed and the "
                 << "kernel module is loaded!";
    throw DeviceIOException(errorMessage.str());
  }
}

ReaderWriter::~ReaderWriter() {
  close(_fileDescriptor);
}
