FROM ubuntu:22.04

ENV TZ=Etc/UTC
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone \
    && apt-get update \
    && apt-get -y install \
        bash \
        git \
        cmake \
        python3 \
        python3-pip \
        python3-virtualenv \
        python3-cryptography \
        python3-future \
        python3-click \
        python3-serial \
        python3-wheel \
        python3-pyparsing \
        python3-pyelftools \
    && rm -rf /var/lib/apt/lists/* \
    && ln -s /usr/bin/python3 /usr/bin/python

# Workaround. The repo 2 levels deep got moved, but the redirect is broken and
# wrong. Needs a bit of manual intervention to fetch from the right place.
RUN git clone https://github.com/espressif/ESP8266_RTOS_SDK.git \
    && cd /ESP8266_RTOS_SDK \
    && git checkout a192988b7906680440213eefc730d99dec0ce237 \
    && git submodule update --init components/coap/libcoap \
    && cd components/coap/libcoap/ \
    && git submodule set-url ext/tinydtls https://github.com/eclipse-tinydtls/tinydtls \
    && git submodule update --init --recursive \
    && cd ../../.. \
    && git submodule update --init --recursive

# Install tools into a world-accessible path so the container can be run as
# an arbitrary non-root user (--user flag) without losing access to the tools.
# Pin setuptools<82: pkg_resources was removed in setuptools v82, ESP8266_RTOS_SDK still needs it.
# Set git safe.directory so export.sh works for non-root users (git >= 2.35.2).
ENV IDF_TOOLS_PATH=/opt/espressif
RUN cd /ESP8266_RTOS_SDK && PYTHONPATH=/usr/lib/python3.9/site-packages ./install.sh \
    && find /opt/espressif/python_env -name python3 -type f -exec {} -m pip install "setuptools<82" \; \
    && chmod -R a+rX /opt/espressif \
    && git config --system --add safe.directory /ESP8266_RTOS_SDK
