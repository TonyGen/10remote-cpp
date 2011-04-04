Install dependent library first

- [10util](https://github.com/TonyGen/10util-cpp)

Download and remove '-ccp' suffix
	git clone git://github.com/TonyGen/remote-cpp.git remote
	cd remote

Build library `libremote.a`
	scons

Install library in `/usr/local/lib` and header files in `/usr/local/include/remote`
	sudo scons install
