this is a mixture of 
https://github.com/bertrandmartel/adafruit-oled-st7735-dk51
and
https://github.com/monpetit/nrf52-spi-i2c-master-ssd1306
and
https://github.com/najnesnaj/platformio-id107/tree/master/lib/squix78_OLED 

sdk11 of nordic was used
wget https://developer.nordicsemi.com/nRF5_SDK/nRF5_SDK_v11.x.x/nRF5_SDK_11.0.0_89a8197.zip

adapt Makefile.posix  (see example in this repository)
<SDK11>/components/toolchain/gcc/Makefile.posix

the purpose is to get a working ssd1306 oled display on a ID107 smartwatch
