/*
 * Logging.cc
 *
 *  Created on: Apr 3, 2018
 *      Author: zenker
 */

#include <fstream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "Logging.h"
#include "boost/date_time/posix_time/posix_time.hpp"

using namespace logging;

std::ostream& logging::operator<<(std::ostream& os, const LogLevel& level) {
  switch(level) {
    case LogLevel::DEBUG:
      os << "DEBUG::";
      break;
    case LogLevel::INFO:
      os << "INFO::";
      break;
    case LogLevel::WARNING:
      os << "WARNING::";
      break;
    case LogLevel::ERROR:
      os << "ERROR::";
      break;
    default:
      break;
  }
  return os;
}

std::string logging::getTime() {
  std::string str;
  str.append(boost::posix_time::to_simple_string(boost::posix_time::microsec_clock::local_time()) + " ");
  str.append(" -> ");
  return str;
}

Logger::Logger(ctk::Module* module)
: message(module, "message", "", "Message of the module to the logging System", {"Logging", module->getName()}) {}

void Logger::sendMessage(const std::string& msg, const logging::LogLevel& level) {
  if(message.isInitialised()) {
    while(!msg_buffer.empty()) {
      message = msg_buffer.front();
      message.write();
      msg_buffer.pop();
    }
    message = std::to_string(level) + msg + "\n";
    message.write();
  }
  else {
    // only use the buffer until ctk initialized the process variables
    msg_buffer.push(std::to_string(level) + msg + "\n");
  }
}

void LoggingModule::broadcastMessage(std::string msg, const bool &isError) {
  if(msg.back() != '\n') {
    msg.append("\n");
  }

  std::string tmpLog = (std::string)logTail;
  if(tailLength == 0 && messageCounter > 20) {
    messageCounter--;
    tmpLog = tmpLog.substr(tmpLog.find_first_of("\n") + 1, tmpLog.length());
  }
  else if(tailLength > 0) {
    while(messageCounter >= tailLength) {
      messageCounter--;
      tmpLog = tmpLog.substr(tmpLog.find_first_of("\n") + 1, tmpLog.length());
    }
  }
  if(targetStream == 0 || targetStream == 2) {
    if(isError)
      std::cerr << msg;
    else
      std::cout << msg;
  }
  if(targetStream == 0 || targetStream == 1) {
    if(file->is_open()) {
      (*file) << msg.c_str();
      file->flush();
    }
  }
  tmpLog.append(msg);
  messageCounter++;
  logTail = tmpLog;
  logTail.write();
}

void LoggingModule::mainLoop() {
  file.reset(new std::ofstream());
  messageCounter = 0;
  std::stringstream greeter;
  greeter << getName() << " " << getTime() << "There are " << msg_list.size()
          << " modules registered for logging:" << std::endl;
  broadcastMessage(greeter.str());
  for(auto& module : msg_list) {
    broadcastMessage(std::string("\t - ") + module.first);
  }
  auto group = readAnyGroup();
  while(1) {
    auto id = group.readAny();
    auto sender = FindSender(id);
    if(targetStream == 3) continue;
    auto strlevel = ((std::string)(sender->second))[0];
    LogLevel level = static_cast<LogLevel>(std::strtoul(&strlevel,NULL, 0));
    LogLevel setLevel = static_cast<LogLevel>((uint)logLevel);
    std::string tmpStr = (std::string)(sender->second);
    // remove message level
    tmpStr = tmpStr.substr(1,tmpStr.size());
    std::stringstream ss;
    ss << level << getName() << "/" << sender->first << " " << getTime() << tmpStr;
    if(targetStream == 0 || targetStream == 1) {
      if(!((std::string)logFile).empty() && !file->is_open()) {
        std::stringstream ss_file;
        file->open((std::string)logFile, std::ofstream::out | std::ofstream::app);
        if(!file->is_open() && setLevel <= LogLevel::ERROR) {
          ss_file << LogLevel::ERROR << getName() << " " << getTime()
                  << "Failed to open log file for writing: " << (std::string)logFile << std::endl;
          broadcastMessage(ss_file.str(), true);
        }
        else if(file->is_open() && setLevel <= LogLevel::INFO) {
          ss_file << LogLevel::INFO << getName() << " " << getTime()
                  << "Opened log file for writing: " << (std::string)logFile << std::endl;
          broadcastMessage(ss_file.str());
        }
      }
    }
    if(level >= setLevel) {
      if(level < LogLevel::ERROR)
        broadcastMessage(ss.str());
      else
        broadcastMessage(ss.str(), true);
    }
  }
}

void LoggingModule::addSource(boost::shared_ptr<Logger> logger) {
  auto acc = getAccessorPair(logger->message.getOwner()->getName());
  logger->message >> acc;
}

ctk::VariableNetworkNode LoggingModule::getAccessorPair(
    const std::string& sender) {
  if(msg_list.count(sender) == 0) {
    msg_list.emplace(std::piecewise_construct, std::make_tuple(sender),
        std::make_tuple(ctk::ScalarPushInput<std::string>{this, sender + "Msg", "", ""}));
  }
  else {
    throw ChimeraTK::logic_error(
        "Cannot add logging for module " + sender + " since logging was already added for this module.");
  }
  return msg_list[sender];
}

std::map<std::string, ctk::ScalarPushInput<std::string> >::iterator LoggingModule::FindSender(const ChimeraTK::TransferElementID& id) {
  for(auto it = msg_list.begin(), iend = msg_list.end(); it != iend; it++) {
    if(it->second.getId() == id)
      return it;
  }
  throw ChimeraTK::logic_error("Cannot find  element id"
                               "when updating logging variables.");
}

void LoggingModule::terminate() {
  if((file.get() != nullptr) && (file->is_open())) file->close();
  ApplicationModule::terminate();
}
