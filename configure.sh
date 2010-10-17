#!/bin/bash

# CONFIGURE SPECTROGEN
cd ./spectrogen
./configure
cd ../

# CONFIGURE spectradio
cd spectradio
qmake
cd ../


