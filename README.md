racert
======

Very fast traceroute tool for Windows. Runs a traceroute to remote
hosts with many threads, performing hop-limited ping and nameserver
lookup in parallel.

[Download from the release page](https://github.com/larsch/racert/releases)

From my location:

tracert google.com: 65.74 seconds

racert google.com: 1 second for full route (5 seconds before all
timeouts)
