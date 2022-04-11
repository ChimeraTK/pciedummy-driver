#ifndef NORMAL_READER_WRITER_H
#define NORMAL_READER_WRITER_H

#include <exception>
#include <stdint.h>

#include "ReaderWriter.h"

/** Implementation of the ReaderWriter without struct.
 */
class NormalReaderWriter : public ReaderWriter {
 public:
  NormalReaderWriter(std::string const& deviceFileName);

  /// The actual read implementation with normal
  int32_t readSingle(uint32_t offset, uint32_t bar) override;
  /// A loop around readSingle
  void readArea(uint32_t offset, uint32_t bar, uint32_t nWords, int32_t* readBuffer) override;

  /// The actual write implementation without struct
  void writeSingle(uint32_t offset, uint32_t bar, int32_t value) override;
  /// A loop around writeSingle
  void writeArea(uint32_t offset, uint32_t bar, uint32_t nWords, int32_t const* writeBuffer) override;
};

#endif // NORMAL_READER_WRITER_H
