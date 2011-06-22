

### Installing

Install dependent library first

- [10util](https://github.com/TonyGen/10util-cpp)

Download and remove '-ccp' suffix

	git clone git://github.com/TonyGen/10remote-cpp.git 10remote
	cd 10remote

Build library `lib10remote.so`

	scons

Install library in `/usr/local/lib` and header files in `/usr/local/include/10remote`

	sudo scons install
