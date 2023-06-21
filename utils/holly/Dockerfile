FROM prusa3d/gcc-multilib:10
RUN apt-get clean && \
    apt-get update -qq -y && \
    apt-get install curl python3 python3-pip libncurses5 g++-multilib -y
RUN pip3 install pip==22
RUN pip3 install Cython
RUN pip3 install pre-commit ecdsa littlefs-python==0.4.0
WORKDIR /work
ADD utils/bootstrap.py utils/bootstrap.py
ADD utils/debug utils/debug
RUN gcc --version
RUN python3 utils/bootstrap.py && rm -rf utils
