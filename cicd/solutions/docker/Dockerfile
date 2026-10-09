# builder
FROM ubuntu:22.04 AS builder
RUN apt-get update && apt-get install -y cmake g++ make && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY workshop/ workshop/
RUN cmake -S workshop -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build

# runtime
FROM debian:bookworm-slim
COPY --from=builder /src/build/forge_cli /usr/local/bin/forge-cli
ENTRYPOINT ["/usr/local/bin/forge-cli"]
CMD ["ping"]
