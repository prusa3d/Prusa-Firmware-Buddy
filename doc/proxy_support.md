# Proxy support for Connect

Currently, the printer doesn't have an explicit support for accessing Connect
through a proxy. However, there are some ways to work around that limitation.

## Using a separate network without proxy

Usually, the reason why there's a proxy is some form of network security
control. In case the printer is used through Connect (and not through Link),
there's no need for it to be in the same network as the other computers.

Therefore, it is possible to isolate the printer into a completely separate
(physically or by a VLAN) network that has access only to the outside Internet
and not at all to the corporate network and leave it without a proxy.

## Doing a full MITM / redirection proxy

It is possible to configure the hostname of Connect, through an ini file (see
below how). That way it is possible to set up a service that'll forward all the
requests to the real Connect.

This way, the inspection can be done either in this service (likely, most proxy
software can be configured to act as this kind of service directly) or configure
this service to go through the proxy.

### Steps needed

* Choose a hostname (or IP) and port of this service, for example
  my-connect.example.com and port 8080.
* Run a service (the proxy) on my-connect.example.com on plain HTTP (the printer
  currently doesn't allow custom certificates for HTTPS, but can be configured
  to run unencrypted). Forward each request to `https://connect.prusa3d.com/` ‒
  that is, `http://my-connect.example.com:8080/p/telemetry` is forwarded to
  `https://connect.prusa3d.com/p/telemetry`.
* Create a `prusa_printer_settings.ini` file with the content below.
* Place it onto an USB flash drive, place it into the printer and load it
  through `Settings -> Network -> Prusa Connect -> Load from file`.
* Proceed to usual printer registration (`Settings -> Network -> Prusa Connect
  -> Register`)

```ini
[service::connect]
hostname = my-connect.example.com
port = 8080
tls = false
```
