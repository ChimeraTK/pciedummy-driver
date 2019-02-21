#include <sstream>

#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;
#include <boost/shared_ptr.hpp>

#include "NormalReaderWriter.h"
#include "ReaderWriter.h"
#include "StructReaderWriter.h"

// we use the defines from the original implementation
#include "mtcadummy.h"
#include "mtcadummy_firmware.h"

#define N_WORDS_DMA (MTCADUMMY_DMA_SIZE / sizeof(int32_t))

class MtcaDummyTest {
public:
  MtcaDummyTest(boost::shared_ptr<ReaderWriter> const &readerWriter);

  void testReadSingle();
  void testReadSingleWithError();
  void testWriteSingle();
  void testWriteSingleWithError();
  void testReadArea();
  void testWriteArea();

private:
  boost::shared_ptr<ReaderWriter> _readerWriter;
  const std::vector<int32_t>
      bufferA; ///< a buffer of the size of the dma bar with content A

  std::vector<int32_t>
      bufferHelix; ///< the default buffer after start of the "ADC"
};

template <class T> class MtcaDummyTestSuite : public test_suite {
public:
  MtcaDummyTestSuite(std::string const &deviceFileName)
      : test_suite("MtcaDummy test suite") {
    // create an instance of the test class
    boost::shared_ptr<ReaderWriter> readerWriter(new T(deviceFileName));

    boost::shared_ptr<MtcaDummyTest> mtcaDummyTest(
        new MtcaDummyTest(readerWriter));

    add(BOOST_CLASS_TEST_CASE(&MtcaDummyTest::testReadSingle, mtcaDummyTest));
    add(BOOST_CLASS_TEST_CASE(&MtcaDummyTest::testReadSingleWithError,
                              mtcaDummyTest));
    add(BOOST_CLASS_TEST_CASE(&MtcaDummyTest::testWriteSingle, mtcaDummyTest));
    add(BOOST_CLASS_TEST_CASE(&MtcaDummyTest::testWriteSingleWithError,
                              mtcaDummyTest));
    add(BOOST_CLASS_TEST_CASE(&MtcaDummyTest::testReadArea, mtcaDummyTest));
    add(BOOST_CLASS_TEST_CASE(&MtcaDummyTest::testWriteArea, mtcaDummyTest));
  }
};

test_suite *init_unit_test_suite(int /*argc*/, char * /*argv*/ []) {
  framework::master_test_suite().p_name.value = "MtcaDummy test suite";
  framework::master_test_suite().add(new MtcaDummyTestSuite<StructReaderWriter>(
      std::string("/dev/") + MTCADUMMY_NAME + "s0"));
  framework::master_test_suite().add(new MtcaDummyTestSuite<NormalReaderWriter>(
      std::string("/dev/") + PCIEUNIDUMMY_NAME + "s6"));

  return NULL;
}

MtcaDummyTest::MtcaDummyTest(
    boost::shared_ptr<ReaderWriter> const &readerWriter)
    : _readerWriter(readerWriter), bufferA(N_WORDS_DMA, 0xAAAAAAAA),
      bufferHelix(N_WORDS_DMA, 0) {
  for (unsigned int i = 0; i < 25; ++i) {
    bufferHelix[i] = i * i;
  }

  // initialise the driver. We do not check it here but in the tests. If it
  // does not work we will detect it then.
  _readerWriter->writeArea(0, 2, N_WORDS_DMA, &bufferHelix[0]);
}

void MtcaDummyTest::testReadSingle() {
  BOOST_CHECK(_readerWriter->readSingle(MTCADUMMY_WORD_DUMMY, 0) ==
              MTCADUMMY_DMMY_AS_ASCII);
  // beginning of bar 2 is a helix. Just pick one value to test
  BOOST_CHECK(_readerWriter->readSingle(8 * sizeof(int32_t), 2) == (8 * 8));
}

void MtcaDummyTest::testReadSingleWithError() {
  BOOST_CHECK_THROW(_readerWriter->readSingle(MTCADUMMY_BROKEN_REGISTER, 0),
                    DeviceIOException);
  BOOST_CHECK_NO_THROW(_readerWriter->readSingle(MTCADUMMY_BROKEN_WRITE, 0));
}

void MtcaDummyTest::testWriteSingle() {
  uint32_t wordUser = _readerWriter->readSingle(MTCADUMMY_WORD_USER, 0);
  _readerWriter->writeSingle(MTCADUMMY_WORD_USER, 0, ++wordUser);
  BOOST_CHECK(_readerWriter->readSingle(MTCADUMMY_WORD_USER, 0) == wordUser);

  uint32_t wordFromBar2 = _readerWriter->readSingle(0x20, 2);
  _readerWriter->writeSingle(0x20, 2, ++wordFromBar2);
  BOOST_CHECK(_readerWriter->readSingle(0x20, 2) == wordFromBar2);
}

void MtcaDummyTest::testWriteSingleWithError() {
  BOOST_CHECK_THROW(
      _readerWriter->writeSingle(MTCADUMMY_BROKEN_REGISTER, 0, 42),
      DeviceIOException);
  BOOST_CHECK_THROW(_readerWriter->writeSingle(MTCADUMMY_BROKEN_WRITE, 0, 42),
                    DeviceIOException);
}

void MtcaDummyTest::testReadArea() {
  // we just test in bar 2. As both readSingle and readArea use each other's
  // implementation, checking both bars in readSingle is enough
  // Note: with the struct, readArea uses readSingle, without struct it's the
  // other way

  // fill the buffer with something we do not expect
  std::vector<int32_t> readBuffer(N_WORDS_DMA, 0xB0B0B0B0);
  static const unsigned int startIndex = 13;
  static const unsigned int nWordsToRead = 5;

  _readerWriter->readArea(startIndex * sizeof(int32_t), 2, nWordsToRead,
                          &readBuffer[startIndex]);

  for (unsigned int i = 0; i < startIndex; ++i) {
    BOOST_CHECK(readBuffer[i] == 0xB0B0B0B0);
  }
  for (unsigned int i = startIndex; i < startIndex + nWordsToRead; ++i) {
    BOOST_CHECK(readBuffer[i] == i * i);
  }
  for (unsigned int i = startIndex + nWordsToRead; i < N_WORDS_DMA; ++i) {
    BOOST_CHECK(readBuffer[i] == 0xB0B0B0B0);
  }
}

void MtcaDummyTest::testWriteArea() {
  // start the "adc" to set bar 2 back to "helix"
  _readerWriter->writeSingle(MTCADUMMY_WORD_ADC_ENA, 0, 1);

  // prepare the expected buffer
  std::vector<int32_t> expectedBuffer = bufferHelix;
  // modify some words
  static const unsigned int startIndex = 100;
  static const unsigned int nWordsToRead = 8;
  for (unsigned int i = startIndex; i < startIndex + nWordsToRead; ++i) {
    expectedBuffer[i] = i * i;
  }

  // only write the modified part
  _readerWriter->writeArea(startIndex * sizeof(int32_t), 2, nWordsToRead,
                           &expectedBuffer[startIndex]);

  // now read back the whole buffer and compare
  std::vector<int32_t> readBuffer(N_WORDS_DMA);
  _readerWriter->readArea(0, 2, N_WORDS_DMA, &readBuffer[0]);

  BOOST_CHECK(readBuffer == expectedBuffer);
}
