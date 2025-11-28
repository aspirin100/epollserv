FROM alpine:3.22.2 AS build

RUN apk add --no-cache build-base

WORKDIR /app
COPY . .

RUN make

FROM alpine:3.22.2

RUN apk add --no-cache libstdc++ bind-tools

COPY --from=build /app/client /usr/local/bin/client

ENTRYPOINT client