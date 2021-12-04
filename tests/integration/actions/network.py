from simulator import Printer, NetworkInterface


def proxy_http_port_get(printer: Printer) -> int:
    return printer.network_proxy_http_port_get()
