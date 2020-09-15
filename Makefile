CXXFLAGS+= -std=c++17
INC+=-I../../../include -I../../../include_interfaces -I$(shell python3 -c "from sysconfig import get_path; print(get_path('include'))")/ -I$(shell python3 -c "import numpy; print(numpy.get_include())") -DNPY_NO_DEPRECATED_API
LIB_PATH+=-L$(DEST_DIR_EXTENSION) -L$(shell python3 -c "import sysconfig; print(sysconfig.get_config_var('LIBDIR'))") $(shell python3 -c "import sysconfig; print(sysconfig.get_config_var('LINKFORSHARED'))")
LDFLAGS+= -lg2s -lz $(shell python3 -c "import sysconfig; print(sysconfig.get_config_var('BLDLIBRARY'))")

%.o: src/%.cpp 
	$(CXX) -c -o $@ $< $(CFLAGS) $(CXXFLAGS) $(INC) $(LIBINC)

pyDev: pyDev.o
	$(CXX) -o $@ $^ $(LIB_PATH) $(LDFLAGS)

build: pyDev
	echo "build pyDev"

install: build
	cp -f pyDev $(DEST_DIR_EXTENSION)
	mkdir -p $(DEST_DIR_EXTENSION)/../PyDevAlgo
	cp  PyDevAlgo/* $(DEST_DIR_EXTENSION)/../PyDevAlgo/
	echo "PyDev	./pyDev -di" >> $(DEST_DIR_EXTENSION)/algosName.config
	echo "pyDev	./pyDev -di" >> $(DEST_DIR_EXTENSION)/algosName.config
	echo "pydev	./pyDev -di" >> $(DEST_DIR_EXTENSION)/algosName.config

algoNames:
	ln -sf ../algosName.config algosName.config

clean:
	rm -rf *.o
	rm -rf pyDev