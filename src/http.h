#ifndef SYSMON_HTTP_H
#define SYSMON_HTTP_H

/*
 * Minimal HTTP/1.0 listener, just enough for a Prometheus scraper:
 * one request per connection, no keep-alive, no TLS.
 * Bind it to localhost or put a reverse proxy in front.
 */
int http_open(const char *bind_addr, int port);
void http_handle(int listen_fd);

#endif
