FROM ubuntu:latest

RUN apt-get update && apt-get install -y \
    build-essential \
    python3 \
    net-tools \
    tshark \
    vim \
    git

WORKDIR /root

RUN git clone https://github.com/mz-automation/libiec61850.git

WORKDIR /root/libiec61850/examples/

RUN git clone https://github.com/allefenes/iec61850Example.git

RUN mv iec61850Example/* .

RUN make 

CMD ["bash"]
