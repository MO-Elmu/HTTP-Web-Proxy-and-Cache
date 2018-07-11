/**
 * File: request-handler.cc
 * ------------------------
 * Provides the implementation for the HTTPRequestHandler class.
 */

#include "request-handler.h"
#include "response.h"
#include "client-socket.h"
#include "request.h"
#include "ostreamlock.h"
#include <socket++/sockstream.h> // for sockbuf, iosockstream
#include <set>
#include <string>
using namespace std;

/* I deliberately chose to design chaining functionality in a seperate method and code path
 * for future expansion purposes. I believe making different code path for chaining makes 
 * the code cleaner and more manageable and overloading serviceRequest method made this 
 * isolation possible. You see a lot of code repetition in the 2 methods but that is ok 
 * to achieve this isolation and I think the pros outway the cons in this case.
 */ 

//Constructor definition and intialization

HTTPRequestHandler::HTTPRequestHandler(void){
  try{bList.addToBlacklist("blocked-domains.txt");}
  catch(const HTTPProxyException& e){
       cerr << oslock << "Warning: " << e.what() << endl << "Continuing without a Blacklist ......" << endl << osunlock;
  }
}

void HTTPRequestHandler::serviceRequest(const pair<int, string>& connection, string proxyServer, unsigned short proxyPortNumber) throw() {
  HTTPRequest request;
  HTTPResponse response;
  int proxy_clientfd;
  sockbuf sb2(connection.first);
  iosockstream ss2(&sb2);
  if(formRequest(ss2, request, response, connection.second) < 0) return; 
  if((proxy_clientfd = clientSocket(proxyServer, proxyPortNumber)) < 0) return;
  sockbuf sb(proxy_clientfd);
  iosockstream ss(&sb);
  sendRequest(ss, request);
  formResponse(ss, response, request);
  publishResponse(ss2, response);
}

void HTTPRequestHandler::serviceRequest(const pair<int, string>& connection) throw() {

  HTTPRequest request;
  HTTPResponse response;
  int proxy_clientfd;
  sockbuf sb2(connection.first);
  iosockstream ss2(&sb2);
  if(formRequest(ss2, request, response, connection.second) < 0) return;
  unsigned short port = request.getPort();
  string server = request.getServer();

  /* Black list check */
  if(blacklistCheck(ss2, server, response) < 0) return;

  /* Acquire the mutex associated with this request from the cache class and lock it */
  size_t index = cach.getHashRequest(request);    
  cach.requests[index].lock();
  
  /* Check if response to this request exist in the Cache before contacting origin server */
  if(checkCache(request, response)){
      publishResponse(ss2, response);
      cach.requests[index].unlock();
      return;
  }

  //Establish connection with the server as a client
  if((proxy_clientfd = clientSocket(server, port)) < 0) return;

  sockbuf sb(proxy_clientfd);
  iosockstream ss(&sb);
  issueRequest(ss, request); 
  formResponse(ss, response, request); 
  cacheResponse(request, response);  //check if the response is cachable if yes cache it. 
  cach.requests[index].unlock();
  publishResponse(ss2, response);
}

/* Helper methods for decomposition purposes */

int HTTPRequestHandler::blacklistCheck(iosockstream& ss, string server, HTTPResponse& response){
  if(!bList.serverIsAllowed(server)){
      response.setResponseCode(403);
      response.setProtocol("HTTP/1.1");
      response.setPayload("Forbidden Content");
      publishResponse(ss, response);
      return -1;
  }
  return 0;
}
/* check for cycles */
bool HTTPRequestHandler::cycle(string IPaddrs){
  string delimiter = ",";
  string ip;
  size_t i=0;
  size_t pos=0; 
  set<string> unique;
  while((pos = IPaddrs.find(delimiter)) != string::npos){
      ip = IPaddrs.substr(0, pos);   
      unique.insert(ip);
      i++;
      IPaddrs.erase(0, pos+1);
  }
  return unique.size() < i;
}

int HTTPRequestHandler::formRequest(iosockstream& ss, HTTPRequest& request, HTTPResponse& response, const string& clientIPAddress){
  try {
        request.ingestRequestLine(ss);
  }
  catch(const HTTPBadRequestException& e) {
     //cerr << oslock << "Error: " << e.what() << endl << "Ignoring Request ......" << endl << osunlock;
     return -1;
  }
  request.ingestHeader(ss, clientIPAddress);
  string value = request.getHeader().getValueAsString("x-forwarded-for");
  if(cycle(value)) {
     response.setResponseCode(504);
     response.setProtocol("HTTP/1.1");
     response.setPayload("Gateway Timeout");
     publishResponse(ss, response);
     return -2;
  }
  if(request.getMethod()!= "HEAD") request.ingestPayload(ss);
  return 0;
}

void HTTPRequestHandler::formResponse(iosockstream& ss, HTTPResponse& response, HTTPRequest& request){
  response.ingestResponseHeader(ss);
  if(request.getMethod()!= "HEAD") response.ingestPayload(ss);
}

void HTTPRequestHandler::sendRequest(iosockstream& ss, HTTPRequest& request){
  ss << request;
  ss.flush();
}

void HTTPRequestHandler::publishResponse(iosockstream& ss, HTTPResponse& response){
  ss << response;
  ss.flush();	
}

int HTTPRequestHandler::clientSocket(const string& server, unsigned short port){
  int fd = createClientSocket(server, port);
  if(fd == kClientSocketError) {
        cerr << "Coudn't not connect to host named \""
         << server << "\"" << endl;
    return kClientSocketError;
  }
  return fd;
}

// Helper Method to issue Http request to the origin server
void HTTPRequestHandler::issueRequest(iosockstream& ss, const HTTPRequest& request) {
  string method = request.getMethod();
  string url = request.getURL();
  string path = request.getPath();
  string server = request.getServer();

  string protocol = request.getProtocol();
  ss << method <<" "<< path <<" "<< protocol<< "\r\n";
  ss << request.getHeader();
  ss << "\r\n";
  ss << request.getPayload();
  ss.flush();
}


// helper methods to handle Caching using HTTPCache Class
bool HTTPRequestHandler::checkCache(const HTTPRequest& request, HTTPResponse& response){
  return cach.containsCacheEntry(request, response);

}

void HTTPRequestHandler::cacheResponse(const HTTPRequest& request, HTTPResponse& response){
  if(cach.shouldCache(request, response)){   // if response is cachable do it if not ignore
     cach.cacheEntry(request, response);
  }
}

void HTTPRequestHandler::clearCache() {
  cach.clear();
}
void HTTPRequestHandler::setCacheMaxAge(long maxAge) {
  cach.setMaxAge(maxAge);
}

