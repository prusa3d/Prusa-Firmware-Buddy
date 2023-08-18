FROM ubuntu:22.04

RUN apt-get update && apt-get -y install \
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
RUN cd /esp-idf && ./install.sh
RUN echo "source /esp-idf/export.sh" >> ~/.bashrc
