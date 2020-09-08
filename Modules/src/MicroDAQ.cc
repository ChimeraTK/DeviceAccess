#include <fstream>

#include <H5Cpp.h>
#include <H5File.h>

#include <boost/filesystem.hpp>

#include "MicroDAQ.h"

namespace ChimeraTK {

  /*********************************************************************************************************************/

  namespace detail {

    /** Callable class for use with  boost::fusion::for_each: Attach the given
     * accessor to the MicroDAQ with proper handling of the UserType. */
    template<typename TRIGGERTYPE>
    struct AccessorAttacher {
      AccessorAttacher(VariableNetworkNode& feeder, MicroDAQ<TRIGGERTYPE>* owner, const std::string& name)
      : _feeder(feeder), _owner(owner), _name(name) {}

      template<typename PAIR>
      void operator()(PAIR&) const {
        // only continue if the call is for the right type
        if(typeid(typename PAIR::first_type) != _feeder.getValueType()) return;

        // register connection
        _feeder >> _owner->template getAccessor<typename PAIR::first_type>(_name);
      }

      VariableNetworkNode& _feeder;
      MicroDAQ<TRIGGERTYPE>* _owner;
      const std::string& _name;
    };

  } // namespace detail

  /*********************************************************************************************************************/

  template<typename TRIGGERTYPE>
  void MicroDAQ<TRIGGERTYPE>::addSource(const Module& source, const RegisterPath& namePrefix) {
    // for simplification, first create a VirtualModule containing the correct
    // hierarchy structure (obeying eliminate hierarchy etc.)
    auto dynamicModel = source.findTag(".*"); /// @todo use virtualise() instead

    // create variable group map for namePrefix if needed
    if(groupMap.find(namePrefix) == groupMap.end()) {
      // search for existing parent (if any)
      auto parentPrefix = namePrefix;
      while(groupMap.find(parentPrefix) == groupMap.end()) {
        if(parentPrefix == "/") break; // no existing parent found
        parentPrefix = std::string(parentPrefix).substr(0, std::string(parentPrefix).find_last_of("/"));
      }
      // create all not-yet-existing parents
      while(parentPrefix != namePrefix) {
        EntityOwner* owner = this;
        if(parentPrefix != "/") owner = &groupMap[parentPrefix];
        auto stop = std::string(namePrefix).find_first_of("/", parentPrefix.length() + 1);
        if(stop == std::string::npos) stop = namePrefix.length();
        RegisterPath name = std::string(namePrefix).substr(parentPrefix.length(), stop - parentPrefix.length());
        parentPrefix /= name;
        groupMap[parentPrefix] = VariableGroup(owner, std::string(name).substr(1), "");
      }
    }

    // add all accessors on this hierarchy level
    for(auto& acc : dynamicModel.getAccessorList()) {
      boost::fusion::for_each(
          accessorListMap.table, detail::AccessorAttacher<TRIGGERTYPE>(acc, this, namePrefix / acc.getName()));
    }

    // recurse into submodules
    for(auto mod : dynamicModel.getSubmoduleList()) {
      addSource(*mod, namePrefix / mod->getName());
    }
  }

  /*********************************************************************************************************************/

  template<typename TRIGGERTYPE>
    void MicroDAQ<TRIGGERTYPE>::addSource(const DeviceModule& source, const RegisterPath& namePrefix) {
    auto mod = source.virtualiseFromCatalog();
    addSource(mod, namePrefix);
  }

  /*********************************************************************************************************************/

  template<typename TRIGGERTYPE>
  template<typename UserType>
  VariableNetworkNode MicroDAQ<TRIGGERTYPE>::getAccessor(const std::string& variableName) {
    // check if variable name already registered
    for(auto& name : overallVariableList) {
      if(name == variableName) {
        throw ChimeraTK::logic_error(
            "Cannot add '" + variableName + "' to MicroDAQ since a variable with that name is already registered.");
      }
    }
    overallVariableList.push_back(variableName);

    // add accessor and name to lists
    auto& tmpAccessorList = boost::fusion::at_key<UserType>(accessorListMap.table);
    auto& nameList = boost::fusion::at_key<UserType>(nameListMap.table);
    auto dirName = variableName.substr(0, variableName.find_last_of("/"));
    auto baseName = variableName.substr(variableName.find_last_of("/") + 1);
    if(dirName != "") {
      tmpAccessorList.emplace_back(&groupMap[dirName], baseName, "", 0, "");
    }
    else {
      // on top level there is no VariableGroup...
      tmpAccessorList.emplace_back(this, baseName, "", 0, "");
    }
    nameList.push_back(variableName);

    // return the accessor
    return tmpAccessorList.back();
  }

  /*********************************************************************************************************************/

  namespace detail {

    template<typename TRIGGERTYPE>
    struct H5storage {
      H5storage(MicroDAQ<TRIGGERTYPE>* owner) : _owner(owner) {}

      H5::H5File outFile;
      std::string currentGroupName;

      /** Unique list of groups, used to create the groups in the file */
      std::list<std::string> groupList;

      /** boost::fusion::map of UserTypes to std::lists containing the H5::DataSpace
       * objects. */
      template<typename UserType>
      using dataSpaceList = std::list<H5::DataSpace>;
      TemplateUserTypeMap<dataSpaceList> dataSpaceListMap;

      /** boost::fusion::map of UserTypes to std::lists containing decimation
       * factors. */
      template<typename UserType>
      using decimationFactorList = std::list<size_t>;
      TemplateUserTypeMap<decimationFactorList> decimationFactorListMap;

      uint32_t currentBuffer{0};
      uint32_t nFillsInBuffer{0};
      bool isOpened{false};
      bool firstTrigger{true};

      void processTrigger();
      void writeData();

      MicroDAQ<TRIGGERTYPE>* _owner;
    };

    /*********************************************************************************************************************/

    template<typename TRIGGERTYPE>
    struct DataSpaceCreator {
      DataSpaceCreator(H5storage<TRIGGERTYPE>& storage) : _storage(storage) {}

      template<typename PAIR>
      void operator()(PAIR& pair) const {
        typedef typename PAIR::first_type UserType;

        // get the lists for the UserType
        auto& accessorList = pair.second;
        auto& decimationFactorList = boost::fusion::at_key<UserType>(_storage.decimationFactorListMap.table);
        auto& dataSpaceList = boost::fusion::at_key<UserType>(_storage.dataSpaceListMap.table);
        auto& nameList = boost::fusion::at_key<UserType>(_storage._owner->nameListMap.table);

        // iterate through all accessors for this UserType
        auto name = nameList.begin();
        for(auto accessor = accessorList.begin(); accessor != accessorList.end(); ++accessor, ++name) {
          // determine decimation factor
          int factor = 1;
          if(accessor->getNElements() > _storage._owner->decimationThreshold_) {
            factor = _storage._owner->decimationFactor_;
          }
          decimationFactorList.push_back(factor);

          // define data space
          hsize_t dimsf[1]; // dataset dimensions
          dimsf[0] = accessor->getNElements() / factor;
          dataSpaceList.push_back(H5::DataSpace(1, dimsf));

          // put all group names in list (each hierarchy level separately)
          size_t idx = 0;
          while((idx = name->find('/', idx + 1)) != std::string::npos) {
            std::string groupName = name->substr(0, idx);
            _storage.groupList.push_back(groupName);
          }
        }
      }

      H5storage<TRIGGERTYPE>& _storage;
    };

  } // namespace detail

  /*********************************************************************************************************************/

  template<typename TRIGGERTYPE>
  void MicroDAQ<TRIGGERTYPE>::mainLoop() {
    // storage object
    detail::H5storage<TRIGGERTYPE> storage(this);

    // create the data spaces
    boost::fusion::for_each(accessorListMap.table, detail::DataSpaceCreator<TRIGGERTYPE>(storage));

    // sort group list and make unique to make sure lower levels get created first
    storage.groupList.sort();
    storage.groupList.unique();

    // loop: process incoming triggers
    auto group = readAnyGroup();
    while(true) {
      group.readUntil(trigger.getId());
      storage.processTrigger();
    }
  }

  /*********************************************************************************************************************/

  namespace detail {

    template<typename TRIGGERTYPE>
    void H5storage<TRIGGERTYPE>::processTrigger() {
      // need to open or close file?
      if(!isOpened && _owner->enable != 0) {
        std::fstream bufferNumber;

        // some things to be done only on first trigger
        if(firstTrigger) {

          // determine current buffer number
          bufferNumber.open(_owner->fileNamePrefix_ + ".currentBuffer", std::ofstream::in);
          bufferNumber.seekg(0);
          if(!bufferNumber.eof()) {
            bufferNumber >> currentBuffer;
            char suffix[64];
            std::sprintf(suffix, ".data%04d.h5", currentBuffer);
            std::string filename = _owner->fileNamePrefix_ + suffix;
            if(boost::filesystem::exists(filename) && boost::filesystem::file_size(filename) > 1000) currentBuffer++;
            if(currentBuffer >= _owner->nMaxFiles) currentBuffer = 0;
          }
          else {
            currentBuffer = 0;
          }
          bufferNumber.close();
        }

        // store current buffer number to disk
        char suffix[64];
        std::sprintf(suffix, ".data%04d.h5", currentBuffer);
        std::string filename = _owner->fileNamePrefix_ + suffix;
        std::cout << "uDAQ: Starting with file: " << filename << std::endl;
        bufferNumber.open(_owner->fileNamePrefix_ + ".currentBuffer", std::ofstream::out);
        bufferNumber << currentBuffer << std::endl;
        bufferNumber.close();

        // update file number process variables
        _owner->currentFile = currentBuffer;
        _owner->currentFile.write();

        // open file
        try {
          outFile = H5::H5File(filename, H5F_ACC_TRUNC);
        }
        catch(H5::FileIException&) {
          return;
        }
        isOpened = true;
      }
      else if(isOpened && _owner->enable == 0) {
        outFile.close();
        isOpened = false;

        // force new file if DAQ is re-enabled
        ++currentBuffer;
      }

      // if file is opened, this trigger should be included in the DAQ
      if(isOpened) {
        // write data
        writeData();

        // increment counter, after nTriggersPerFile triggers written to the same
        // file, switch the file
        nFillsInBuffer++;
        if(nFillsInBuffer > _owner->nTriggersPerFile) {
          // increment file number. use at most nMaxFiles files, overwrite old files
          currentBuffer++;
          if(currentBuffer >= _owner->nMaxFiles) currentBuffer = 0;
          nFillsInBuffer = 0;

          // just close the file here, will re-open on next trigger
          outFile.close();
          isOpened = false;
        }
      }
    }

    /*********************************************************************************************************************/

    template<typename TRIGGERTYPE>
    struct DataWriter {
      DataWriter(detail::H5storage<TRIGGERTYPE>& storage) : _storage(storage) {}

      template<typename PAIR>
      void operator()(PAIR& pair) const {
        typedef typename PAIR::first_type UserType;

        // get the lists for the UserType
        auto& accessorList = pair.second;
        auto& decimationFactorList = boost::fusion::at_key<UserType>(_storage.decimationFactorListMap.table);
        auto& dataSpaceList = boost::fusion::at_key<UserType>(_storage.dataSpaceListMap.table);
        auto& nameList = boost::fusion::at_key<UserType>(_storage._owner->nameListMap.table);

        // iterate through all accessors for this UserType
        auto decimationFactor = decimationFactorList.begin();
        auto dataSpace = dataSpaceList.begin();
        auto name = nameList.begin();
        for(auto accessor = accessorList.begin(); accessor != accessorList.end();
            ++accessor, ++decimationFactor, ++dataSpace, ++name) {
          // form full path name of data set
          std::string dataSetName = _storage.currentGroupName + "/" + *name;

          // write to file (this is mainly a function call to allow template
          // specialisations at this point)
          try {
            write2hdf<UserType>(*accessor, dataSetName, *decimationFactor, *dataSpace);
          }
          catch(H5::FileIException&) {
            std::cout << "MicroDAQ: ERROR writing data set " << dataSetName << std::endl;
            throw;
          }
        }
      }

      template<typename UserType>
      void write2hdf(ArrayPushInput<UserType>& accessor, std::string& name, size_t decimationFactor,
          H5::DataSpace& dataSpace) const;

      H5storage<TRIGGERTYPE>& _storage;
    };

    /*********************************************************************************************************************/

    template<typename TRIGGERTYPE>
    template<typename UserType>
    void DataWriter<TRIGGERTYPE>::write2hdf(ArrayPushInput<UserType>& accessor, std::string& dataSetName,
        size_t decimationFactor, H5::DataSpace& dataSpace) const {
      // prepare decimated buffer
      size_t n = accessor.getNElements() / decimationFactor;
      std::vector<float> buffer(n);
      for(size_t i = 0; i < n; ++i) {
        buffer[i] = userTypeToNumeric<float>(accessor[i * decimationFactor]);
      }

      // write data from internal buffer to data set in HDF5 file
      H5::DataSet dataset = _storage.outFile.createDataSet(dataSetName, H5::PredType::NATIVE_FLOAT, dataSpace);
      dataset.write(buffer.data(), H5::PredType::NATIVE_FLOAT);
    }

    /*********************************************************************************************************************/

    template<typename TRIGGERTYPE>
    void H5storage<TRIGGERTYPE>::writeData() {
      // format current time
      struct timeval tv;
      gettimeofday(&tv, nullptr);
      time_t t = tv.tv_sec;
      if(t == 0) t = time(nullptr);
      struct tm* tmp = localtime(&t);
      char timeString[64];
      std::sprintf(timeString, "%04d-%02d-%02d %02d:%02d:%02d.%03d", 1900 + tmp->tm_year, tmp->tm_mon + 1, tmp->tm_mday,
          tmp->tm_hour, tmp->tm_min, tmp->tm_sec, static_cast<int>(tv.tv_usec / 1000));

      // create groups
      currentGroupName = std::string("/") + std::string(timeString);
      try {
        outFile.createGroup(currentGroupName);
        for(auto& group : groupList) outFile.createGroup(currentGroupName + "/" + group);
      }
      catch(H5::FileIException&) {
        outFile.close();
        isOpened = false; // will re-open file on next trigger
        return;
      }

      // write all data to file
      boost::fusion::for_each(_owner->accessorListMap.table, DataWriter<TRIGGERTYPE>(*this));
    }

    /*********************************************************************************************************************/

  } // namespace detail

  INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(MicroDAQ);

} // namespace ChimeraTK
