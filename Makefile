# targets:
#   all            - build all binaries using cmake
#   ninja          - build all binaries using google-ninja
#   vc             - build all binaries using cmake + msvc
#   clean          - remove the build directory

# Transform Makefile arguments to CMake args
CMAKEARGS_LOCAL+=$(if $(GENERATOR),-G"$(GENERATOR)")
CMAKEARGS_LOCAL+=$(if $(D),-DCMAKE_BUILD_TYPE=Debug,-DCMAKE_BUILD_TYPE=Release)
CMAKEARGS_LOCAL+=$(if $(LOG),-DOPT_LOGGING=ON)
CMAKEARGS_LOCAL+=$(if $(and $(LOG), $(D)),-DOPT_LOGGING_UDP=ON)
CMAKEARGS_LOCAL+=$(if $(COV),-DOPT_COV=1)
CMAKEARGS_LOCAL+=$(if $(SYM),-DOPT_SYMBOLS=1)
CMAKEARGS_LOCAL+=$(if $(PROF),-DOPT_PROF=1)
CMAKEARGS_LOCAL+=$(if $(LIBCXX),-DOPT_LIBCXX=1)
CMAKEARGS_LOCAL+=$(if $(STLDEBUG),-DOPT_STL_DEBUGGING=1)
CMAKEARGS_LOCAL+=$(if $(SANITIZE),-DOPT_ASAN=1) # backward compatibility
CMAKEARGS_LOCAL+=$(if $(ASAN),-DOPT_ASAN=1)
CMAKEARGS_LOCAL+=$(if $(TSAN),-DOPT_TSAN=1)
CMAKEARGS_LOCAL+=$(if $(CLANGTIDY),-DOPT_CLANG_TIDY=1)
CMAKEARGS_LOCAL+=$(if $(ANALYZE),-DOPT_ANALYZE=1)
CMAKEARGS_LOCAL+=$(if $(TEST),-DBUILD_TESTING=1)
CMAKEARGS_LOCAL+=$(if $(TOOLS),-DBUILD_TOOLS=1)
CMAKEARGS_LOCAL+=$(if $(COMPCMD),-DOPT_COMPILE_COMMANDS=1)

# add user provided CMAKEARGS
CMAKEARGS_LOCAL+=$(CMAKEARGS)

CMAKE=cmake
CTEST=ctest

.DEFAULT_GOAL:=all
.PHONY: all
all: build


# uses os default generator (if not GENERATOR is provided)
.PHONY: generate
generate:
	$(CMAKE) -B build $(CMAKEARGS_LOCAL)

JOBSFLAG=$(filter -j%,$(MAKEFLAGS))
# only build without regenerating build system files (prevents overwrite of previous provided ARGS for generate)
.PHONY: build
build: generate
	$(CMAKE) --build build  $(JOBSFLAG) $(if $(V),--verbose) --config $(if $(D),Debug,Release)

# runs all tests
ctest: TEST=1
ctest: all
	$(CTEST) --verbose --test-dir build -C $(if $(D),Debug,Release) $(TESTARGS)

llvm: export CC=clang
llvm: export CXX=clang++
llvm: all


SCANBUILD=$(firstword $(wildcard /usr/bin/scan-build*))
scan: CMAKE:=$(SCANBUILD) -o $$(pwd)/build/scanbuild $(CMAKE)
scan: ctest

vc: CMAKE:=cmake.exe
vc: GENERATOR=Visual Studio 16 2019
vc: CMAKEARGS+=$(VC_CMAKEARGS)
vc: all

nmake: CMAKE:=cmake.exe
nmake: GENERATOR=NMake Makefiles
nmake: CMAKEARGS+=$(VC_CMAKEARGS)
nmake: all

clean:
	$(RM) -r build CMakeFiles CMakeCache.txt CMakeOutput.log
	$(RM) $(wildcard *.gcov)

