CXXFLAGS+= -std=c++17 -g
INC+=-I../../../include -I../../../include_interfaces -I/opt/local/Library/Frameworks/Python.framework/Versions/3.8/include/python3.8/ -I$(shell python3.8 -c "import numpy; print(numpy.get_include())") -DNPY_NO_DEPRECATED_API
LIB_PATH+=-L$(DEST_DIR_EXTENSION) -L/opt/local/Library/Frameworks/Python.framework/Versions/3.8/lib/
LDFLAGS+= -lg2s -lz -lpython3.8

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