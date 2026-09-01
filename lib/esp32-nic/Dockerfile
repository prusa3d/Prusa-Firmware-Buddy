FROM ubuntu:22.04

RUN apt-get update \
    && apt-get -y install \
        bash \
        git \
        cmake \
        python3 \
        python3-pip \
        python3-venv \
        python3-cryptography \
        python3-future \
        python3-click \
        python3-serial \
        python3-wheel \
        python3-pyparsing \
        python3-pyelftools \
        gperf \
        flex \
        bison \
        libncurses-dev \
        libusb-1.0.0-dev \
    && rm -rf /var/lib/apt/lists/*

RUN git clone -b v5.0.3 --recursive https://github.com/espressif/esp-idf.git

# Install tools into a world-accessible path so the container can be run as
# an arbitrary non-root user (--user flag) without losing access to the tools.
# Pin setuptools<82: pkg_resources was removed in setuptools v82, ESP-IDF v5.0 still needs it.
# Set git safe.directory so export.sh works for non-root users (git >= 2.35.2).
ENV IDF_TOOLS_PATH=/opt/espressif
RUN cd /esp-idf && ./install.sh \
    && /opt/espressif/python_env/idf5.0_py3.10_env/bin/pip install "setuptools<82" \
    && chmod -R a+rX /opt/espressif \
    && git config --system --add safe.directory /esp-idf
