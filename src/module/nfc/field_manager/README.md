# field_manager

This module provides `nfcv::FieldManager`, which owns the lifetime of the RF field of a
`nfcv::ReaderWriterInterface`: which antenna radiates, for whom, and until when.

It implements the listen-before-talk arbitration (randomised backoff on a foreign field), the
trailing linger that lets a burst of operations share a single field bring-up, and the absolute
hold cap that hands contending neighbours a window. Callers state their intent by holding a
`FieldManager::Guard` rather than by driving the field directly.
