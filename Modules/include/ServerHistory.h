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
 * runtime.
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
 *
 *  TestModule test{ this, "test", "" };
 *  ...
 *  };
 *
 *
 *  void myAPP::defineConnctions(){
 *  history.addSource(test.findTag("History"), "history" + test->getName())
 *  history.findTag("CS").connectTo(cs);
 *  // will show up in the control system as history/test/measurement
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
#include <vector>

namespace ChimeraTK { namespace history {

  struct AccessorAttacher;

  struct ServerHistory : public ApplicationModule {
    ServerHistory(EntityOwner* owner, const std::string& name, const std::string& description,
        size_t historyLength = 1200, bool eliminateHierarchy = false, const std::unordered_set<std::string>& tags = {})
    : ApplicationModule(owner, name, description, eliminateHierarchy, tags), _historyLength(historyLength) {}

    /** Default constructor, creates a non-working module. Can be used for late
     * initialisation. */
    ServerHistory() : _historyLength(1200) {}

    /** Add a Module as a source to this History module. */
    void addSource(const Module& source, const RegisterPath& namePrefix, const VariableNetworkNode &trigger = {});

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
    using AccessorList = std::list<std::pair<ArrayPushInput<UserType>, std::vector<ArrayOutput<UserType>>>>;
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

    friend struct AccessorAttacher;
  };

}}     // namespace ChimeraTK::history
#endif /* MODULES_SERVERHISTORY_H_ */
