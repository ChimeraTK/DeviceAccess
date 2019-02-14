///@todo FIXME My dynamic init header is a hack. Change the test to use BOOST_AUTO_TEST_CASE!
#include "boost_dynamic_init_test.h"
using namespace boost::unit_test_framework;

#include <thread>
#include <iostream>

#include "VersionNumber.h"

using namespace ChimeraTK;

class VersionNumberTest {
 public:
  void testEqual();
  void testNotEqual();
  void testSmaller();
  void testSmallerOrEqual();
  void testGreater();
  void testGreaterOrEqual();
  void testCopyConstruct();
  void testAssign();
  void testThreadedCreation();
  void testStringConvert();
  void testTimeStamp();

  VersionNumber v1;
  VersionNumber v2;
  VersionNumber v3;
  VersionNumber v4;

  std::list<VersionNumber> listA;
  std::list<VersionNumber> listB;
};

class VersionNumberTestSuite : public test_suite {
 public:
  VersionNumberTestSuite() : test_suite("VersionNumber test suite") {
    boost::shared_ptr<VersionNumberTest> test(new VersionNumberTest);
    add(BOOST_CLASS_TEST_CASE(&VersionNumberTest::testEqual, test));
    add(BOOST_CLASS_TEST_CASE(&VersionNumberTest::testNotEqual, test));
    add(BOOST_CLASS_TEST_CASE(&VersionNumberTest::testSmaller, test));
    add(BOOST_CLASS_TEST_CASE(&VersionNumberTest::testSmallerOrEqual, test));
    add(BOOST_CLASS_TEST_CASE(&VersionNumberTest::testGreater, test));
    add(BOOST_CLASS_TEST_CASE(&VersionNumberTest::testGreaterOrEqual, test));
    add(BOOST_CLASS_TEST_CASE(&VersionNumberTest::testAssign, test));
    add(BOOST_CLASS_TEST_CASE(&VersionNumberTest::testThreadedCreation, test));
    add(BOOST_CLASS_TEST_CASE(&VersionNumberTest::testStringConvert, test));
    add(BOOST_CLASS_TEST_CASE(&VersionNumberTest::testTimeStamp, test));
  }
};

bool init_unit_test() {
  framework::master_test_suite().p_name.value = "VersionNumber test suite";
  framework::master_test_suite().add(new VersionNumberTestSuite);
  return true;
}

void VersionNumberTest::testEqual() {
  BOOST_CHECK((v1 == v1));
  BOOST_CHECK(!(v1 == v2));
  BOOST_CHECK(!(v1 == v3));
  BOOST_CHECK(!(v1 == v4));

  BOOST_CHECK(!(v2 == v1));
  BOOST_CHECK((v2 == v2));
  BOOST_CHECK(!(v2 == v3));
  BOOST_CHECK(!(v2 == v4));

  BOOST_CHECK(!(v3 == v1));
  BOOST_CHECK(!(v3 == v2));
  BOOST_CHECK((v3 == v3));
  BOOST_CHECK(!(v3 == v4));

  BOOST_CHECK(!(v4 == v1));
  BOOST_CHECK(!(v4 == v2));
  BOOST_CHECK(!(v4 == v3));
  BOOST_CHECK((v4 == v4));
}

void VersionNumberTest::testNotEqual() {
  BOOST_CHECK(!(v1 != v1));
  BOOST_CHECK((v1 != v2));
  BOOST_CHECK((v1 != v3));
  BOOST_CHECK((v1 != v4));

  BOOST_CHECK((v2 != v1));
  BOOST_CHECK(!(v2 != v2));
  BOOST_CHECK((v2 != v3));
  BOOST_CHECK((v2 != v4));

  BOOST_CHECK((v3 != v1));
  BOOST_CHECK((v3 != v2));
  BOOST_CHECK(!(v3 != v3));
  BOOST_CHECK((v3 != v4));

  BOOST_CHECK((v4 != v1));
  BOOST_CHECK((v4 != v2));
  BOOST_CHECK((v4 != v3));
  BOOST_CHECK(!(v4 != v4));
}

void VersionNumberTest::testSmaller() {
  BOOST_CHECK(!(v1 < v1));
  BOOST_CHECK((v1 < v2));
  BOOST_CHECK((v1 < v3));
  BOOST_CHECK((v1 < v4));

  BOOST_CHECK(!(v2 < v1));
  BOOST_CHECK(!(v2 < v2));
  BOOST_CHECK((v2 < v3));
  BOOST_CHECK((v2 < v4));

  BOOST_CHECK(!(v3 < v1));
  BOOST_CHECK(!(v3 < v2));
  BOOST_CHECK(!(v3 < v3));
  BOOST_CHECK((v3 < v4));

  BOOST_CHECK(!(v4 < v1));
  BOOST_CHECK(!(v4 < v2));
  BOOST_CHECK(!(v4 < v3));
  BOOST_CHECK(!(v4 < v4));
}

void VersionNumberTest::testSmallerOrEqual() {
  BOOST_CHECK((v1 <= v1));
  BOOST_CHECK((v1 <= v2));
  BOOST_CHECK((v1 <= v3));
  BOOST_CHECK((v1 <= v4));

  BOOST_CHECK(!(v2 <= v1));
  BOOST_CHECK((v2 <= v2));
  BOOST_CHECK((v2 <= v3));
  BOOST_CHECK((v2 <= v4));

  BOOST_CHECK(!(v3 <= v1));
  BOOST_CHECK(!(v3 <= v2));
  BOOST_CHECK((v3 <= v3));
  BOOST_CHECK((v3 <= v4));

  BOOST_CHECK(!(v4 <= v1));
  BOOST_CHECK(!(v4 <= v2));
  BOOST_CHECK(!(v4 <= v3));
  BOOST_CHECK((v4 <= v4));
}

void VersionNumberTest::testGreater() {
  BOOST_CHECK(!(v1 > v1));
  BOOST_CHECK(!(v1 > v2));
  BOOST_CHECK(!(v1 > v3));
  BOOST_CHECK(!(v1 > v4));

  BOOST_CHECK((v2 > v1));
  BOOST_CHECK(!(v2 > v2));
  BOOST_CHECK(!(v2 > v3));
  BOOST_CHECK(!(v2 > v4));

  BOOST_CHECK((v3 > v1));
  BOOST_CHECK((v3 > v2));
  BOOST_CHECK(!(v3 > v3));
  BOOST_CHECK(!(v3 > v4));

  BOOST_CHECK((v4 > v1));
  BOOST_CHECK((v4 > v2));
  BOOST_CHECK((v4 > v3));
  BOOST_CHECK(!(v4 > v4));
}

void VersionNumberTest::testGreaterOrEqual() {
  BOOST_CHECK((v1 >= v1));
  BOOST_CHECK(!(v1 >= v2));
  BOOST_CHECK(!(v1 >= v3));
  BOOST_CHECK(!(v1 >= v4));

  BOOST_CHECK((v2 >= v1));
  BOOST_CHECK((v2 >= v2));
  BOOST_CHECK(!(v2 >= v3));
  BOOST_CHECK(!(v2 >= v4));

  BOOST_CHECK((v3 >= v1));
  BOOST_CHECK((v3 >= v2));
  BOOST_CHECK((v3 >= v3));
  BOOST_CHECK(!(v3 >= v4));

  BOOST_CHECK((v4 >= v1));
  BOOST_CHECK((v4 >= v2));
  BOOST_CHECK((v4 >= v3));
  BOOST_CHECK((v4 >= v4));
}

void VersionNumberTest::testCopyConstruct() {
  VersionNumber v1copied(v1);
  VersionNumber v2copied(v2);
  VersionNumber v3copied(v3);
  VersionNumber v4copied(v4);

  BOOST_CHECK(v1copied == v1);
  BOOST_CHECK(v2copied == v2);
  BOOST_CHECK(v3copied == v3);
  BOOST_CHECK(v4copied == v4);

  BOOST_CHECK(v1copied != v2);
  BOOST_CHECK(v1copied != v3);
  BOOST_CHECK(v1copied != v4);
  BOOST_CHECK(v2copied != v3);
  BOOST_CHECK(v2copied != v4);
  BOOST_CHECK(v3copied != v4);
}

void VersionNumberTest::testAssign() {
  VersionNumber v1assigned;
  VersionNumber v2assigned;
  VersionNumber v3assigned;
  VersionNumber v4assigned;

  BOOST_CHECK(v1assigned > v4);
  BOOST_CHECK(v2assigned > v4);
  BOOST_CHECK(v3assigned > v4);
  BOOST_CHECK(v4assigned > v4);

  v1assigned = v1;
  v2assigned = v2;
  v3assigned = v3;
  v4assigned = v4;

  BOOST_CHECK(v1assigned == v1);
  BOOST_CHECK(v2assigned == v2);
  BOOST_CHECK(v3assigned == v3);
  BOOST_CHECK(v4assigned == v4);

  v1assigned = VersionNumber();
  BOOST_CHECK(v1assigned > v4assigned);

  v2assigned = VersionNumber();
  BOOST_CHECK(v2assigned > v1assigned);

  v3assigned = VersionNumber();
  BOOST_CHECK(v3assigned > v2assigned);

  v4assigned = VersionNumber();
  BOOST_CHECK(v4assigned > v3assigned);

  v1assigned = VersionNumber(v1);
  v2assigned = VersionNumber(v2);
  v3assigned = VersionNumber(v3);
  v4assigned = VersionNumber(v4);

  BOOST_CHECK(v1assigned == v1);
  BOOST_CHECK(v2assigned == v2);
  BOOST_CHECK(v3assigned == v3);
  BOOST_CHECK(v4assigned == v4);
}

void VersionNumberTest::testThreadedCreation() {
  std::cout << "Filling lists of VersionNumbers..." << std::endl;

  std::thread t([this] {
    for(int i = 0; i < 1000; ++i) this->listA.push_back(VersionNumber());
  });
  for(int i = 0; i < 1000; ++i) listB.push_back(VersionNumber());
  t.join();

  std::cout << "Now comparing all pairs of elements in the lists..." << std::endl;

  for(auto& a : listA) {
    for(auto& b : listB) {
      BOOST_CHECK(a != b);
    }
    int nSelfMatches = 0;
    for(auto& a2 : listA) {
      if(a == a2) nSelfMatches++;
    }
    BOOST_CHECK_EQUAL(nSelfMatches, 1);
  }

  for(auto& b : listB) {
    int nSelfMatches = 0;
    for(auto& b2 : listB) {
      if(b == b2) nSelfMatches++;
    }
    BOOST_CHECK_EQUAL(nSelfMatches, 1);
  }

  std::cout << "done." << std::endl;
}

void VersionNumberTest::testStringConvert() {
  std::string s1 = (std::string)v1;
  std::string s2 = (std::string)v2;
  std::string s3 = (std::string)v3;
  std::string s4 = (std::string)v4;

  VersionNumber v1copied(v1);
  VersionNumber v2copied(v2);
  VersionNumber v3copied(v3);
  VersionNumber v4copied(v4);

  std::string s1c = (std::string)v1copied;
  std::string s2c = (std::string)v2copied;
  std::string s3c = (std::string)v3copied;
  std::string s4c = (std::string)v4copied;

  BOOST_CHECK(s1 != "");
  BOOST_CHECK(s2 != "");
  BOOST_CHECK(s3 != "");
  BOOST_CHECK(s4 != "");

  BOOST_CHECK(s1c != "");
  BOOST_CHECK(s2c != "");
  BOOST_CHECK(s3c != "");
  BOOST_CHECK(s4c != "");

  BOOST_CHECK_EQUAL(s1, s1c);
  BOOST_CHECK_EQUAL(s2, s2c);
  BOOST_CHECK_EQUAL(s3, s3c);
  BOOST_CHECK_EQUAL(s4, s4c);
}

void VersionNumberTest::testTimeStamp() {
  auto t0 = std::chrono::system_clock::now();
  VersionNumber vv0;
  BOOST_CHECK(vv0.getTime() >= t0);
  VersionNumber vv1;
  BOOST_CHECK(vv1.getTime() >= vv0.getTime());
  sleep(1);
  VersionNumber vv2;
  BOOST_CHECK(vv2.getTime() > vv1.getTime());
  sleep(1);
  auto t1 = std::chrono::system_clock::now();
  BOOST_CHECK(vv2.getTime() < t1);
}
