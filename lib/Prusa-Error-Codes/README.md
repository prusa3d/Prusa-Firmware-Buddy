# Prusa-Error-Codes

## Error code format <ErrorCode>

XXYZZ

- XX - number of printer according to USB PID
- Y - error category (common for all printers)
- ZZ - specific error code

Example: 12201

12 - printer number 12: Original Prusa MINI

2 - error category: temperature error

01 - specific error code: Heatbed heating failed

## Error categories
1. Mechanical - XYZ motors, tower, axis range
2. Temperature - thermistors/heating
3. Electronics - MINDA, FINDA, Motion Controller, …
4. Connection - Wi-Fi, LAN, Prusa Connect Cloud
5. System - FW crash, …
6. Bootloader - FW update, USB/SD card fail
7. Warnings
