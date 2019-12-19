FROM python:3.8

WORKDIR /a3ides

COPY ./utils/bootstrap.py /a3ides/utils/bootstrap.py
RUN python /a3ides/utils/bootstrap.py
