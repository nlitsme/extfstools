all: ext2rd ext2dump

ifeq ($(D),)
OPT=-O3
else
OPT=-O0
endif

ifneq ($(wildcard $(SystemRoot)/explorer.exe $(SYSTEMROOT)/explorer.exe),)
OSTYPE=windows
endif

O=$(if $(filter $(OSTYPE),windows),.obj,.o)
E=$(if $(filter $(OSTYPE),windows),.exe,)
clean:
	$(RM) -r $(wildcard *$(O)) *.dSYM ext2rd ext2dump

ext2rd$(E): ext2rd$(O)  stringutils$(O)
ext2dump$(E): ext2dump$(O) stringutils$(O)

CXXFLAGS+=-g -Wall -c $(OPT) -I itslib -D_UNIX -D_NO_RAPI -I /usr/local/include -I . -std=c++17
CXXFLAGS+=$(if $(filter $(OSTYPE),windows),-I c:/local/boost_1_74_0)

# include CDEFS from make commandline
CXXFLAGS+=$(CDEFS) -MD

CXXFLAGS+=-DNOMINMAX

LDFLAGS+=-g -Wall -L/usr/local/lib -std=c++17

vpath .cpp itslib/src .

%$(O): itslib/src/%.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@ 

%$(O): %.cpp
	$(CXX) $(CXXFLAGS) $(filter %.cpp,$^) -o $@ 

%$(E): %$(O)
	$(CXX) $(LDFLAGS) $^ -o $@


install:
	cp ext2rd ~/bin/
