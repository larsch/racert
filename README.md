racert
======

Very fast traceroute tool for Windows. Runs a traceroute to remote
hosts with many threads, performing hop-limited ping and nameserver
lookup in parallel.

From my location:

tracert google.com: 65.74 seconds

racert google.com: 1 second for full route (5 seconds before all
timeouts)
