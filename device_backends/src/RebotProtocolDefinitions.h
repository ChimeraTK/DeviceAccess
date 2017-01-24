#ifndef CHIMERAT_REBOT_PROTOCOL_DEFINITIONS
#define CHIMERAT_REBOT_PROTOCOL_DEFINITIONS

#include <cstdint>

namespace ChimeraTK{

namespace rebot{
  
  static const int READ_BLOCK_SIZE = 361;
  static const uint32_t HELLO_TOKEN = 0x00000004;
  static const uint32_t LENGTH_OF_HELLO_TOKEN_MESSAGE = 3; // 3 words
  static const uint32_t MAGIC_WORD = 0x72626f74; // rbot in ascii.. start from msb

  static const uint32_t SINGLE_WORD_READ = 0;
  static const uint32_t SINGLE_WORD_WRITE = 1;
  static const uint32_t MULTI_WORD_WRITE = 2;
  static const uint32_t MULTI_WORD_READ = 3;
  static const uint32_t PING = 5;
  static const uint32_t SET_SESSION_TIMEOUT = 6;

  // Most Significant 16 bits ==  major version
  // Least Significant 16 bits == minor version
  static const uint32_t CLIENT_PROTOCOL_VERSION = 0x00000001;

  static const uint32_t READ_ACK = 1000;
  static const uint32_t WRITE_ACK = 1001;
  static const uint32_t PONG = 1005;
  static const uint32_t TOO_MUCH_DATA = -1010; // FIXME: use unsigned
  static const uint32_t UNKNOWN_INSTRUCTION = -1040; // FIXME: use unsigned
  
  static const unsigned int DEFAULT_SERVER_PORT = 5001;

}//namespace rebot

}//namespace ChimeraTK

#endif // CHIMERAT_REBOT_PROTOCOL_DEFINITIONS
