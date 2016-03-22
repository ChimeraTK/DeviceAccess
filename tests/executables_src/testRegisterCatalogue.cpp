#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "RegisterCatalogue.h"

using namespace mtca4u;

class RegisterCatalogueTest {
  public:
    void testRegisterCatalogue();
};

class registerCatalogueTestSuite : public test_suite {
  public:
    registerCatalogueTestSuite(): test_suite("RegisterCatalogue class test suite"){
      boost::shared_ptr<RegisterCatalogueTest> test(new RegisterCatalogueTest());

      add( BOOST_CLASS_TEST_CASE(&RegisterCatalogueTest::testRegisterCatalogue, test) );
    }
};

test_suite* init_unit_test_suite( int /*argc*/, char* /*argv*/ [] )
{
  framework::master_test_suite().p_name.value = "RegisterCatalogue class test suite";
  framework::master_test_suite().add(new registerCatalogueTestSuite());

  return NULL;
}

class myRegisterInfo : public RegisterInfo {
  public:
    myRegisterInfo(std::string path, unsigned int nbOfElements, unsigned int nbOfChannels, unsigned int nbOfDimensions)
    : _path(path), _nbOfElements(nbOfElements),_nbOfChannels(nbOfChannels), _nbOfDimensions(nbOfDimensions) {}
    virtual RegisterPath getRegisterName() const { return _path; }
    virtual unsigned int getNumberOfElements() const { return _nbOfElements; }
    virtual unsigned int getNumberOfChannels() const { return _nbOfChannels; }
    virtual unsigned int getNumberOfDimensions() const { return _nbOfDimensions; }
  protected:
    RegisterPath _path;
    unsigned int _nbOfElements, _nbOfChannels, _nbOfDimensions;
};

void RegisterCatalogueTest::testRegisterCatalogue() {
  RegisterCatalogue catalogue;
  boost::shared_ptr<RegisterInfo> info;
  catalogue.addRegister( boost::shared_ptr<RegisterInfo>(new myRegisterInfo("/some/register/name",42, 3, 2)) );
  info = catalogue.getRegister("/some/register/name");
  BOOST_CHECK( info->getRegisterName() == "/some/register/name" );
  BOOST_CHECK( info->getNumberOfElements() == 42 );
  BOOST_CHECK( info->getNumberOfChannels() == 3 );
  BOOST_CHECK( info->getNumberOfDimensions() == 2 );
  
}
