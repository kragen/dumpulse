FROM python:3.6

RUN pip install pytest hypothesis

ADD . /dumpulse
WORKDIR /dumpulse