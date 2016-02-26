/*
 * ScaleRegisterPlugin.h
 *
 *  Created on: Feb 25, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_SCALE_REGISTER_PLUGIN_H
#define MTCA4U_SCALE_REGISTER_PLUGIN_H

#include "RegisterPlugin.h"
#include "BufferingRegisterAccessorImpl.h"

namespace mtca4u {

  /** RegisterPlugin to scale the register content with a given factor. */
  class ScaleRegisterPlugin : public RegisterPlugin {
  
    public:
      
      template<typename UserType>
      boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> >* getBufferingRegisterAccessorImpl(void *accessor_ptr);

      VIRTUAL_FUNCTION_TEMPLATE_DECLARATION(getBufferingRegisterAccessorImpl, void*);

      static boost::shared_ptr<RegisterPlugin> createInstance(const std::map<std::string, Value<std::string> > &parameters);

    protected:

      /** constructor, only internally called from createInstance() */
      ScaleRegisterPlugin(const std::map<std::string, Value<std::string> > &parameters);

      /** The scaling factor to multiply the data with */
      Value<double> scalingFactor;
  
  };

  /********************************************************************************************************************/
  
  /** The register accessor used by the ScaleRegisterPlugin */
  template<typename T>
  class ScaleRegisterPluginAccessor : public BufferingRegisterAccessorImpl<T> {
    public:

      /** The constructor takes the original accessor and the scaling factor as arguments */
      ScaleRegisterPluginAccessor(boost::shared_ptr< BufferingRegisterAccessorImpl<T> > accessor,
          Value<double> scalingFactor)
      : _accessor(accessor), _scalingFactor(scalingFactor)
      {}

      virtual ~ScaleRegisterPluginAccessor() {};

      virtual void read() {
        // read from hardware
        _accessor->read();
        // apply scaling factor while copying buffer from underlying accessor to our buffer
        auto itTarget = BufferingRegisterAccessorImpl<T>::cookedBuffer.begin();
        for(auto itSource = _accessor->begin(); itSource != _accessor->end(); ++itSource) {
          *itTarget = (*itSource) * _scalingFactor;
          ++itTarget;
        }
      }

      virtual void write() {
        // apply scaling factor while copying buffer from our buffer to underlying accessor
        auto itSource = BufferingRegisterAccessorImpl<T>::cookedBuffer.begin();
        for(auto itTarget = _accessor->begin(); itTarget != _accessor->end(); ++itTarget) {
          *itTarget = (*itSource) / _scalingFactor;
          ++itSource;
        }
        // write to hardware
        _accessor->write();
      }

      virtual bool isSameRegister(const boost::shared_ptr<TransferElement const> &other) const {
        auto rhsCasted = boost::dynamic_pointer_cast< const ScaleRegisterPluginAccessor<T> >(other);
        if(!rhsCasted) return false;
        if(_accessor != rhsCasted->_accessor) return false;
        if(_scalingFactor != rhsCasted->_scalingFactor) return false;
        return true;
      }

      virtual bool isReadOnly() const {
        return _accessor->isReadOnly();
      }

    protected:

      /** The underlying register accessor */
      boost::shared_ptr< BufferingRegisterAccessorImpl<T> > _accessor;

      /** The scaling factor */
      Value<double> _scalingFactor;

      virtual std::vector< boost::shared_ptr<TransferElement> > getHardwareAccessingElements() {
        return _accessor->getHardwareAccessingElements();
      }

      virtual void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) {
        if(_accessor->isSameRegister(newElement)) {
          _accessor = boost::static_pointer_cast< BufferingRegisterAccessorImpl<T> >(newElement);
        }
      }

  };

  template<>
  void ScaleRegisterPluginAccessor<std::string>::read();

  template<>
  void ScaleRegisterPluginAccessor<std::string>::write();

} /* namespace mtca4u */

#endif /* MTCA4U_REGISTER_PLUGIN_H */
