FROM gcc:11.1
RUN apt-get clean && \
    apt-get update -qq -y && \
    apt-get install curl python3 python3-pip libncurses5 -y
RUN pip3 install pre-commit ecdsa pandas
WORKDIR /work
ADD utils/bootstrap.py bootstrap.py
RUN gcc --version
RUN python3 bootstrap.py
