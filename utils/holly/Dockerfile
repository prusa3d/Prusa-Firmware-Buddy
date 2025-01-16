FROM ubuntu:24.04
RUN apt-get clean && \
    apt-get update -qq -y && \
    apt-get install libjpeg-dev git python3 python3-pip python3-venv python3-requests -y
WORKDIR /work
ADD utils/bootstrap.py utils/bootstrap.py
ADD utils/debug/10_custom_config_template.cfg utils/debug/10_custom_config_template.cfg
ADD utils/holly/build-requirements.txt requirements.txt
RUN python3 utils/bootstrap.py && (cd .dependencies && find . -type f -executable  | xargs chmod +x) && rm -rf utils
