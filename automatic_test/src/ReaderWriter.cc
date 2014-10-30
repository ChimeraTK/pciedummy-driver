#include "ReaderWriter.h"
#include <unistd.h>
#include <fcntl.h>
#include <iostream>

ReaderWriter::ReaderWriter(std::string const & deviceFileName){
  _fileDescriptor = open(deviceFileName.c_str(), O_RDWR);

  if (_fileDescriptor <= 0)
  {
    std::cerr << "Could not open file " <<deviceFileName
	      << ". Check that the udev rules are installed and the "
	      << "kernel module is loaded!" << std::endl;
    throw DeviceIOException();
  }
}

ReaderWriter::~ReaderWriter(){
  close(_fileDescriptor);
}
