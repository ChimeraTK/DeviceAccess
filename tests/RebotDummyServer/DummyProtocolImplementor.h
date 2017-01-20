#ifndef DUMMY_PROTOCOL_IMPLEMENTOR_H
#define DUMMY_PROTOCOL_IMPLEMENTOR_H

#include <vector>
#include <cstdint>

/// Only put commands which don't exist in all versions, or behave differently
struct DummyProtocolImplementor{
  virtual void multiWordRead(std::vector<uint32_t>& buffer)=0;
};

#endif //DUMMY_PROTOCOL_IMPLEMENTOR_H
