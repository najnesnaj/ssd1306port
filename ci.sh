#!/bin/bash

#wget -q https://launchpad.net/gcc-arm-embedded/5.0/5-2015-q4-major/+download/gcc-arm-none-eabi-5_2-2015q4-20151219-linux.tar.bz2
#bzip2 -q -dc gcc-arm-none-eabi-5_2-2015q4-20151219-linux.tar.bz2 | tar xf -
#rm gcc-arm-none-eabi-5_2-2015q4-20151219-linux.tar.bz2
#chmod +x $PWD/gcc-arm-none-eabi-5_2-2015q4/bin/*
#ls -al $PWD/gcc-arm-none-eabi-5_2-2015q4/bin
mkdir -p sdk
wget -q https://developer.nordicsemi.com/nRF5_SDK/nRF5_SDK_v11.x.x/nRF5_SDK_11.0.0_89a8197.zip -O sdk.zip
unzip -qq sdk.zip -d ./sdk
rm sdk.zip
export NRF51_SDK_DIR=$PWD/sdk
#echo "GNU_INSTALL_ROOT := `pwd`/gcc-arm-none-eabi-5_2-2015q4"$'\r\n'"GNU_VERSION := 5.2.1"$'\r\n'"GNU_PREFIX := arm-none-eabi"$'\r\n' > ${NRF51_SDK_DIR}/components/toolchain/gcc/Makefile.posix
#mkdir -p $NRF51_SDK_DIR/components/drivers_ext/segger_rtt
git clone https://github.com/najnesnaj/ssd1306port.git 
cp ./ssd1306/bsp/* $NRF51_SDK_DIR/examples/bsp
echo $NRF51_SDK_DIR
echo $PATH
make
