/*
 * VariableNetworkNode.h
 *
 *  Created on: Jun 23, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_VARIABLE_NETWORK_NODE_H
#define CHIMERATK_VARIABLE_NETWORK_NODE_H

#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include <assert.h>

#include <boost/shared_ptr.hpp>

#include <ChimeraTK/NDRegisterAccessorAbstractor.h>

#include "ConstantAccessor.h"
#include "Flags.h"
#include "MetaDataPropagatingRegisterDecorator.h"
#include "Visitor.h"

namespace ChimeraTK {

  class VariableNetwork;
  class AccessorBase;
  class EntityOwner;
  struct VariableNetworkNode_data;

  /** Pseudo type to identify nodes which can have arbitrary types */
  class AnyType {};

  /** Class describing a node of a variable network */
  class VariableNetworkNode {
   public:
    /** Copy-constructor: Just copy the pointer to the data storage object */
    VariableNetworkNode(const VariableNetworkNode& other);

    /** Copy by assignment operator: Just copy the pointer to the data storage
     * object */
    VariableNetworkNode& operator=(const VariableNetworkNode& rightHandSide);

    /** Constructor for an Application node */
    VariableNetworkNode(EntityOwner* owner, ChimeraTK::TransferElementAbstractor* accessorBridge,
        const std::string& name, VariableDirection direction, std::string unit, size_t nElements, UpdateMode mode,
        const std::string& description, const std::type_info* valueType,
        const std::unordered_set<std::string>& tags = {});

    /** Constructor for a Device node */
    VariableNetworkNode(const std::string& name, const std::string& deviceAlias, const std::string& registerName,
        UpdateMode mode, VariableDirection direction, const std::type_info& valTyp = typeid(AnyType),
        size_t nElements = 0);

    /** Constructor for a ControlSystem node */
    VariableNetworkNode(std::string publicName, VariableDirection direction,
        const std::type_info& valTyp = typeid(AnyType), size_t nElements = 0);

    /** Constructor for a TriggerReceiver node triggering the data transfer of
     * another network. The additional dummy argument is only there to
     * discriminate the signature from the copy constructor and will be ignored.
     */
    VariableNetworkNode(VariableNetworkNode& nodeToTrigger, int);

    /** Constructor to wrap a VariableNetworkNode_data pointer */
    VariableNetworkNode(boost::shared_ptr<VariableNetworkNode_data> pdata);

    /** Default constructor for an invalid node */
    VariableNetworkNode();

    /** Factory function for a constant (a constructor cannot be templated) */
    template<typename UserType>
    static VariableNetworkNode makeConstant(bool makeFeeder, UserType value = 0, size_t length = 1);

    /** Change meta data (name, unit, description and optionally tags). This
     * function may only be used on Application-type nodes. If the optional
     * argument tags is omitted, the tags will not be changed. To clear the
     *  tags, an empty set can be passed. */
    void setMetaData(const std::string& name, const std::string& unit, const std::string& description);
    void setMetaData(const std::string& name, const std::string& unit, const std::string& description,
        const std::unordered_set<std::string>& tags);

    /** Set the owner network of this node. If an owner network is already set, an
     * assertion will be raised */
    void setOwner(VariableNetwork* network);

    /** Clear the owner network of this node. */
    void clearOwner();

    /** Set the value type for this node. Only possible of the current value type
     * is undecided (i.e. AnyType). */
    void setValueType(const std::type_info& newType) const;

    /** Set the direction for this node. Only possible if current direction is
     * VariableDirection::feeding and the node type is NodeType::ControlSystem. */
    void setDirection(VariableDirection newDirection) const;

    /** Function checking if the node requires a fixed implementation */
    bool hasImplementation() const;

    /** Compare two nodes */
    bool operator==(const VariableNetworkNode& other) const;
    bool operator!=(const VariableNetworkNode& other) const;
    bool operator<(const VariableNetworkNode& other) const;

    /** Connect two nodes */
    VariableNetworkNode operator>>(VariableNetworkNode other);

    /** Add a trigger */
    VariableNetworkNode operator[](VariableNetworkNode trigger);

    /** Check for presence of an external trigger */
    bool hasExternalTrigger() const;

    /** Return the external trigger node. if no external trigger is present, an
     * assertion will be raised. */
    VariableNetworkNode getExternalTrigger();

    /** Removes an external trigger */
    void removeExternalTrigger();

    /** Print node information to std::cout */
    void dump(std::ostream& stream = std::cout) const;

    /** Check if the node already has an owner */
    bool hasOwner() const;

    /** Add a tag. This function may only be used on Application-type nodes. Valid
     * names for tags only contain
     *  alpha-numeric characters (i.e. no spaces and no special characters). @todo
     * enforce this!*/
    void addTag(const std::string& tag);

    /** Set the hasInitialValue flag for Application-type feeding nodes. */
    void setHasInitialValue(bool hasInitialValue);

    /** Getter for the properties */
    NodeType getType() const;
    UpdateMode getMode() const;
    VariableDirection getDirection() const;
    const std::type_info& getValueType() const;
    std::string getName() const;
    std::string getQualifiedName() const;
    const std::string& getUnit() const;
    const std::string& getDescription() const;
    VariableNetwork& getOwner() const;
    VariableNetworkNode getNodeToTrigger();
    const std::string& getPublicName() const;
    const std::string& getDeviceAlias() const;
    const std::string& getRegisterName() const;
    const std::unordered_set<std::string>& getTags() const;
    void setNumberOfElements(size_t nElements);
    size_t getNumberOfElements() const;
    ChimeraTK::TransferElementAbstractor& getAppAccessorNoType();

    void setPublicName(const std::string& name) const;

    template<typename UserType>
    ChimeraTK::NDRegisterAccessorAbstractor<UserType>& getAppAccessor() const;

    template<typename UserType>
    void setAppAccessorImplementation(boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> impl) const;

    template<typename UserType>
    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> createConstAccessor(
        AccessModeFlags accessModeFlags) const;

    /** Return the unique ID of this node (will change every time the application
     * is started). */
    const void* getUniqueId() const { return pdata.get(); }

    /** Change pointer to the accessor. May only be used for application nodes. */
    void setAppAccessorPointer(ChimeraTK::TransferElementAbstractor* accessor);

    EntityOwner* getOwningModule() const;

    void setOwningModule(EntityOwner* newOwner) const;

    void accept(Visitor<VariableNetworkNode>& visitor) const;

    /** Enum required for hasInitialValue() */
    enum class InitialValueMode { None = 0, Poll, Push };

    /** Check whether an initial value is present. This flag is valid for all NodeTypes and VariableDirections. It
     *  specifies whether an initial value is present and if yes, whether to read it with poll or push transfer mode. */
    InitialValueMode hasInitialValue() const;

    // protected:  @todo make protected again (with proper interface extension)

    boost::shared_ptr<VariableNetworkNode_data> pdata;
  };

  /*********************************************************************************************************************/

  /** A helper class to create accessors with the right length and value. We use this
   *  to create one constant accessor for each consumer so e don't have to use a fanout: The consumers might be mixed
   *  push or poll type, and as we don't have a sender/receive pair but just one side, it has to be adapted.
   *  If the constant is the feeder of the network (which conceptually always is push type), the actual implentation is
   *  the "receiving" part which is plugged into the consumer. And this needs the correct access mode flags.
   *
   *  This is the pure virtual base class to be put into the VariableNetworkNode. The actual implementation is
   *  templated to the user type.
   */
  struct ConstantAccessorCreator {
    virtual boost::shared_ptr<ChimeraTK::TransferElement> create(AccessModeFlags accessModeFlags) = 0;
    virtual ~ConstantAccessorCreator() {}
  };

  /*********************************************************************************************************************/

  /** We use a pimpl pattern so copied instances of VariableNetworkNode refer to
   * the same instance of the data structure and thus stay consistent all the
   * time. */
  struct VariableNetworkNode_data {
    VariableNetworkNode_data() {}

    /** Type of the node (Application, Device, ControlSystem, Trigger) */
    NodeType type{NodeType::invalid};

    /** Update mode: poll or push */
    UpdateMode mode{UpdateMode::invalid};

    /** Node direction: feeding or consuming */
    VariableDirection direction{VariableDirection::invalid, false};

    /** Value type of this node. If the type_info is the typeid of AnyType, the
     * actual type can be decided when making the connections. */
    const std::type_info* valueType{&typeid(AnyType)};

    /** Engineering unit. If equal to ChimeraTK::TransferElement::unitNotSet, no
     * unit has been defined (and any unit is allowed). */
    std::string unit{ChimeraTK::TransferElement::unitNotSet};

    /** Description */
    std::string description{""};

    /** The network this node belongs to */
    VariableNetwork* network{nullptr};

    /** Pointer to instance creator if type == Constant */
    boost::shared_ptr<ConstantAccessorCreator> constNodeCreator;

    /** Pointer to implementation if type == Application */
    ChimeraTK::TransferElementAbstractor* appNode{nullptr};

    /** Pointer to network which should be triggered by this node */
    VariableNetworkNode nodeToTrigger{nullptr};

    /** Pointer to the network providing the external trigger. May only be used
     * for feeding nodes with an update mode poll. When enabled, the update mode
     * will be converted into push. */
    VariableNetworkNode externalTrigger{nullptr};

    /** Public name if type == ControlSystem */
    std::string publicName;

    /** Accessor name if type == Application */
    std::string name;
    std::string qualifiedName;

    /** Device information if type == Device */
    std::string deviceAlias;
    std::string registerName;

    /** Number of elements in the variable. 0 means not yet decided. */
    size_t nElements{0};

    /** Set of tags  if type == Application */
    std::unordered_set<std::string> tags;

    /** Map to store triggered versions of this node. The map key is the trigger
     * node and the value is the node with the respective trigger added. */
    std::map<VariableNetworkNode, VariableNetworkNode> nodeWithTrigger;

    /** Pointer to the module owning this node */
    EntityOwner* owningModule{nullptr};

    /** Flag whether an initial value has been provided by the application in prepare(). This flag is only used for
     *  NodeType::Application, since the presence of an initial value for other types is fixed by the type. The flags
     *  also is only meaningful for VariableDirection::feeding. */
    bool hasInitialValue{false};
  };

  /*********************************************************************************************************************/

  // Templated implementation of the ConstantAccessorCreator
  template<typename UserType>
  struct ConstantAccessorCreatorImpl : public ConstantAccessorCreator {
    UserType value;
    size_t length;

    ConstantAccessorCreatorImpl(UserType val, size_t len) : value(val), length(len) {}

    boost::shared_ptr<ChimeraTK::TransferElement> create(AccessModeFlags accessModeFlags) override {
      return boost::make_shared<ConstantAccessor<UserType>>(value, length, accessModeFlags);
    }
  };

  /*********************************************************************************************************************/
  /*** Implementations
   * *************************************************************************************************/
  /*********************************************************************************************************************/

  template<typename UserType>
  VariableNetworkNode VariableNetworkNode::makeConstant(bool makeFeeder, UserType value, size_t length) {
    VariableNetworkNode node;
    node.pdata = boost::make_shared<VariableNetworkNode_data>();
    node.pdata->constNodeCreator.reset(new ConstantAccessorCreatorImpl<UserType>(value, length));
    node.pdata->type = NodeType::Constant;
    node.pdata->valueType = &typeid(UserType);
    node.pdata->nElements = length;
    node.pdata->name = "*UNNAMED CONSTANT*";
    if(makeFeeder) {
      node.pdata->direction = {VariableDirection::feeding, false};
      node.pdata->mode = UpdateMode::push;
    }
    else {
      node.pdata->direction = {VariableDirection::consuming, false};
      node.pdata->mode = UpdateMode::poll;
    }
    return node;
  }

  /*********************************************************************************************************************/

  template<typename UserType>
  ChimeraTK::NDRegisterAccessorAbstractor<UserType>& VariableNetworkNode::getAppAccessor() const {
    assert(typeid(UserType) == getValueType());
    assert(pdata->type == NodeType::Application);
    auto accessor = static_cast<ChimeraTK::NDRegisterAccessorAbstractor<UserType>*>(pdata->appNode);
    assert(accessor != nullptr);
    return *accessor;
  }

  /*********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> VariableNetworkNode::createConstAccessor(
      AccessModeFlags accessModeFlags) const {
    return boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<UserType>>(
        pdata->constNodeCreator->create(accessModeFlags));
  }

  /*********************************************************************************************************************/

  template<typename UserType>
  void VariableNetworkNode::setAppAccessorImplementation(boost::shared_ptr<NDRegisterAccessor<UserType>> impl) const {
    auto decorated = boost::make_shared<MetaDataPropagatingRegisterDecorator<UserType>>(impl, getOwningModule());
    getAppAccessor<UserType>().replace(decorated);
  }

} /* namespace ChimeraTK */

#endif /* CHIMERATK_VARIABLE_NETWORK_NODE_H */
