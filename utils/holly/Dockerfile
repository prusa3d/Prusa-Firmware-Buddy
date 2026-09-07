FROM ubuntu:24.04
RUN apt-get clean && \
    apt-get update -qq -y && \
    apt-get install libjpeg-dev libusb-1.0-0 gettext git python3 python3-pip python3-venv python3-requests -y
WORKDIR /work
ADD utils/bootstrap.py utils/bootstrap.py
ADD requirements.txt requirements.txt
RUN python3 utils/bootstrap.py && (cd .dependencies && find . -type f -executable  | xargs chmod +x) && rm -rf utils
