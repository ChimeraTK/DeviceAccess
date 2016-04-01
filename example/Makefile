#define the dependeny locations here
DOOCSROOT = /export/doocs

# to define DOOCSROOT as an absolute path
include $(DOOCSROOT)/$(DOOCSARCH)/DEFINEDOOCSROOT

# to define the arch dependend things
include $(DOOCSROOT)/$(DOOCSARCH)/CONFIG

#include /usr/share/mtca4u/ControlSystemAdapter.CONFIG
include /space/mhier/builddirs/u14/ControlSystemAdapter/ControlSystemAdapter.CONFIG

CPPFLAGS += $(ControlSystemAdapter_INCLUDE_FLAGS)
LDFLAGS += $(ControlSystemAdapter_LIB_FLAGS) $(ControlSystemAdapter_RUNPATH_FLAGS)

CPPFLAGS += `mtca4uInstaCoSADev-config --cppflags`
LDFLAGS += `mtca4uInstaCoSADev-config --ldflags`

CPPFLAGS += `mtca4u-deviceaccess-config --cppflags`
LDFLAGS += `mtca4u-deviceaccess-config --ldflags`

OBJDIR = $(DOOCSROOT)/$(DOOCSARCH)/obj/server/test/InstaCoSADevExample
SRCDIR = $(PWD)
ADAPTER_OBJDIR = $(DOOCSROOT)/$(DOOCSARCH)/obj/library/common/DoocsAdapter

SOURCEOBJ = $(OBJDIR)/InstaCoSADevExample_server.o 
SOURCEHFILES =  
ALLPROGS = $(OBJDIR)/InstaCoSADevExample_server 

CPPFLAGS += -I/space/mhier/builddirs/u14/DOOCS_Adapter/include
#include ../CPP_DEBUG_FLAGS.CONFIG

BOOST_MPL_FLAGS=-DFUSION_MAX_VECTOR_SIZE=30 -DBOOST_MPL_LIMIT_VECTOR_SIZE=30 -DFUSION_MAX_DEQUE_SIZE=30 -DFUSION_MAX_MAP_SIZE=30 -DBOOST_FUSION_DONT_USE_PREPROCESSED_FILES -DBOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
CPPFLAGS += $(BOOST_MPL_FLAGS) -std=c++11

CPPFLAGS += -Wall -Wextra -Wshadow -pedantic -Wuninitialized $(CPP_DEBUG_FLAGS)
CPPFLAGS += -I../include -I../example -Iinclude -isystem/local/lib/include
LDFLAGS += -lboost_thread -lboost_system
#link the adapter with runpath, so it is found at execution time
LDFLAGS += -L$(ADAPTER_OBJDIR) -lDoocsAdapter -Wl,-rpath=$(ADAPTER_OBJDIR),--enable-new-dtags

all: $(ALLPROGS)

$(OBJDIR)/.depend depend:
		@if [ ! -f $(OBJDIR) ] ; then \
		  echo ---------- create dir $(OBJDIR) --------------; \
		  mkdir -p $(OBJDIR) ; \
		fi
		for i in $(SRCDIR)/*.cc ;do $(CCDEP) $$i ;done > $(OBJDIR)/.depend_temp
		cat $(OBJDIR)/.depend_temp | sed -e "/:/s/^/\$$\(OBJDIR\)\//g" > $(OBJDIR)/.depend
		chmod g+w $(OBJDIR)/.depend*

include $(OBJDIR)/.depend

$(OBJDIR)/InstaCoSADevExample_server: $(SOURCEOBJ)	
		$(LINK.cc) \
		-o $(OBJDIR)/InstaCoSADevExample_server $(SOURCEOBJ) \
		           -lEqServer -lDOOCSapi \
			   $(LDFLAGS) $(LDLIBS)
		@chmod g+w $(OBJDIR)/InstaCoSADevExample_server
		@echo "---------------- $(OBJDIR)/InstaCoSADevExample_server done---------------"

static $(OBJDIR)/static_InstaCoSADevExample_server:    $(SOURCEOBJ)	
		$(LINK.cc.static) $(LDFLAGS) -o $(OBJDIR)/static_InstaCoSADevExample_server $(SOURCEOBJ) \
		           -lEqServer -lDOOCSapi \
			   $(LDLIBS) 
		@chmod g+w $(OBJDIR)/static_InstaCoSADevExample_server
		@echo "----------------$(OBJDIR)/static_InstaCoSADevExample_server done---------------"

clean:
	rm -f $(SOURCEOBJ) $(OBJDIR)/*.o $(OBJDIR)/cosade_server $(OBJDIR)/.depend* *.gcda *.gcno

test: $(OBJDIR)/CTestTestfile.cmake
	(cd $(OBJDIR); ctest)

$(OBJDIR)/CTestTestfile.cmake: CTestTestfile.cmake.in
	cat $< | sed "{s|@__OBJDIR__@|$(OBJDIR)|}" > $@
