FROM --platform=linux/386 debian:bookworm-slim
RUN apt-get update \
 && apt-get install -y --no-install-recommends gcc libc6-dev make \
 && rm -rf /var/lib/apt/lists/*
WORKDIR /srv
COPY Makefile server.c ./
RUN make
EXPOSE 8080 8081
CMD ["./chainboard"]
