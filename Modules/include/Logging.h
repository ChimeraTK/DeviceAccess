/*!
 * \author Klaus Zenker (HZDR)
 * \date 03.04.2018
 * \page loggingdoc Logging module and Logger
 * \section into Introduction to the logging mechanism
 * The logging provided here requires to add the LoggingModule to your Application.
 * The module introduces the following input variables, that need to be connected to the control system:
 * - targetStream: Allows to choose where messages send to the logging module end up:
 *   - 0: cout/cerr+logfile
 *   - 1: logfile
 *   - 2: cout/cerr
 *   - 3: nowhere
 * - logFile: Give the logfile name. If the file is not empty logging messages will be appended. If
 *   you choose targetStream 0 or 1 and don't set a logFile the Logging module simply skips the file
 *   writing.
 * - logLevel: Choose a certain logging level of the Module. Messages send to the Logging module also include a logging
 *   level. The Logging module compares both levels and decides if a message is dropped (e.g. message level is
 *   DEBUG and Module level is ERROR) or broadcasted.
 * - tailLength: The number of messages published by the Logging module (see logTail), i.e. to the control system.
 *   This length has no influence on the targetStreams, that receive all messages (depending on the logLevel). The
 *   logLevel also applies to messages that are published by the Logging module via the logTail
 *
 * Available logging levels are:
 *  - DEBUG
 *  - INFO
 *  - WARNING
 *  - ERROR
 *  - SILENT
 *
 *  The only variable that is published by the Logging module is the logTail. It contains the list of latest messages.
 *  Messages are separated by a newline character. The number of messages published in the logTail is set via the
 *  input variable tailLength. Other than that, messages are written to cout/cerr and/or a log file as explained above.
 *
 *  In order to add a source to the Logging module the method \c addSource(Logger*) is
 *  available.
 *  The foreseen way of using the Logger is to add a Logger to a module that should send log messages.
 *  In the definition of connections of the application (\c defineConnections() ) one can add this source to the Logging module.
 *
 *  The following example shows how to integrate the Logging module and the Logging into an application (myApp) and one module sending
 *  messages (TestModule).
 *  \code
 *  sruct TestModule: public ChimeraTK::ApplicationModule{
 *  Logger{this};
 *  ...
 *  };
 *  struct myApp : public ChimeraTK::Application{
 *
 *  LoggingModule log { this, "LoggingModule", "LoggingModule test" };
 *
 *  ChimeraTK::ControlSystemModule cs;
 *
 *  TestModule { this, "test", "" };
 *  ...
 *  };
 *
 *
 *  void myAPP::defineConnctions(){
 *  cs["Logging"]("targetStream") >> log.targetStream;
 *  cs["Logging"]("logLevel") >> log.logLevel;
 *  cs["Logging"]("logFile") >> log.logFile;
 *  cs["Logging"]("tailLength") >> log.tailLength;
 *  log.addSource(&TestModule.logger)
 *  ...
 *  }
 *
 *  void TestModule::mainLoop{
 *    logger.sendMessage("Test",LogLevel::DEBUG);
 *    ...
 *  }
 *  \endcode
 *
 *  A message always looks like this:
 *  LogLevel::LoggingModuleName/SendingModuleName TimeString -> message\n
 *  In the example given above the meassge could look like:
 *  \code
 *  DEBUG::LogggingModule/test 2018-Apr-12 14:03:07.947949 -> Test
 *  \endcode
 *  \remark Instead of adding a Logger to every module that should feed the Logging module, one could also consider using only one Logger object.
 *  This is not thread safe and would not work for multiple modules trying to send messages via the Logger object to the Logging module at the same time.

 */


#ifndef MODULES_LOGGING_H_
#define MODULES_LOGGING_H_

#undef GENERATE_XML
#include "ApplicationCore.h"
#include <queue>

namespace ctk = ChimeraTK;

namespace logging{

/** Pair of message and messageLevel */
using Message = std::pair<ctk::ScalarPushInput<std::string>,ctk::ScalarPushInput<uint> >;

/** Define available logging levels. */
enum LogLevel { DEBUG, INFO, WARNING, ERROR, SILENT };

/** Define stream operator to use the LogLevel in streams, e.g. std::cout */
std::ostream& operator<<(std::ostream& os, const LogLevel &level);

/** Construct a sting containing the current time. */
std::string getTime();

/**
 * \brief Class used to send messages in a convenient way to the LoggingModule.
 *
 * In principle this class only adds two output variables and provides simple method
 * to fill these variables. They are supposed to be connected to the LoggingModule via LoggingModule::addSource.
 * If sendMessage is used before chimeraTK process variables are initialized an internal buffer is used to store those
 * messages. Once the process variables are initialized the messages from the buffer are send.
 * \attention This only happens once a message is send after chimeraTK process variables are initialized!
 * In other words if no message is send in the mainLoop messages from defineConnections will never be shown.
 */
class Logger{
private:
  std::queue<std::pair<std::string, logging::LogLevel> > msg_buffer;
public:
  /**
   * \brief Default constructor: Allows late initialization of modules (e.g. when creating arrays of modules).
   *
   *  This constructor also has to be here to mitigate a bug in gcc. It is needed to allow constructor
   *  inheritance of modules owning other modules. This constructor will not actually be called then.
   *  See this bug report: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67054 */
  Logger(){};

  /**
   * \brief Constructor to be used.
   *
   * \param module The owning module that is using the Logger. It will appear as sender in the LoggingModule,
   * which is receiving messages from the Logger.
   */
  Logger(ctk::Module *module);
  /** Message to be send to the logging module */
  ctk::ScalarOutput<std::string> message;

  /** Message to be send to the logging module */
  ctk::ScalarOutput<uint> messageLevel;

  /**
   * \brief Send a message, which means to update the message and messageLevel member variables.
   *
   */
  void sendMessage(const std::string &msg, const logging::LogLevel &level);
};

/**
 * \brief Module used to handle logging messages.
 *
 * A ChimeraTK module is producing messages, that are send to the LoggingModule
 * via the \c message variable. The message is then put into the logfile ring buffer
 * and published in the \c LogFileTail. In addidtion the message can be put to an ostream.
 * Available streams are:
 * - file stream
 * - cout/cerr
 *
 * You can control which stream is used by setting the targetStream varibale:
 * 0: cout/cerr and logfile
 * 1: logfile
 * 2: cout/cerr
 * 3: none
 *
 * The logfile is given by the client using the logFile variable.
 */
class LoggingModule: public ctk::ApplicationModule {
private:
  std::pair<ctk::VariableNetworkNode,ctk::VariableNetworkNode> getAccessorPair(const std::string &sender);

  /** Map key is the feeding module */
  std::map<std::string,Message> msg_list;

  /** Update message or messageLevel from the sending module
   * Basically the first element read by readAny() could be either the message or
   * the messageLevel.
   * Here the element not updated by readSAny is also read, since both are PushInput variables.
   */
  std::map<std::string, Message>::iterator UpdatePair(const mtca4u::TransferElementID &id);

  /** Number of messages stored in the tail */
  size_t messageCounter;

  /** Broadcast message to cout/cerr and log file
   * \param msg The mesage
   * \param isError If true cerr is used. Else cout is used.
   */
  void broadcastMessage(std::string msg, bool isError = false);

public:
  using ctk::ApplicationModule::ApplicationModule;

  ctk::ScalarPollInput<uint> targetStream { this, "targetStream", "",
          "Set the tagret stream: 0 (cout/cerr+logfile), 1 (logfile), 2 (cout/cerr), 3 (none)" };

  ctk::ScalarPollInput<std::string> logFile { this, "Logfile", "",
    "Name of the external logfile. If empty messages are pushed to cout/cerr" };

  ctk::ScalarPollInput<uint> tailLength { this, "maxLength", "",
      "Maximum number of messages to be shown in the logging stream tail." };

  ctk::ScalarPollInput<uint> logLevel { this, "logLevel", "",
      "Current log level used for messages." };

  ctk::ScalarOutput<std::string> logTail { this, "LogTail", "", "Tail of the logging stream.",
      { "CS", "PROCESS", getName() } };

  std::unique_ptr<std::ofstream> file; ///< Log file where to write log messages

  /** Add a Module as a source to this DAQ. */
  void addSource(Logger *logger);

  /**
   * Application core main loop.
   */
  void mainLoop() override;

  void terminate() override;

};

}

#endif /* MODULES_LOGGING_H_ */
