all:
	+scons -C spectradio
	+scons -C lib
	+scons -C utils
	+scons -C experiments
	+scons -C spectgen
clean:
	+scons -C spectradio -c
	+scons -C lib -c
	+scons -C utils -c
	+scons -C experiments -c
	+scons -C spectgen -c

install:
	+scons -C spectradio install
	+scons -C lib install
	+scons -C utils install
	+scons -C spectgen install
	cp genhistdb.pl /usr/local/bin/genhistdb
	chmod +x /usr/local/bin/genhistdb

uninstall:
	+scons -C spectradio uninstall
	+scons -C lib uninstall
	+scons -C utils uninstall
	+scons -C spectgen uninstall
	rm -f /usr/local/bin/genhistdb

