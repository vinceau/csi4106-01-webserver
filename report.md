---
title: "Project 2: Simple Webserver"
author: Vincent Au (2016840200)
date: |
    CSI4106-01 Computer Networks (Fall 2016)
header-includes:
    - \usepackage{fullpage}
---

#Introduction

This program serves a website with the root directory specified by the command line. You can compile it using the `make` command or alternatively:

```
gcc -o project_2 project_2.c -std=c99
```

To run the program, execute:

```
./project_2 8080 website
```

This will serve the files found in the `website/` directory (defaulting to `index.html`) to the address: [*127.0.0.1:8080*](http://127.0.0.1:8080).

#Implementation Scope
The program currently only has GET and POST methods implemented. There are 5 MIME types that are currently supported: `text/html`, `text/css`, `text/javascript`, `image/jpeg`, and `image/png`. If an unsupported file type is requested, it will use the MIME type `application/octet-stream` to specify arbitrary data.

The POST method is also implemented but only to detect whether or not `id=yonsei&pw=network` is sent in the entity body to the URL `/secret/`. If it is, a cookie containing `id=2016840200` is sent to the client. Only clients that have this cookie will be able to access the files in the `/secret/` folder. If a client attempts to access the contents in `/secret/` without this cookie, a `HTTP/1.1 403 Forbidden` status is returned.

If any other HTTP method is requested, a `HTTP/1.1 405 Method Not Allowed` status is returned.

The program also supports redirection via the `HTTP/1.1 302 Found` status. If the page `/go/facebook` is requested, it will redirect to [*www.facebook.com*](http://www.facebook.com). This can be generalised to `/go/%name%` redirects to *www.%name%.com* where `%name%` is the regular expression `[a-zA-Z]+`.

Different files are also served based on the client's User-Agent string. If the User-Agent is a mobile device, the webserver will serve not from the `website/` directory, but from the `website/mobile/` directory.

#Code Commentary

The program first ensures that a sufficient number of arguments is passed in. If not, the program drops out printing the usage information. Otherwise, the program passes the port that was specified in the command line to `setup_server()`.

`setup_server()` will open up a socket and listen on the specified port. It will do this in a protocol agnostic way so that it can accept both IPv4 and IPv6. If everything goes smoothly it will start to listen for connections.

If a connection is detected the server will print out the IP address of the client and then fork a child process to handle the request. This allows the parent process of the server to continue listening for new requests while the child process handles the serving of the requests. The child process will close the connection to the listener since it no longer cares about listening for new connections before reading the request from the socket and passing it to the `handle_request()` function.

`handle_request()` will then call `parse_request()` in order to extract the required information from the request header. `parse_request()` reads through the request line by line and extracts information such as the HTTP method, the requested URL, whether or not the User-Agent is a mobile device, any cookie information, and any entity body information. This information is stored in a pointer to a custom structure `struct request`.

Once `parse_request()` resolves, `handle_request()` will first determine whether the HTTP method is supported and if it isn't, it will respond with status code 405 Method Not Allowed. Otherwise, `handle_request()` will check the requested URL to see if it is requesting the protected directory `/secret/`. If it is and the HTTP method is POST with `id=yonsei&pw=network` in the entity body, it will call `set_cookie()`. This function returns to the client a response with status code 200 OK with `id=2016840200` as the cookie. If `id=yonsei&pw=network` isn't found in the entity body, or if the request was a GET request without a cookie, or the cookie did not have `id=2016840200` in it, response code 403 Forbidden is returned.

If `handle_request()` has not already returned, it will continue by determining the path of the file that is requested by taking into account the requested URL, whether or not the client is a mobile device, whether the requested URL is a directory or not, etc. Using the `stat()` function, it will determine if the file at the path is a regular file. If it is, it calls `write_file()` to write the contents of the file to the socket.

`write_file()` opens the file at `path` in read-only mode, and opens the socket as a file to write to. It passes the file path to `get_mime()` which uses the file extension to return the appropriate MIME type. `write_file()` uses this to write the correct HTTP header and `Content-Type` out to the socket. It then reads in `MAX_BUF` bytes at a time from the file to write back out to the socket until everything has been written out. It then closes all open files and returns back to `main()`.

If `handle_request()` detects that the file at the requested path is not a regular file, i.e. the path is either a directory or the file is missing, it will determine if the requested path contains the string `/go/`. If it does and the website requested after `/go/` matches the regular expression `[a-zA-Z]+`, it will call `handle_redirect()` which will return a 302 Found response which redirects to the requested website. Otherwise, it will return a 404 Not Found response.

Finally, the `main()` function then closes any remaining connections and since it's the child process, terminates itself using `exit()`.

<!--
#Disclaimer

The ethernet structures and declarations used in this program were inspired and adapted from the [*Programming with pcap*](http://www.tcpdump.org/pcap.html)[^1] tutorial by Tim Carstens of the Tcpdump Group.



[^1]: <http://www.tcpdump.org/pcap.html>


Used websites:
get last occurence of character
http://www.ibm.com/support/knowledgecenter/ssw_ibm_i_72/rtref/strrchr.htm

printf function guide
http://www.ozzu.com/cpp-tutorials/tutorial-writing-custom-printf-wrapper-function-t89166.html

-->
