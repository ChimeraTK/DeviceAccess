#include "ServerHistory.h"

//#include "ChimeraTK/TransferElementID.h"

namespace ChimeraTK{
namespace history{

/** Callable class for use with  boost::fusion::for_each: Attach the given accessor to the History with proper
  *  handling of the UserType. */
struct AccessorAttacher {
  AccessorAttacher(VariableNetworkNode& feeder, ServerHistory *owner, const std::string &name)
  : _feeder(feeder), _owner(owner), _name(name) {}

  template<typename PAIR>
  void operator()(PAIR&) const {

    // only continue if the call is for the right type
    if(typeid(typename PAIR::first_type) != _feeder.getValueType()) return;

    // register connection
    _feeder >> _owner->template getAccessor<typename PAIR::first_type>(_name, _feeder.pdata->nElements);

  }

  VariableNetworkNode &_feeder;
  ServerHistory *_owner;
  const std::string &_name;
};

void ServerHistory::addSource(const Module &source, const std::string &namePrefix) {

  // for simplification, first create a VirtualModule containing the correct hierarchy structure (obeying eliminate
  // hierarchy etc.)
  auto dynamicModel = source.findTag(".*");     /// @todo use virtualise() instead

  // add all accessors on this hierarchy level
  for(auto &acc : dynamicModel.getAccessorList()) {
    boost::fusion::for_each(_accessorListMap.table, AccessorAttacher(acc, this, namePrefix+"/"+acc.getName()));
  }

  // recurse into submodules
  for(auto mod : dynamicModel.getSubmoduleList()) {
    addSource(*mod, namePrefix+"/"+mod->getName());
  }

}

template<typename UserType>
VariableNetworkNode ServerHistory::getAccessor(const std::string &variableName, const size_t &nElements) {

  // check if variable name already registered
  for(auto &name : _overallVariableList) {
    if(name == variableName) {
      throw ChimeraTK::logic_error("Cannot add '"+variableName+
                "' to History since a variable with that name is already registered.");
    }
  }
  _overallVariableList.push_back(variableName);

  // add accessor and name to lists
  auto &accessorList = boost::fusion::at_key<UserType>(_accessorListMap.table);
  auto &nameList = boost::fusion::at_key<UserType>(_nameListMap.table);
  accessorList.emplace_back(std::piecewise_construct,
          std::forward_as_tuple(ArrayPushInput<UserType>{this, variableName + "_in", "", 0, "",}),
          std::forward_as_tuple(std::vector<ArrayOutput<UserType> >{}));
  for(size_t i =0; i < nElements; i++){
    if(nElements == 1) {
      // in case of a scalar history only use the variableName
      accessorList.back().second.emplace_back(ArrayOutput<UserType>{this, variableName, "", _historyLength, "",
        { "CS", getName() }});
    } else {
      // in case of an array history append the index to the variableName
      accessorList.back().second.emplace_back(ArrayOutput<UserType>{this, variableName + "_" + std::to_string(i), "", _historyLength, "",
        { "CS", getName() }});
    }
  }
  nameList.push_back(variableName);

  // return the accessor
  return accessorList.back().first;
}

struct Update {
  Update(ChimeraTK::TransferElementID id): _id(id){}

  template<typename PAIR>
  void operator()(PAIR &pair) const{
    auto &accessorList = pair.second;
    for(auto accessor = accessorList.begin() ; accessor != accessorList.end() ; ++accessor){
      if(accessor->first.getId() == _id){
        for(size_t i = 0; i < accessor->first.getNElements(); i++) {
          std::rotate(accessor->second.at(i).begin(), accessor->second.at(i).begin()+1, accessor->second.at(i).end());
          *(accessor->second.at(i).end()-1) = accessor->first[i];
          accessor->second.at(i).write();
        }
      }
    }
  }

  TransferElementID _id;
};

void ServerHistory::mainLoop(){
  auto group = readAnyGroup();
  while(true){
    auto id = group.readAny();
    boost::fusion::for_each(_accessorListMap.table, Update(id));
  }
}

}// namespace history
}// namespace ChimeraTK
