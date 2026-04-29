FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y     gcc-arm-none-eabi     libnewlib-arm-none-eabi     make     openocd     gdb-multiarch     git     && rm -rf /var/lib/apt/lists/*
