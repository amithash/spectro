import os
import re
env = Environment()
pwd = os.getcwd()
env.Append(CCFLAGS = ['-Wall'])
env.Append(CCFLAGS = ['-mtune=native', '-march=native', '-mfpmath=sse', '-msse4'])
env.Append(CPPPATH = [pwd + '/include'])
env.Append(LIBPATH = [pwd + '/lib/decoder', pwd + '/lib/spectgen', pwd + '/lib/histdb'])

#env.Append(CCFLAGS = ['-g'])
env.Append(CCFLAGS = ['-O3'])

# Let the programs know how much memory you have
for line in open('/proc/meminfo', 'r').read().split('\n'):
	if(not line.strip().find("MemTotal: ")):
		numbers = re.search("([0-9]+)", line.strip()).group(0)
		env.Append(CCFLAGS = '-DMEMORY_SIZE=' + numbers)

Export('env')

subdirs = [
	"lib",
	"utils",
	"spectradio",
	"experiments"
]
SConscript(dirs = subdirs, name = "SConstruct")
