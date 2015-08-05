#Include the MTCA4U configuration. The location can vary, depending 
#where you installed it. For Ubuntu installations from .deb packages
#this example should work:
include /usr/share/mtca4u/MTCA4U.CONFIG

CPPFLAGS += $(MtcaMappedDevice_INCLUDE_FLAGS)
LDFLAGS +=  $(MtcaMappedDevice_LIB_FLAGS) $(MtcaMappedDevice_RUNPATH_FLAGS)

all: readWrite mapped accessor dmap ExampleDevice

% : %.cpp
	g++ $^ $(CPPFLAGS) $(LDFLAGS) -o $@
