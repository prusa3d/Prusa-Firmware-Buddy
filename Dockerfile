FROM ubuntu:22.04

ENV TZ=Etc/UTC
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && \
    echo $TZ > /etc/timezone

RUN apt-get update && apt-get -y install \
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
        python-is-python3 \
        libncurses-dev \
        flex \
        bison \
        gperf \
    && rm -rf /var/lib/apt/lists/*

RUN git clone --recursive -b v3.4 --recurse-submodules https://github.com/espressif/ESP8266_RTOS_SDK.git && \
    cd /ESP8266_RTOS_SDK && PYTHONPATH=/usr/lib/python3.9/site-packages ./install.sh && \
    echo "source /ESP8266_RTOS_SDK/export.sh" >> ~/.bashrc

