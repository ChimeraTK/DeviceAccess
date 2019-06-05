/*
 * Application.h
 *
 *  Created on: Jun 07, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_APPLICATION_H
#define CHIMERATK_APPLICATION_H

#include <atomic>
#include <mutex>

#include <ChimeraTK/ControlSystemAdapter/ApplicationBase.h>
#include <ChimeraTK/DeviceBackend.h>

#include "EntityOwner.h"
#include "Flags.h"
#include "InternalModule.h"
#include "Profiler.h"
#include "VariableNetwork.h"
//#include "DeviceModule.h"

namespace ChimeraTK {

  class Module;
  class AccessorBase;
  class VariableNetwork;
  class TriggerFanOut;
  class TestFacility;
  class DeviceModule;

  template<typename UserType>
  class Accessor;

  class Application : public ApplicationBase, public EntityOwner {
   public:
    /** The constructor takes the application name as an argument. The name must
     * have a non-zero length and must not contain any spaces or special
     * characters. Use only alphanumeric characters and underscores. */
    Application(const std::string& name);

    ~Application() {}

    using ApplicationBase::getName;

    /** This will remove the global pointer to the instance and allows creating
     * another instance afterwards. This is mostly useful for writing tests, as it
     * allows to run several applications sequentially in the same executable.
     * Note that any ApplicationModules etc. owned by this Application are no
     * longer
     *  valid after destroying the Application and must be destroyed as well (or
     * at least no longer used). */
    void shutdown() override;

    /** Define the connections between process variables. Must be implemented by
     * the application developer. */
    virtual void defineConnections() = 0;

    void initialise() override;

    void run() override;

    /** Instead of running the application, just initialise it and output the
     * published variables to an XML file. */
    void generateXML();

    /** Output the connections requested in the initialise() function to
     * std::cout. This may be done also before
     *  makeConnections() has been called. */
    void dumpConnections();

    /** Create Graphviz dot graph and write to file. The graph will contain the
     * connections made in the initilise() function. @see dumpConnections */
    void dumpConnectionGraph(const std::string& filename = {"connections-graph.dot"});

    /** Enable warning about unconnected variables. This can be helpful to
     * identify missing connections but is
     *  disabled by default since it may often be very noisy. */
    void warnUnconnectedVariables() { enableUnconnectedVariablesWarning = true; }

    /** Obtain instance of the application. Will throw an exception if called
     * before the instance has been created by the control system adapter, or if
     * the instance is not based on the Application class. */
    static Application& getInstance();

    /** Enable the testable mode. This allows to pause and resume the application
     * for testing purposes using the functions pauseApplication() and
     * resumeApplication(). The application will start in paused state.
     *
     *  This function must be called before the application is initialised (i.e.
     * before the call to initialise()).
     *
     *  Note: Enabling the testable mode will have a singificant impact on the
     * performance, since it will prevent any module threads to run at the same
     * time! */
    void enableTestableMode() {
      threadName() = "TEST THREAD";
      testableMode = true;
      testableModeLock("enableTestableMode");
    }

    /**
     * Returns true if application is in testable mode else returns
     * false.
     **/
    bool isTestableModeEnabled() { return testableMode; }

    /** Resume the application until all application threads are stuck in a
     * blocking read operation. Works only when the testable mode was enabled. */
    void stepApplication();

    /** Enable some additional (potentially noisy) debug output for the testable
     * mode. Can be useful if tests
     *  of applications seem to hang for no reason in stepApplication. */
    void debugTestableMode() { enableDebugTestableMode = true; }

    /** Lock the testable mode mutex for the current thread. Internally, a
     * thread-local std::unique_lock<std::mutex> will be created and re-used in
     * subsequent calls within the same thread to this function and to
     *  testableModeUnlock().
     *
     *  This function should generally not be used in user code. */
    static void testableModeLock(const std::string& name);

    /** Unlock the testable mode mutex for the current thread. See also
     * testableModeLock().
     *
     *  Initially the lock will not be owned by the current thread, so the first
     * call to this function will throw an exception (see
     * std::unique_lock::unlock()), unless testableModeLock() has been called
     * first.
     *
     *  This function should generally not be used in user code. */
    static void testableModeUnlock(const std::string& name);

    /** Test if the testable mode mutex is locked by the current thread.
     *
     *  This function should generally not be used in user code. */
    static bool testableModeTestLock() {
      if(!getInstance().testableMode) return false;
      return getTestableModeLockObject().owns_lock();
    }

    /** Get string holding the name of the current thread. This is used e.g. for
     * debugging output of the testable mode and for the internal profiler. */
    static std::string& threadName();

    /** Register the thread in the application system and give it a name. This
     * should be done for all threads used by the application to help with
     * debugging and to allow profiling. */
    static void registerThread(const std::string& name) {
      threadName() = name;
      Profiler::registerThread(name);
      pthread_setname_np(pthread_self(), name.substr(0, std::min(name.length(), 15UL)).c_str());
    }

    ModuleType getModuleType() const override { return ModuleType::ModuleGroup; }

    std::string getQualifiedName() const override { return "/" + _name; }

    std::string getFullDescription() const override { return ""; }

    /** Special exception class which will be thrown if tests with the testable
     * mode are stalled. Normally this exception should never be caught. The only
     * reason for catching it might be a situation where the expected behaviour of
     * an app is to do nothing and one wants to test this. Note that the stall
     * condition only appears after a certain timeout, thus tests relying on this
     * will take time.
     *
     *  This exception must not be based on a generic exception class to prevent
     * catching it unintentionally. */
    class TestsStalled {};

    /** Enable debug output for a given variable. */
    void enableVariableDebugging(const VariableNetworkNode& node) { debugMode_variableList.insert(node.getUniqueId()); }

    /** Incremenet counter for how many write() operations have overwritten unread
     * data */
    static void incrementDataLossCounter() { getInstance().dataLossCounter++; }

    static size_t getAndResetDataLossCounter() {
      size_t counter = getInstance().dataLossCounter.load(std::memory_order_relaxed);
      while(!getInstance().dataLossCounter.compare_exchange_weak(
          counter, 0, std::memory_order_release, std::memory_order_relaxed))
        ;
      return counter;
    }

    /** Convenience function for creating constants. See
     * VariableNetworkNode::makeConstant() for details. */
    template<typename UserType>
    static VariableNetworkNode makeConstant(UserType value, size_t length = 1, bool makeFeeder = true) {
      return VariableNetworkNode::makeConstant(makeFeeder, value, length);
    }

    void registerDeviceModule(DeviceModule* deviceModule);
    void unregisterDeviceModule(DeviceModule* deviceModule);

   protected:
    friend class Module;
    friend class VariableNetwork;
    friend class VariableNetworkNode;
    friend class VariableNetworkGraphDumpingVisitor;
    friend class XMLGeneratorVisitor;

    template<typename UserType>
    friend class Accessor;

    /** Finalise connections, i.e. decide still-undecided details mostly for
     * Device and ControlSystem variables */
    void finaliseNetworks();

    /** Check if all connections are valid. Internally called in initialise(). */
    void checkConnections();

    /** Obtain the lock object for the testable mode lock for the current thread.
     * The returned object has thread_local storage duration and must only be used
     * inside the current thread. Initially (i.e. after the first call in one
     * particular thread) the lock will not be owned by the returned object, so it
     * is important to catch the corresponding exception when calling
     * std::unique_lock::unlock(). */
    static std::unique_lock<std::mutex>& getTestableModeLockObject();

    /** Register the connections to constants for previously unconnected nodes. */
    void processUnconnectedNodes();

    /** Make the connections between accessors as requested in the initialise()
     * function. */
    void makeConnections();

    /** Apply optimisations to the VariableNetworks, e.g. by merging networks
     * sharing the same feeder. */
    void optimiseConnections();

    /** Make the connections for a single network */
    void makeConnectionsForNetwork(VariableNetwork& network);

    /** UserType-dependent part of makeConnectionsForNetwork() */
    template<typename UserType>
    void typedMakeConnection(VariableNetwork& network);

    /** Functor class to call typedMakeConnection() with the right template
     * argument. */
    struct TypedMakeConnectionCaller {
      TypedMakeConnectionCaller(Application& owner, VariableNetwork& network);

      template<typename PAIR>
      void operator()(PAIR&) const;

      Application& _owner;
      VariableNetwork& _network;
      mutable bool done{false};
    };

    /** Register a connection between two VariableNetworkNode */
    VariableNetwork& connect(VariableNetworkNode a, VariableNetworkNode b);

    /** Perform the actual connection of an accessor to a device register */
    template<typename UserType>
    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> createDeviceVariable(const std::string& deviceAlias,
        const std::string& registerName, VariableDirection direction, UpdateMode mode, size_t nElements);

    /** Create a process variable with the PVManager, which is exported to the
       control system adapter. nElements will be the array size of the created
       variable. */
    template<typename UserType>
    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> createProcessVariable(VariableNetworkNode const& node);

    /** Create a local process variable which is not exported. The first element
     * in the returned pair will be the sender, the second the receiver. If two
     * nodes are passed, the first node should be the sender and the second the
     * receiver. */
    template<typename UserType>
    std::pair<boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>>,
        boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>>>
        createApplicationVariable(VariableNetworkNode const& node, VariableNetworkNode const& consumer = {});

    /** List of InternalModules */
    std::list<boost::shared_ptr<InternalModule>> internalModuleList;

    /** List of variable networks */
    std::list<VariableNetwork> networkList;

    /** List of constant variable nodes */
    std::list<VariableNetworkNode> constantList;

    /** Map of trigger consumers to their corresponding TriggerFanOuts. Note: the
     * key is the ID (address) of the externalTiggerImpl. */
    std::map<const void*, boost::shared_ptr<TriggerFanOut>> triggerMap;

    /** Create a new, empty network */
    VariableNetwork& createNetwork();

    /** Instance of VariableNetwork to indicate an invalid network */
    VariableNetwork invalidNetwork;

    /** Map of DeviceBackends used by this application. The map key is the alias
     * name from the DMAP file */
    std::map<std::string, boost::shared_ptr<ChimeraTK::DeviceBackend>> deviceMap;

    /** List of DeviceModules */
    std::list<DeviceModule*> deviceModuleList;

    /** Flag if connections should be made in testable mode (i.e. the
     * TestableModeAccessorDecorator is put around all push-type input accessors
     * etc.). */
    bool testableMode{false};

    /** Mutex used in testable mode to take control over the application threads.
     * Use only through the lock object obtained through
     * getLockObjectForCurrentThread().
     *
     *  This member is static, since it should survive destroying an application
     * instance and creating a new one. Otherwise getTestableModeLockObject()
     * would not work, since it relies on thread_local instances which have to be
     * static. The static storage duration presents no problem in either case,
     * since there can only be one single instance of Application at a time (see
     * ApplicationBase constructor). */
    static std::mutex testableMode_mutex;

    /** Semaphore counter used in testable mode to check if application code is
     * finished executing. This value may only be accessed while holding the
     * testableMode_mutex. */
    size_t testableMode_counter{0};

    /** Flag if noisy debug output is enabled for the testable mode */
    bool enableDebugTestableMode{false};

    /** Flag whether to warn about unconnected variables or not */
    bool enableUnconnectedVariablesWarning{false};

    /** Map from ProcessArray uniqueId to the variable ID for control system
     * variables. This is required for the TestFacility. */
    std::map<size_t, size_t> pvIdMap;

    /** Return a fresh variable ID which can be assigned to a sender/receiver
     * pair. The ID will always be non-zero. */
    static size_t getNextVariableId() {
      static size_t nextId{0};
      return ++nextId;
    }

    /** Last thread which successfully obtained the lock for the testable mode.
     * This is used to prevent spamming repeating messages if the same thread
     * acquires and releases the lock in a loop without another thread
     *  activating in between. */
    std::thread::id testableMode_lastMutexOwner;

    /** Counter how often the same thread has acquired the testable mode mutex in
     * a row without another thread owning it in between. This is an indicator for
     * the test being stalled due to data send through a process
     *  variable but not read by the receiver. */
    std::atomic<size_t> testableMode_repeatingMutexOwner{false};

    /** Testable mode: like testableMode_counter but broken out for each variable.
     * This is not actually used as a semaphore counter but only in case of a
     * detected stall (see testableMode_repeatingMutexOwner) to print a list of
     * variables which still contain unread values. The index of the map is the
     * unique ID of the variable. */
    std::map<size_t, size_t> testableMode_perVarCounter;

    /** Map of unique IDs to namess, used along with testableMode_perVarCounter to
     * print sensible information. */
    std::map<size_t, std::string> testableMode_names;

    /** Map of unique IDs to process variables which have been decorated with the
     * TestableModeAccessorDecorator. */
    std::map<size_t, boost::shared_ptr<TransferElement>> testableMode_processVars;

    /** Map of unique IDs to flags whether the update mode is UpdateMode::poll (so
     * we do not use the decorator) */
    std::map<size_t, bool> testableMode_isPollMode;

    /** List of variables for which debug output was requested via
     * enableVariableDebugging(). Stored is the unique id of the
     * VariableNetworkNode.*/
    std::unordered_set<const void*> debugMode_variableList;

    template<typename UserType>
    friend class TestableModeAccessorDecorator; // needs access to the
                                                // testableMode_mutex and
                                                // testableMode_counter and the
                                                // idMap

    friend class TestFacility; // needs access to testableMode_variables
    friend class DeviceModule; // needs access to testableMode_variables

    template<typename UserType>
    friend class DebugPrintAccessorDecorator; // needs access to the idMap

    /** Counter for how many write() operations have overwritten unread data */
    std::atomic<size_t> dataLossCounter{0};

    VersionNumber getCurrentVersionNumber() const override {
      throw ChimeraTK::logic_error("getCurrentVersionNumber() called on the application. This is probably "
                                   "caused by incorrect ownership of variables/accessors or VariableGroups.");
    }
    void setCurrentVersionNumber(VersionNumber) override {
      throw ChimeraTK::logic_error("setCurrentVersionNumber() called on the application. This is probably "
                                   "caused by incorrect ownership of variables/accessors or VariableGroups.");
    }
    DataValidity getDataValidity() const override {
      throw ChimeraTK::logic_error("getDataValidity() called on the application. This is probably "
                                   "caused by incorrect ownership of variables/accessors or VariableGroups.");
    }
    void incrementDataFaultCounter() override {
      throw ChimeraTK::logic_error("incrementDataFaultCounter() called on the application. This is probably "
                                   "caused by incorrect ownership of variables/accessors or VariableGroups.");
    }
    void decrementDataFaultCounter() override {
      throw ChimeraTK::logic_error("decrementDataFaultCounter() called on the application. This is probably "
                                   "caused by incorrect ownership of variables/accessors or VariableGroups.");
    }
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_APPLICATION_H */
