
all:
	cmake -B build . $(if $(D),-DCMAKE_BUILD_TYPE=Debug,-DCMAKE_BUILD_TYPE=Release) $(CMAKEARGS)
	$(MAKE) -C build $(if $(V),VERBOSE=1)

vc:
	"C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe" -G"Visual Studio 16 2019" -B build .
	"C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/MSBuild/Current/Bin/amd64/MSBuild.exe" build/extfstools.sln -t:Rebuild


clean:
	$(RM) -r build CMakeFiles CMakeCache.txt CMakeOutput.log
