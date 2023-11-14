// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE MappedImageTest

#include "Device.h"
#include "MappedImage.h"
#include "OneDRegisterAccessor.h"

#include <boost/test/unit_test.hpp>

using namespace boost::unit_test_framework;
using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE(MappedImageTestSuite)

struct DummyFixture {
  unsigned bufLen = 100; // includes header: 64 bytes
  ChimeraTK::OneDRegisterAccessor<unsigned char> buf;
  DummyFixture() {
    setDMapFilePath("testMappedImage.dmap");
    Device device;
    device.open("DUMMY");

    buf.replace(device.getOneDRegisterAccessor<unsigned char>("DAQ/IMAGE", bufLen));
  }
};

BOOST_FIXTURE_TEST_CASE(testStructMapping, DummyFixture) {
  // this test shows an example how to map user-defined opaque structs onto a byte array
  struct AStruct : public OpaqueStructHeader {
    int a = 0;
    float x = 0, y = 1;
    AStruct() : OpaqueStructHeader(typeid(AStruct)) {}
  };

  MappedStruct<AStruct> ms(buf, MappedStruct<AStruct>::InitData::Yes);
  auto* h = ms.header();
  h->x = 4.;
  BOOST_CHECK(h->totalLength == sizeof(AStruct));

  MappedStruct<AStruct> ms1(buf, MappedStruct<AStruct>::InitData::No);
  BOOST_CHECK(ms1.header()->x == 4.);
  BOOST_CHECK(ms1.header()->y == 1.); // a value set in constructor of AStruct

  buf.write();
  // test that values can be restored by reading from device
  ms.header()->x = 0;
  buf.read();
  BOOST_CHECK(ms.header()->x == 4.);
}

BOOST_FIXTURE_TEST_CASE(testMappedImage, DummyFixture) {
  // this test shows MappedImage usage

  MappedImage A0(buf);
  unsigned w = 4, h = 2;
  A0.setShape(w, h, ImgFormat::Gray16);
  auto Av = A0.interpretedView<uint16_t>();
  Av(0, 0) = 8;
  Av(1, 0) = 7;
  Av(2, 0) = 6;
  Av(3, 0) = 5;
  Av(0, 1) = 4;
  Av(1, 1) = 3;
  Av(2, 1) = 2;
  Av(3, 1) = 1;
  float val = Av(2, 0);
  BOOST_CHECK(val == 6);

  // also test iterator-style access
  for(unsigned y = 0; y < h; y++) {
    unsigned x = 0;
    for(auto* it = Av.beginRow(y); it != Av.endRow(y); ++it) {
      BOOST_CHECK(*it == Av(x, y));
      x++;
    }
  }
  // iterate over whole image
  unsigned counter = 0;
  for(auto& pixVal : Av) {
    pixVal = ++counter;
  }
  counter = 0;
  for(auto& pixVal : Av) {
    counter++;
    BOOST_CHECK(pixVal == counter);
  }

  // test actual header contents of our buffer
  auto* bufData0 = buf.data();
  auto* head = reinterpret_cast<ImgHeader*>(bufData0);
  BOOST_CHECK(head->width == w);
  BOOST_CHECK(head->height == h);
  BOOST_CHECK(head->image_format == ImgFormat::Gray16);
  BOOST_CHECK(head->channels == 1);
  BOOST_CHECK(head->bytesPerPixel == 2);

  // test actual image body contents of buffer
  auto* imgBody = reinterpret_cast<uint16_t*>(buf.data() + sizeof(ImgHeader));
  unsigned i = 0;
  for(auto& pixVal : Av) {
    BOOST_CHECK(imgBody[i] == pixVal);
    i++;
  }

  ImgHeader head0 = *head;
  buf.write(); // this allows to analyse data in dummy device, e.g. with shm dummy
  // set up memory location buf1 with slightly modified image content
  std::vector<unsigned char> buf1(buf.getNElements());
  *reinterpret_cast<ImgHeader*>(buf1.data()) = head0; // copy back header
  auto* imgBody1 = reinterpret_cast<uint16_t*>(buf1.data() + sizeof(ImgHeader));
  imgBody1[w * h - 1] = 42;
  buf.swap(buf1);
  BOOST_CHECK(buf.data() != bufData0);
  // check that ImgView can still be used, even though user buffer of accessor was swapped
  // compare bottow right value with previously written content.
  auto lastVal = Av(w - 1, h - 1);
  BOOST_CHECK(lastVal == 42);

  // re-use as float and check pixel channel access
  A0.setShape(2, 1, ImgFormat::FLOAT2);
  auto AvFloat2 = A0.interpretedView<float>();
  AvFloat2(0, 0, 1) = 0.1F;
  AvFloat2(1, 0, 1) = 1.1F;
  BOOST_TEST(AvFloat2(0, 0, 1) == 0.1F);
  BOOST_TEST(AvFloat2(1, 0, 1) == 1.1F);
}

BOOST_AUTO_TEST_SUITE_END()
