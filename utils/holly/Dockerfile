FROM python:3.7.6-stretch
RUN pip install pre-commit ecdsa
ADD utils/bootstrap.py bootstrap.py
RUN python bootstrap.py
