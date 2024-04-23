# The ESP firmware

Comes from <https://github.com/prusa3d/Prusa-ESP-Nic-FW/>. Current version is
built from `1e0740cf0fc831124614538ef689748a83141596`.

Flasher stub is built from commit 1deb1c65c140363bbfa16fbd889cff8e6d33b8c0
of the repository git@github.com:espressif/esptool.git
Well, almost exactly, just with compression support and some other commands
removed to reduce size. And also sending hexdigest instead of bytes.
