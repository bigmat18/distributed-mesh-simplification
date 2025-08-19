.PHONY: all clean

all:
	bash ./build_all.sh

debug:
	cmake --build build/Debug -- -j

release:
	cmake --build build/Release -- -j

reldeb:
	cmake --build build/RelWithDebInfo -- -j

clean:
	rm -rf build/ out/
