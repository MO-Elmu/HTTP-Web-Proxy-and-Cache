/**
 * File: request-handler.h
 * -----------------------
 * Defines the HTTPRequestHandler class, which fully proxies and
 * services a single client request.  
 */

#ifndef _request_handler_
#define _request_handler_

#include <utility>
#include <string>
#include <socket++/sockstream.h> // for sockbuf, iosockstream
#include "blacklist.h"
#include "cache.h"

class HTTPRequestHandler {
 public:
  void serviceRequest(const std::pair<int, std::string>& connection) throw();
  void serviceRequest(const std::pair<int, std::string>& connection, std::string proxyServer, unsigned short proxyPortNumber) throw();
  void clearCache();
  void setCacheMaxAge(long maxAge);
  HTTPRequestHandler(); //Constructor
  
 private:
  void issueRequest(iosockstream&, const HTTPRequest&);
  bool checkCache(const HTTPRequest&, HTTPResponse&);
  void cacheResponse(const HTTPRequest&, HTTPResponse&);
  int  formRequest(iosockstream&, HTTPRequest&, HTTPResponse&, const std::string&);
  void formResponse(iosockstream&, HTTPResponse&, HTTPRequest&);
  void sendRequest(iosockstream&, HTTPRequest&);
  void publishResponse(iosockstream&, HTTPResponse&);
  int  clientSocket(const std::string& , unsigned short);
  int  blacklistCheck(iosockstream&, std::string, HTTPResponse&);
  bool cycle(std::string);
  HTTPBlacklist bList;
  HTTPCache cach;
};

#endif

