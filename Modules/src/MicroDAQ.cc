#include <fstream>

#include <H5Cpp.h>
#include <H5File.h>

#include <boost/filesystem.hpp>

#include "MicroDAQ.h"

namespace ChimeraTK {

  /*********************************************************************************************************************/

  namespace detail {

    /** Callable class for use with  boost::fusion::for_each: Attach the given accessor to the MicroDAQ with proper
      *  handling of the UserType. */
    struct AccessorAttacher {
      AccessorAttacher(VariableNetworkNode& feeder, MicroDAQ *owner, const std::string &name)
      : _feeder(feeder), _owner(owner), _name(name) {}

      template<typename PAIR>
      void operator()(PAIR&) const {

        // only continue if the call is for the right type
        if(typeid(typename PAIR::first_type) != _feeder.getValueType()) return;

        // register connection
        _feeder >> _owner->template getAccessor<typename PAIR::first_type>(_name);

      }

      VariableNetworkNode &_feeder;
      MicroDAQ *_owner;
      const std::string &_name;
    };

  }

  /*********************************************************************************************************************/

  void MicroDAQ::addSource(const Module &source, const RegisterPath &namePrefix) {

    // for simplification, first create a VirtualModule containing the correct hierarchy structure (obeying eliminate
    // hierarchy etc.)
    auto dynamicModel = source.findTag(".*");     /// @todo use virtualise() instead

    // create variable group map for namePrefix if needed
    if(groupMap.find(namePrefix) == groupMap.end()) {
      // search for existing parent (if any)
      auto parentPrefix = namePrefix;
      while(groupMap.find(parentPrefix) == groupMap.end()) {
        if(parentPrefix == "/") break;  // no existing parent found
        parentPrefix = std::string(parentPrefix).substr(0,std::string(parentPrefix).find_last_of("/"));
      }
      // create all not-yet-existing parents
      while(parentPrefix != namePrefix) {
        EntityOwner *owner = this;
        if(parentPrefix != "/") owner = &groupMap[parentPrefix];
        auto stop = std::string(namePrefix).find_first_of("/", parentPrefix.length()+1);
        if(stop == std::string::npos) stop = namePrefix.length();
        RegisterPath name = std::string(namePrefix).substr(parentPrefix.length(),stop-parentPrefix.length());
        parentPrefix /= name;
        groupMap[parentPrefix] = VariableGroup(owner, std::string(name).substr(1), "");
      }
    }

    // add all accessors on this hierarchy level
    for(auto &acc : dynamicModel.getAccessorList()) {
      boost::fusion::for_each(accessorListMap.table, detail::AccessorAttacher(acc, this, namePrefix/acc.getName()));
    }

    // recurse into submodules
    for(auto mod : dynamicModel.getSubmoduleList()) {
      addSource(*mod, namePrefix/mod->getName());
    }

  }

  /*********************************************************************************************************************/

  template<typename UserType>
  VariableNetworkNode MicroDAQ::getAccessor(const std::string &variableName) {

    // check if variable name already registered
    for(auto &name : overallVariableList) {
      if(name == variableName) {
        throw ChimeraTK::logic_error("Cannot add '"+variableName+
                  "' to MicroDAQ since a variable with that name is already registered.");
      }
    }
    overallVariableList.push_back(variableName);

    // add accessor and name to lists
    auto &accessorList = boost::fusion::at_key<UserType>(accessorListMap.table);
    auto &nameList = boost::fusion::at_key<UserType>(nameListMap.table);
    auto dirName = variableName.substr(0,variableName.find_last_of("/"));
    auto baseName = variableName.substr(variableName.find_last_of("/")+1);
    accessorList.emplace_back(&groupMap[dirName], baseName, "", 0, "");
    nameList.push_back(variableName);

    // return the accessor
    return accessorList.back();
  }

  /*********************************************************************************************************************/

  namespace detail {

    struct H5storage {
        H5storage(MicroDAQ *owner) : _owner(owner) {}

        H5::H5File outFile;
        std::string currentGroupName;

        /** Unique list of groups, used to create the groups in the file */
        std::list <std::string> groupList;

        /** boost::fusion::map of UserTypes to std::lists containing the H5::DataSpace objects. */
        template<typename UserType>
        using dataSpaceList = std::list<H5::DataSpace>;
        TemplateUserTypeMap<dataSpaceList> dataSpaceListMap;

        /** boost::fusion::map of UserTypes to std::lists containing decimation factors. */
        template<typename UserType>
        using decimationFactorList = std::list<size_t>;
        TemplateUserTypeMap<decimationFactorList> decimationFactorListMap;

        uint32_t currentBuffer{0};
        uint32_t nFillsInBuffer{0};
        bool isOpened{false};
        bool firstTrigger{true};

        void processTrigger();
        void writeData();

        MicroDAQ *_owner;
    };

    /*********************************************************************************************************************/

    struct DataSpaceCreator {
        DataSpaceCreator(H5storage &storage) : _storage(storage) {}

        template<typename PAIR>
        void operator()(PAIR &pair) const {
          typedef typename PAIR::first_type UserType;

          // get the lists for the UserType
          auto &accessorList = pair.second;
          auto &decimationFactorList = boost::fusion::at_key<UserType>(_storage.decimationFactorListMap.table);
          auto &dataSpaceList = boost::fusion::at_key<UserType>(_storage.dataSpaceListMap.table);
          auto &nameList = boost::fusion::at_key<UserType>(_storage._owner->nameListMap.table);

          // iterate through all accessors for this UserType
          auto name = nameList.begin();
          for(auto accessor = accessorList.begin() ; accessor != accessorList.end() ; ++accessor , ++name) {
            // determine decimation factor
            int factor = 1;
            if(accessor->getNElements() > _storage._owner->decimationThreshold_) {
              factor = _storage._owner->decimationFactor_;
            }
            decimationFactorList.push_back(factor);

            // define data space
            hsize_t dimsf[1];  // dataset dimensions
            dimsf[0] = accessor->getNElements()/factor;
            dataSpaceList.push_back( H5::DataSpace(1, dimsf) );

            // put all group names in list (each hierarchy level separately)
            size_t idx = 0;
            while( (idx = name->find('/', idx+1)) != std::string::npos ) {
              std::string groupName = name->substr(0,idx);
              _storage.groupList.push_back(groupName);
            }
          }
        }

        H5storage &_storage;
    };

  }

  /*********************************************************************************************************************/

  void MicroDAQ::mainLoop() {
      std::cout << "Initialising MicroDAQ system...";

      // storage object
      detail::H5storage storage(this);

      // create the data spaces
      boost::fusion::for_each(accessorListMap.table, detail::DataSpaceCreator(storage));

      // sort group list and make unique to make sure lower levels get created first
      storage.groupList.sort();
      storage.groupList.unique();

      // loop: process incoming triggers
      while(true) {
        trigger.read();
        storage.processTrigger();
      }

      std::cout << " done." << std::endl;
  }

  /*********************************************************************************************************************/

  namespace detail {

    void H5storage::processTrigger() {

        // update configuration variables
        _owner->enable.readLatest();
        _owner->nMaxFiles.readLatest();
        _owner->nTriggersPerFile.readLatest();

        // need to open or close file?
        if(!isOpened && _owner->enable != 0) {
          std::fstream bufferNumber;

          // some things to be done only on first trigger
          if(firstTrigger) {
            // create sub-directory
            boost::filesystem::create_directory("uDAQ");

            // determine current buffer number
            bufferNumber.open("uDAQ/currentBuffer", std::ofstream::in);
            bufferNumber.seekg(0);
            if(!bufferNumber.eof()) {
              bufferNumber >> currentBuffer;
              char filename[64];
              std::sprintf(filename, "uDAQ/data%04d.h5", currentBuffer);
              if(boost::filesystem::exists(filename) && boost::filesystem::file_size(filename) > 1000) currentBuffer++;
              if(currentBuffer >= _owner->nMaxFiles) currentBuffer = 0;
            }
            else {
              currentBuffer = 0;
            }
            bufferNumber.close();
          }

          // store current buffer number to disk
          char filename[64];
          std::sprintf(filename, "uDAQ/data%04d.h5", currentBuffer);
          std::cout << "uDAQ: Starting with file: " << filename << std::endl;
          bufferNumber.open("uDAQ/currentBuffer", std::ofstream::out);
          bufferNumber << currentBuffer << std::endl;
          bufferNumber.close();

          // update file number process variables
          _owner->currentFile = currentBuffer;
          _owner->currentFile.write();

          // open file
          try {
            outFile = H5::H5File(filename, H5F_ACC_TRUNC);
          }
          catch(H5::FileIException &) {
            return;
          }
          isOpened = true;

        }
        else if(isOpened && _owner->enable == 0) {
          outFile.close();
          isOpened = false;
        }

        // if file is opened, this trigger should be included in the DAQ
        if(isOpened) {

          // write data
          writeData();

          // increment counter, after nTriggersPerFile triggers written to the same file, switch the file
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

    struct DataWriter {
        DataWriter(detail::H5storage &storage) : _storage(storage) {}

        template<typename PAIR>
        void operator()(PAIR &pair) const {
          typedef typename PAIR::first_type UserType;

          // get the lists for the UserType
          auto &accessorList = pair.second;
          auto &decimationFactorList = boost::fusion::at_key<UserType>(_storage.decimationFactorListMap.table);
          auto &dataSpaceList = boost::fusion::at_key<UserType>(_storage.dataSpaceListMap.table);
          auto &nameList = boost::fusion::at_key<UserType>(_storage._owner->nameListMap.table);

          // iterate through all accessors for this UserType
          auto decimationFactor = decimationFactorList.begin();
          auto dataSpace = dataSpaceList.begin();
          auto name = nameList.begin();
          for(auto accessor = accessorList.begin() ; accessor != accessorList.end() ; ++accessor , ++decimationFactor, ++dataSpace, ++name) {

            // form full path name of data set
            std::string dataSetName = _storage.currentGroupName+"/"+*name;

            // write to file (this is mainly a function call to allow template specialisations at this point)
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
        void write2hdf(ArrayPollInput<UserType> &accessor, std::string &name, size_t decimationFactor, H5::DataSpace &dataSpace) const;

        H5storage &_storage;
    };

    /*********************************************************************************************************************/

    template<typename UserType>
    void DataWriter::write2hdf(ArrayPollInput<UserType> &accessor, std::string &dataSetName, size_t decimationFactor, H5::DataSpace &dataSpace) const {

      // prepare decimated buffer
      size_t n = accessor.getNElements()/decimationFactor;
      std::vector<float> buffer(n);
      for(size_t i=0; i<n; ++i) {
        buffer[i] = accessor[i*decimationFactor];
      }

      // write data from internal buffer to data set in HDF5 file
      H5::DataSet dataset = _storage.outFile.createDataSet(dataSetName, H5::PredType::NATIVE_FLOAT, dataSpace);
      dataset.write(buffer.data(), H5::PredType::NATIVE_FLOAT);
    }

    /*********************************************************************************************************************/

    template<>
    void DataWriter::write2hdf<std::string>(ArrayPollInput<std::string> &accessor, std::string &dataSetName, size_t, H5::DataSpace &dataSpace) const {

      // write data from internal buffer to data set in HDF5 file
      H5::DataSet dataset = _storage.outFile.createDataSet(dataSetName, H5::PredType::C_S1, dataSpace);
      dataset.write(accessor[0].c_str(), H5::PredType::NATIVE_FLOAT);
    }

    /*********************************************************************************************************************/

    void H5storage::writeData() {

        // format current time
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        time_t t = tv.tv_sec;
        if(t == 0) t = time(nullptr);
        struct tm *tmp = localtime(&t);
        char timeString[64];
        std::sprintf(timeString, "%04d-%02d-%02d %02d:%02d:%02d.%03d", 1900+tmp->tm_year, tmp->tm_mon+1, tmp->tm_mday,
            tmp->tm_hour, tmp->tm_min, tmp->tm_sec, static_cast<int>(tv.tv_usec / 1000));

        // create groups
        currentGroupName = std::string("/")+std::string(timeString);
        try {
          outFile.createGroup(currentGroupName);
          for(auto &group : groupList) outFile.createGroup(currentGroupName+"/"+group);
        }
        catch(H5::FileIException &) {
          outFile.close();
          isOpened = false;    // will re-open file on next trigger
          return;
        }

        // read all input data
        _owner->readAllLatest();

        // write all data to file
        boost::fusion::for_each(_owner->accessorListMap.table, DataWriter(*this));

    }

    /*********************************************************************************************************************/

  } // namespace detail

} // namespace ChimeraTK
