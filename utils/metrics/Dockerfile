FROM python:3.8
COPY requirements.txt .
RUN pip install -r requirements.txt
COPY collect.py .
CMD sleep 5 && python collect.py --database-host influxdb --syslog-port 8500
