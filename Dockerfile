FROM python:3.8

WORKDIR /buddy

COPY ./utils/bootstrap.py /buddy/utils/bootstrap.py
RUN python /buddy/utils/bootstrap.py && (cd .dependencies && find . -type f -executable  | xargs echo chmod +x)
