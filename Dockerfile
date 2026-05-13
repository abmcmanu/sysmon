# syntax=docker/dockerfile:1
FROM alpine:3.20 AS build
RUN apk add --no-cache build-base
WORKDIR /src
COPY . .
RUN make WERROR=1 && make test

FROM alpine:3.20
RUN adduser -D -H -s /sbin/nologin sysmon
COPY --from=build /src/build/sysmond /src/build/sysmonctl /usr/local/bin/
COPY --from=build /src/build/plugins/ /usr/local/lib/sysmon/plugins/
COPY contrib/docker/sysmond.conf /etc/sysmond.conf
USER sysmon
EXPOSE 9273
HEALTHCHECK --interval=30s --timeout=3s CMD sysmonctl -s /tmp/sysmond.sock ping || exit 1
ENTRYPOINT ["sysmond", "-f", "-c", "/etc/sysmond.conf"]
