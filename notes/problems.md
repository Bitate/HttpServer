## When is the EPOLLHUP triggered?

Use **tcpdump** to trace down into TCP transactions.


## Why there are so many CLOSE_WAIT tcp sockets in server side and so many FIN_WAIT_2 tcp sockets in client side?

To cut a long story short, those who in CLOSE_WAIT did not properly close the TCP connection. In my case, the server failed to close the connection after no more data to read/send.


If client call shutdown(SHUT_WR), the server will receive an EOF from read() system call. In TCP level, the server will receive a FIN from client. After the server's tcp send back an ACK. The server is in CLOSE_WAIT and client is in FIN_WAIT_2 and the TCP is in half-close.

### Analysis  
1. Use tcpdump to captuer all tcp packets on 127.0.0.1/localhost.

2. Use *ss* command to filter out all tcp connection that is in CLOSE_WAIT state. In this case, I choose the port*45080* which is used by the client socket.

![ss command](../notes/screenshots/20210124114332.png)

3. Open capture file in wireshark and filter out the tcp connection with *127.0.0.1:45080*. Here we can clearly find out that the server(127.0.0.1:18135) do not close the connection! 

![packet info](../notes/screenshots/20210124114902.png)

The server do send back an ACK to the FIN sent by client. At this time, server is in CLOSE_WAIT while client is in FIN_WAIT_2. The problem is that the server do not send a FIN after reading/writing. 

![packet info](../notes/screenshots/20210124115011.png)

Conclusion: The server needs to close the connection properly. 

## IS EPOLLRDDUP equal to read() return 0, which means I do not need to handle the return value in read() call?
If EPOLLRDHUP got triggered, do not read the socket but close it immediately. However, it's possible that, before you call read(), a FIN arrives. So to be safer, close the socket with the return value of 0 of read().

## POST vs PUT ?
POST request uri points to the resource's manager/parent.
PUT request uri is exactly the resource uri.

```
// post a new user to the users collection.
POST /resources HTTP/1.1  

// create/update the john resource under the users collection.
PUT /resources/<newResourceId> HTTP/1.1 
```

RFC:
Let me highlight some important parts of the RFC:

POST
The POST method is used to request that the origin server accept the entity enclosed in the request as a **new subordinate** of the resource identified by the Request-URI in the Request-Line

Hence, creates a new **resource** on a **collection**.

PUT
The PUT method requests that the enclosed entity be stored under the supplied Request-URI. If the Request-URI refers to an already existing resource, the enclosed entity SHOULD be considered as a modified version of the one residing on the origin server. If the Request-URI does not point to an existing resource, and that URI is capable of being defined as a **new resource** by the requesting user agent, the origin server can create the resource with that URI."

Hence, create or update based on existence of the resource.

Reference:
* https://stackoverflow.com/a/18243587/11850070