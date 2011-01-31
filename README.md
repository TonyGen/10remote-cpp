Install dependent library first
- [job-cpp](https://github.com/TonyGen/job-cpp)

Remove '-ccp' suffix when downloading
	git clone git://github.com/TonyGen/remote-cpp.git remote
	cd remote

Build library `libremote.a`
	scons

Install library in `/usr/local/lib` and header files in `/usr/local/include/remote`
	sudo scons install
