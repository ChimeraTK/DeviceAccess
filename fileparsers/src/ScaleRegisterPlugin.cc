/*
 * ScaleRegisterPlugin.cc
 *
 *  Created on: Feb 26, 2016
 *      Author: Martin Hierholzer
 */

#include "RegisterPluginFactory.h"
#include "ScaleRegisterPlugin.h"
#include "RegisterAccessor.h"

namespace mtca4u {

  /********************************************************************************************************************/

  /** Register ScaleRegisterPlugin with the RegisterPluginFactory */
  class ScaleRegisterPluginRegisterer {
    public:
      ScaleRegisterPluginRegisterer() {
        RegisterPluginFactory::getInstance().registerPlugin("scale",&ScaleRegisterPlugin::createInstance);
      }
  };
  ScaleRegisterPluginRegisterer scaleRegisterPluginRegisterer;

  /********************************************************************************************************************/

  /** The register accessor used by the ScaleRegisterPlugin */
  template<typename UserType>
  class ScaleRegisterPluginRegisterAccessor : public NDRegisterAccessor<UserType> {
    public:

      /** The constructor takes the original accessor and the scaling factor as arguments */
      ScaleRegisterPluginRegisterAccessor(boost::shared_ptr< NDRegisterAccessor<UserType> > accessor,
          DynamicValue<double> scalingFactor)
      : _accessor(accessor), _scalingFactor(scalingFactor)
      {
        // reserve space for cooked buffer
        NDRegisterAccessor<UserType>::buffer_2D.resize(_accessor->getNumberOfChannels());
        NDRegisterAccessor<UserType>::buffer_2D[0].resize(_accessor->getNumberOfSamples());
      }

      virtual ~ScaleRegisterPluginRegisterAccessor() {};

      void postRead() override {
        // apply scaling factor while copying buffer from underlying accessor to our buffer
        for(unsigned int i=0; i<NDRegisterAccessor<UserType>::buffer_2D.size(); i++) {
          for(unsigned int k=0; k<NDRegisterAccessor<UserType>::buffer_2D[i].size(); k++) {
            NDRegisterAccessor<UserType>::buffer_2D[i][k] = _accessor->accessData(i,k) * _scalingFactor;
          }
        }
      }
    
      void doReadTransfer() override {
        _accessor->read();
      }

      bool doReadTransferNonBlocking() override {
	return _accessor->readNonBlocking();
      }

      bool doReadTransferLatest() override {
        doReadTransfer();
        return true;
      }

      void preWrite() override {
        // apply scaling factor while copying buffer from our buffer to underlying accessor
        for(unsigned int i=0; i<NDRegisterAccessor<UserType>::buffer_2D.size(); i++) {
          for(unsigned int k=0; k<NDRegisterAccessor<UserType>::buffer_2D[i].size(); k++) {
            _accessor->accessData(i,k) = NDRegisterAccessor<UserType>::buffer_2D[i][k] / _scalingFactor;
          }
        }
      }

      bool write(ChimeraTK::VersionNumber versionNumber={}) override {
        preWrite();
        return _accessor->write(versionNumber);
      }

      bool isSameRegister(const boost::shared_ptr<TransferElement const> &other) const override {
        auto rhsCasted = boost::dynamic_pointer_cast< const ScaleRegisterPluginRegisterAccessor<UserType> >(other);
        if(!rhsCasted) return false;
        if(_accessor != rhsCasted->_accessor) return false;
        if(_scalingFactor != rhsCasted->_scalingFactor) return false;
        return true;
      }

      bool isReadOnly() const override {
        return _accessor->isReadOnly();
      }

      bool isReadable() const override {
        return true;
      }

      bool isWriteable() const override {
        return true;
      }

      FixedPointConverter getFixedPointConverter() const override {
        return _accessor->getFixedPointConverter();
      }

    protected:

      /** The underlying register accessor */
      boost::shared_ptr< NDRegisterAccessor<UserType> > _accessor;

      /** The scaling factor */
      DynamicValue<double> _scalingFactor;

      std::vector< boost::shared_ptr<TransferElement> > getHardwareAccessingElements() override {
        return _accessor->getHardwareAccessingElements();
      }

      void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) override {
        if(_accessor->isSameRegister(newElement)) {
          _accessor = boost::static_pointer_cast< NDRegisterAccessor<UserType> >(newElement);
        }
      }

  };

  template<>
  void ScaleRegisterPluginRegisterAccessor<std::string>::postRead();

  template<>
  void ScaleRegisterPluginRegisterAccessor<std::string>::preWrite();

  /********************************************************************************************************************/

  ScaleRegisterPlugin::ScaleRegisterPlugin(const std::map<std::string, DynamicValue<std::string> > &parameters) {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(decorateRegisterAccessor_impl);
    try {
      scalingFactor = parameters.at("factor");
    }
    catch(std::out_of_range &e) {
      throw DeviceException("ScaleRegisterPlugin: Missing parameter 'factor'.", DeviceException::WRONG_PARAMETER);
    }
  }

  /********************************************************************************************************************/

  boost::shared_ptr<RegisterInfoPlugin> ScaleRegisterPlugin::createInstance(
      const std::map<std::string, DynamicValue<std::string> > &parameters) {
    return boost::shared_ptr<RegisterInfoPlugin>(new ScaleRegisterPlugin(parameters));
  }

  /********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr< NDRegisterAccessor<UserType> > ScaleRegisterPlugin::decorateRegisterAccessor_impl(
      boost::shared_ptr< NDRegisterAccessor<UserType> > accessor) const {
    return boost::shared_ptr< NDRegisterAccessor<UserType> >(
        new ScaleRegisterPluginRegisterAccessor<UserType>(accessor, scalingFactor));
  }

  /********************************************************************************************************************/

  template<>
  void ScaleRegisterPluginRegisterAccessor<std::string>::postRead() {
    // apply scaling factor while copying buffer from underlying accessor to our buffer
    for(unsigned int i=0; i<NDRegisterAccessor<std::string>::buffer_2D.size(); i++) {
      for(unsigned int k=0; k<NDRegisterAccessor<std::string>::buffer_2D[i].size(); k++) {
        NDRegisterAccessor<std::string>::buffer_2D[i][k] =
            std::to_string(std::stod(_accessor->accessData(i,k)) * _scalingFactor);
      }
    }
  }

  /********************************************************************************************************************/

  template<>
  void ScaleRegisterPluginRegisterAccessor<std::string>::preWrite() {
    // apply scaling factor while copying buffer from our buffer to underlying accessor
    for(unsigned int i=0; i<NDRegisterAccessor<std::string>::buffer_2D.size(); i++) {
      for(unsigned int k=0; k<NDRegisterAccessor<std::string>::buffer_2D[i].size(); k++) {
        _accessor->accessData(i,k) =
            std::to_string(std::stod(NDRegisterAccessor<std::string>::buffer_2D[i][k]) / _scalingFactor);
      }
    }
  }

} /* namespace mtca4u */
