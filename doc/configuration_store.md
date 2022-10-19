# Configuration store
Configuration store is replacement of current EEPROM implementation. This implementation should solve these problems:
- need to add things to multiple files in FW
- unaven wearing
- removes the need to use variant

It is split into two layers. Configuration store and Eeprom access. Configuration store is structure which holds the data in runtime and enables access to items. Eeprom access enables access to the eeprom itself.
## Configuration store
Configuration store is structure which holds items as its members and each item represents data in eeprom.
Items are initialized with default values and then eeprom access is initialized and loads changed values to the items.
Item is stored in eeprom only when its value is changed. So it wont take up space if the value is left as default value.
The items are generated from JSON with python script. This python script support creating:
- Simple one value items
- Arrays of values
- Struct items
- Strings

## Eeprom access
This layer stores the data in eeprom in journal like style. Each write is written after the previous one. When end of bank is reached, cleanup routine is called.
It only supports write operation during runtime. Reading data is possible only during initialization. To be able to read data during runtime we would need some cache for storing position of each item to prevent O(n) complexity for read.
### Banks
The EEPROM is split in two same sized banks and one bank is in use. Eeprom access stores which bank is used in byte before the first bank. End of bank is marked with 0xFF.
### Item
| size | data | crc32  |
|------|------|--------|
Each item consists of size, data and crc32. And data is serialized as msgpack.
### Cleanup routine
Cleanup routine copies only newest values for each key to the new bank. The old bank is left valid in case new bank gets corupted.
