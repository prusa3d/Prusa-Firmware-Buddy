# Metrics

## What is it?

Let's say you want to fine-tune some real-time algorithm - metrics allow
you to easily specify the values you care about, stream and observe them
in real-time while your algorithm runs on a real printer.

## Quickstart
1. Define a metric

    ```C
    METRIC_DEF(cpu_usage, "cpu_usage", METRIC_VALUE_FLOAT, 100, METRIC_DISABLED);
    ```

    - The first parameter - `cpu_usage` - is the variable name
    - The second parameter - `cpu_usage` - is the metric's name. Keep it short!
    - The third parameter defines the type of data points (values) for this metric.
    - The fourth parameter - `100` - defines the minimal interval between consecutive recorded points in ms.
        - E.g., the value 100 ms makes the `cpu_usage` metric being transmitted at a maximum frequency of 10 Hz.
        - If you want to disable throttling and send the values as fast as possible, set it to 0.
    - The last parameter determines whether the metric should be enabled by default or not.
        - `METRIC_DISABLED` is the safe value you should use for most metrics. It makes the metric disabled by default.


2. Record your values

    ```C
    metric_record_float(&metric_cpu_usage, 0.04);
    ```

3. Enable sending the recorded metrics

    ```gcode
    M334 <ip address> <port>  ; Where to send the metrics
    M331 cpu_usage            ; Enable the metric we just created
    ```

## Becoming a metrics wizard 🧙

### Gcodes

Let you configure your metrics at runtime.

- **M331**` <metric>` -- Enable `metric`.
    - Example: `M331 pos_z`
- **M332**` <metric>` -- Disable `metric`.
    - Example: `M332 pos_z`
- **M333** -- List all metrics and whether they are enabled.
- **M334**` [ip address] [metrics port] [log port]` -- Enable metrics and configure them to be sent to the given target
    * All parameters are optional. Not providing a parameter causes the given setting not be changed.
    * If no parameters are provided, the gcode disables the metrics.


### Custom Metrics

...or sending multiple values per data point.

#### Motivation

Each metric has an associated type of its values.
Some basic ones are `METRIC_VALUE_FLOAT` or `METRIC_VALUE_STRING`, allowing you to send data points with a single value
of the given type (by calling `metric_record_float` or `metric_record_string`, respectively).

This works great in most cases. However, sometimes it comes in handy to send multiple values as one data point. Imagine you want to send information about the current heap usage.

```C
void record_heap_usage() {
    METRIC_DEF(heap, "heap", METRIC_VALUE_INTEGER, ...);
    metric_record_integer(&heap, GetFreeHeapSize());
}
```

This will work. But what if you want to add a second piece of information - the total available heap size, so you know the relative heap usage? Well, we can add a second metric...

```C
void record_heap_usage() {
    METRIC_DEF(heap_free, "heap_free", METRIC_VALUE_INTEGER, ...);
    METRIC_DEF(heap_total, "heap_total", METRIC_VALUE_INTEGER, ...);
    metric_record_integer(&heap_free, GetFreeHeapSize());
    metric_record_integer(&heap_total, GetTotalHeapSize());
}
```

This also works. But as soon as you start using the recorded data, you realize that the data points of the `heap_free` metric have slightly different timestamps from the `heap_total`. To calculate the relative heap usage, you will have to pair data points between the two metrics. That is doable but a lot of work.
Can we do better? Sure we can!

```C
void record_heap_usage() {
    METRIC_DEF(heap, "heap", METRIC_VALUE_CUSTOM, ...);
    metric_record_custom(&heap, " free=%ii,total=%ii", GetFreeHeapSize(), GetTotalHeapSize());
}
```

What happened there? First, we defined a metric of a "custom type".
Next, we recorded a datapoint having **two values**. As those values have the same timestamp and are part of a single data point, it is easy to query them later as a pair, do the math on them (calculating the percents of heap used), etc. Yay!

#### The format

The `METRIC_VALUE_CUSTOM` provides you with great power. In addition to sending multiple values, you can attach multiple tags to the data point. But as always, with great power comes great responsibility! So make sure you call the `metric_record_custom` function with proper arguments.

Our metrics use a slightly adjusted version of the [InfluxDB line protocol](https://docs.influxdata.com/influxdb/v1.8/write_protocols/line_protocol_tutorial/). The full format of a data point in our case might be as follows

    heap,some_tag=tag_value free=82i,total=102i 102
      |  ------------------ -------------------  |
      |             |             |              |
      |             |             |              |
    +-----------+--------+-+---------+-+-----------+
    |metric_name|,tag_set| |value_set| | timestamp |
    +-----------+--------+-+---------+-+-----------+


This is the format metrics use internally. If you use `METRIC_VALUE_INTEGER` or other basic value type, the representation in the line protocol is automatically created for you. However, with `METRIC_VALUE_CUSTOM`, you have the option to manually specify the middle part (`tag_set` and `value_set`).

The result is formatted as `<metric_name><what you provided via metric_record_custom> <timestamp>`.

This is all you need to know. If you are not familiar with it, it is time for you to study the [InfluxDB line protocol](https://docs.influxdata.com/influxdb/v1.8/write_protocols/line_protocol_tutorial/). Make sure you understand why there is a single space at the beginning of `" free=%ii,total=%ii"` (in the example above), or why there is the `i` after the values itself!

### Running Own Server (for the SYSLOG handler)

What are metrics good for, if you have no way to store, view, and process them? In case you don't have access to some already-deployed server supporting Buddy's metrics, you can easily deploy your own server.

##### What do you need?
- [Docker](https://www.docker.com)
- [Docker Compose](https://docs.docker.com/compose/)

##### Steps to start the services
1. Enter the `utils/metrics` directory
    ```bash
    cd utils/metrics
    ```
1. Download, build and start the docker containers
    ```bash
    docker-compose build
    docker-compose up -d
    ```
1. Done!

    > - You can view logs of all the running services using `docker-compose logs -f`
    > - To stop the services, run `docker-compose stop`

1. If you're on **WSL**, you might need to forward the UDP packets from your host machine to the WSL. For that purpose, we've written a simple UDP proxy application you can compile and run on your PC.

    1. Download [DMD](https://dlang.org/) compiler.
    1. Compile `metricsProxy.d` located in this directory (`doc`): `dmd metricsProxy.d`
    1. Find out your WSL IP address: `(wsl) ifconfig`
    1. Start the proxy: `metricsProxy.exe (your-wsl-address)`.


##### This will start the following services

1. An InfluxDB database instance storing all your metrics.
    - By default, the metrics are stored within a database named `buddy` (no authorization required)
1. A metric-handler service, listening on port 8500 for incoming metrics and storing them to the InfluxDB database.
    - You can setup your printer to send metrics to this handler using the gcodes below
        - `M334 <ip address of your computer> 8500`
1. A Grafana instance for viewing the metrics.
    - Accessible on port 3000 (http://localhost:3000)
    - Default login credentials are admin/admin
    - It is automatically preconfigured with the InfluxDB instance mentioned above as a datasource.
        However, it is up to you to create some dashboards viewing your metrics.
