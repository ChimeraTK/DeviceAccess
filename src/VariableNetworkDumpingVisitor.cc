#include "VariableNetwork.h"
#include "VariableNetworkDumpingVisitor.h"

namespace ChimeraTK {

VariableNetworkDumpingVisitor::VariableNetworkDumpingVisitor(const std::string& prefix, std::ostream &stream)
    : Visitor<ChimeraTK::VariableNetwork>()
    , VariableNetworkNodeDumpingVisitor(stream, " ")
    , _prefix(prefix) {}

void VariableNetworkDumpingVisitor::dispatch(const VariableNetwork& t) {
    stream() << _prefix << "VariableNetwork";
    stream() << " {" << std::endl;
    stream() << _prefix << "  value type = " << boost::core::demangle(t.getValueType().name()) << ", engineering unit = " << t.getUnit() << std::endl;
    stream() << _prefix << "  trigger type = ";
    try {
      auto tt = t.getTriggerType(false);
      if(tt == VariableNetwork::TriggerType::feeder) stream() << "feeder";
      if(tt == VariableNetwork::TriggerType::pollingConsumer) stream() << "pollingConsumer";
      if(tt == VariableNetwork::TriggerType::external) stream() << "external";
      if(tt == VariableNetwork::TriggerType::none) stream() << "none";
      stream() << std::endl;
    }
    catch(ChimeraTK::logic_error &e) {
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
      if(consumer.getDirection().dir != VariableDirection::consuming) continue;
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


} // namespace ChimeraTK
