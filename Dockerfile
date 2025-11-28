FROM alpine:3.22.2 AS build

RUN apk add --no-cache make build-base

WORKDIR /app
COPY . .

RUN make

FROM alpine:3.22.2

RUN apk add --no-cache libstdc++ gcc

COPY --from=build /app/epollserv /usr/local/bin/epollserv

EXPOSE 8888

ENTRYPOINT epollserv