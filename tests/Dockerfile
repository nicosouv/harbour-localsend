# The CI Qt lane's packages, so a run here fails exactly where CI would.
# Ubuntu ships Qt 5.15 while the device runs 5.6; scripts/check_qt56.py covers
# the gap that a newer compiler would happily let through.
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      qtbase5-dev \
      qtdeclarative5-dev \
      qtdeclarative5-dev-tools \
      qttools5-dev-tools \
      libssl-dev \
      cmake \
      ninja-build \
      g++ \
      pkg-config \
      python3 \
      clang \
      libclang-rt-18-dev \
 && rm -rf /var/lib/apt/lists/*
