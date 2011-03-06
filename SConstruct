import os
env = Environment()
Export('env')

subdirs = [
	"lib",
	"spectradio",
	"utils",
	"experiments"
]

installdirs = [
	"lib",
	"spectradio",
	"utils",
]

if 'uninstall' in COMMAND_LINE_TARGETS:
	for dir in installdirs:
		os.system("scons -C " + dir + " uninstall")
else:
	SConscript(dirs=subdirs, name = "SConstruct", varient_dir='build')
