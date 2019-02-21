#include <libxml++/libxml++.h>

#include "ConfigReader.h"

namespace ChimeraTK {

/*********************************************************************************************************************/

/** Functor to fill variableMap */
struct FunctorFill {
  FunctorFill(ConfigReader *owner, const std::string &type,
              const std::string &name, const std::string &value,
              bool &processed)
      : _owner(owner), _type(type), _name(name), _value(value),
        _processed(processed) {}

  template <typename PAIR> void operator()(PAIR &) const {

    // extract the user type from the pair
    typedef typename PAIR::first_type T;

    // skip this type, if not matching the type string in the config file
    if (_type != boost::fusion::at_key<T>(_owner->typeMap))
      return;

    _owner->createVar<T>(_name, _value);
    _processed = true;
  }

  ConfigReader *_owner;
  const std::string &_type, &_name, &_value;
  bool &_processed; // must be a non-const reference, since we want to return
                    // this to the caller

  typedef boost::fusion::pair<std::string, ConfigReader::Var<std::string>>
      StringPair;
};

/*********************************************************************************************************************/

/** Functor to fill variableMap for arrays */
struct ArrayFunctorFill {
  ArrayFunctorFill(ConfigReader *owner, const std::string &type,
                   const std::string &name,
                   const std::map<size_t, std::string> &values, bool &processed)
      : _owner(owner), _type(type), _name(name), _values(values),
        _processed(processed) {}

  template <typename PAIR> void operator()(PAIR &) const {

    // extract the user type from the pair
    typedef typename PAIR::first_type T;

    // skip this type, if not matching the type string in the config file
    if (_type != boost::fusion::at_key<T>(_owner->typeMap))
      return;

    _owner->createArray<T>(_name, _values);
    _processed = true;
  }

  ConfigReader *_owner;
  const std::string &_type, &_name;
  const std::map<size_t, std::string> &_values;
  bool &_processed; // must be a non-const reference, since we want to return
                    // this to the caller

  typedef boost::fusion::pair<std::string, ConfigReader::Var<std::string>>
      StringPair;
};

/*********************************************************************************************************************/

template <typename T>
void ConfigReader::createVar(const std::string &name,
                             const std::string &value) {
  // convert value into user type
  /// @todo error handling!
  std::stringstream stream(value);
  T convertedValue;
  if (typeid(T) == typeid(int8_t) ||
      typeid(T) ==
          typeid(uint8_t)) { // prevent interpreting int8-types as characters
    int16_t intermediate;
    stream >> intermediate;
    convertedValue = intermediate;
  } else { // note: string is done in template specialisation
    stream >> convertedValue;
  }

  // place the variable onto the vector
  std::map<std::string, ConfigReader::Var<T>> &theMap =
      boost::fusion::at_key<T>(variableMap.table);
  theMap.emplace(
      std::make_pair(name, ConfigReader::Var<T>(this, name, convertedValue)));
}

/*********************************************************************************************************************/

template <>
void ConfigReader::createVar<std::string>(const std::string &name,
                                          const std::string &value) {
  // place the variable onto the vector
  std::map<std::string, ConfigReader::Var<std::string>> &theMap =
      boost::fusion::at_key<std::string>(variableMap.table);
  theMap.emplace(
      std::make_pair(name, ConfigReader::Var<std::string>(this, name, value)));
}

/*********************************************************************************************************************/

/*********************************************************************************************************************/

template <typename T>
void ConfigReader::createArray(const std::string &name,
                               const std::map<size_t, std::string> &values) {
  std::vector<T> Tvalues;

  size_t expectedIndex = 0;
  for (auto it = values.begin(); it != values.end(); ++it) {

    // check index (std::map should be ordered by the index)
    if (it->first != expectedIndex) {
      parsingError("Array index " + std::to_string(expectedIndex) +
                   " not found, but " + std::to_string(it->first) +
                   " was. "
                   "Sparse arrays are not supported!");
    }
    ++expectedIndex;

    // convert value into user type
    std::stringstream stream(it->second);
    T convertedValue;
    if (typeid(T) == typeid(int8_t) ||
        typeid(T) ==
            typeid(uint8_t)) { // prevent interpreting int8-types as characters
      int16_t intermediate;
      stream >> intermediate;
      convertedValue = intermediate;
    } else { // note: string is done in template specialisation
      stream >> convertedValue;
    }

    // store value in vector
    Tvalues.push_back(convertedValue);
  }

  // place the variable onto the vector
  std::map<std::string, ConfigReader::Array<T>> &theMap =
      boost::fusion::at_key<T>(arrayMap.table);
  theMap.emplace(
      std::make_pair(name, ConfigReader::Array<T>(this, name, Tvalues)));
}

/*********************************************************************************************************************/

template <>
void ConfigReader::createArray<std::string>(
    const std::string &name, const std::map<size_t, std::string> &values) {
  std::vector<std::string> Tvalues;

  size_t expectedIndex = 0;
  for (auto it = values.begin(); it != values.end(); ++it) {

    // check index (std::map should be ordered by the index)
    if (it->first != expectedIndex) {
      parsingError("Array index " + std::to_string(expectedIndex) +
                   " not found, but " + std::to_string(it->first) +
                   " was. "
                   "Sparse arrays are not supported!");
    }
    ++expectedIndex;

    // store value in vector
    Tvalues.push_back(it->second);
  }

  // place the variable onto the vector
  std::map<std::string, ConfigReader::Array<std::string>> &theMap =
      boost::fusion::at_key<std::string>(arrayMap.table);
  theMap.emplace(std::make_pair(
      name, ConfigReader::Array<std::string>(this, name, Tvalues)));
}

/*********************************************************************************************************************/

ConfigReader::ConfigReader(EntityOwner *owner, const std::string &name,
                           const std::string &fileName,
                           const std::unordered_set<std::string> &tags)
    : ApplicationModule(owner, name,
                        "Configuration read from file '" + fileName + "'",
                        false, tags),
      _fileName(fileName) {
  // parse the file into a DOM structure
  xmlpp::DomParser parser;
  try {
    parser.parse_file(fileName);
  } catch (xmlpp::exception &e) { /// @todo change exception!
    throw ChimeraTK::logic_error(
        "ConfigReader: Error opening the config file '" + fileName +
        "': " + e.what());
  }

  // get root element
  const auto root = parser.get_document()->get_root_node();
  if (root->get_name() != "configuration") {
    parsingError("Expected 'configuration' tag instead of: " +
                 root->get_name());
  }

  // parsing loop
  for (const auto &child : root->get_children()) {
    // cast into element, ignore if not an element (e.g. comment)
    const xmlpp::Element *element = dynamic_cast<const xmlpp::Element *>(child);
    if (!element)
      continue;
    if (element->get_name() != "variable") {
      parsingError("Expected 'variable' tag instead of: " + root->get_name());
    }

    // obtain attributes from the element
    auto name = element->get_attribute("name");
    if (!name)
      parsingError("Missing attribute 'name' for the 'variable' tag.");
    auto type = element->get_attribute("type");
    if (!type)
      parsingError("Missing attribute 'type' for the 'variable' tag.");

    // scalar value: obtain value from attribute
    auto value = element->get_attribute("value");
    if (value) {
      // create accessor and store value in map using the functor
      bool processed{false};
      boost::fusion::for_each(variableMap.table,
                              FunctorFill(this, type->get_value(),
                                          name->get_value(), value->get_value(),
                                          processed));
      if (!processed)
        parsingError("Incorrect value '" + type->get_value() +
                     "' for attribute 'type' of the 'variable' tag.");
    }
    // array value: obtain values from child elements
    else {
      bool valueFound = false;
      std::map<size_t, std::string> values;
      for (const auto &valueChild : child->get_children()) {

        // obtain value child element and extract index and value from
        // attributes
        const xmlpp::Element *valueElement =
            dynamic_cast<const xmlpp::Element *>(valueChild);
        if (!valueElement)
          continue; // ignore comments etc.
        if (valueElement->get_name() != "value") {
          parsingError("Expected 'value' tag instead of: " + root->get_name());
        }
        valueFound = true;
        auto index = valueElement->get_attribute("i");
        if (!index)
          parsingError("Missing attribute 'index' for the 'value' tag.");
        auto value = valueElement->get_attribute("v");
        if (!value)
          parsingError("Missing attribute 'value' for the 'value' tag.");

        // get index as number and store value as a string
        size_t intIndex;
        try {
          intIndex = std::stoi(index->get_value());
        } catch (std::exception &e) {
          parsingError("Cannot parse string '" +
                       std::string(index->get_value()) +
                       "' as an index number: " + e.what());
        }
        values[intIndex] = value->get_value();
      }

      // make sure there is at least one value
      if (!valueFound) {
        parsingError("Each variable must have a value, either specified as an "
                     "attribute or as child tags.");
      }

      // create accessor and store array value in map using functor
      bool processed{false};
      boost::fusion::for_each(variableMap.table,
                              ArrayFunctorFill(this, type->get_value(),
                                               name->get_value(), values,
                                               processed));
      if (!processed)
        parsingError("Incorrect value '" + type->get_value() +
                     "' for attribute 'type' of the 'variable' tag.");
    }
  }
}

/********************************************************************************************************************/

void ConfigReader::parsingError(const std::string &message) {
  throw ChimeraTK::logic_error("ConfigReader: Error parsing the config file '" +
                               _fileName + "': " + message);
}

/*********************************************************************************************************************/

/** Functor to set values to the scalar accessors */
struct FunctorSetValues {
  FunctorSetValues(ConfigReader *owner) : _owner(owner) {}

  template <typename PAIR> void operator()(PAIR &) const {

    // get user type and vector
    typedef typename PAIR::first_type T;
    std::map<std::string, ConfigReader::Var<T>> &theMap =
        boost::fusion::at_key<T>(_owner->variableMap.table);

    // iterate vector and set values
    for (auto &pair : theMap) {
      auto &var = pair.second;
      var._accessor = var._value;
      var._accessor.write();
    }
  }

  ConfigReader *_owner;
};

/*********************************************************************************************************************/

/** Functor to set values to the array accessors */
struct FunctorSetValuesArray {
  FunctorSetValuesArray(ConfigReader *owner) : _owner(owner) {}

  template <typename PAIR> void operator()(PAIR &) const {

    // get user type and vector
    typedef typename PAIR::first_type T;
    std::map<std::string, ConfigReader::Array<T>> &theMap =
        boost::fusion::at_key<T>(_owner->arrayMap.table);

    // iterate vector and set values
    for (auto &pair : theMap) {
      auto &var = pair.second;
      var._accessor = var._value;
      var._accessor.write();
    }
  }

  ConfigReader *_owner;
};

/*********************************************************************************************************************/

void ConfigReader::prepare() {
  boost::fusion::for_each(variableMap.table, FunctorSetValues(this));
  boost::fusion::for_each(arrayMap.table, FunctorSetValuesArray(this));
}

/*********************************************************************************************************************/

} // namespace ChimeraTK
