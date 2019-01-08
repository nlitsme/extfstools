all: ext2rd ext2dump

ifeq ($(D),)
OPT=-O3
else
OPT=-O0
endif

clean:
	$(RM) $(wildcard *.o) *.dSYM ext2rd ext2dump

ext2rd: ext2rd.o  stringutils.o
ext2dump: ext2dump.o stringutils.o

CXXFLAGS=-g -Wall -c $(OPT) -I itslib -D_UNIX -D_NO_RAPI -I /usr/local/include -I . -std=c++11

# include CDEFS from make commandline
CXXFLAGS+=$(CDEFS) -MD

LDFLAGS+=-g -Wall -L/usr/local/lib -std=c++11

vpath .cpp itslib/src .

%.o: itslib/src/%.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@ 

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(filter %.cpp,$^) -o $@ 

%: %.o
	$(CXX) $(LDFLAGS) $^ -o $@


install:
	cp ext2rd ~/bin/
