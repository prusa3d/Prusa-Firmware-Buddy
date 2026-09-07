# RTT metrics serialization format

Each serialized metric is a `MetricWrapper` produced by `serialize_metric()`.
Encoding is done with [bitsery] (little-endian, no padding). Every record is
laid out as a fixed-size **header** followed by a metric-specific **data**
section.

## Header

| Field       | Size    | Description                                         |
|-------------|---------|-----------------------------------------------------|
| metric type | 1 byte  | `MetricType` enum, identifies the data that follows |
| timestamp   | 4 bytes | `uint32_t`, see [Units](#units)                     |

## Data

The data section immediately follows the header. Its structure and size are
specific to the metric type given in the header. A reader must dispatch on the
metric type to know how many bytes to consume and how to interpret them.

### `RawAcceleration`

| Field  | Size    | Description      |
|--------|---------|------------------|
| val[0] | 2 bytes | `int16_t` X axis |
| val[1] | 2 bytes | `int16_t` Y axis |
| val[2] | 2 bytes | `int16_t` Z axis |

### `LoadcellTaredZ`

| Field  | Size    | Description   |
|--------|---------|---------------|
| z_load | 4 bytes | `float` value |

### `StepperPositions`

| Field    | Size    | Description      |
|----------|---------|------------------|
| steps[0] | 4 bytes | `int32_t` X axis |
| steps[1] | 4 bytes | `int32_t` Y axis |
| steps[2] | 4 bytes | `int32_t` Z axis |
| steps[3] | 4 bytes | `int32_t` E axis |

The array length is fixed at `stepper_count` (4), so the payload is always 16
bytes. `stepper_count` mirrors the firmware's `XYZE_N` (asserted at compile
time); if it ever changes, this layout — and every decoder — must change with
it.

## Units

### Timestamp

Microseconds since system start (`ticks_us()`). There is no defined epoch — it
is a monotonic uptime counter, not a wall-clock time. As a `uint32_t` it wraps
around roughly every 71 minutes.

### `RawAcceleration`

Raw, unscaled sample values as read from the accelerometer chip
(`lis2dh12_acceleration_raw_get()`). The physical meaning of a count therefore
depends on the chip and its current configuration (full-scale range and
resolution mode); this format does not carry that scaling. To convert to
acceleration, the consumer must apply the chip-specific factor.

### `LoadcellTaredZ`

The tared Z load (`Loadcell::get_tared_z_load()`) in **grams**, computed on the
firmware as `scale * (raw - offset)`. Mind that the value is *tared* and
therefore represents change instead of precise measurement of the chip itself.

### `StepperPositions`

Per-stepper position in **steps** (`Stepper::position_from_startup()` per axis)
— a signed count relative to the position at power-on, not an absolute machine
coordinate. Elements are indexed by `AxisEnum` (X, Y, Z, E). Converting to
millimetres needs the per-axis steps-per-mm, which this format does not carry.
On CoreXY kinematics the X/A and Y/B entries are motor-space counters (a single
Cartesian move changes both), not Cartesian axes.

## Transport

The serialized packet is a single logical frame. Before transport it is run
through [COBS] encoding, which produces a byte-stuffed frame delimited by a
zero byte, so that frame boundaries survive the byte stream. The receiver
COBS-decodes the frame back into the packet described above.

[bitsery]: https://github.com/fraillt/bitsery
[COBS]: https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing
