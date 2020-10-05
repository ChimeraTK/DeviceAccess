#include "LNMDoubleBufferPlugin.h"

std::string getValue(const std::map<std::string, std::string>& m, std::string key) {
  auto it = m.find(key);
  if(it != m.end()) {
    return it->second;
  }
  else {
    std::string message =
        "LogicalNameMappingBackend DoubleBufferPlugin: Missing parameter " + std::string("'") + key + "'.";
    throw ChimeraTK::logic_error(message);
  }
}

namespace ChimeraTK { namespace LNMBackend {
  DoubleBufferPlugin::DoubleBufferPlugin(
      boost::shared_ptr<LNMBackendRegisterInfo> info, std::map<std::string, std::string> parameters)
  : AccessorPlugin(info), _parameters(std::move(parameters)) {}

  void DoubleBufferPlugin::updateRegisterInfo() {
    auto p = _info.lock();
    if(p) {
      p->writeable = false;
    }
  }

  /*************************************************/
  /** Helper class to implement MultiplierPlugin::decorateAccessor (can later be realised with if constexpr) */
  template<typename UserType, typename TargetType>
  struct DoubleBufferPlugin_Helper {
    static boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<LogicalNameMappingBackend>& backend, boost::shared_ptr<NDRegisterAccessor<TargetType>>&,
        const std::map<std::string, std::string>&) {
      assert(false); // only specialisation is valid
      return {};
    }
  };
  template<typename UserType>
  struct DoubleBufferPlugin_Helper<UserType, double> {
    static boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<LogicalNameMappingBackend>& backend, boost::shared_ptr<NDRegisterAccessor<double>>& target,
        const std::map<std::string, std::string>& parameters) {
      return boost::make_shared<DoubleBufferAccessor<UserType>>(backend, target, parameters);
    }
  };
  /*************************************************/
  template<typename UserType, typename TargetType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> DoubleBufferPlugin::decorateAccessor(
      boost::shared_ptr<LogicalNameMappingBackend>& backend,
      boost::shared_ptr<NDRegisterAccessor<TargetType>>& target) const {
    return DoubleBufferPlugin_Helper<UserType, TargetType>::decorateAccessor(backend, target, _parameters);
  }

  template<typename UserType>
  DoubleBufferAccessor<UserType>::DoubleBufferAccessor(boost::shared_ptr<LogicalNameMappingBackend>& backend,
      boost::shared_ptr<NDRegisterAccessor<double>>& target, const std::map<std::string, std::string>& parameters)
  : ChimeraTK::NDRegisterAccessorDecorator<UserType, double>(target) {
    _bufferRegister = backend->getRegisterAccessor<double>(getValue(parameters, "bufferregister"), 0, 0, false);
    _controlRegister = backend->getRegisterAccessor<double>(getValue(parameters, "controlregister"), 0, 0, false);
    _statusRegister = backend->getRegisterAccessor<double>(getValue(parameters, "statusregister"), 0, 0, false);
  }

  template<typename UserType>
  void DoubleBufferAccessor<UserType>::doPreRead(TransferType type) {
    _controlRegister = 0;
    _controlRegister->write();
    _target->preRead(type);
  }

  template<typename UserType>
  void DoubleBufferAccessor<UserType>::doReadTransferSynchronously() {
    _statusRegister->read();
    if(_statusRegister == 0) {
      _bufferRegister->read();
      buffer_2D[0][0] = static_cast<double>(_bufferRegister);
    }
    else if(_statusRegister == 1) {
      _target->read();
    }
  } 

  template<typename UserType>
  void DoubleBufferAccessor<UserType>:: doPostRead(TransferType type, bool hasNewData) override{
    _controlRegister = 1;
    _controlRegister->write();
  }

}} // namespace ChimeraTK::LNMBackend
