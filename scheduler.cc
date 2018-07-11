/**
 * File: scheduler.cc
 * ------------------
 * Presents the implementation of the HTTPProxyScheduler class.
 */

#include "scheduler.h"
#include <utility>
using namespace std;

static size_t numTotalThreads = 64;
//ThreadPool proxyPool(numTotalThreads);
HTTPProxyScheduler::HTTPProxyScheduler(void){
     proxyPool(numTotalThreads);

}
void HTTPProxyScheduler::scheduleRequest(int clientfd, const string& clientIPAddress, string proxyServer, unsigned short proxyPortNumber) throw () {
  proxyPool.schedule([this, clientfd, clientIPAddress, proxyServer, proxyPortNumber](){
        this->requestHandler.serviceRequest(make_pair(clientfd, clientIPAddress), proxyServer, proxyPortNumber);
  });
}

void HTTPProxyScheduler::scheduleRequest(int clientfd, const string& clientIPAddress) throw () {
  proxyPool.schedule([this, clientfd, clientIPAddress](){
	this->requestHandler.serviceRequest(make_pair(clientfd, clientIPAddress));
  });
}
