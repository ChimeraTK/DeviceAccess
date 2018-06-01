#include "Application.h"
#include "ApplicationException.h"
#include "Module.h"
#include "VariableNetwork.h"
#include "VariableNetworkNode.h"
#include "Visitor.h"

#include <libxml++/libxml++.h>

#ifdef HAVE_DEMANGLE
#include <cxxabi.h>
#endif

namespace {

#ifdef HAVE_DEMANGLE
/**
 * @brief demangle
 * @param name a type name (e.g. the result of std::type_info::name()
 * @return a pretty-print type name if possible
 */
std::string demangle(const char* name) {
    int status = 0;

    std::unique_ptr<char, decltype (&::free)> pointer(abi::__cxa_demangle(name, nullptr, nullptr, &status), ::free);

    return status == 0 ? pointer.get() : name;
}
#else
std::string demangle(const char *name) {
    return name;
}
#endif

/**
 * @brief encodeName mangle a name to be suitable for Graphviz files
 * @param name to mangle
 * @return an encoded node name usable in Graphviz files
 */
std::string encodeName(std::string name) {
    std::replace(name.begin(), name.end(), ':', '_');
    std::replace(name.begin(), name.end(), '/', 'y');
    std::replace(name.begin(), name.end(), '.', 't');
    std::replace(name.begin(), name.end(), ' ', 's');

    return name;
}

// TODO: Move to VariableNetworkNode?
std::string nodeName(const ChimeraTK::VariableNetworkNode &node) {
    return node.getQualifiedName().empty() ? node.getName() : node.getQualifiedName();
}

}

namespace ChimeraTK {

VariableNetworkNodeDumpingVisitor::VariableNetworkNodeDumpingVisitor(std::ostream &stream, const std::string& separator)
    : Visitor<ChimeraTK::VariableNetworkNode> ()
    , PushableStream(stream)
    , _separator(separator) {}

void VariableNetworkNodeDumpingVisitor::dispatch(const VariableNetworkNode& t) {
    if(t.getType() == NodeType::Application) stream() << " type = Application ('" << t.getQualifiedName() << "')";
    if(t.getType() == NodeType::ControlSystem) stream() << " type = ControlSystem ('" << t.getPublicName() << "')";
    if(t.getType() == NodeType::Device) stream() << " type = Device (" << t.getDeviceAlias() << ": " << t.getRegisterName() << ")";
    if(t.getType() == NodeType::TriggerReceiver) stream() << " type = TriggerReceiver";
    if(t.getType() == NodeType::Constant) stream() << " type = Constant";
    if(t.getType() == NodeType::invalid) stream() << " type = **invalid**";

    if(t.getMode() == UpdateMode::push) stream() << _separator << "pushing";
    if(t.getMode() == UpdateMode::poll) stream() << _separator << "polling";
    stream() << _separator;

    stream() << "data type: " << demangle(t.getValueType().name());
    stream() << _separator;
    stream() << "length: " << t.getNumberOfElements();
    stream() << _separator;

    stream() << "[ptr: " << &(*(t.pdata)) << "]";
    stream() << _separator;

    stream() << "tags: [";
    bool first = true;
    for(auto &tag : t.getTags()) {
        stream() << tag;
        if (!first) stream() << ",";
        first = false;
    }
    stream() << "]";
    stream() << _separator;

    stream() << std::endl;
}

VariableNetworkDumpingVisitor::VariableNetworkDumpingVisitor(const std::string& prefix, std::ostream &stream)
    : Visitor<ChimeraTK::VariableNetwork>()
    , VariableNetworkNodeDumpingVisitor(stream, " ")
    , _prefix(prefix) {}

void VariableNetworkDumpingVisitor::dispatch(const VariableNetwork& t) {
    stream() << _prefix << "VariableNetwork";
    stream() << " [ptr: " << this << "]";
    stream() << " {" << std::endl;
    stream() << _prefix << "  value type = " << demangle(t.getValueType().name()) << ", engineering unit = " << t.getUnit() << std::endl;
    stream() << _prefix << "  trigger type = ";
    try {
      auto tt = t.getTriggerType(false);
      if(tt == VariableNetwork::TriggerType::feeder) stream() << "feeder" << std::endl;
      if(tt == VariableNetwork::TriggerType::pollingConsumer) stream() << "pollingConsumer" << std::endl;
      if(tt == VariableNetwork::TriggerType::external) stream() << "external" << std::endl;
      if(tt == VariableNetwork::TriggerType::none) stream() << "none" << std::endl;
    }
    catch(ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork> &e) {
      stream() << "**error**" << std::endl;
    }
    stream() << _prefix << "  feeder";
    if(t.hasFeedingNode()) {
      t.getFeedingNode().accept(*this);
    }
    else {
      stream() << " **error, no feeder found**" << std::endl;
    }
    stream() << _prefix << "  consumers: " << t.countConsumingNodes() << std::endl;
    size_t count = 0;
    for(auto &consumer : t.getConsumingNodes()) {
      if(consumer.getDirection() != VariableDirection::consuming) continue;
      stream() << _prefix << "    # " << ++count << ":";
      consumer.accept(*this);
    }
    if(t.hasFeedingNode()) {
      if(t.getFeedingNode().hasExternalTrigger()) {
        stream() << _prefix << "  external trigger node: ";
        t.getFeedingNode().getExternalTrigger().accept(*this);
      }
    }
    stream() << _prefix << "}" << std::endl;
}


VariableNetworkGraphDumpingVisitor::VariableNetworkGraphDumpingVisitor(std::ostream& stream)
    : Visitor<Application, VariableNetwork> ()
    , VariableNetworkNodeDumpingVisitor(stream, "\\n")
    , _triggerMap()
    , _triggerConnections()
    , _networkCount(0)
    , _triggerCount(0) {}


void VariableNetworkGraphDumpingVisitor::dispatch(const Application& t) {
    stream() << "digraph application {\n"
         //<< "  rankdir = LR;\n"
         << "  fontname=\"Sans\";\n"
         << "  fontsize=\"10\";\n"
         << "  labelloc=t;\n"
         << "  nodesep=1;\n"
         //<< "  splines=ortho;\n"
         << "  concentrate=true;\n"
         << "  label=\"<" << demangle(typeid(t).name()) << ">" << t.getName() << "\";\n"
         << "  node [style=\"filled,rounded\", shape=box, fontsize=\"9\", fontname=\"sans\"];\n"
         << "  edge [labelfontsize=\"6\", fontsize=\"9\", fontname=\"monospace\"];\n"
         << "  \n";

    for (auto &network : t.networkList) {
        network.accept(*this);
    }

    for (auto &t : _triggerMap) {
        stream() << t.second;
    }

    for (auto &t : _triggerConnections) {
        stream() << t;
    }

    stream() << "}\n";
}

void VariableNetworkGraphDumpingVisitor::dispatch(const VariableNetwork& network) {
    std::string networkPrefix = "network_" + std::to_string(_networkCount++);
    pushPrefix(networkPrefix);

    stream() << "  subgraph cluster_" << prefix() << " {\n"
         << "    fontsize=\"8\";\n"
         << "    style=\"filled,rounded\";\n"
         << "    color=black;\n"
         << "    fillcolor=white;\n"
         << "    ordering=out;\n"
         << "    label=\"" << demangle(typeid(network).name()) << "(" << &network << ")\\n"
         <<              network.getDescription() << "\\n"
         <<              "value type = " << demangle(network.getValueType().name()) << "\\n"
         <<              "engineering unit = " << network.getUnit() << "\";\n";

    std::string feeder;
    std::string trigger;
    if (network.hasFeedingNode()) {
        auto feederNode = network.getFeedingNode();
        // We are inside a trigger network... Consumers will be skipped below. Make the feeder a "global" variable
        if (!network.getConsumingNodes().empty() && network.getConsumingNodes().begin()->getType() == NodeType::TriggerReceiver) {
            feeder = encodeName(nodeName(feederNode));
            if (_triggerMap.find(feeder) == _triggerMap.end()) {
                std::stringstream ss;
                pushStream(ss); pushPrefix("");
                feederNode.accept(*this);
                _triggerMap[feeder] = ss.str();
                popPrefix(); popStream();
            }
        } else {
            feeder = prefix() + encodeName(nodeName(feederNode));
            feederNode.accept(*this);
        }

        if (feederNode.hasExternalTrigger()) {
            auto triggerNode = feederNode.getExternalTrigger();
            trigger = encodeName(nodeName(triggerNode));
            if (_triggerMap.find(trigger) == _triggerMap.end()) {
                std::stringstream ss;
                pushStream(ss); pushPrefix("");
                triggerNode.accept(*this);
                _triggerMap[trigger] = ss.str();
                popPrefix(); popStream();
            }
        }
    }

    for (auto &consumerNode : network.getConsumingNodes()) {
        if (consumerNode.getDirection() != VariableDirection::consuming)
            continue;

        if (consumerNode.getType() == NodeType::TriggerReceiver)
            continue;

        auto consumer = prefix() + encodeName(nodeName(consumerNode));
        consumerNode.accept(*this);
        std::string helperNode;

        if (!trigger.empty()) {
            // Create trigger connection diamond
            _triggerCount++;
            helperNode = prefix() + "_trigger_helper_" + std::to_string(_triggerCount);
            stream() << helperNode << "[label=\"\",shape=diamond,style=\"filled\",color=black,width=.3,height=.3,fixedsize=true,fillcolor=\"#ffcc00\"]\n";
        }

        stream() << feeder << " -> ";
        if (!helperNode.empty()) {
            stream() << helperNode << " -> " << consumer << "\n";
            // Hack: Make trigger lower in rank than all entry points to subgraphs
            _triggerConnections.push_back(trigger + " -> " + feeder + "[style = invis]\n");

            _triggerConnections.push_back(trigger + " -> " + helperNode + "[style = dashed, color=grey,tailport=s]\n");
        } else {
            stream() << consumer << "\n";
        }
    }

    stream() << "}\n";
    popPrefix();
}

void VariableNetworkGraphDumpingVisitor::dispatch(const VariableNetworkNode& t) {
    std::string nodeName = prefix() + encodeName(::nodeName(t));
    stream() << nodeName << "[\n";
    if (t.getMode() == UpdateMode::push)
        stream() << "    fillcolor=\"#ff9900\";\n";
    else if (t.getMode() == UpdateMode::poll)
        stream() << "    fillcolor=\"#b4d848\";\n";
    else {
        stream() << "    fillcolor=\"#ffffff\";\n";
    }

    stream() << "    label=\"" << ::nodeName(t) << "\\n";

    VariableNetworkNodeDumpingVisitor::dispatch(t);

    stream() << "\"]\n";
}

XMLGeneratorVisitor::XMLGeneratorVisitor()
    : Visitor<ChimeraTK::Application, ChimeraTK::VariableNetworkNode>()
    , _doc(std::make_shared<xmlpp::Document>())
    , _rootElement(_doc->create_root_node("application", "https://github.com/ChimeraTK/ApplicationCore"))
{
}

void XMLGeneratorVisitor::save(const std::string &fileName) {
    _doc->write_to_file_formatted(fileName);
}

void XMLGeneratorVisitor::dispatch(const Application& app) {
    _rootElement->set_attribute("name", app.getName());
    for (auto &network : app.networkList) {
        network.check();

        auto feeder = network.getFeedingNode();
        feeder.accept(*this);

        for (auto &consumer : network.getConsumingNodes()) {
            consumer.accept(*this);
        }
    }
}

void XMLGeneratorVisitor::dispatch(const VariableNetworkNode &node) {
    if(node.getType() != NodeType::ControlSystem) return;

    // Create the directory for the path name in the XML document with all parent directories, if not yet existing:
    // First split the publication name into components and loop over each component. For each component, try to find
    // the directory node and create it it does not exist. After the loop, the "current" will point to the Element
    // representing the directory.

    // strip the variable name from the path
    mtca4u::RegisterPath directory(node.getPublicName());
    directory--;

    // the namespace map is needed to properly refer to elements with an xpath expression in xmlpp::Element::find()
    /// @todo TODO move this somewhere else, or at least take the namespace URI from a common place!
    xmlpp::Node::PrefixNsMap nsMap{{"ac", "https://github.com/ChimeraTK/ApplicationCore"}};

    // go through each directory path component
    xmlpp::Element *current = _rootElement;
    for(auto pathComponent : directory.getComponents()) {
      // find directory for this path component in the current directory
      std::string xpath = std::string("ac:directory[@name='")+pathComponent+std::string("']");
      auto list = current->find(xpath, nsMap);
      if(list.size() == 0) {  // not found: create it
        xmlpp::Element *newChild = current->add_child("directory");
        newChild->set_attribute("name",pathComponent);
        current = newChild;
      }
      else {
        assert(list.size() == 1);
        current = dynamic_cast<xmlpp::Element*>(list[0]);
        assert(current != nullptr);
      }
    }

    // now add the variable to the directory
    xmlpp::Element *variable = current->add_child("variable");
    mtca4u::RegisterPath pathName(node.getPublicName());
    auto pathComponents = pathName.getComponents();

    // set the name attribute
    variable->set_attribute("name",pathComponents[pathComponents.size()-1]);

    // add sub-element containing the data type
    std::string dataTypeName{"unknown"};
    auto &owner = node.getOwner();
    auto &type = owner.getValueType();
    if(type == typeid(int8_t)) { dataTypeName = "int8"; }
    else if(type == typeid(uint8_t)) { dataTypeName = "uint8"; }
    else if(type == typeid(int16_t)) { dataTypeName = "int16"; }
    else if(type == typeid(uint16_t)) { dataTypeName = "uint16"; }
    else if(type == typeid(int32_t)) { dataTypeName = "int32"; }
    else if(type == typeid(uint32_t)) { dataTypeName = "uint32"; }
    else if(type == typeid(float)) { dataTypeName = "float"; }
    else if(type == typeid(double)) { dataTypeName = "double"; }
    else if(type == typeid(std::string)) { dataTypeName = "string"; }
    xmlpp::Element *valueTypeElement = variable->add_child("value_type");
    valueTypeElement->set_child_text(dataTypeName);

    // add sub-element containing the data flow direction
    std::string dataFlowName{"application_to_control_system"};
    if(owner.getFeedingNode() == node) { dataFlowName = "control_system_to_application"; }
    xmlpp::Element *directionElement = variable->add_child("direction");
    directionElement->set_child_text(dataFlowName);

    // add sub-element containing the engineering unit
    xmlpp::Element *unitElement = variable->add_child("unit");
    unitElement->set_child_text(owner.getUnit());

    // add sub-element containing the description
    xmlpp::Element *descriptionElement = variable->add_child("description");
    descriptionElement->set_child_text(owner.getDescription());

    // add sub-element containing the description
    xmlpp::Element *nElementsElement = variable->add_child("numberOfElements");
    nElementsElement->set_child_text(std::to_string(owner.getFeedingNode().getNumberOfElements()));
}

ModuleGraphVisitor::ModuleGraphVisitor(std::ostream& stream, bool showVariables)
    : Visitor<ChimeraTK::EntityOwner, ChimeraTK::Module, ChimeraTK::VariableNetworkNode> ()
    , _stream(stream)
    , _showVariables(showVariables) {}

void ModuleGraphVisitor::dispatch(const EntityOwner &owner) {
    /* If we start with an entity owner, consider ourselves the start of a graph */
    /* When descending from here, we only use Module directly */
    _stream << "digraph G {" << "\n";
    dumpEntityOwner(owner);
    _stream << "}" << std::endl;
}

void ModuleGraphVisitor::dispatch(const VariableNetworkNode &node) {
    std::string dotNode = encodeName(node.getQualifiedName());
    _stream << "  " <<  dotNode << "[label=\"{" << node.getName() << "| {";
    bool first = true;
    for(auto tag : node.getTags()) {
      if(!first) {
        _stream << "|";
      }
      else {
        first = false;
      }
      _stream << tag;
    }
    _stream << "}}\", shape=record]" << std::endl;
}

void ModuleGraphVisitor::dispatch(const Module &module) {
    dumpEntityOwner(static_cast<const EntityOwner &>(module));
}

void ModuleGraphVisitor::dumpEntityOwner(const EntityOwner &module) {
    std::string myDotNode = encodeName(module.getQualifiedName());
    _stream << "  " << myDotNode << "[label=\"" << module.getName() << "\"";
    if(module.getEliminateHierarchy()) {
      _stream << ",style=dotted";
    }
    if(module.getModuleType() == EntityOwner::ModuleType::ModuleGroup) {
      _stream << ",peripheries=2";
    }
    if(module.getModuleType() == EntityOwner::ModuleType::ApplicationModule) {
      _stream << ",penwidth=3";
    }
    _stream << "]" << std::endl;

    if(_showVariables) {
      for(auto &node : module.getAccessorList()) {
          std::string dotNode = encodeName(nodeName(node));
          node.accept(*this);
          _stream << "  " << myDotNode << " -> " << dotNode << std::endl;
      }
    }

    for(const Module *submodule : module.getSubmoduleList()) {
      if(submodule->getModuleType() == EntityOwner::ModuleType::Device ||
         submodule->getModuleType() == EntityOwner::ModuleType::ControlSystem) continue;
      std::string dotNode = encodeName(submodule->getQualifiedName());
      _stream << "  " << myDotNode <<  " -> " << dotNode << std::endl;
      submodule->accept(*this);
    }
}
}
