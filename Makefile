all:
	+make -C spectradio
	+make -C utils
	+make -C spectrogen
clean:
	+make -C spectradio clean
	+make -C utils clean
	+make -C spectrogen clean
