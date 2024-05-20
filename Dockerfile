# Base image olarak güncel Ubuntu
FROM ubuntu:latest

# Gerekli paketleri yükle
RUN apt-get update && apt-get install -y \
    build-essential \
    python3 \
    net-tools \
    tshark \
    vim \
    git

# ~/ dizinine geç
WORKDIR /root

# Git repositorysini klonla
RUN git clone https://github.com/mz-automation/libiec61850.git

# Klonlanan dizinin içine gir
WORKDIR /root/libiec61850

# Örnekleri derle
RUN make examples

# examples dizinine geç
WORKDIR /root/libiec61850/examples

# Container çalıştırıldığında terminali başlat
CMD ["bash"]
