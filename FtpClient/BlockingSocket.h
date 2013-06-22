////////////////////////////////////////////////////////////////////////////////
//
// From David J. Kruglinski (Inside Visual C++).
//
////////////////////////////////////////////////////////////////////////////////

// removed baseclass CObject
// removed baseclass CException
// removed all LPCTSTR LPTSTR
// _select only defined to avoid compiler warning in WIN32
// make it run on linux

#ifndef INC_BLOCKINGSOCKET_H
#define INC_BLOCKINGSOCKET_H

#include <string>
#include <string.h>
#include <memory>
#include "Definements.h"

#ifdef WIN32
   #ifdef _WINSOCKAPI_
      #ifndef _WINSOCK2API_
      // NOTICE: In Microsoft Visual Studio you can add the header 'winsock2.h" to 
      //         "Configuration --> Properties --> C/C++ --> Advanced --> Forced Include File"
      #pragma message("\nIt is recommended to include 'winsock2.h' instead of 'winsock.h'\n")
      #endif
   #else
      #include <winsock2.h>
   #endif
#elif defined(unix)
   #include <sys/socket.h>
   #include <arpa/inet.h>   // needed for inet_ntoa and inet_addr
   #include <netdb.h>       // needed for gethostbyname and gethostbyaddr
   #include <errno.h>
#endif

namespace nsSocket
{

#ifdef WIN32
   typedef int socklen_t;
   inline int _select(UINT_PTR /*nfds*/, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, const struct timeval* timeout)
   {
      // The nfds parameter is ignored in windows (included only for 
      // compatibility with Berkeley sockets).
      return select(0, readfds, writefds, exceptfds, timeout);
   }
#elif defined(unix)
   typedef unsigned int SOCKET;
   typedef sockaddr SOCKADDR;
   typedef sockaddr* LPSOCKADDR;
   typedef sockaddr_in SOCKADDR_IN;
   typedef sockaddr_in* LPSOCKADDR_IN;
   typedef timeval TIMEVAL;
   #define SOCKET_ERROR   (-1) 
   #define INVALID_SOCKET (SOCKET)(~0)
   #define closesocket close 
   #define WSAGetLastError() errno
   #define WSAEINTR EINTR
   inline int _select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timeval *__restrict timeout)
   {
      return select(nfds, readfds, writefds, exceptfds, timeout);
   }
#endif   

   typedef const struct sockaddr* LPCSOCKADDR;

///////////////////////////////////////////////////////////////////////////////////////
//***************************** CBlockingSocketException  ***************************//
///////////////////////////////////////////////////////////////////////////////////////
class CBlockingSocketException
{
public:
   // Constructor
   CBlockingSocketException(LPCTSTR pchMessage);
   virtual ~CBlockingSocketException() {}
   
   virtual bool GetErrorMessage(LPTSTR lpstrError, UINT nMaxError, PUINT pnHelpContext = NULL);
   virtual tstring GetErrorMessage(PUINT pnHelpContext = NULL);
   
private:
   int     m_nError;
   tstring m_strMessage;
};

///////////////////////////////////////////////////////////////////////////////////////
//*********************************** CSockAddr  ************************************//
///////////////////////////////////////////////////////////////////////////////////////
class CSockAddr : public sockaddr_in
{
public:
   // constructors
   CSockAddr()
   {
      sin_family        = AF_INET; // specifies address family
      sin_port          = 0;       // ip port
      sin_addr.s_addr   = 0;       // ip address
   }
   
   CSockAddr(const SOCKADDR& sa)
            { memcpy(this, &sa, sizeof(SOCKADDR)); }
   
   CSockAddr(const SOCKADDR_IN& sin) 
            { memcpy(this, &sin, sizeof(SOCKADDR_IN)); }
   
   CSockAddr(ULONG ulAddr, USHORT ushPort = 0) // parms are host byte ordered
            { 
               sin_family      = AF_INET;          // specifies address family
               sin_port        = htons(ushPort);   // ip port
               sin_addr.s_addr = htonl(ulAddr);    // ip address
            }
   
   CSockAddr(const char* pchIP, USHORT ushPort = 0) // dotted IP addr string
            { 
               sin_family = AF_INET;               // specifies address family
               sin_port = htons(ushPort);          // ip port
               sin_addr.s_addr = inet_addr(pchIP); // ip address
            } // already network byte ordered
   
   // Return the address in dotted-decimal format
   tstring DottedDecimal()
      { return nsHelper::CCnv::ConvertToTString(inet_ntoa(sin_addr)); }

   // Get port and address (even though they're public)
   USHORT Port() const
            { return ntohs(sin_port); }
   
   ULONG IPAddr() const
            { return ntohl(sin_addr.s_addr); }
   
   // operators added for efficiency
   CSockAddr& operator=(const SOCKADDR& sa)
            {
               memcpy(this, &sa, sizeof(SOCKADDR));
               return *this;
            }

   CSockAddr& operator=(const SOCKADDR_IN& sin)
            { 
               memcpy(this, &sin, sizeof(SOCKADDR_IN));
               return *this; 
            }

   operator SOCKADDR()
            { return *((LPSOCKADDR) this); }

   operator LPSOCKADDR()
            { return (LPSOCKADDR) this; }

   operator LPSOCKADDR_IN()
            { return (LPSOCKADDR_IN) this; }
};

///////////////////////////////////////////////////////////////////////////////////////
//********************************* CBlockingSocket  ********************************//
///////////////////////////////////////////////////////////////////////////////////////

class IBlockingSocket
{
public:
   virtual ~IBlockingSocket() {};
   virtual IBlockingSocket* CreateInstance() const = 0;
   virtual void Create(int nType = SOCK_STREAM) = 0;
   virtual void Connect(LPCSOCKADDR psa) const = 0;
   virtual void Bind(LPCSOCKADDR psa) const = 0;
   virtual void Listen() const = 0;
   virtual void Cleanup() = 0;
   virtual bool Accept(IBlockingSocket& s, LPSOCKADDR psa) const = 0;
   virtual void Close() = 0;
   virtual int  Write(const char* pch, int nSize, int nSecs) const = 0;
   virtual int  Receive(char* pch, int nSize, int nSecs) const = 0;
   virtual void GetPeerAddr(LPSOCKADDR psa) const = 0;
   virtual void GetSockAddr(LPSOCKADDR psa) const = 0;
   virtual operator SOCKET() const = 0;
   virtual bool CheckReadability() const = 0;
   virtual CSockAddr   GetHostByName(const char* pchName, USHORT ushPort = 0) = 0;
   virtual const char* GetHostByAddr(LPCSOCKADDR psa) = 0;
};

// member functions truly block and must not be used in UI threads
// use this class as an alternative to the MFC CSocket class
class CBlockingSocket : public IBlockingSocket
{
public:
   CBlockingSocket();
   virtual ~CBlockingSocket();

   virtual IBlockingSocket* CreateInstance() const;

   void Cleanup();
   void Create(int nType = SOCK_STREAM);
   void Close();

   void Bind(LPCSOCKADDR psa) const;
   void Listen() const;
   void Connect(LPCSOCKADDR psa) const;
   bool Accept(IBlockingSocket& s, LPSOCKADDR psa) const;
   int  Send(const char* pch, int nSize, int nSecs) const;
   int  Write(const char* pch, int nSize, int nSecs) const;
   int  Receive(char* pch, int nSize, int nSecs) const;
   int  SendDatagram(const char* pch, int nSize, LPCSOCKADDR psa, int nSecs) const;
   int  ReceiveDatagram(char* pch, int nSize, LPSOCKADDR psa, int nSecs) const;
   void GetPeerAddr(LPSOCKADDR psa) const;
   void GetSockAddr(LPSOCKADDR psa) const;
   bool CheckReadability() const;

   CSockAddr   GetHostByName(const char* pchName, USHORT ushPort = 0);
   const char* GetHostByAddr(LPCSOCKADDR psa);

   operator SOCKET() const { return m_hSocket; }

private:
   SOCKET m_hSocket;
};

std::auto_ptr<IBlockingSocket> CreateDefaultBlockingSocketInstance();

///////////////////////////////////////////////////////////////////////////////////////
//******************************** CHttpBlockingSocket ******************************//
///////////////////////////////////////////////////////////////////////////////////////
class CHttpBlockingSocket : public CBlockingSocket
{
public:
   enum {nSizeRecv = 1000}; // max receive buffer size (> hdr line length)

   CHttpBlockingSocket();
   ~CHttpBlockingSocket();

   int ReadHttpHeaderLine(char* pch, int nSize, int nSecs);
   int ReadHttpResponse(char* pch, int nSize, int nSecs);

private:
   char*  m_pReadBuf; // read buffer
   int    m_nReadBuf; // number of bytes in the read buffer
};

}

#endif // INC_BLOCKINGSOCKET_H
