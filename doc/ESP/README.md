# Original Prusa MINI printer WiFI network connection #
Recently you can find in our Prusa-Firmware-Buddy/Master repository a wireless printer network connection feature codes. In order to be able to connect your MINI to the wireless network and run the PrusaLink feature you will need an ESP01 board connected to the MINI and execute the below tasks. Some of the ESP01 boards are already flashed with FW so you could skip the ESP Flashing routine. 
Be aware please, that the Prusa-Firmware-Buddy/Master repository is a **development environment** and features are under development as well, so we can not guarantee that the feature will be 100% working. Building your own FW from these pages instead of using official release is on **your solely risk**. 


## ESP board Flashing  ##

- Build the last MINI firmware from [https://github.com/prusa3d/Prusa-Firmware-Buddy](https://github.com/prusa3d/Prusa-Firmware-Buddy) and load to printer
- Download the appropriate files from[ ESP8266 NonOS AT Bin V1.7.4](https://www.espressif.com/en/support/download/at?keys=&field_type_tid%5B%5D=14&field_type_tid%5B%5D=799) or you can use the . bin files in attached .zip file. 
- Move all 4 files to root/ESP folder of the USB flash drive in , plug in the ESP card and the USB drive to printer.
- Make sure the **Settings->Menu->Timeout** is Off (otherwise the ESP Update menu will time out and interrupt the update)
- Run **Settings->ESP Update->FLASH ESP AT** 
- Wait until you will see the "**ESP succesfully flashed. WiFI initiation started**"  message
- If you already have loaded your ssid and password into the printer, you should see the WiFi icon ON and in **Settings->Lan settings** you should find the assigned IP address
- - If no ssid and password is loaded, proceed with next chapter..


## Load network ssid and password ##


- In the prusa\_printer_settings.ini file and under [wifi] tag edit the ssid and psk fields with the correct WiFi data (you can omit everything else).

[wifi]
ssid= enter here your recent SSID

psk= enter here the valid passwotrd

key_mgmt=WPA

- Store prusa\_printer_settings.ini file in USB flash drive root folder and plug in to printer
- Run the **Setting-> Load Settings from** command from printer
- Make sure in **Settings -> Lan setting**s is set to WiFi
- Check the WiFI icon to be light and check in **Settings -> Lan settings** if the IP address is assigned. If so your printer is connected to network.



## Connect printer to PRUSA LINK ##
- Make sure the printer is connected to the network the WiFI icon is ON and in **Setting->Lan settings->WiFI** you can find the apropriate IP address
- Open the Browser and copy the IP address
- PrusaLink page should open and request you to enter api-key, which you will find in **Settings->PRUSA LINK->X-Api-Key**
- Enter the api key to the browser
-  Telemetric and graphical information regarding the printer status should be displayed
