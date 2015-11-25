#include <string>
#include "argumentParser.h"

static const unsigned int DEFAULT_SERVER_PORT = 5001;
static const std::string DEFAULT_MAP_FILE = "testFile.map";


struct Argument {
  std::string _shortName;
  std::string _longName;

  Argument(std::string shortName, std::string longName)
      : _shortName(shortName), _longName(longName) {}
};

std::string getArgumentValue(Argument& argument, char** argumentArray);


unsigned int getPortNumber(char** argumentArray) {

  Argument portFlag("-p", "--port");
  std::string portNumber = getArgumentValue(portFlag, argumentArray);

  try {
    return std::stoul(portNumber);
  }
  catch (...) {
    return DEFAULT_SERVER_PORT;
  }
}

std::string getMapFileLocation(char** argumentArray) {

  Argument mapFileLocationFlag("-m", "--mapfile");
  std::string mapFileLocation = getArgumentValue(mapFileLocationFlag, argumentArray);

  if (mapFileLocation.empty()) {
    return DEFAULT_MAP_FILE;
  } else {
    return mapFileLocation;
  }
}

std::string getArgumentValue(Argument& argument, char** argumentArray) {
  if (*argumentArray == nullptr || *(argumentArray + 1) == nullptr) {
    return std::string();
  } else if (argument._shortName == *argumentArray ||
             argument._longName == *argumentArray) {
    return std::string(*(argumentArray + 1));
  }
  return getArgumentValue(argument, argumentArray + 1);
}
