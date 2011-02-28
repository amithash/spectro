import os
env = Environment()
Export('env')

subdirs = [
	"spectgen",
	"spectradio",
	"lib",
	"utils",
	"experiments"
]

installdirs = [
	"spectgen",
	"spectradio",
	"lib",
	"utils",
]

if 'uninstall' in COMMAND_LINE_TARGETS:
	for dir in installdirs:
		os.system("scons -C " + dir + " uninstall")
else:
	SConscript(dirs=subdirs, name = "SConstruct")
