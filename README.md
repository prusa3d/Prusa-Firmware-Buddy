Original Readme.md: https://github.com/prusa3d/Prusa-Firmware-Buddy/blob/master/README.md

I wanted to share my progress and findings with the prospect of printing high-temperature filaments with the Prusa Core ONE. This fork assumes the following:

1. You have broken off the appendix on the xbuddy board.
2. You have a Slice Engineering Copperhead hotblock, a PT1000, a 50W heater cartridge, and the means to either splice or recrimp the Molex Clik-Mate connector for the PT1000, and the Microfit connector for the heater. (Thermal paste/grease is completely optional, but helps with heat transfer.)
3. Have Visual Studio Code to compile firmware.

The main changes this fork provides are pretuned PID settings, A maximum temperature of 350C, and support for a PT1000 temperature sensor.
