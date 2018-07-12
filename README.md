# HTTP-Web-Proxy-and-Cache
As part of the Stanford CS110, I was asked to implement a multithreaded HTTP proxy and cache. HTTP proxy intercepts each 
and every HTTP request and (generally) forwards it on the intended recipient. The servers direct their HTTP responses back to 
the proxy, which in turn passes them on to the client. This implementation doesn't account for HTTPS only HTTP.