libname = '10remote'

lib = SharedLibrary (libname, Glob('src/*.cpp'),
	CCFLAGS = ['-pg', '-rdynamic'],
	CPPPATH = ['.', '/usr/local/include'],
	LIBPATH = ['/usr/local/lib'],
	LIBS = Split ('10util dl boost_thread-mt boost_serialization-mt') )

Alias ('install', '/usr/local')
Install ('/usr/local/lib', lib)
Install ('/usr/local/include/' + libname, Glob('src/*.h'))
