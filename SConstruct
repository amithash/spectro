#*******************************************************************************
#    This file is part of spectro
#    Copyright (C) 2011  Amithash Prasad <amithash@gmail.com>
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
#*******************************************************************************
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
