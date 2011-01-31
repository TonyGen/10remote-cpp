libname = 'remote'

lib = Library (libname, Glob('*.cpp'), CPPPATH = ['.', '/opt/local/include'])

Alias ('install', '/usr/local')
Install ('/usr/local/lib', lib)
Install ('/usr/local/include/' + libname, Glob('*.h'))
