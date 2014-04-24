#include <sstream>

#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#define INITIAL_VALUE 4711

class ExampleException{
};

class ExampleClass{
public:
  ExampleClass(bool someCondition=true) : _a(INITIAL_VALUE) {
    if (someCondition==false){throw ExampleException();}
  }
  int read(){return _a;}
  void write(int a){_a=a;}

private:
  int _a;
};

/** The unit tests for the class under test (A in this case), based on the 
 *  boost unit test library. We use a class which holds a private 
 *  instance of A, which avoids code duplication.
 */
class ExampleClassTest
{
 public:
  /** The constructor test is static. It does not use the internal variable.
   *  Needed to check if the constructor does thow (if it can throw).
   *  There might be other cases for static tests.
   */
  static void testConstructor();

  void testRead();
  void testWrite();
  void testSomethingElse();// could also be static, but is not for demonstation

 private:
  ExampleClass _exampleClass;
};

class ExampleClassTestSuite : public test_suite {
public:
  ExampleClassTestSuite(): test_suite("ExampleClass test suite") {
    // create an instance of the test class
    boost::shared_ptr<ExampleClassTest> exampleClassTest( new ExampleClassTest );

    // add the tests
    // static tests are added with BOOST_TEST_CASE
    add( BOOST_TEST_CASE( &ExampleClassTest::testConstructor ) );
    // tests of non-static functions are added using BOOST_CLASS_TEST_CASE, which needs a (shared) pointer to the instance
    add( BOOST_CLASS_TEST_CASE( &ExampleClassTest::testSomethingElse, exampleClassTest ) );

    // in case of dependencies store the test cases before adding them and declare the dependency
    test_case* writeTestCase = BOOST_CLASS_TEST_CASE( &ExampleClassTest::testWrite, exampleClassTest );
    test_case* readTestCase = BOOST_CLASS_TEST_CASE( &ExampleClassTest::testRead, exampleClassTest );

    // the write test uses read, so we first have to check that reading works before we know that writing also does
    writeTestCase->depends_on(readTestCase);

    add( readTestCase );
    add( writeTestCase );
  }
};

test_suite*
init_unit_test_suite( int /*argc*/, char* /*argv*/ [] )
{
  framework::master_test_suite().p_name.value = "ExampleClass test suite";
  return new ExampleClassTestSuite;
}

void ExampleClassTest::testConstructor(){
  // in case an exception can be thrown, always check that this happens when it
  // has to, and that it does not happen when it should not happen.
  BOOST_CHECK_THROW( ExampleClass(false), ExampleException );
  BOOST_CHECK_NO_THROW( ExampleClass(true) );
}

void ExampleClassTest::testSomethingElse(){
  int value = 12 / 4;
  BOOST_CHECK( value  == 3 );
}

void ExampleClassTest::testRead(){
  // check for value as set by the constructor
  BOOST_CHECK( _exampleClass.read() == INITIAL_VALUE );
}

void ExampleClassTest::testWrite(){
  // typical three step test
  // 1) prepare
  //    here we create a value which we know is different from the current 
  //    content to see if writing really changed something
  int value =  _exampleClass.read() + 1;

  // 2) execute
  _exampleClass.write( value );

  // 3) check
  // we use read to check that the value was written. We checked before that read acutally works.
  BOOST_CHECK( _exampleClass.read() == value); 
}
