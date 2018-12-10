
#include "VariableNetworkNodeDumpingVisitor.h"
#include "VariableNetworkNode.h"

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

    stream() << "data type: " << boost::core::demangle(t.getValueType().name());
    stream() << _separator;
    stream() << "length: " << t.getNumberOfElements();
    stream() << _separator;

    stream() << "[ptr: " << &(*(t.pdata)) << "]";
    stream() << _separator;

    stream() << "tags: [";
    bool first = true;
    for(auto &tag : t.getTags()) {
      if (!first) stream() << ",";
      stream() << tag;
      first = false;
    }
    stream() << "]";
    stream() << _separator;

    stream() << std::endl;
}

} // namespace ChimeraTK
