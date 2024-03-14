# Logging

Logging is a way to track *events* that have happened at the application's runtime.
Those *events* are text-based and have associated metadata, such as the time at which the event happened, the importance of the event (called *severity*), the *component* of the software from which the log comes from, and few others.

When recorded, those events can be presented to the user in several ways. They can appear in the terminal of the debugger, be sent over the network for later inspection, or be - for example - logged to a file. Those ways of presentation are in our implementation called *destinations*. The user can register multiple destinations to have the logs delivered to.

## Quick Start

### Recording a custom event

    ```Swift
    log_info(MyComponent, "Current value is %u", current_value);
    ```

- Example output at some destination
    ```
    5.064s [INFO  - MyComponent:2] Current value is 5
    ```
- Use the other `log_*` functions to log an event of another severity (e.g. `log_debug`, `log_warning`, `log_error` or `log_critical`).
- There is no need to "include" the declaration of the component *MyComponent* from some header file. The only requirement is that the component has to be defined in some `.c`/`.cpp` file. See below.


### Definition of a new component
    ```Swift
    LOG_COMPONENT_DEF(MyComponent, LOG_SEVERITY_INFO);
    ```
- Defines a log component with the name `MyComponent`, which, by default, ignores events of lower severity than *info*.
- Such a definition must be in a `.c` or `.cpp` file (not in a header) and appear only once in the project.


### Referencing an existing component
    ```Swift
    LOG_COMPONENT_REF(MyComponent);
    ```
- References an existing component with the name `MyComponent`. This component must have been previously defined in another file with `LOG_COMPONENT_DEF(MyComponent, LOG_SEVERITY);`
- This is required for logging in `.cpp` files because of a difference in logging in `c` and `cpp` files.


### Registration of a custom log destination
    ```C
    static void log_event_custom(log_destination_t *destination, log_event_t *event) {
        // ... log the event to, for example, a file
    }

    static log_destination_t custom_log_destination = {
        .name = "CUSTOM",
        .lowest_severity = LOG_SEVERITY_DEBUG,
        .log_event_fn = swo_log_event,
        .log_format_fn = log_format_simple,
        .next = NULL,
    };
    log_destination_register(&custom_log_destination);
    ```

- This registers the function `log_event_custom` to be called with every event recorded by the application.
- See the `log_dest_*` files for already implemented destinations. The file `log_dest_shared.h/c` implements shared functions among the log destinations.


### Event filtering
Tho whole system might generate an enormous number of events. To make a presentation of events for the user informative, some kind of filtering must be put in place so the user can focus only on the events it cares about at the moment.

- **Global Severity Filter** (compile-time)
    - At compile time, the developer can define the `LOG_LOWEST_SEVERITY` macro (see `log.h`). This macro defines the lowest severity to be recorded by the system. All `log_<severity>` calls with severity lower than `LOG_LOWEST_SEVERITY`) will be removed at compile time.
- **Component's Severity Filter** (runtime)
    - Every component has its severity filter. This filter is not enforced at compile-time but runtime. The initial value of the filter is part of the definition of the component. For example,
        `LOG_COMPONENT_DEF(MyComponent, LOG_SEVERITY_INFO);`
        will ignore (at runtime) all events coming from the component with severity lower than `info`. This can be changed at runtime (e.g. `log_component_find("MyComponent")->lowest_severity = LOG_SEVERITY_DEBUG;`).
- **Destination's Severity Filter** (runtime)
    - Similarly to components, destinations have `lowest_severity`. Events with severity lower than `lowest_severity` won't be delivered to the destination.
