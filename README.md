# Description

Simple TCP/UDP epoll-based echo-server

## Supported commands

1. /time - returns current time in format "2025-11-10 17:28:45"
2. /stats - returns active clients and total all-time connections count
3. /shutdown - shutdowns the server

Any message that doesn't start with a '/' will be echoed back.


## Testing

### [client for test](./test/) 
Instructions provided in the directory above