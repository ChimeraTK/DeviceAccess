#include <libxml++/libxml++.h>

#include "ConfigReader.h"
#include <iostream>

namespace ChimeraTK {

  template<typename Element>
  static Element prefix(std::string s, Element e) {
    e.name = s + e.name;
    return e;
  }

  static std::unique_ptr<xmlpp::DomParser> createDomParser(const std::string& fileName);
  static std::string branch(std::string flattened_name);
  static std::string leaf(std::string flattened_name);

  struct Variable {
    std::string name;
    std::string type;
    std::string value;
  };

  struct Array {
    std::string name;
    std::string type;
    std::map<size_t, std::string> values;
  };

  using VariableList = std::vector<Variable>;
  using ArrayList = std::vector<Array>;

  class ConfigParser {
    std::string fileName_{};
    std::unique_ptr<xmlpp::DomParser> parser_{};
    std::unique_ptr<VariableList> variableList_{};
    std::unique_ptr<ArrayList> arrayList_{};

   public:
    ConfigParser(const std::string& fileName) : fileName_(fileName), parser_(createDomParser(fileName)) {}

    std::unique_ptr<VariableList> getVariableList();
    std::unique_ptr<ArrayList> getArrayList();

   private:
    std::tuple<std::unique_ptr<VariableList>, std::unique_ptr<ArrayList>> parse();
    xmlpp::Element* getRootNode(xmlpp::DomParser& parser);
    void error(const std::string& message);
    bool isVariable(const xmlpp::Element* element);
    bool isArray(const xmlpp::Element* element);
    bool isModule(const xmlpp::Element* element);
    Variable parseVariable(const xmlpp::Element* element);
    Array parseArray(const xmlpp::Element* element);
    void parseModule(const xmlpp::Element* element, std::string parent_name);

    void validateValueNode(const xmlpp::Element* valueElement);
    std::map<size_t, std::string> gettArrayValues(const xmlpp::Element* element);
  };

  class ModuleList {
    std::unordered_map<std::string, ChimeraTK::VariableGroup> map_;
    ChimeraTK::Module* owner_;

   public:
    ModuleList(ChimeraTK::Module* o) : owner_(o) {}
    ChimeraTK::Module* lookup(std::string flattened_module_name);
    ChimeraTK::Module* get(std::string flattened_name);
  };

  /*********************************************************************************************************************/

  /** Functor to fill variableMap */
  struct FunctorFill {
    FunctorFill(ConfigReader* owner, const std::string& type, const std::string& name, const std::string& value,
        bool& processed)
    : _owner(owner), _type(type), _name(name), _value(value), _processed(processed) {
      _processed = false;
    }

    template<typename PAIR>
    void operator()(PAIR&) const {
      // extract the user type from the pair
      typedef typename PAIR::first_type T;

      // skip this type, if not matching the type string in the config file
      if(_type != boost::fusion::at_key<T>(_owner->typeMap)) return;

      _owner->createVar<T>(_name, _value);
      _processed = true;
    }

    ConfigReader* _owner;
    const std::string &_type, &_name, &_value;
    bool& _processed; // must be a non-const reference, since we want to return
                      // this to the caller
  };

  /*********************************************************************************************************************/

  /** Functor to fill variableMap for arrays */
  struct ArrayFunctorFill {
    ArrayFunctorFill(ConfigReader* owner, const std::string& type, const std::string& name,
        const std::map<size_t, std::string>& values, bool& processed)
    : _owner(owner), _type(type), _name(name), _values(values), _processed(processed) {
      _processed = false;
    }

    template<typename PAIR>
    void operator()(PAIR&) const {
      // extract the user type from the pair
      typedef typename PAIR::first_type T;

      // skip this type, if not matching the type string in the config file
      if(_type != boost::fusion::at_key<T>(_owner->typeMap)) return;

      _owner->createArray<T>(_name, _values);
      _processed = true;
    }

    ConfigReader* _owner;
    const std::string &_type, &_name;
    const std::map<size_t, std::string>& _values;
    bool& _processed; // must be a non-const reference, since we want to return
                      // this to the caller
  };

  /*********************************************************************************************************************/

  template<typename T>
  void ConfigReader::createVar(const std::string& name, const std::string& value) {
    // convert value into user type
    /// @todo error handling!
    std::stringstream stream(value);
    T convertedValue;
    if(typeid(T) == typeid(int8_t) || typeid(T) == typeid(uint8_t)) { // prevent interpreting int8-types as characters
      int16_t intermediate;
      stream >> intermediate;
      convertedValue = intermediate;
    }
    else { // note: string is done in template specialisation
      stream >> convertedValue;
    }

    auto moduleName = branch(name);
    auto varName = leaf(name);
    auto varOwner = _moduleList->lookup(moduleName);

    // place the variable onto the vector
    std::unordered_map<std::string, ConfigReader::Var<T>>& theMap = boost::fusion::at_key<T>(variableMap.table);
    theMap.emplace(std::make_pair(name, ConfigReader::Var<T>(varOwner, varName, convertedValue)));
  }

  /*********************************************************************************************************************/

  template<>
  void ConfigReader::createVar<std::string>(const std::string& name, const std::string& value) {
    auto moduleName = branch(name);
    auto varName = leaf(name);
    auto varOwner = _moduleList->lookup(moduleName);

    // place the variable onto the vector
    std::unordered_map<std::string, ConfigReader::Var<std::string>>& theMap =
        boost::fusion::at_key<std::string>(variableMap.table);
    theMap.emplace(std::make_pair(name, ConfigReader::Var<std::string>(varOwner, varName, value)));
  }

  /*********************************************************************************************************************/

  /*********************************************************************************************************************/

  template<typename T>
  void ConfigReader::createArray(const std::string& name, const std::map<size_t, std::string>& values) {
    std::vector<T> Tvalues;

    size_t expectedIndex = 0;
    for(auto it = values.begin(); it != values.end(); ++it) {
      // check index (std::map should be ordered by the index)
      if(it->first != expectedIndex) {
        parsingError("Array index " + std::to_string(expectedIndex) + " not found, but " + std::to_string(it->first) +
            " was. "
            "Sparse arrays are not supported!");
      }
      ++expectedIndex;

      // convert value into user type
      std::stringstream stream(it->second);
      T convertedValue;
      if(typeid(T) == typeid(int8_t) || typeid(T) == typeid(uint8_t)) { // prevent interpreting int8-types as characters
        int16_t intermediate;
        stream >> intermediate;
        convertedValue = intermediate;
      }
      else { // note: string is done in template specialisation
        stream >> convertedValue;
      }

      // store value in vector
      Tvalues.push_back(convertedValue);
    }

    auto moduleName = branch(name);
    auto arrayName = leaf(name);
    auto arrayOwner = _moduleList->lookup(moduleName);

    // place the variable onto the vector
    std::unordered_map<std::string, ConfigReader::Array<T>>& theMap = boost::fusion::at_key<T>(arrayMap.table);
    theMap.emplace(std::make_pair(name, ConfigReader::Array<T>(arrayOwner, arrayName, Tvalues)));
  }

  /*********************************************************************************************************************/

  template<>
  void ConfigReader::createArray<std::string>(const std::string& name, const std::map<size_t, std::string>& values) {
    std::vector<std::string> Tvalues;

    size_t expectedIndex = 0;
    for(auto it = values.begin(); it != values.end(); ++it) {
      // check index (std::map should be ordered by the index)
      if(it->first != expectedIndex) {
        parsingError("Array index " + std::to_string(expectedIndex) + " not found, but " + std::to_string(it->first) +
            " was. "
            "Sparse arrays are not supported!");
      }
      ++expectedIndex;

      // store value in vector
      Tvalues.push_back(it->second);
    }

    auto moduleName = branch(name);
    auto arrayName = leaf(name);
    auto arrayOwner = _moduleList->lookup(moduleName);

    // place the variable onto the vector
    std::unordered_map<std::string, ConfigReader::Array<std::string>>& theMap =
        boost::fusion::at_key<std::string>(arrayMap.table);
    theMap.emplace(std::make_pair(name, ConfigReader::Array<std::string>(arrayOwner, arrayName, Tvalues)));
  }

  /*********************************************************************************************************************/

  ConfigReader::ConfigReader(EntityOwner* owner, const std::string& name, const std::string& fileName,
      const std::unordered_set<std::string>& tags)
  : ApplicationModule(owner, name, "Configuration read from file '" + fileName + "'", false, tags), _fileName(fileName),
    _moduleList(std::make_unique<ModuleList>(this)) {
    auto fillVariableMap = [this](const Variable& var) {
      bool processed{false};
      boost::fusion::for_each(variableMap.table, FunctorFill(this, var.type, var.name, var.value, processed));
      if(!processed) parsingError("Incorrect value '" + var.type + "' for attribute 'type' of the 'variable' tag.");
    };

    auto fillArrayMap = [this](const ChimeraTK::Array& arr) {
      // create accessor and store array value in map using functor
      bool processed{false};
      boost::fusion::for_each(arrayMap.table, ArrayFunctorFill(this, arr.type, arr.name, arr.values, processed));
      if(!processed) parsingError("Incorrect value '" + arr.type + "' for attribute 'type' of the 'variable' tag.");
    };

    auto parser = ConfigParser(fileName);
    auto v = parser.getVariableList();
    auto a = parser.getArrayList();

    for(const auto& var : *v) {
      fillVariableMap(var);
    }
    for(const auto& arr : *a) {
      fillArrayMap(arr);
    }
  }

  // workaround for std::unique_ptr static assert.
  ConfigReader::~ConfigReader() = default;
  /********************************************************************************************************************/

  void ConfigReader::parsingError(const std::string& message) {
    throw ChimeraTK::logic_error("ConfigReader: Error parsing the config file '" + _fileName + "': " + message);
  }

  /*********************************************************************************************************************/

  /** Functor to set values to the scalar accessors */
  struct FunctorSetValues {
    FunctorSetValues(ConfigReader* owner) : _owner(owner) {}

    template<typename PAIR>
    void operator()(PAIR&) const {
      // get user type and vector
      typedef typename PAIR::first_type T;
      std::unordered_map<std::string, ConfigReader::Var<T>>& theMap =
          boost::fusion::at_key<T>(_owner->variableMap.table);

      // iterate vector and set values
      for(auto& pair : theMap) {
        auto& var = pair.second;
        var._accessor = var._value;
        var._accessor.write();
      }
    }

    ConfigReader* _owner;
  };

  /*********************************************************************************************************************/

  /** Functor to set values to the array accessors */
  struct FunctorSetValuesArray {
    FunctorSetValuesArray(ConfigReader* owner) : _owner(owner) {}

    template<typename PAIR>
    void operator()(PAIR&) const {
      // get user type and vector
      typedef typename PAIR::first_type T;
      std::unordered_map<std::string, ConfigReader::Array<T>>& theMap =
          boost::fusion::at_key<T>(_owner->arrayMap.table);

      // iterate vector and set values
      for(auto& pair : theMap) {
        auto& var = pair.second;
        var._accessor = var._value;
        var._accessor.write();
      }
    }

    ConfigReader* _owner;
  };

  /*********************************************************************************************************************/

  void ConfigReader::prepare() {
    boost::fusion::for_each(variableMap.table, FunctorSetValues(this));
    boost::fusion::for_each(arrayMap.table, FunctorSetValuesArray(this));
  }

  /*********************************************************************************************************************/

  ChimeraTK::Module* ModuleList::lookup(std::string flattened_module_name) { return get(flattened_module_name); }

  /*********************************************************************************************************************/

  ChimeraTK::Module* ModuleList::get(std::string flattened_name) {
    if(flattened_name == "") {
      return owner_;
    }
    auto e = map_.find(flattened_name);
    if(e == map_.end()) {
      auto module_name = leaf(flattened_name);
      auto owner = get(branch(flattened_name));

      map_[flattened_name] = ChimeraTK::VariableGroup(owner, module_name, "");
      return &map_[flattened_name];
    }
    return &e->second;
  }

  /*********************************************************************************************************************/

  std::unique_ptr<VariableList> ConfigParser::getVariableList() {
    if(variableList_ == nullptr) {
      std::tie(variableList_, arrayList_) = parse();
    }
    return std::move(variableList_);
  }

  /*********************************************************************************************************************/

  std::unique_ptr<ArrayList> ConfigParser::getArrayList() {
    if(arrayList_ != nullptr) {
      std::tie(variableList_, arrayList_) = parse();
    }
    return std::move(arrayList_);
  }

  /*********************************************************************************************************************/

  std::tuple<std::unique_ptr<VariableList>, std::unique_ptr<ArrayList>> ConfigParser::parse() {
    const auto root = getRootNode(*parser_);
    if(root->get_name() != "configuration") {
      error("Expected 'configuration' tag instead of: " + root->get_name());
    }

    //start with clean lists: parseModule accumulates elements into these.
    variableList_ = std::make_unique<VariableList>();
    arrayList_ = std::make_unique<ArrayList>();

    const xmlpp::Element* element = dynamic_cast<const xmlpp::Element*>(root);
    std::string parent_module_name = "";
    parseModule(element, parent_module_name);

    return std::tuple<std::unique_ptr<VariableList>, std::unique_ptr<ArrayList>>{
        std::move(variableList_), std::move(arrayList_)};
  }

  /*********************************************************************************************************************/

  void ConfigParser::parseModule(const xmlpp::Element* element, std::string parent_name) {
    auto module_name = (element->get_name() == "configuration") // root node gets special treatment
        ?
        "" :
        element->get_attribute("name")->get_value() + "/";

    parent_name += module_name;

    for(const auto& child : element->get_children()) {
      element = dynamic_cast<const xmlpp::Element*>(child);
      if(!element) {
        continue; // ignore if not an element (e.g. comment)
      }
      else if(isVariable(element)) {
        variableList_->emplace_back(prefix(parent_name, parseVariable(element)));
      }
      else if(isArray(element)) {
        arrayList_->emplace_back(prefix(parent_name, parseArray(element)));
      }
      else if(isModule(element)) {
        parseModule(element, parent_name);
      }
      else {
        error("Unknown tag: " + element->get_name());
      }
    }
  }

  /*********************************************************************************************************************/

  Variable ConfigParser::parseVariable(const xmlpp::Element* element) {
    auto name = element->get_attribute("name")->get_value();
    auto type = element->get_attribute("type")->get_value();
    auto value = element->get_attribute("value")->get_value();
    return Variable{name, type, value};
  }

  /*********************************************************************************************************************/

  Array ConfigParser::parseArray(const xmlpp::Element* element) {
    auto name = element->get_attribute("name")->get_value();
    auto type = element->get_attribute("type")->get_value();
    std::map<size_t, std::string> values = gettArrayValues(element);
    return Array{name, type, values};
  }

  /*********************************************************************************************************************/

  xmlpp::Element* ConfigParser::getRootNode(xmlpp::DomParser& parser) {
    auto root = parser.get_document()->get_root_node();
    if(root->get_name() != "configuration") {
      error("Expected 'configuration' tag instead of: " + root->get_name());
    }
    return root;
  }

  /*********************************************************************************************************************/

  void ConfigParser::error(const std::string& message) {
    throw ChimeraTK::logic_error("ConfigReader: Error parsing the config file '" + fileName_ + "': " + message);
  }

  /*********************************************************************************************************************/

  bool ConfigParser::isVariable(const xmlpp::Element* element) {
    if((element->get_name() == "variable") && element->get_attribute("value")) {
      // validate variable node
      if(!element->get_attribute("name")) {
        error("Missing attribute 'name' for the 'variable' tag.");
      }
      else if(!element->get_attribute("type")) {
        error("Missing attribute 'type' for the 'variable' tag.");
      }
      return true;
    }
    else {
      return false;
    }
  }

  /*********************************************************************************************************************/

  bool ConfigParser::isArray(const xmlpp::Element* element) {
    if((element->get_name() == "variable") && !element->get_attribute("value")) {
      // validate array node
      if(!element->get_attribute("name")) {
        error("Missing attribute 'name' for the 'variable' tag.");
      }
      else if(!element->get_attribute("type")) {
        error("Missing attribute 'type' for the 'variable' tag.");
      }
      return true;
    }
    else {
      return false;
    }
  }

  /*********************************************************************************************************************/

  bool ConfigParser::isModule(const xmlpp::Element* element) {
    if(element->get_name() == "module") {
      if(!element->get_attribute("name")) {
        error("Missing attribute 'name' for the 'module' tag.");
      }
      return true;
    }
    else {
      return false;
    }
  }

  /*********************************************************************************************************************/

  std::map<size_t, std::string> ConfigParser::gettArrayValues(const xmlpp::Element* element) {
    bool valueFound = false;
    std::map<size_t, std::string> values;

    for(const auto& valueChild : element->get_children()) {
      const xmlpp::Element* valueElement = dynamic_cast<const xmlpp::Element*>(valueChild);
      if(!valueElement) continue; // ignore comments etc.
      validateValueNode(valueElement);
      valueFound = true;

      auto index = valueElement->get_attribute("i");
      auto value = valueElement->get_attribute("v");

      // get index as number and store value as a string
      size_t intIndex;
      try {
        intIndex = std::stoi(index->get_value());
      }
      catch(std::exception& e) {
        error("Cannot parse string '" + std::string(index->get_value()) + "' as an index number: " + e.what());
      }
      values[intIndex] = value->get_value();
    }
    // make sure there is at least one value
    if(!valueFound) {
      error("Each variable must have a value, either specified as an attribute or as child tags.");
    }
    return values;
  }

  /*********************************************************************************************************************/

  void ConfigParser::validateValueNode(const xmlpp::Element* valueElement) {
    if(valueElement->get_name() != "value") {
      error("Expected 'value' tag instead of: " + valueElement->get_name());
    }
    if(!valueElement->get_attribute("i")) {
      error("Missing attribute 'index' for the 'value' tag.");
    }
    if(!valueElement->get_attribute("v")) {
      error("Missing attribute 'value' for the 'value' tag.");
    }
  }

  /*********************************************************************************************************************/

  std::unique_ptr<xmlpp::DomParser> createDomParser(const std::string& fileName) {
    try {
      return std::make_unique<xmlpp::DomParser>(fileName);
    }
    catch(xmlpp::exception& e) { /// @todo change exception!
      throw ChimeraTK::logic_error("ConfigReader: Error opening the config file '" + fileName + "': " + e.what());
    }
  }

  /*********************************************************************************************************************/

  std::string branch(std::string flattened_name) {
    auto pos = flattened_name.find_last_of('/');
    pos = (pos == std::string::npos) ? 0 : pos;
    return flattened_name.substr(0, pos);
  }

  /*********************************************************************************************************************/
  std::string leaf(std::string flattened_name) {
    auto pos = flattened_name.find_last_of('/');
    return flattened_name.substr(pos + 1, flattened_name.size());
  }

  /*********************************************************************************************************************/

} // namespace ChimeraTK
