all:
	+make -C spectradio
	+make -C lib
	+make -C utils
	+make -C spectgen
	+make -C experiments
clean:
	+make -C spectradio clean
	+make -C lib clean
	+make -C utils clean
	+make -C spectgen clean
	+make -C experiments clean

install:
	+make -C spectradio install
	+make -C lib install
	+make -C utils install
	+make -C spectgen install
	cp genhistdb.pl /usr/local/bin/genhistdb
	chmod +x /usr/local/bin/genhistdb

uninstall:
	+make -C spectradio uninstall
	+make -C lib uninstall
	+make -C utils uninstall
	+make -C spectgen uninstall
	rm -f /usr/local/bin/genhistdb

