# Prusa-Error-Codes

## Error code format <ErrorCode>

XXYZZ
- XX - number of printer according to USB PID
- Y - error category (common for all printers)
- ZZ - specific error code

Example: 12201
* 12 - printer number 12: Original Prusa MINI+
* 2 - error category: temperature error
* 01 - specific error code: Heatbed heating failed

## Printer number
* 04 `MMU` - Original Prusa MMU
* 10 `SL1` - Original Prusa SL1/SL1S
* 12 `MINI` - Original Prusa MINI/MINI+
* 13 `MK4` - Original Prusa MK4
* 16 `iX` - AFS IX
* 17 `XL` - Original Prusa XL
* 21 `MK3.9` - Original Prusa MK3.9
* 23 `MK3.5` - Original Prusa MK3.5
* 26 `MK4S` - Original Prusa MK4S
* 27 `MK3.9S` - Original Prusa MK3.9S
* 28 `MK3.5S` - Original Prusa MK3.5S
* 29 `M1` - Original Medical One
* 31 `COREONE` - Prusa Core One

## Error categories
1. Mechanical - XYZ motors, tower, axis range
2. Temperature - thermistors/heating
3. Electronics - MINDA, FINDA, Motion Controller, …
4. Connection - Wi-Fi, Ethernet, Prusa Connect
5. System - FW crash, …
6. Bootloader - FW update, USB/SD card fail
7. Warnings
8. Dialogs for Connect - not really errors, identifiers for communicating
   dialogs to the server. If there's an error in one of the above categories, it
   is possible to reuse that error code directly, no need to create a duplicate
   one in this category.
9. Other 

More information about the error codes can be found at:
[prusa.io/error-codes](https://prusa.io/error-codes)


## YAML files structure (Buddy)
The `.yaml` format structure is as follows:

* Root [dict]
   * `Errors` [list of dict]: Specific error codes
      * `printers` (optional) [list of string]: same as root-level printer filter
      * `code` [string]: Error code in the format `XXYZZ`
         * Leave `XX` as `XX`, the code applies to multiple printers.
         * For example `XX101`
      * `title` [string]: Error message title
      * `text` [string]: Error message string
      * `id` [string]: Error identifier used for referencing the error in the code
         * For example `BED_MINTEMP_ERROR`
      * `approved` [bool]: Not really good for anything
