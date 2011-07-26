libname = '10remote'

lib = SharedLibrary (libname, Glob('src/*.cpp'),
	CCFLAGS = ['-pg', '-rdynamic'],
	CPPPATH = ['.', '/opt/local/include'],
	LIBPATH = ['/opt/local/lib'],
	LIBS = Split ('10util boost_thread-mt boost_serialization-mt') )

Alias ('install', '/usr/local')
Install ('/usr/local/lib', lib)
Install ('/usr/local/include/' + libname, Glob('src/*.h'))
