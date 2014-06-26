all: main

scan: main
	../../../scan-build/scan-build -plist -enable-checker core.NullDereference \
	--use-analyzer="../../../../../../../cmake-debug/bin/clang-3.5" -o ../scan_results make main

main: main.cpp library
	$(CXX) -I/usr/include/c++/4.2.1/ -Wl,-L/usr/lib/ -Wl,-lstdc++ main.cpp -llibrary -Lbin -o bin/main

library: library.cpp
	$(CXX) -I/usr/include/c++/4.2.1/ -Wl,-L/usr/lib/ -Wl,-lstdc++ library.cpp -shared -o bin/liblibrary.so 

clean:
	rm -rf bin/*