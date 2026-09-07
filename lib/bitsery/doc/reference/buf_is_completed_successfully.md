### BufferReader.isCompletedSuccessfully()

Returns true when buffer was fully read, and there was no [errors](buf_get_error.md).

If buffer contains multiple serialized objects and you want to know deserialization state for each object, then use [getError](buf_get_error.md) after reading each object.

Use this function when buffer contains one object, or you know that you read all data.
