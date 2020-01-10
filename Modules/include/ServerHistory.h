/*!
 * \author Klaus Zenker (HZDR)
 * \date 10.08.2018
 * \page historydoc Server based history module
 * \section historyintro Server based history
 *  Some control systems offer a variable history but some do not. In this case
 * the \c ServerHistory can be used to create a history ring buffer on the
 * server. If only a local history is needed consider to use the \c MicroDAQ
 * module. In order to do so you connect the variable that should have a history
 * on the server to the \c ServerHistory module. The history length is set
 * during module construction and fixed per module. Every time one of the
 * variable handled by the history module is updated it will be filled into the
 * history buffer. The buffer length (history length) can not be changed during
 * runtime. Finally, one can create an addition buffer for each history
 * buffer that includes the time stamps of each data point in the history buffer.
 * This is useful if not all history buffers are filled with the same rate or the
 * rate is not known.
 *
 *
 *  Output variables created by the \c ServerHistory module are named like their
 * feeding process variables with a prefixed name that is set when the process
 * variables is added to the history module. In case of Array type feeding
 * process variables n history buffers are created (where n is the Array size)
 * and the element index i is appended to the feeding process variable name. In
 * consequence an input array of length i will result in i output history
 * arrays. The following tags are added to the history output variable:
 *  - CS
 *  - name of the history module
 *
 * It is also possible to connect a DeviceModule to the ServerHistory module. This
 * requires a trigger, which is given as optional parameter to the \c addSource
 * method. If the device variables are writable they are of push type. In this case
 * the trigger will not be added. One has to use the LogicalNameMapping backend to
 * force the device variables to be read only by using the \c forceReadOnly plugin.
 * Using the LogicalNameMapping backend also allows to select individual device
 * process variables to be connected to the \c ServerHistory.
 *
 *
 *  The following example shows how to integrate the \c ServerHistory module.
 *  \code
 *  sruct TestModule: public ChimeraTK::ApplicationModule{
 *  chimeraTK::ScalarOutput<float> measurement{this, "measurement", "" ,
 * "measurement variable", {"CS", History"}};
 *  ...
 *  };
 *  struct myApp : public ChimeraTK::Application{
 *
 *  history::ServerHistory<float> history{this, "ServerHistory", "History for
 * certain scalara float variables", 20}; // History buffer length is 20
 *
 *  ChimeraTK::ControlSystemModule cs;
 *
 *  ChimeraTK::DeviceModule dev{this, "Dummy"};
 *
 *  ChimeraTK::PeriodicTrigger trigger{this, "Trigger", "Trigger used for other modules"};
 *
 *
 *  TestModule test{ this, "test", "" };
 *  ...
 *  };
 *
 *
 *  void myAPP::defineConnctions(){
 *  // connect a module with variables that are updated by the module, which
 *  // triggers an update of the history buffer
 *  history.addSource(test.findTag("History"), "history" + test->getName())
 *  // will show up in the control system as history/test/measurement
 *  // add a device. Updating of the history buffer is trigger external by the given trigger
 *  history.addSource(dev,"device_history",trigger.tick);
 *
 *  history.findTag("CS").connectTo(cs);
 *  ...
 *  }
 *
 *  \endcode
 */

#ifndef MODULES_SERVERHISTORY_H_
#define MODULES_SERVERHISTORY_H_

#include "ApplicationCore.h"
#include <ChimeraTK/SupportedUserTypes.h>

#include <tuple>
#include <unordered_set>
#include <vector>

namespace ChimeraTK { namespace history {

  struct AccessorAttacher;

  template<typename UserType>
  struct HistoryEntry{
    HistoryEntry(bool enableHistory): data(std::vector<ArrayOutput<UserType> >{}),
                                      timeStamp(std::vector<ArrayOutput<uint64_t> >{}),
                                      withTimeStamps(enableHistory){  }
    std::vector<ArrayOutput<UserType> > data;
    std::vector<ArrayOutput<uint64_t> > timeStamp;
    bool withTimeStamps;
  };

  struct ServerHistory : public ApplicationModule {

    /**
     * Constructor.
     * Addition parameters to a normal application module constructor:
     * \param owner Module owner passed to ApplicationModule constructor.
     * \param name Module name passed to ApplicationModule constructor.
     * \param description Module description passed to ApplicationModule constructor.
     * \param historyLength Length of the history buffers.
     * \param enableTimeStamps An additional ring buffer per variable will be added that holds the time stamps
     *                         corresponding to the data ring buffer entries.
     * \param eliminateHierarchy Flag passed to ApplicationModule constructor.
     * \param tags Module tags passed to ApplicationModule constructor.
     */
    ServerHistory(EntityOwner* owner, const std::string& name, const std::string& description,
        size_t historyLength = 1200, bool enableTimeStamps = false, bool eliminateHierarchy = false, const std::unordered_set<std::string>& tags = {})
    : ApplicationModule(owner, name, description, eliminateHierarchy, tags), _historyLength(historyLength), _enbaleTimeStamps(enableTimeStamps) {  }

    /** Default constructor, creates a non-working module. Can be used for late
     * initialisation. */
    ServerHistory() : _historyLength(1200), _enbaleTimeStamps(false) {}

    /**
     * Add a Module as a source to this History module.
     *
     * \param source For all variables of this module ring buffers are created. Use \c findTag in combination
     *                   with a dedicated history tag. In case device modules use the LogicalNameMapping to create
     *                   a virtual device module that holds all variables that should be passed to the history module.
     * \param namePrefix This prefix is added to variable names added to the root directory in the process variable
     *                       tree. E.g. a prefix \c history for a variable names data will appear as history/dummy/data
     *                       if dummy is the name of the source module.
     * \param trigger This trigger is used for all poll type variable found in the source module.
     *
     */
    void addSource(const Module& source, const RegisterPath& namePrefix, const VariableNetworkNode &trigger = {});

    /**
     * Overload that calls virtualiseFromCatalog.
     */
    void addSource(const DeviceModule& source, const RegisterPath& namePrefix, const VariableNetworkNode &trigger = {});
   protected:
    void mainLoop() override;

    template<typename UserType>
    VariableNetworkNode getAccessor(const std::string& variableName, const size_t& nElements);

    /** Map of VariableGroups required to build the hierarchies. The key it the
     * full path name. */
    std::map<std::string, VariableGroup> groupMap;

    /** boost::fusion::map of UserTypes to std::lists containing the
     * ArrayPushInput and ArrayOutput accessors. These accessors are dynamically
     * created by the AccessorAttacher. */
    template<typename UserType>
    using AccessorList = std::list<std::pair<ArrayPushInput<UserType>, HistoryEntry<UserType> > >;
    TemplateUserTypeMap<AccessorList> _accessorListMap;

    /** boost::fusion::map of UserTypes to std::lists containing the names of the
     * accessors. Technically there would be no need to use TemplateUserTypeMap
     * for this (as type does not depend on the UserType), but since these lists
     * must be filled consistently with the accessorListMap, the same construction
     * is used here. */
    template<typename UserType>
    using NameList = std::list<std::string>;
    TemplateUserTypeMap<NameList> _nameListMap;

    /** Overall variable name list, used to detect name collisions */
    std::list<std::string> _overallVariableList;

    size_t _historyLength;
    bool _enbaleTimeStamps;

    friend struct AccessorAttacher;
  };

}}     // namespace ChimeraTK::history
#endif /* MODULES_SERVERHISTORY_H_ */
