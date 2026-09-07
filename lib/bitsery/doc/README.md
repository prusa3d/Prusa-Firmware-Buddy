To get the most out of **Bitsery**, start with the [tutorial](tutorial/README.md).
Once you're familiar with the library consider the following reference material.

Library design:
* `fundamental types`
* `valueNb instead of value`
* `brief syntax`
* `serializer/deserializer functions overloads`
* [extending library functionality](design/extensions.md)
* `errors handling`
* `forward/backward compatibility via Growable extension`
* [pointers](design/pointers.md)
* `inheritance`
* `polymorphism`


Core Serializer/Deserializer functions (alphabetical order):
* `operator()` (4.6.1) (when brief syntax is enabled)
* `adapter` (5.0.0)
* `boolValue` (4.0.0)
* `context<T>` (4.1.0)
* `contextOrNull<T>` (4.2.0)
* `enableBitPacking` (4.0.0)
* `ext` (2.0.0)
* `object` (1.0.0)
* `text` (1.0.0)
* `value` (1.0.0)

Serializer/Deserializer extensions via `ext` method (alphabetical order):
* `BaseClass` (4.2.0)
* `CompactValue` (4.4.0)
* `CompactValueAsObject` (4.4.0)
* `Entropy` (3.0.0)
* `Growable` (3.0.0)
* `PointerOwner` (4.1.0)
* `PointerObserver` (4.1.0)
* `ReferencedByPointer` (4.1.0)
* `StdDuration` (4.6.0)
* `StdMap` (3.0.0)
* `StdOptional` (2.0.0)
* `StdQueue` (4.0.0)
* `StdSet` (4.0.0)
* `StdSmartPrt` (4.3.0)
* `StdStack` (4.0.0)
* `StdTimePoint` (4.6.0)
* `StdTuple` (4.6.0) (requires c++17)
* `StdVariant` (4.6.0) (requires c++17)
* `ValueRange` (3.0.0)
* `VirtualBaseClass` (4.2.0)

Input adapters (buffer and stream) functions:
* `align`
* `readBits`
* `readBytes`
* `readBuffer`
* `currentReadPos (get/set)` (buffer adapter only)
* `currentReadEndPos (get/set)` (buffer adapter only)
* `error (get/set)`
* `isCompletedSuccessfully`

Output adapters (buffer and stream) functions:
* `align`
* `writeBits`
* `writeBytes`
* `writeBuffer`
* `flush`
* `currentyWritePos (get/set)` (buffer adapter only) gets/sets write position in buffer, it can jump past the buffer end, in this case buffer will be resized.
This function doesn't write any bytes.
* `writtenBytesCount` (buffer adapter only) this doesn't necessary mean how many bytes are written, but rather how many bytes in the buffer was "affected" during serialization.
E.g. if `currentyWritePos` (set) jumps from 0 to 100, and then 4 bytes are written, `writtenBytesCount` return 104, it also returns 104 if you jump in somewhere in the middle.


Tips and tricks:
* if you're getting static assert "please define 'serialize' function", please define **serialize** function in same namespace as object, or in **bitsery** namespace, for more info [ADL](https://en.cppreference.com/w/cpp/language/adl).

Other:
* [Contributing](../CONTRIBUTING.md)
* [Change log](../CHANGELOG.md)
