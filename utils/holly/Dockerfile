FROM ubuntu:22.04
RUN apt-get clean && \
    apt-get update -qq -y && \
    apt-get install git curl python3 python3-pip python3-venv libncurses5 g++-multilib -y
WORKDIR /work
ADD utils/bootstrap.py utils/bootstrap.py
ADD utils/debug/10_custom_config_template.cfg utils/debug/10_custom_config_template.cfg
ADD requirements.txt requirements.txt
ADD utils/debug utils/debug
RUN python3 utils/bootstrap.py && (cd .dependencies && find . -type f -executable  | xargs chmod +x) && rm -rf utils
