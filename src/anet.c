/* anet.c -- Basic TCP socket stuff made a bit less boring
 *
 * Copyright (c) 2006-2010, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "fmacros.h"

#include <sys/types.h>
#ifdef _WIN32
  #include "win32fixes.h"
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <netinet/tcp.h>
  #include <arpa/inet.h>
  #include <netdb.h>
#endif
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

#include "anet.h"

static void anetSetError(char *err, const char *fmt, ...)
{
    va_list ap;

    if (!err) return;
    va_start(ap, fmt);
    vsnprintf(err, ANET_ERR_LEN, fmt, ap);
    va_end(ap);
}

int anetNonBlock(char *err, int fd)
{
    /* Set the socket nonblocking.
     * Note that fcntl(2) for F_GETFL and F_SETFL can't be
     * interrupted by a signal. */
#ifdef _WIN32
  // If iMode = 0, blocking is enabled;
  // If iMode != 0, non-blocking mode is enabled.
  u_long iMode = 1;
  if (ioctlsocket(fd, FIONBIO, &iMode) == SOCKET_ERROR) {
    errno = WSAGetLastError();
    anetSetError(err, "ioctlsocket(FIONBIO): %d\n", errno);
    return ANET_ERR;
  };

  return ANET_OK;
#else
    int flags;

    if ((flags = fcntl(fd, F_GETFL)) == -1) {
        anetSetError(err, "fcntl(F_GETFL): %s\n", strerror(errno));
        return ANET_ERR;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        anetSetError(err, "fcntl(F_SETFL,O_NONBLOCK): %s\n", strerror(errno));
        return ANET_ERR;
    }

    return ANET_OK;
#endif
}

int anetTcpNoDelay(char *err, int fd)
{
    int yes = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes)) == -1)
    {
        anetSetError(err, "setsockopt TCP_NODELAY: %s\n", strerror(errno));
        return ANET_ERR;
    }
    return ANET_OK;
}

int anetSetSendBuffer(char *err, int fd, int buffsize)
{
    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &buffsize, sizeof(buffsize)) == -1)
    {
        anetSetError(err, "setsockopt SO_SNDBUF: %s\n", strerror(errno));
        return ANET_ERR;
    }
    return ANET_OK;
}

int anetTcpKeepAlive(char *err, int fd)
{
    int yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes)) == -1) {
        anetSetError(err, "setsockopt SO_KEEPALIVE: %s\n", strerror(errno));
        return ANET_ERR;
    }
    return ANET_OK;
}

int anetResolve(char *err, char *host, char *ipbuf)
{
    struct sockaddr_in sa;
#ifdef _WIN32
    unsigned long inAddress;

    sa.sin_family = AF_INET;
    inAddress = inet_addr(host);
    if (inAddress == INADDR_NONE || inAddress == INADDR_ANY) {
#else
    sa.sin_family = AF_INET;
    if (inet_aton(host, &sa.sin_addr) == 0) {
#endif
        struct hostent *he;

        he = gethostbyname(host);
        if (he == NULL) {
            anetSetError(err, "can't resolve: %s\n", host);
            return ANET_ERR;
        }
        memcpy(&sa.sin_addr, he->h_addr, sizeof(struct in_addr));
    }
#ifdef _WIN32
    else {
      sa.sin_addr.s_addr = inAddress;
    };
#endif
    strcpy(ipbuf,inet_ntoa(sa.sin_addr));
    return ANET_OK;
}

#define ANET_CONNECT_NONE 0
#define ANET_CONNECT_NONBLOCK 1
static int anetTcpGenericConnect(char *err, char *addr, int port, int flags)
{
    int s, on = 1;
    struct sockaddr_in sa;

#ifdef _WIN32
    memset(&sa, 0, sizeof(sa));

    if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        errno = WSAGetLastError();
        anetSetError(err, "create socket error: %d\n", errno);
        return ANET_ERR;
    }
#else

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        anetSetError(err, "creating socket: %s\n", strerror(errno));
        return ANET_ERR;
    }
#endif
    /* Make sure connection-intensive things like the redis benckmark
     * will be able to close/open sockets a zillion of times */
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);

#ifdef _WIN32
    unsigned long inAddress;

    inAddress = inet_addr(addr);
    if (inAddress == INADDR_NONE || inAddress == INADDR_ANY) {
        struct hostent *he;

        he = gethostbyname(addr);
        if (he == NULL) {
            anetSetError(err, "can't resolve: %s\n", addr);
            closesocket(s);
            return ANET_ERR;
        }
        memcpy(&sa.sin_addr, he->h_addr, sizeof(struct in_addr));
    }
    else {
      sa.sin_addr.s_addr = inAddress;
    }
    if (flags & ANET_CONNECT_NONBLOCK) {
        if (anetNonBlock(err,s) != ANET_OK) {
            return ANET_ERR;
        }
    }

#else
    if (inet_aton(addr, &sa.sin_addr) == 0) {
        struct hostent *he;

        he = gethostbyname(addr);
        if (he == NULL) {
            anetSetError(err, "can't resolve: %s\n", addr);
            close(s);
            return ANET_ERR;
        }
        memcpy(&sa.sin_addr, he->h_addr, sizeof(struct in_addr));
    }

    if (flags & ANET_CONNECT_NONBLOCK) {
        if (anetNonBlock(err,s) != ANET_OK)
            return ANET_ERR;
    }
#endif
    if (connect(s, (struct sockaddr*)&sa, sizeof(sa)) == -1) {
#ifdef _WIN32
        errno = WSAGetLastError();
        if ((errno == WSAEINVAL) || (errno == WSAEWOULDBLOCK)) errno = EINPROGRESS;
#endif
        if (errno == EINPROGRESS &&
            flags & ANET_CONNECT_NONBLOCK)
            return s;
#ifdef _WIN32
        anetSetError(err, "connect: %d\n", errno);
        closesocket(s);
#else
        anetSetError(err, "connect: %s\n", strerror(errno));
        close(s);
#endif
        return ANET_ERR;
    }
    return s;
}

int anetTcpConnect(char *err, char *addr, int port)
{
    return anetTcpGenericConnect(err,addr,port,ANET_CONNECT_NONE);
}

int anetTcpNonBlockConnect(char *err, char *addr, int port)
{
    return anetTcpGenericConnect(err,addr,port,ANET_CONNECT_NONBLOCK);
}

/* Like read(2) but make sure 'count' is read before to return
 * (unless error or EOF condition is encountered) */
int anetRead(int fd, char *buf, int count)
{
    int nread, totlen = 0;
    while(totlen != count) {
#ifdef _WIN32
        nread = recv(fd,buf,count-totlen,0);
#else
        nread = read(fd,buf,count-totlen);
#endif
        if (nread == 0) return totlen;
        if (nread == -1) return -1;
        totlen += nread;
        buf += nread;
    }
    return totlen;
}

/* Like write(2) but make sure 'count' is read before to return
 * (unless error is encountered) */
int anetWrite(int fd, char *buf, int count)
{
    int nwritten, totlen = 0;
    while(totlen != count) {
#ifdef _WIN32
        nwritten = send(fd,buf,count-totlen,0);
        if (nwritten == -1) errno = WSAGetLastError();
        if ((errno == ENOENT) || (errno == WSAEWOULDBLOCK))
            errno = EAGAIN;
#else
        nwritten = write(fd,buf,count-totlen);
#endif
        if (nwritten == 0) return totlen;
        if (nwritten == -1) return -1;
        totlen += nwritten;
        buf += nwritten;
    }
    return totlen;
}

int anetTcpServer(char *err, int port, char *bindaddr)
{
    int s, on = 1;
    struct sockaddr_in sa;

#ifdef _WIN32
        if(!w32initWinSock()) {
            /* "Error iError at WSAStartup()  */
            errno = WSAGetLastError();
            anetSetError(err, "Winsock init error: %d\n", errno);
            return ANET_ERR;
        };

        if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
            errno = WSAGetLastError();
            anetSetError(err, "socket: %d\n", errno);
            return ANET_ERR;
        }
#else
    if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        anetSetError(err, "socket: %s\n", strerror(errno));
        return ANET_ERR;
    }
#endif

    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) {
#ifdef _WIN32
        errno = WSAGetLastError();
        anetSetError(err, "setsockopt SO_REUSEADDR: %d\n", errno);
        closesocket(s);
#else
        anetSetError(err, "setsockopt SO_REUSEADDR: %s\n", strerror(errno));
        close(s);
#endif
        return ANET_ERR;
    }
    memset(&sa,0,sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bindaddr) {
#ifdef _WIN32
        unsigned long inAddress;

        inAddress = inet_addr(bindaddr);
        if (inAddress == INADDR_NONE || inAddress == INADDR_ANY) {
            anetSetError(err, "Invalid bind address\n");
            closesocket(s);
            return ANET_ERR;
        }
        else {
            sa.sin_addr.s_addr = inAddress;
        };
#else
        if (inet_aton(bindaddr, &sa.sin_addr) == 0) {
            anetSetError(err, "Invalid bind address\n");
            close(s);
            return ANET_ERR;
        }
#endif

    }
    if (bind(s, (struct sockaddr*)&sa, sizeof(sa)) == -1) {
#ifdef _WIN32
        errno = WSAGetLastError();
        anetSetError(err, "bind error: %d\n", errno);
        closesocket(s);
#else
        anetSetError(err, "bind: %s\n", strerror(errno));
        close(s);
#endif
        return ANET_ERR;
    }
    if (listen(s, 511) == -1) { /* the magic 511 constant is from nginx */
#ifdef _WIN32
        errno = WSAGetLastError();
        anetSetError(err, "listen error: %d\n", errno);
        closesocket(s);
#else
        anetSetError(err, "listen: %s\n", strerror(errno));
        close(s);
#endif
        return ANET_ERR;
    }
    return s;
}

int anetAccept(char *err, int serversock, char *ip, int *port)
{
    int fd;
    struct sockaddr_in sa;
#ifdef _WIN32
    int saLen;
#else
    unsigned int saLen;
#endif

    while(1) {
        saLen = sizeof(sa);
        fd = accept(serversock, (struct sockaddr*)&sa, &saLen);
        if (fd == -1) {
            if (errno == EINTR)
                continue;
            else {
#ifdef _WIN32
                errno = WSAGetLastError();
                anetSetError(err, "accept: %d\n", errno);
#else
                anetSetError(err, "accept: %s\n", strerror(errno));
#endif
                return ANET_ERR;
            }
        }
        break;
    }
    if (ip) strcpy(ip,inet_ntoa(sa.sin_addr));
    if (port) *port = ntohs(sa.sin_port);
    return fd;
}
