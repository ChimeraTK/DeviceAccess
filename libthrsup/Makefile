#===================================================================================
CXX             = g++
OBJ_PATH        = ./obj
SRC_PATH        = ./src
PROG_NAME       = libthrsup
LIB_PATH        = ../../lib
INCLUDE_PATH    = ./include ../../lib/include
LIB             = 
CXXFLAGS        = -Wall $(INCLUDE_PATH:%=-I%) -g -fPIC -O3  -D__DEBUG_MODE__  
#===================================================================================

VPATH = $(SRC_PATH):$(OBJ_PATH):$(INCLUDE_PATH)
ALL =   $(wildcard $(SRC_PATH)/*.cpp)
OBJ =   $(ALL:$(SRC_PATH)/%.cpp=%.o)
DEP =   $(ALL:$(SRC_PATH)/%.cpp=%.d) 


%.o : %.cpp
	mkdir -p $(OBJ_PATH)
	$(CXX) -c $(CXXFLAGS) $< -o $(OBJ_PATH)/$@

$(PROG_NAME).a :    $(OBJ)
			mkdir -p $(OBJ_PATH)
			ar  rcs $(PROG_NAME).a $(OBJ:%.o=$(OBJ_PATH)/%.o)
			cp $(PROG_NAME).a  ../../lib/
			cp ./include/*.h ../../lib/include/
			make libtest

libtest:
			$(CXX) $(CXXFLAGS) -o test-$(PROG_NAME) test-$(PROG_NAME).cpp  -L./ -l$(PROG_NAME:lib%=%) -L../../lib/  -lexbase -pthread

$(DEP): %.d : %.cpp
	@$(CXX) -MM $< $(CXXFLAGS) > $@  
	@cp $@ $@.tmp
	@sed -e s/\\.o/\\.d/ < $@.tmp >> $@
	@rm $@.tmp

-include $(DEP)

clean:
	-rm -f  $(DEP)
	-rm -f  $(OBJ_PATH)/*
	-rm -f  $(PROG_NAME).a
	-rm -f  test-$(PROG_NAME)


.PHONY :  clean libtest
