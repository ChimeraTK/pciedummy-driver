#include <sstream>

#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;
#include <boost/shared_ptr.hpp>

#include "ReaderWriter.h"
#include "StructReaderWriter.h"

// we use the defines from the original implementation
#include "mtcadummy.h"
#include "mtcadummy_firmware.h"

#define N_WORDS_DMA (MTCADUMMY_DMA_SIZE/sizeof(int32_t))

class MtcaDummyTest
{
 public:
  MtcaDummyTest( boost::shared_ptr<ReaderWriter> const & readerWriter );

  void testReadSingle();
  void testReadSingleWithError();
  void testWriteSingle();
  void testWriteSingleWithError();

 private:
  boost::shared_ptr<ReaderWriter> _readerWriter;
  const std::vector<int32_t> bufferA; ///< a buffer of the size of the dma bar with content A

  std::vector<int32_t> bufferHelix; ///< the default buffer after start of the "ADC"
};

template<class T>
class MtcaDummyTestSuite : public test_suite {
public:
  MtcaDummyTestSuite(std::string const & deviceFileName): test_suite("MtcaDummy test suite") {
    // create an instance of the test class
    boost::shared_ptr<ReaderWriter> readerWriter( new T(deviceFileName) );

    boost::shared_ptr<MtcaDummyTest> mtcaDummyTest( new MtcaDummyTest(readerWriter) );

    add( BOOST_CLASS_TEST_CASE( &MtcaDummyTest::testReadSingle, mtcaDummyTest ) );
    add( BOOST_CLASS_TEST_CASE( &MtcaDummyTest::testReadSingleWithError, mtcaDummyTest ) );
    add( BOOST_CLASS_TEST_CASE( &MtcaDummyTest::testWriteSingle, mtcaDummyTest ) );
    add( BOOST_CLASS_TEST_CASE( &MtcaDummyTest::testWriteSingleWithError, mtcaDummyTest ) );
  }
};

test_suite*
init_unit_test_suite( int /*argc*/, char* /*argv*/ [] )
{
  framework::master_test_suite().p_name.value = "MtcaDummy test suite";
  framework::master_test_suite().add( 
    new MtcaDummyTestSuite<StructReaderWriter>(std::string("/dev/")+MTCADUMMY_NAME+"s0") );

  return NULL;
}

MtcaDummyTest::MtcaDummyTest( boost::shared_ptr<ReaderWriter> const & readerWriter)
  : _readerWriter( readerWriter ), bufferA(N_WORDS_DMA, 0xAAAAAAAA),
    bufferHelix(N_WORDS_DMA, 0)
{
  for (unsigned int i = 0; i < 25 ; ++i){
    bufferHelix[i]=i*i;
  }

  // initialise the driver. We do not check it here but in the tests. If it
  // does not work we will detect it then.
  _readerWriter->writeArea(0, 2, N_WORDS_DMA, &bufferHelix[0]);
}

void MtcaDummyTest::testReadSingle(){
  BOOST_CHECK( _readerWriter->readSingle(MTCADUMMY_WORD_DUMMY, 0) == MTCADUMMY_DMMY_AS_ASCII );
  // all bar 2 has to be A, just pick one
  BOOST_CHECK( _readerWriter->readSingle(5*sizeof(int32_t), 2) == 5*5);
}

void MtcaDummyTest::testReadSingleWithError(){
  std::cerr << "The folloging read error is expected:  ";
  BOOST_CHECK_THROW( _readerWriter->readSingle(MTCADUMMY_BROKEN_REGISTER, 0),
		     DeviceIOException );
  BOOST_CHECK_NO_THROW( _readerWriter->readSingle(MTCADUMMY_BROKEN_WRITE, 0) );
}

void MtcaDummyTest::testWriteSingle(){
  uint32_t wordUser =   _readerWriter->readSingle( MTCADUMMY_WORD_USER, 0 );
  _readerWriter->writeSingle( MTCADUMMY_WORD_USER, 0, ++wordUser);
  BOOST_CHECK( _readerWriter->readSingle(MTCADUMMY_WORD_USER, 0) == wordUser );

  uint32_t wordFromBar2 =   _readerWriter->readSingle( 0x20, 2 );
  _readerWriter->writeSingle( 0x20, 0, ++wordFromBar2);
  BOOST_CHECK( _readerWriter->readSingle( 0x20, 0) == wordFromBar2 );
}

void MtcaDummyTest::testWriteSingleWithError(){
  std::cerr << "The folloging read error is expected:  ";
  BOOST_CHECK_THROW( _readerWriter->writeSingle(MTCADUMMY_BROKEN_REGISTER, 0, 42),
		     DeviceIOException );
  std::cerr << "The folloging read error is expected:  ";
  BOOST_CHECK_THROW( _readerWriter->writeSingle(MTCADUMMY_BROKEN_WRITE, 0, 42),
		     DeviceIOException );
}

