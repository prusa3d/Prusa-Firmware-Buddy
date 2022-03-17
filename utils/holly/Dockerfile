FROM prusa3d/gcc-multilib:10
RUN apt-get clean && \
    apt-get update -qq -y && \
    apt-get install curl python3 python3-pip libncurses5 g++-multilib -y
RUN pip3 install Cython
RUN pip3 install pre-commit ecdsa littlefs-python
WORKDIR /work
ADD utils/bootstrap.py bootstrap.py
RUN gcc --version
RUN python3 bootstrap.py
