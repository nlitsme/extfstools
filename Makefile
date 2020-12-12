
all:
	cmake -B build . $(if $(D),-DCMAKE_BUILD_TYPE=Debug,-DCMAKE_BUILD_TYPE=Release) $(CMAKEARGS) $(if $(LOG),-DWITH_LOGGING=ON)
	$(MAKE) -C build $(if $(V),VERBOSE=1)

clean:
	$(RM) -r build
