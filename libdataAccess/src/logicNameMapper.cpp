/* 
 * File:   logicNameMapper.cpp
 * Author: apiotro
 * 
 */

#include "logicNameMapper.h"
#include "exLogicNameMapper.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <vector>
#include <bits/stl_vector.h>

logicNameMapper::logicNameMapper() {
}

std::string logicNameMapper::getMapFileName() const {
    return logicMapFileName;
}

logicNameMapper::~logicNameMapper() {
    std::map<std::string, logicNameData*>::iterator iter;
    for (iter = logicNameMapping.begin(); iter != logicNameMapping.end(); iter++) {
        delete (*iter).second;
    }
}

logicNameData* logicNameMapper::get(const std::string& logName) const {
    std::map<std::string, logicNameData*>::const_iterator citer;
    citer = logicNameMapping.find(logName);
    if (citer == logicNameMapping.end()) {
        throw exLogicNameMapper("Unknown logical name \"" + logName + "\"", exLogicNameMapper::EX_UNKNOWN_LOG_NAME);
    }
    return logicNameMapping.at(logName);
}

std::vector<std::string> logicNameMapper::parseRegularExpr(const std::string& val, const std::string& fileName, const std::string& line, size_t lineNr, size_t& posOpen, size_t& posClose) {
    std::vector<std::string> regVal;
    std::string reg;
    int iStart, iEnd, iStep = 1;
    char cVal;
    std::ostringstream os;
    size_t sPos;

    posOpen = val.find("[");
    if (posOpen != std::string::npos) {
        posClose = val.find("]");
        if (posClose != std::string::npos) {
            reg = val.substr(posOpen + 1, posClose - posOpen - 1);
            if (reg.size() == 0) {
                os << lineNr;
                throw exLogicNameMapper("Error in file \"" + fileName + "\" [" + os.str() + ": " + line + "]", exLogicNameMapper::EX_ERROR_IN_FILE);
            }
            std::istringstream is(reg);

            sPos = reg.find_first_of(':');
            if (sPos != std::string::npos) {
                is >> iStart;
                if (!is) {
                    os << lineNr;
                    throw exLogicNameMapper("Error in file \"" + fileName + "\" [" + os.str() + ": " + line + "]", exLogicNameMapper::EX_ERROR_IN_FILE);
                }
                is.ignore(1);
                is >> iEnd;
                if (!is) {
                    os << lineNr;
                    throw exLogicNameMapper("Error in file \"" + fileName + "\" [" + os.str() + ": " + line + "]", exLogicNameMapper::EX_ERROR_IN_FILE);
                }
                is.ignore(1);
                if (is) {
                    iStep = iEnd;
                    is >> iEnd;
                    if (!is) {
                        os << lineNr;
                        throw exLogicNameMapper("Error in file \"" + fileName + "\" [" + os.str() + ": " + line + "]", exLogicNameMapper::EX_ERROR_IN_FILE);
                    }
                }
                for (int i = iStart; i <= iEnd; i += iStep) {
                    os.str("");
                    os << i;
                    regVal.push_back(os.str());
                }
                return regVal;
            }
            size_t startPoint = 0;

            sPos = reg.find_first_of(',', startPoint);
            if (sPos == std::string::npos) {
                regVal.push_back(reg);
                return regVal;
            }
            std::string retVal;
            do {
                retVal = reg.substr(startPoint, sPos - startPoint);
                if (!retVal.size()) {
                    os << lineNr;
                    throw exLogicNameMapper("Error in file \"" + fileName + "\" [" + os.str() + ": " + line + "]", exLogicNameMapper::EX_ERROR_IN_FILE);
                }
                startPoint = sPos + 1;
                sPos = reg.find_first_of(',', startPoint);
                regVal.push_back(retVal);
            } while (sPos != std::string::npos);
            retVal = reg.substr(startPoint);
            if (!retVal.size()) {
                os << lineNr;
                throw exLogicNameMapper("Error in file \"" + fileName + "\" [" + os.str() + ": " + line + "]", exLogicNameMapper::EX_ERROR_IN_FILE);
            }
            regVal.push_back(retVal);
            return regVal;

            is >> iStart;
            if (!is) {
                os << lineNr;
                throw exLogicNameMapper("Error in file \"" + fileName + "\" [" + os.str() + ": " + line + "]", exLogicNameMapper::EX_ERROR_IN_FILE);
            }
            is >> cVal;
            if (is.eof()) {
                os.str("");
                os << iStart;
                regVal.push_back(os.str());
            } else if (cVal == ',') {
                os.str("");
                os << iStart;
                regVal.push_back(os.str());
                while (1) {
                    is >> iStart;
                    os.str("");
                    os << iStart;
                    regVal.push_back(os.str());
                    is >> cVal;
                    if (!is)
                        break;
                }
            } else {
                os << lineNr;
                throw exLogicNameMapper("Error in file \"" + fileName + "\" [" + os.str() + ": " + line + "]", exLogicNameMapper::EX_ERROR_IN_FILE);
            }
        }
    }
    return regVal;
}

void logicNameMapper::parse(const std::string& fileName) {
    std::ifstream file;
    std::string line;
    size_t pos;
    size_t posOpenLogic;
    size_t posCloseLogic;
    size_t posOpenAddr;
    size_t posCloseAddr;
    std::string::iterator i;
    std::map<std::string, logicNameData*>::const_iterator citer;
    std::ostringstream os;
    size_t lineNr = 0;
    std::string logName;
    std::string phiName;
    std::string protName;
    std::string address;
    std::string addressPrefix;
    file.open(fileName.c_str());
    if (!file) {
        throw exLogicNameMapper("File lmap: \"" + fileName + "\" not found.", exLogicNameMapper::EX_FILE_NOT_FOUND);
    }

    logicMapFileName = fileName;

    logicNameMapping.clear();

    while (!file.eof()) {
        lineNr++;
        std::getline(file, line);
        pos = line.find("#");
        if (pos != std::string::npos) line.erase(pos);
        i = std::remove_if(line.begin(), line.end(), isspace);
        if (i != line.end()) line.erase(i, line.end());
        if (line.length() == 0) continue;
        pos = line.find("=");
        if (pos == std::string::npos) {
            os << lineNr;
            throw exLogicNameMapper("Error in file \"" + fileName + "\" [" + os.str() + ": " + line + "]", exLogicNameMapper::EX_ERROR_IN_FILE);
        }
        if (line[0] == '%') {
            if (line.substr(1, pos - 1) == "ADDRESS_PREFIX") {
                addressPrefix = line.substr(pos + 1);
            }
            continue;
        }

        logName = line.substr(0, pos);
        phiName = line.substr(pos + 1);

        std::vector<std::string> regValLogicName;
        regValLogicName = parseRegularExpr(logName, fileName, line, lineNr, posOpenLogic, posCloseLogic);

        if (logName.length() == 0 || phiName.length() == 0) {
            os << lineNr;
            throw exLogicNameMapper("Error in file \"" + fileName + "\" [" + os.str() + ": " + line + "]", exLogicNameMapper::EX_ERROR_IN_FILE);
        }
        pos = phiName.find("@");
        if (pos == std::string::npos) {
            os << lineNr;
            throw exLogicNameMapper("Error in file \"" + fileName + "\" [" + os.str() + ": " + line + "]", exLogicNameMapper::EX_ERROR_IN_FILE);
        }
        protName = phiName.substr(0, pos);
        address = addressPrefix + phiName.substr(pos + 1);
        if (protName.length() == 0 || address.length() == 0) {
            os << lineNr;
            throw exLogicNameMapper("Error in file \"" + fileName + "\" [" + os.str() + ": " + line + "]", exLogicNameMapper::EX_ERROR_IN_FILE);
        }
        citer = logicNameMapping.find(logName);
        if (citer != logicNameMapping.end()) {
            throw exLogicNameMapper("Logical name redefinition \"" + logName + "\"", exLogicNameMapper::EX_UNKNOWN_LOG_NAME);
        }

        std::vector<std::string> regValAddress;
        regValAddress = parseRegularExpr(address, fileName, line, lineNr, posOpenAddr, posCloseAddr);



        if (regValLogicName.empty() && regValAddress.empty()) {
            logicNameMapping[logName] = new logicNameData(logName, protName, address, lineNr);
        } else if (!regValLogicName.empty() && regValAddress.empty()) {
            std::string prefixLogic = logName.substr(0, posOpenLogic);
            std::string sufixLogic = logName.substr(posCloseLogic + 1);
            std::vector<std::string>::iterator it;
            for (it = regValLogicName.begin(); it != regValLogicName.end(); ++it) {
                os.str("");
                os << prefixLogic << *it << sufixLogic;
                logicNameMapping[os.str()] = new logicNameData(os.str(), protName, address, lineNr);
            }
        } else if (regValLogicName.size() == regValAddress.size()) {
            std::string prefixLogic = logName.substr(0, posOpenLogic);
            std::string sufixLogic = logName.substr(posCloseLogic + 1);

            std::string prefixAddress = address.substr(0, posOpenAddr);
            std::string sufixAddress = address.substr(posCloseAddr + 1);

            std::vector<std::string>::iterator itL;
            std::vector<std::string>::iterator itA;
            std::string newAddr, newLogic;
            for (itL = regValLogicName.begin(), itA = regValAddress.begin(); itL != regValLogicName.end(); ++itL, ++itA) {
                os.str("");
                os << prefixLogic << *itL << sufixLogic;
                newLogic = os.str();
                os.str("");
                os << prefixAddress << *itA << sufixAddress;
                newAddr = os.str();
                logicNameMapping[newLogic] = new logicNameData(newLogic, protName, newAddr, lineNr);
            }
        } else {
            os << lineNr;
            throw exLogicNameMapper("Error in file \"" + fileName + "\" [" + os.str() + ": " + line + "]", exLogicNameMapper::EX_ERROR_IN_FILE);
        }
    }
    if (logicNameMapping.size() == 0) {
        throw exLogicNameMapper("No mapping information in file \"" + fileName + "\"", exLogicNameMapper::EX_ERROR_IN_FILE);
    }
}

logicNameMapper::iterator logicNameMapper::begin() {
    return iterator(logicNameMapping.begin());
}

logicNameMapper::iterator logicNameMapper::end() {
    return iterator(logicNameMapping.end());
}

logicNameMapper::iterator::iterator() {
}

logicNameMapper::iterator::iterator(std::map<std::string, logicNameData*>::iterator _iter)
: iter(_iter) {
}

bool logicNameMapper::iterator::operator!=(const logicNameMapper::iterator& _iter) {
    return iter != _iter.iter;
}

logicNameMapper::iterator& logicNameMapper::iterator::operator++() {
    iter++;
    return *this;
}

logicNameData* logicNameMapper::iterator::operator*() {
    return (*iter).second;
}


#ifdef __DEBUG_MODE__

std::ostream& operator<<(std::ostream &os, const logicNameMapper& lnm) {
    std::map<std::string, logicNameData*>::const_iterator iter;
    for (iter = lnm.logicNameMapping.begin(); iter != lnm.logicNameMapping.end(); iter++) {
        os << *((*iter).second);
    }
    return os;
}
#endif 