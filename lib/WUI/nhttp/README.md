# The Next HTTPD

This is our own implementation of a http server.

## Motivation

We are in a somewhat unusual situation.

* We are an embedded device. Therefore, there's not much resources to work with
  and the ones we have need to be managed with care.
* We are not primarily a network device, therefore other things take
  precedence.
* We still want somewhat full-featured „web server“ ‒ handling POSTs,
  dynamically generated content, etc.
* Browsers ofter create stand-by connections. These are idle most of the time,
  but still need to be held.

There are two kinds of http server implementations out there:

* The „big“ ones. These tend to assume gigabytes of memory and are able to
  juggle thousands of concurrent connections and hundreds of thousands requests
  per second.
* The „embedded“ ones. These are usually:
  - Limited what they can do. Mostly just serving static files, maybe providing
    small get pages.
  - Often not correct ‒ not handling things allowed by the protocols and seen in
    the wild (eg. case-insensitive headers, headers split across packet
    boundaries...)

Neither cuts it for us.

## Design goals

* Parsing needs to happen in streaming fashion. Whenever arbitrarily partial
  data arrive, we consume it, not keep around. This is possible since all the
  used protocols are ancient and were designed with easy parsing & simplicity in
  mind. Mostly…
* Idle connections should not take any additional memory apart from what TCP
  already takes.
* Allow juggling of limited resources ‒ for example, send buffers are shared
  between the connections and if none are available to serve a request, the
  request will simply wait a while for one of the buffers to get freed.
* Mostly correct, as far as practicality goes. We may decide that a feature is
  out of scope for us even though it's considered „mandatory“ (eg. reading
  chunked encoding), but we should at least refuse it gracefully.
* Composable/pluggable/sane: Writing endpoints should not have much impact on
  the server itself, endpoint author should be able to use ready-made parts (eg.
  sending an error page).

## Code Layout

The code is split into several „parts“ or „areas“, each with its own
responsibilities.

### The Low Level Networking

The server uses the LwIP's altcp layer. There are several reasons for this:

* This is part of the LwIP's raw API, which is the cheapest API available and
  with the most control (eg. ability not to ACK some incoming data yet).
* The server is going to have multiple connections, so all the costs involved
  would be multiple times. We also need some kind of callback-based API (with
  poll-based one with some kind of sockets, we would have to implement callbacks
  on top of that ourselves to manage the connections).
* The complexity of the API is going to be hidden in the Server layer and actual
  request handling won't come into contact with it.
* By using the raw API here, we don't need an extra thread.
* Our usage of the raw API doesn't stop other parts of the firmware from using
  other abstractions (eg. BSD sockets).

The altcp wrapper around lw-tcp can be used to unit-test the server code, by
providing mocks. On the other hand, if we manage to unify the ESP into LwIP
directly, we'll be able to turn the altcp wrapper off for the real
implementation. That will call the lw-tcp layer directly (through preprocessor
macros), getting us to the cheapest possible solution.

### The Server

The Server class is responsible for scheduling actions, managing resources,
sending and receiving data, etc. It doesn't actually understand the HTTP
protocol, it only holds the connections for it and plugs them to the bits that
do understand it. The bits are called Handlers.

In fact, the Server class could be modified to support other protocols with just
switching the „initial“ handler (and doing few other small modifications).

Connections inside the server are either idle or active. Active are the ones
that had some communication and have a handler (with some state) bound to them.
Inactive have no state. Connection can change from inactive to active and back.
There's limited number of allowed active connections, inactive ones simply wait
their turn if some data arrive on them (the data is not consumed until there's a
free slot for it to become active).

Besides limiting the number of active connections and send buffers, the server
also maintains timeouts (different for active and inactive connections) and
priorities (LwIP is supposed to kill connections based on priorities when it's
short on resources). Active and inactive have different timeouts and priorities.

### Handlers

Handlers are the bits that provide some actual functionality. A connection or
even a request can go through multiple ones. Each handler can express its
intention to either read or write data (or both, though that's unusual for
HTTP).

The handler is called, possibly multiple times, with incoming data or with a
buffer for outgoing data (or possibly with both). When the call happens is up to
the Server to figure.

When called, the handler does a bit of work towards the means, signals how much
it read from the data (it doesn't have to be everything thrown at it), how much
it written and if it wants to be called again. If it doesn't want to be called
again, it returns another handler to take its place.

There's a special termination handler the server knows about ‒ when that one is
returned, the sending of data is finished and the connection is either closed
(as signalled in the termination handler), starts from the beginning (if there's
more incoming data to process) or becomes idle (if there's nothing to do).

*Note:* Currently, the valid handlers must be known to the server. That's
because the slot of active connection needs to be able to hold it inside a
variant. This is the one of the places where the server itself needs to be
modified according to the needs of the generated content/application. The other
is listing all the needed headers inside the parser automaton. Nevertheless, if
we preferred dynamic allocation of handlers, we could replace this with virtual
method calls and make the server agnostic of the code.

*Note:* Currently, several handlers expect the output buffer to be large enough
to fit their output. This is guaranteed by a constant in the server defining the
size of the buffer; the server always guarantees to pass the whole buffer, not a
smaller one.

### The request parser

The request parser is one of the handlers (the initial one for an active
connection).

Internally, it uses a search automaton to be able to eat the data on
per-character basis. The automaton in generated with a help of python code ‒ see
`utils/gen-automata` and the `lib/WUI/automata` for details on these.

The usage of such automaton allows us to parse in streaming fashion (not worry
about how the data is split into parts and eat everything right away without
buffering). We can also extract just the parts that we care about and not
remember stringy versions of relevant headers (unless we really need them as
strings). This potentially saves memory that needs to be kept between
invocations (but see below).

Nevertheless, we still need to extract some non-trivial amount of data. For now,
we extract the URL and the multipart boundary. Therefore, the parser is somewhat
biggish structure and we may want to figure some way not to have space for one
in each slot.

Once the request is parsed (and any possible body or next request is left in the
input), it chooses what to do next by passing it to the selectors.

### The selectors

The selectors are the way to provide "content" for the server.

Each selector is given access to the parsed request and decides if it wants to
take care of the request. If so, it returns a handler that does the actual
handling of the request. This can be something trivial (a status page, a static
page from a file, ...) or something more complex (generating the message by a
code).

The selectors are chained ‒ first one to accept the request provides the handler
and others are not consulted. The last selector is a 404-catch-all (therefore
the server always gets _some_ handler to do something with the request).

### Generic and less generic handlers

There are handlers for the actual content. Some of them are generic („Return a
status page“ or „Return some static memory“), others are tailored to provide
specific functionality.

It is possible and expected for one request to pass through multiple handlers.
For example, uploading a gcode can be split into parsing the request, processing
the upload and returning the response (or returning an error in a status page).

## Current problems

As it is, there are always some issues. Finding solutions to them would be nice:

* There's always a bit more of the HTTP-related protocols to handle. For
  example, the client is in full right to send us the body as chunked encoding,
  have the headers quoted with encoding, there are rules for URL
  equivalence/parsing, etc. It's unclear how far it is worth going in supporting
  them, both from the effort perspective and from the code size/complexity PoV.
* ESP currently doesn't play that well with it (it seems to be killing
  connections in the middle for unknown reasons). This is likely because the
  LwESP‒altcp layer is not complete and we do use the full scale of what LwIP
  supports. We hope we'll get rid of LwESP and integrate ESP directly as a
  network card into LwIP, which'll solve this problem.
* The request parser is quite large. This inflates the size of each active slot
  even though the parser is not alive most of the time. Either dynamic
  allocation or some way to „parasite“ on the send buffers could be a solution.
* Even though the server needs no resources for an inactive connection, LwIP
  does (~200 bytes). There's currently a limit of 6 TCP connections and it is
  possible to exhaust these (and it happens in practice; thought it often is not
  visible in a browser/client). Increasing this limit would be nice if we could
  afford it.
* Writing the automata is a bit hard; spending a bit more time on utilities to
  do so and helpers (eg. to output them as some graphical representation) might
  help somewhat.
* Current implementation misses several places of optimizing size (the automata,
  memory layouts, ...).
* Aside from this, the code size seems to have grown a bit; we do support more
  flexibility, but knowing what took the size would be nice as well, so we can
  come up with somewhat smaller implementation.
