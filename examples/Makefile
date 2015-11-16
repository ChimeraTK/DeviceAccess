#Make sure the mtca4u-deviceaccess-config executable is in your path.
#For standard installations from debian packages this is the case.
#For custom installations you might have to adapt you PATH variable.
CPPFLAGS += $(shell mtca4u-deviceaccess-config --cppflags)
LDFLAGS +=  $(shell mtca4u-deviceaccess-config --ldflags)

SOURCES = $(shell ls *.cpp)
TARGETS = $(basename $(SOURCES))

#LDFLAGS += 

all: print readWrite mapped accessor dmap MuxedAccessor DeviceClient

print:
	echo SOURCES = $(SOURCES)
	echo TARGETS = $(TARGETS)

% : %.cpp
	g++ $^ $(CPPFLAGS) $(LDFLAGS) -o $@
