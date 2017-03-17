---
title: "Project 2: Simple Webserver"
author: Vincent Au (2016840200)
date: |
    CSI4106-01 Computer Networks (Fall 2016)
header-includes:
    - \usepackage{fullpage}
---

# Introduction

This program serves a website with the root directory specified by the command line. You can compile it using the `make` command or alternatively:

```
gcc -o project_2 project_2.c -std=c99
```

To run the program, execute:

```
./project_2 8080 website
```

This will serve the files found in the `website/` directory (defaulting to `index.html`) to the address: [*127.0.0.1:8080*](http://127.0.0.1:8080).

# Implementation Scope
The program currently only has GET and POST methods implemented. There are 5 MIME types that are currently supported: `text/html`, `text/css`, `text/javascript`, `image/jpeg`, and `image/png`. If an unsupported file type is requested, it will use the MIME type `application/octet-stream` to specify arbitrary data.

The POST method is also implemented but only to detect whether or not `id=yonsei&pw=network` is sent in the entity body to the URL `/login`. If it is, a cookie containing `cookie=2016840200` is sent to the client and redirects to `/secret/index.html`. Only clients that have this cookie will be able to access the files in the `/secret/` folder. If a client attempts to access the contents in `/secret/` without this cookie, a `403 Forbidden` status is returned.

If the POST method is used for any URL other than `/login`, or if any HTTP method other than GET and POST is requested, a `405 Method Not Allowed` status is returned.

The program also supports redirection via the `302 Found` status. If the page `/go/facebook` is requested, it will redirect to [*www.facebook.com*](http://www.facebook.com). This can be generalised to `/go/%name%` redirects to *www.%name%.com* where `%name%` is the regular expression `[a-zA-Z]+`.

Different files are also served based on the client's User-Agent string. If the User-Agent is a mobile device, the webserver will serve not from the `website/` directory, but from the `website/mobile/` directory (see Figures 1 and 2).

# Code Commentary

The program first ensures that a sufficient number of arguments is passed in. If not, the program drops out printing the usage information. Otherwise, the program passes the port that was specified in the command line to `setup_server()`.

`setup_server()` first defines a set of hints which will call `getaddrinfo()` to return a linked list of address information which we store in `servinfo`. We loop through the list until we can successfully instantiate a socket. We call `setsockopt()` in order to enable the reuse of ports and once we have successfully bound the socket, we break the loop and free the memory of `servinfo` since it is no longer needed. We then begin to listen on the socket for incoming connections.

If a connection is detected the server will print out the IP address of the client and then fork a child process to handle the request. This allows the parent process of the server to continue listening for new requests while the child process handles the serving of the requests. The child process will close the connection to the listener (since it no longer cares about listening for new connections) and then read and pass the request to the `handle_request()` function.

`handle_request()` will then call `parse_request()` in order to extract the required information from the request header. `parse_request()` first uses `sscanf()` to extract the request method and URL from the header. It then sets the `is_mobile`, `has_cookie`, and `has_body` flags to zero. Using `strsep()` it reads through the request line by line by using `\r\n` as the delimiter. `strncmp()` is used to check the request lines for the User-Agent and the cookie. The pointer `string` is incremented at the end of each loop in order to skip past the `\n` character. All the information extracted from the header is stored in a pointer to a custom structure `struct request`.

Once `parse_request()` resolves, `handle_request()` will look at the HTTP method and serve any POST requests directed at `/login` by checking if `id=yonsei&pw=network` is found in the entity body. If it is, it will call `set_cookie_and_redirect()` which sets a cookie of `cookie=2016840200` and redirects to `/secret/index.html`. If `id=yonsei&pw=network` is not found in the entity body, a 403 Forbidden status code is returned (see Figure 3). Otherwise, if the POST request was sent to a URL that is not `/login`, a 405 Method Not Allowed response is returned.

Next, `handle_request()` will ensure that the HTTP method is GET (returning a 405 Method Not Allowed response, otherwise) before checking the requested URL to see if it is requesting the protected directory `/secret/`. If it is, and the custom cookie is not detected, a 403 Forbidden response is returned.

If `handle_request()` has not already returned by this stage, it will continue by determining the path of the file that is requested by taking into account the requested URL, whether or not the client is a mobile device, whether the requested URL is a directory or not, etc. Using the `stat()` function, it will determine if the file at the path is a regular file. If it is, it calls `write_file()` to write the contents of the file to the socket.

`write_file()` opens the file at `path` in read-only mode, and opens the socket as a file to write to. It passes the file path to `get_mime()` which uses the file extension to return the appropriate MIME type. `write_file()` uses this to write the correct HTTP header and `Content-Type` out to the socket. It then reads in `MAX_BUF` bytes at a time from the file to write back out to the socket until everything has been written out. It then closes all open files and returns back to `main()`.

If `handle_request()` detects that the file at the requested path is not a regular file but a directory, it calls `handle_redirect()` to redirect to the current URL appended with `/` character. Otherwise, if the requested path is neither a file nor a directory, it will determine if the requested path contains the string `/go/`. If it does and the website requested after `/go/` matches the regular expression `[a-zA-Z]+`, it will call `handle_redirect()` which returns a 302 Found response and redirects to the requested website. Otherwise, it returns a 404 Not Found response.

Finally, back in the `main()` function, any remaining connections are closed and as it is the child process, it terminates itself using `exit()`.


# How It Works
The code can be broken into 3 main parts - 1) network setup; 2) request parsing; and 3) request handling.

## Network Setup
The network setup component was based on [*Beej's Guide to Network Programming: Using Internet Sockets*](http://www.beej.us/guide/bgnet/output/html/singlepage/bgnet.html) by Brian "Beej Jorgensen" Hall. The socket is setup in a way to accept both IPv4 and IPv6. This is mainly done via the `get_in_addr()` function which can successfully translate the IP addresses into the correct form. Once the socket is setup and listening, if a connection is detected, the process forks a child process which then handles the parsing and handling of the request, while the parent process continues to listen for new connections.

## Request Parsing
The heavy lifting of understanding the request is done by the `parse_request()` function. It first uses `sscanf()` to get the request method and URL. It then loops through the request line by line in order to extract information such as whether or not the client is a mobile device, if the request contains a cookie and what it is if it does, as well as any request body if there are any. This is all stored in a custom `struct request`.

## Request Handling
The request handling is done by `handle_request()` and it first handles POST requests. Since we are only handling POST requests at the URL `/login`, we check the URL and handle that first. To deal with GET requests, we first ensure that we have a secret cookie if we happen to be dealing dealing with `/secret/` requests. Then we check the URL that is being requested and get the actual file path by joining the URL with the root directory. If the path turns out to be a file, we write it to the socket. If the path is a directory, we redirect to the same URL appended with `/` to indicate a directory.

If the requested file or folder cannot be found, we check the URL for `/go/` redirects. If the redirect website is valid (contains only alphabetical characters) we redirect to the requested website.

## Other Remarks
The cookie setting and error responses are written to the socket using the `write_response()` function. This opens the socket as a file, and writes a status number, status code, and any formatted string to the socket. This provides a lot more flexibility than the typical `send()` or `write()` functions used for socket communication which also require the number of bytes to be written.

# Screenshots

The following are some screenshots of the webserver in action.

![Webserver reads the default Desktop Chrome User-Agent string and serves the desktop files](screenshots/fig1.png)

![Webserver detects an Android user agent (notice the blue AND in the top right corner indicating a custom Android User-Agent string) and serves the mobile website](screenshots/fig2.png)

![403 Forbidden Error is returned when the incorrect credentials are sent to `/login`](screenshots/fig3.png)

![The contents in the `/secret/` folder are correctly displayed when the cookie is detected](screenshots/fig4.png)

# Things I Learnt
After discovering how tedious it was having to always specify the number of bytes that I am sending when writing to the socket using `write()` and `send()`, I endeavoured to find a way to simplify writing to the socket. This was when I learnt that it was actually possible to open up a socket like a file and read and write to it as you would any other file in C. As a result, I wrote the `write_response()` function that would allow me to pass in a formatted string (just like `printf()`) and it made writing to the socket much, much easier.

I also learnt that the Content-Length field in the HTTP header is actually quite particular and can lead to several errors. For example if the Content-Length field differs from the actual number of sent bytes by even one byte, errors can occur. Some browsers appear to be more forgiving than others but it still shows how important it is to not only correctly identify the size of the file, but also correctly send that number of bytes to the client.

When implementing the additional component involving cookies I discovered that cookies can exist at a certain scope of the website, determined by the URL of which the cookie was set. For example, when I was attempting to unset the cookie required by `/secret/`, no matter what I did I could still access the contents in `/secret/` which should have been protected by the cookie. It turns out that the cookie that I had set had the path unique to `/secret/` but when I was unsetting the cookie, the cookie I was unsetting was located at `/`. This meant that although the cookie at `/` was successfully being cleared, the cookie at `/secret/` was not. I managed to solve this by explicitly declaring the path (which I set to `/`) when both setting and unsetting the cookie.

# Further Research
In its current state, this webserver is hardly perfect and still has lots of missing features. For example, out of the 9 HTTP methods that exist in HTTP 1.1, only 2 methods have been implemented (GET and POST). Further more, although the POST method is implemented, the example website provided does not even include a form for the credentials. This means that testing of the cookie and `/secret/` directory needs to be done using a tool such as the [*Postman Chrome extension*](https://chrome.google.com/webstore/detail/postman/fhbjgbiflinjbdggehcddcbncdddomop).

In addition to all of this, the program currently has no caching mechanisms in place, no more than two headers (Content-Type and Content-Length) implemented, no persistent connections (a crucial feature that makes HTTP 1.1 significantly more efficient over HTTP 1.0), and barely any status codes implemented.

As a result, further research and implementation would be needed to make this webserver more usable in the future. However, in its current state it is sufficient to demonstrate socket programming in C as well as illustrate HTTP requests and responses.

