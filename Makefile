all:
	+make -C spectradio
	+make -C lib
	+make -C utils
	+make -C spectrogen
	+make -C experiments
clean:
	+make -C spectradio clean
	+make -C lib clean
	+make -C utils clean
	+make -C spectrogen clean
	+make -C experiments clean

install:
	+make -C spectradio install
	+make -C lib install
	+make -C utils install
	+make -C spectrogen install

uninstall:
	+make -C spectradio uninstall
	+make -C lib uninstall
	+make -C utils uninstall
	+make -C spectrogen uninstall

