FROM ubuntu:22.04

ENV TZ=Etc/UTC
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt-get update
RUN apt-get -y install bash git cmake python3 python3-pip python3-virtualenv python3-cryptography python3-future python3-click python3-serial python3-wheel python3-pyparsing python3-pyelftools
# idf.py menuconfig dependencies
#RUN apt-get -y install gperf flex bison libncurses-dev
RUN ln -s /usr/bin/python3 /usr/bin/python

RUN git clone --recursive https://github.com/espressif/ESP8266_RTOS_SDK.git
RUN cd /ESP8266_RTOS_SDK && git checkout a192988b7906680440213eefc730d99dec0ce237
RUN cd /ESP8266_RTOS_SDK && PYTHONPATH=/usr/lib/python3.9/site-packages ./install.sh
RUN echo "source /ESP8266_RTOS_SDK/export.sh" >> ~/.bashrc

