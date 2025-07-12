FROM ubuntu:24.04

ENV TZ=Europe/Moscow

RUN apt-get -y update

RUN : \
    echo $TZ > /etc/localtime && \
    apt-get install tzdata && \
    ln --symbolic --no-dereference -f /usr/share/zoneinfo/$TZ /etc/localtime

RUN apt-get install -y \
    gcc \
    g++ \
    clang \
    make \
    cmake \
    git \
    nix

RUN apt-get install -y \
    python3 \
    python3-venv

RUN : \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*
