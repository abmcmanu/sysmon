# syntax=docker/dockerfile:1
FROM alpine:3.20 AS build
RUN apk add --no-cache build-base
WORKDIR /src
COPY . .
RUN make WERROR=1 && make test

FROM alpine:3.20
COPY --from=build /src/build/sysmond /src/build/sysmonctl /usr/local/bin/
COPY --from=build /src/build/plugins/ /usr/local/lib/sysmon/plugins/
COPY contrib/docker/sysmond.conf /etc/sysmond.conf
EXPOSE 9273
ENTRYPOINT ["sysmond", "-f", "-c", "/etc/sysmond.conf"]
