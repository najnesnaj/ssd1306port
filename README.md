this is software for the ID107HR smartwatch, which contains a nrf51822 from nordic

it is based upon

https://github.com/monpetit/nrf52-spi-i2c-master-ssd1306
https://github.com/najnesnaj/platformio-id107/tree/master/lib/squix78_OLED 

sdk11 of nordic was used
wget https://developer.nordicsemi.com/nRF5_SDK/nRF5_SDK_v11.x.x/nRF5_SDK_11.0.0_89a8197.zip

under sdk11/examples/bsp replace boards.h and add id107.h from the bsp directory in this repository


adapt Makefile.posix  (see example in this repository)
<SDK11>/components/toolchain/gcc/Makefile.posix

