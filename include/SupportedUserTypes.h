/*
 * SupportedUserTypes.h
 *
 *  Created on: Jun 14, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_APPLICATION_CORE_SUPPORTED_USER_TYPES_H
#define CHIMERATK_APPLICATION_CORE_SUPPORTED_USER_TYPES_H

namespace ChimeraTK {

/** Map of UserType to value of the UserType. */
typedef boost::fusion::map<
    boost::fusion::pair<int8_t, int8_t>, boost::fusion::pair<uint8_t, uint8_t>,
    boost::fusion::pair<int16_t, int16_t>,
    boost::fusion::pair<uint16_t, uint16_t>,
    boost::fusion::pair<int32_t, int32_t>,
    boost::fusion::pair<uint32_t, uint32_t>, boost::fusion::pair<float, float>,
    boost::fusion::pair<double, double>>
    ApplicationCoreUserTypeMap;

/** Map of UserType to a class template with the UserType as template argument.
 */
template <template <typename> class TemplateClass>
class ApplicationCoreTemplateUserTypeMap {
public:
  boost::fusion::map<boost::fusion::pair<int8_t, TemplateClass<int8_t>>,
                     boost::fusion::pair<uint8_t, TemplateClass<uint8_t>>,
                     boost::fusion::pair<int16_t, TemplateClass<int16_t>>,
                     boost::fusion::pair<uint16_t, TemplateClass<uint16_t>>,
                     boost::fusion::pair<int32_t, TemplateClass<int32_t>>,
                     boost::fusion::pair<uint32_t, TemplateClass<uint32_t>>,
                     boost::fusion::pair<float, TemplateClass<float>>,
                     boost::fusion::pair<double, TemplateClass<double>>>
      table;
};

} /* namespace ChimeraTK */

#endif /* CHIMERATK_APPLICATION_CORE_SUPPORTED_USER_TYPES_H */
