# NooBot-PowerModule

The power module for NooBot, used to control the power of the bot and monitor the battery voltage.

## Hardware
* STM32F030F4P6 as the main controller
* TPS54202 for the standby power supply
* A button to control the power
* 4 LEDs to indicate the power status
* I2C interface for communication with the bot

## Software
The project is generated as a CMake project using STM32CubeMX
and uses the HAL library for the STM32F0 series.  

You can use configure the project using STM32CubeMX
and then build it using CMake.  

To flash it to the STM32F0, you can use ST-Link or any other ARM debugger with SWD support.