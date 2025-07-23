# Simple HTTP Web Server

## Important Notice
The project is under active development and cannot be used yet. 
As soon as the project is finished, the note will be removed.
**However**, files in **_headers_** and **_utils_** directories can already be used in your own applications.

## About
Simple HTTP 1.1 web server. Responses to **GET** requests and serves a resource from **_public_** directory

## For Developers
It is suggested that you take needed pieces of code from files in **_headers_** and **_utils_** directories for your own network projects

## How to Use

### Windows
#### Using GCC
```
cd http-server
```

```
gcc -Wall -Wextra -O2 \
    src/main.c \
    src/utils/sockshelp.c \
    src/utils/tcp_socks.c \
    src/utils/http_parsers.c \
    -lws2_32 \
    -Isrc/headers \
    -o http_server.exe
```

```
http_server.exe
```

#### Using MSVC
Run the program from **Visual Studio**

### Unix
```
cd http-server
```

```
gcc -Wall -Wextra -O2 \
    src/main.c \
    src/utils/sockshelp.c \
    src/utils/tcp_socks.c \
    src/utils/http_parsers.c \
    -Isrc/headers \
    -o http_server
```

```
./http_server
```

## Licence
The project is protected under **MIT licence**
