#ifndef READER_WRITER_H
#define READER_WRITER_H

#include <exception>
#include <stdint.h>
#include <string>

/** a class to throw which basically is a std::exception,
    but as it's derived it allows selective catching,
    plus it gets the what() message in the constructor

*/
class DeviceIOException : public std::exception {
 protected:
  std::string _message; /**< exception description*/
 public:
  DeviceIOException(const std::string& message) : _message(message) {}
  const char* what() const noexcept override { return _message.c_str(); }
  ~DeviceIOException() noexcept override {}
};

/** Just a class for testing.
    The actual read/write functions are purely virtual so a
    version with and without struct can be implemented.
*/
class ReaderWriter {
 public:
  ReaderWriter(std::string const& deviceFileName);
  virtual ~ReaderWriter();

  /// The actual read implementation with struct
  virtual int32_t readSingle(uint32_t offset, uint32_t bar) = 0;
  /// A loop around readSingle
  virtual void readArea(uint32_t offset, uint32_t bar, uint32_t nWords, int32_t* readBuffer) = 0;

  /// The actual write implementation with struct
  virtual void writeSingle(uint32_t offset, uint32_t bar, int32_t value) = 0;
  /// A loop around writeSingle
  virtual void writeArea(uint32_t offset, uint32_t bar, uint32_t nWords, int32_t const* writeBuffer) = 0;

 protected:
  int _fileDescriptor;
};

#endif // READER_WRITER_H
