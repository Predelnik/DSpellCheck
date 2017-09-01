////////////////////////////////////////////////////////////////////////////////
//
// The official specification of the File Transfer Protocol (FTP) is the RFC 959.
// Most of the documentation are taken from this RFC.
// This is an implementation of an simple FTP client. I have tried to implement
// platform independent. For the communication i used the classes CBlockingSocket,
// CSockAddr, ... from David J. Kruglinski (Inside Visual C++). These classes are
// only small wrappers for the sockets-API.
// Further I used a smart pointer-implementation from Scott Meyers (Effective C++,
// More Effective C++, Effective STL).
// The implementation of the logon-sequence (with firewall support) was published
// in an article on Codeguru by Phil Anderson.
// The code for the parsing of the different FTP LIST responses is taken from
// D. J. Bernstein (http://cr.yp.to/ftpparse.html). I only wrapped the c-code in
// a class.
// I have tested the code on Windows and Linux (Kubuntu).
//
// Copyright (c) 2004-2012 Thomas Oswald
//
// Permission to copy, use, sell and distribute this software is granted
// provided this copyright notice appears in all copies.
// Permission to modify the code and to distribute modified code is granted
// provided this copyright notice appears in all copies, and a notice
// that the code was modified is included with the copyright notice.
//
// This software is provided "as is" without express or implied warranty,
// and with no claim as to its suitability for any purpose.
//
// History:
// v1.5 released 2007-06-01
//+# TODO
// v1.1 released 2005-12-04
//      - Bug in OpenPassiveDataConnection removed: SendCommand was called before data connection was established.
//      - Bugs in GetSingleResponseLine removed:
//         * Infinite loop if response line doesn't end with CRLF.
//         * Return value of std:string->find must be checked against npos.
//      - Now runs in unicode.
//      - Streams removed.
//      - Explicit detaching of observers are not necessary anymore.
//      - ExecuteDatachannelCommand now accepts an ITransferNotification object.
//        Through this concept there is no need to write the received files to a file.
//        For example the bytes can be written only in memory or an other tcp stream.
//      - Added an interface for the blocking socket (IBlockingSocket).
//        Therefore it is possible to exchange the socket implementation, e.g. for
//        writing unit tests (by simulating an specific scenario of a FTP communication).
//      - Replaced the magic numbers concerning the reply codes by a class.
// v1.0 released 2004-10-25
////////////////////////////////////////////////////////////////////////////////

//#include "stdafx.h"
#include "FTPClient.h"
#include <limits>
#include <algorithm>
#include <iterator>
#include <assert.h>
#include <stdio.h>
#include "FTPListParse.h"
#include "FTPFileStatus.h"

#undef max

#ifdef __AFX_H__ // MFC only
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
#endif

using namespace nsHelper;

namespace nsFTP
{
  class CMakeString
  {
  public:
    CMakeString& operator<<(DWORD dwNum)
    {
      DWORD dwTemp = dwNum;
      int iCnt=1; // name lookup of 'iCnt' changed for new ISO 'for' scoping
      for( ; (dwTemp/=10) != 0; iCnt++ )
        ;

      m_str.resize(m_str.size() + iCnt);
      // string size +1 because tsprintf needs the size including the terminating NULL character;
      // the size method returns the size without the terminating NULL character
      tsprintf(&(*m_str.begin()), m_str.size()+1, _T("%s%u"), m_str.c_str(), dwNum);

      return *this;
    }

    CMakeString& operator<<(const tstring& strAdd)
    {
      m_str += strAdd;
      return *this;
    }

    operator tstring() const { return m_str; }

  private:
    tstring m_str;
  };

  tstring& ReplaceStr(tstring& strTarget, const tstring& strToReplace, const tstring& strReplacement)
  {
    size_t pos = strTarget.find(strToReplace);
    while( pos != tstring::npos )
    {
      strTarget.replace(pos, strToReplace.length(), strReplacement);
      pos = strTarget.find(strToReplace, pos+1);
    }
    return strTarget;
  }

  class CFile : public CFTPClient::ITransferNotification
  {
    FILE* m_pFile;
    tstring m_strFileName;
  public:
    enum TOriginEnum { orBegin=SEEK_SET, orEnd=SEEK_END, orCurrent=SEEK_CUR };

    CFile() : m_pFile(NULL) {}
    virtual ~CFile()
    {
      Close();
    }

    bool Open(const tstring& strFileName, const tstring& strMode)
    {
      m_strFileName = strFileName;
#if _MSC_VER >= 1500
      return fopen_s(&m_pFile, CCnv::ConvertToString(strFileName).c_str(),
        CCnv::ConvertToString(strMode).c_str()) == 0;
#else
      m_pFile = fopen(CCnv::ConvertToString(strFileName).c_str(),
        CCnv::ConvertToString(strMode).c_str());
      return m_pFile!=NULL;
#endif
    }

    bool Close()
    {
      FILE* pFile = m_pFile;
      m_pFile = NULL;
      return pFile && fclose(pFile)==0;
    }

    bool Seek(long lOffset, TOriginEnum enOrigin)
    {
      return m_pFile && fseek(m_pFile, lOffset, enOrigin)==0;
    }

    long Tell()
    {
      if( !m_pFile )
        return -1L;
      return ftell(m_pFile);
    }

    size_t Write(const void* pBuffer, size_t itemSize, size_t itemCount)
    {
      if( !m_pFile )
        return 0;
      return fwrite(pBuffer, itemSize, itemCount, m_pFile);
    }

    size_t Read(void* pBuffer, size_t itemSize, size_t itemCount)
    {
      if( !m_pFile )
        return 0;
      return fread(pBuffer, itemSize, itemCount, m_pFile);
    }

    virtual tstring GetLocalStreamName() const override
    {
      return m_strFileName;
    }

    virtual UINT GetLocalStreamSize() const override
    {
      if( !m_pFile )
        return 0;

      const long lCurPos = ftell(m_pFile);
      fseek(m_pFile, 0, SEEK_END);
      const long lEndPos = ftell(m_pFile);
      fseek(m_pFile, lCurPos, SEEK_SET);

      return lEndPos;
    }

    virtual void SetLocalStreamOffset(DWORD dwOffsetFromBeginOfStream) override
    {
      Seek(dwOffsetFromBeginOfStream, CFile::orBegin);
    }

    virtual void OnBytesReceived(const TByteVector& vBuffer, long lReceivedBytes) override
    {
      Write(&(*vBuffer.begin()), sizeof(TByteVector::value_type), lReceivedBytes);
    }

    virtual void OnPreBytesSend(char* pszBuffer, size_t bufferSize, size_t& bytesToSend) override
    {
      bytesToSend = Read(pszBuffer, sizeof(char), bufferSize);
    }
  };

  class COutputStream::CPimpl
  {
    CPimpl& operator=(const CPimpl&); // no implementation for assignment operator
  public:
    CPimpl(const tstring& strEolCharacterSequence, const tstring& strStreamName) :
      mc_strEolCharacterSequence(strEolCharacterSequence),
      m_itCurrentPos(m_vBuffer.end()),
      m_strStreamName(strStreamName)
    {
    }

    const tstring mc_strEolCharacterSequence;
    tstring m_vBuffer;
    tstring::iterator m_itCurrentPos;
    tstring m_strStreamName;
  };

  COutputStream::COutputStream(const tstring& strEolCharacterSequence, const tstring& strStreamName) :
    m_spPimpl(new CPimpl(strEolCharacterSequence, strStreamName))
  {
  }

  COutputStream::~COutputStream() {}

  void COutputStream::SetBuffer(const tstring& strBuffer)
  {
    m_spPimpl->m_vBuffer = strBuffer;
  }

  const tstring& COutputStream::GetBuffer() const
  {
    return m_spPimpl->m_vBuffer;
  }

  void COutputStream::SetStartPosition()
  {
    m_spPimpl->m_itCurrentPos = m_spPimpl->m_vBuffer.begin();
  }

  bool COutputStream::GetNextLine(tstring& strLine)// const
  {
    tstring::iterator it = std::search(m_spPimpl->m_itCurrentPos, m_spPimpl->m_vBuffer.end(), m_spPimpl->mc_strEolCharacterSequence.begin(), m_spPimpl->mc_strEolCharacterSequence.end());
    if( it == m_spPimpl->m_vBuffer.end() )
      return false;

    strLine.assign(m_spPimpl->m_itCurrentPos, it);

    m_spPimpl->m_itCurrentPos = it + m_spPimpl->mc_strEolCharacterSequence.size();

    return true;
  }

  tstring COutputStream::GetLocalStreamName() const
  {
    return m_spPimpl->m_strStreamName;
  }

  UINT COutputStream::GetLocalStreamSize() const
  {
    return m_spPimpl->m_vBuffer.size();
  }

  void COutputStream::SetLocalStreamOffset(DWORD dwOffsetFromBeginOfStream)
  {
    m_spPimpl->m_itCurrentPos = m_spPimpl->m_vBuffer.begin() + dwOffsetFromBeginOfStream;
  }

  void COutputStream::OnBytesReceived(const TByteVector& vBuffer, long lReceivedBytes)
  {
    std::copy(vBuffer.begin(), vBuffer.begin()+lReceivedBytes, std::back_inserter(m_spPimpl->m_vBuffer));
  }

  void COutputStream::OnPreBytesSend(char* pszBuffer, size_t bufferSize, size_t& bytesToSend)
  {
    for( bytesToSend=0; m_spPimpl->m_itCurrentPos!=m_spPimpl->m_vBuffer.end() && bytesToSend < bufferSize; ++m_spPimpl->m_itCurrentPos, ++bytesToSend )
      pszBuffer[bytesToSend] = *m_spPimpl->m_itCurrentPos;
  }
}

using namespace nsFTP;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

/// constructor
/// @param[in] apSocket Instance of socket class which will be used for
///                     communication with the FTP server.
///                     CFTPClient class takes ownership of this instance.
///                     It will be deleted on destruction of this object.
///                     If this pointer is NULL, the CBlockingSocket implementation
///                     will be used.
///                     This gives the ability to set an other socket class.
///                     For example a socket class can be implemented which simulates
///                     a FTP server (for unit testing).
/// @param[in] uiTimeout Timeout used for socket operation.
/// @param[in] uiBufferSize Size of the buffer used for sending and receiving
///                         data via sockets. The size have an influence on
///                         the performance. Through empiric test i come to the
///                         conclusion that 2048 is a good size.
/// @param[in] uiResponseWait Sleep time between receive calls to socket when getting
///                           the response. Sometimes the socket hangs if no wait time
///                           is set. Normally not wait time is necessary.
CFTPClient::CFTPClient(std::unique_ptr<IBlockingSocket> apSocket, unsigned int uiTimeout/*=10*/,
                       unsigned int uiBufferSize/*=2048*/, unsigned int uiResponseWait/*=0*/,
                       const tstring& strRemoteDirectorySeparator/*=_T("/")*/) :
mc_uiTimeout(uiTimeout),
  mc_uiResponseWait(uiResponseWait),
  mc_strEolCharacterSequence(_T("\r\n")),
  mc_strRemoteDirectorySeparator(strRemoteDirectorySeparator),//+# documentation missing
  m_vBuffer(uiBufferSize),
  m_apSckControlConnection(std::move (apSocket)),
  m_apFileListParser(new CFTPListParser()),
  m_fTransferInProgress(false),
  m_fAbortTransfer(false),
  m_fResumeIfPossible(true)
{
  ASSERT( m_apSckControlConnection.get() );
}

/// destructor
CFTPClient::~CFTPClient()
{
  if( IsTransferringData() )
    Abort();

  if( IsConnected() )
    Logout();
}

/// Attach an observer to the client. You can attach as many observers as you want.
/// The client send notifications (more precisely the virtual functions are called)
/// to the observers.
void CFTPClient::AttachObserver(CFTPClient::CNotification* pObserver)
{
  ASSERT( pObserver );
  if( pObserver )
    m_setObserver.Attach(pObserver);
}

/// Detach an observer from the client.
void CFTPClient::DetachObserver(CFTPClient::CNotification* pObserver)
{
  ASSERT( pObserver );
  if( pObserver )
    m_setObserver.Detach(pObserver);
}

/// Sets the file list parser which is used for parsing the results of the LIST command.
void CFTPClient::SetFileListParser(std::unique_ptr<IFileListParser> apFileListParser)
{
  m_apFileListParser = std::move (apFileListParser);
}

/// Returns a set of all observers currently attached to the client.
CFTPClient::TObserverSet& CFTPClient::GetObservers()
{
  return m_setObserver;
}

/// Enables or disables resuming for file transfer operations.
/// @param[in] fEnable True enables resuming, false disables it.
void CFTPClient::SetResumeMode(bool fEnable/*=true*/)
{
  m_fResumeIfPossible=fEnable;
}

/// Indicates if the resume mode is set.
bool CFTPClient::IsResumeModeEnabled() const
{
  return m_fResumeIfPossible;
}

/// Opens the control channel to the FTP server.
/// @param[in] strServerHost IP-address or name of the server
/// @param[in] iServerPort Port for channel. Usually this is port 21.
bool CFTPClient::OpenControlChannel(const tstring& strServerHost, USHORT ushServerPort/*=DEFAULT_FTP_PORT*/)
{
  CloseControlChannel();

  try
  {
    m_apSckControlConnection->Create(SOCK_STREAM);
    CSockAddr adr = m_apSckControlConnection->GetHostByName(CCnv::ConvertToString(strServerHost).c_str(), ushServerPort);
    m_apSckControlConnection->Connect(adr);
  }
  catch(CBlockingSocketException& blockingException)
  {
    ReportError(blockingException.GetErrorMessage(), CCnv::ConvertToTString(__FILE__), __LINE__);
    m_apSckControlConnection->Cleanup();
    return false;
  }

  return true;
}

/// Returns the connection state of the client.
bool CFTPClient::IsConnected() const
{
  return m_apSckControlConnection->operator SOCKET()!=INVALID_SOCKET;
}

/// Returns true if a download/upload is running, otherwise false.
bool CFTPClient::IsTransferringData() const
{
  return m_fTransferInProgress;
}

/// Closes the control channel to the FTP server.
void CFTPClient::CloseControlChannel()
{
  try
  {
    m_apSckControlConnection->Close();
    m_apCurrentRepresentation.reset(NULL);
  }
  catch(CBlockingSocketException& blockingException)
  {
    blockingException.GetErrorMessage();
    m_apSckControlConnection->Cleanup();
  }
}

/// Analyse the repy code of a FTP server-response.
/// @param[in] Reply Reply of a FTP server.
/// @retval FTP_OK    All runs perfect.
/// @retval FTP_ERROR Something went wrong. An other response was expected.
/// @retval NOT_OK    The command was not accepted.
int CFTPClient::SimpleErrorCheck(const CReply& Reply) const
{
  if( Reply.Code().IsNegativeReply() )
    return FTP_NOTOK;
  else if( Reply.Code().IsPositiveCompletionReply() )
    return FTP_OK;

  ASSERT( Reply.Code().IsPositiveReply() );

  return FTP_ERROR;
}

/// Logs on to a FTP server.
/// @param[in] logonInfo Structure with logon information.
bool CFTPClient::Login(const CLogonInfo& logonInfo)
{
  m_LastLogonInfo = logonInfo;

  enum {LO=-2,      ///< Logged On
    ER=-1,      ///< Error
    NUMLOGIN=9, ///< currently supports 9 different login sequences
  };

  int iLogonSeq[NUMLOGIN][18] = {
    // this array stores all of the logon sequences for the various firewalls
    // in blocks of 3 nums.
    // 1st num is command to send,
    // 2nd num is next point in logon sequence array if 200 series response
    //         is rec'd from server as the result of the command,
    // 3rd num is next point in logon sequence if 300 series rec'd
    { 0,LO,3,    1,LO, 6,   2,LO,ER                                  }, // no firewall
    { 3, 6,3,    4, 6,ER,   5,ER, 9,   0,LO,12,   1,LO,15,   2,LO,ER }, // SITE hostname
    { 3, 6,3,    4, 6,ER,   6,LO, 9,   1,LO,12,   2,LO,ER            }, // USER after logon
    { 7, 3,3,    0,LO, 6,   1,LO, 9,   2,LO,ER                       }, // proxy OPEN
    { 3, 6,3,    4, 6,ER,   0,LO, 9,   1,LO,12,   2,LO,ER            }, // Transparent
    { 6,LO,3,    1,LO, 6,   2,LO,ER                                  }, // USER with no logon
    { 8, 6,3,    4, 6,ER,   0,LO, 9,   1,LO,12,   2,LO,ER            }, // USER fireID@remotehost
    { 9,ER,3,    1,LO, 6,   2,LO,ER                                  }, // USER remoteID@remotehost fireID
    {10,LO,3,   11,LO, 6,   2,LO,ER                                  }  // USER remoteID@fireID@remotehost
  };

  // are we connecting directly to the host (logon type 0) or via a firewall? (logon type>0)
  tstring   strTemp;
  USHORT    ushPort=0;

  if( logonInfo.FwType() == CFirewallType::None())
  {
    strTemp = logonInfo.Hostname();
    ushPort = logonInfo.Hostport();
  }
  else
  {
    strTemp = logonInfo.FwHost();
    ushPort = logonInfo.FwPort();
  }

  tstring strHostnamePort(logonInfo.Hostname());
  if( logonInfo.Hostport()!=DEFAULT_FTP_PORT )
    strHostnamePort = CMakeString() << logonInfo.Hostname() << _T(":") << logonInfo.Hostport(); // add port to hostname (only if port is not 21)

  if( IsConnected() )
    Logout();

  if( !OpenControlChannel(strTemp, ushPort) )
    return false;

  // get initial connect msg off server
  CReply Reply;
  if( !GetResponse(Reply) || !Reply.Code().IsPositiveCompletionReply() )
    return false;

  int iLogonPoint=0;

  // go through appropriate logon procedure
  for ( ; ; )
  {
    // send command, get response
    CReply Reply;
    switch(iLogonSeq[logonInfo.FwType().AsEnum()][iLogonPoint])
    {
      // state                 command           command argument                                                                              success     fail
    case 0:  if( SendCommand(CCommand::USER(), logonInfo.Username(),                                                                Reply) ) break; else return false;
    case 1:  if( SendCommand(CCommand::PASS(), logonInfo.Password(),                                                                Reply) ) break; else return false;
    case 2:  if( SendCommand(CCommand::ACCT(), logonInfo.Account(),                                                                 Reply) ) break; else return false;
    case 3:  if( SendCommand(CCommand::USER(), logonInfo.FwUsername(),                                                              Reply) ) break; else return false;
    case 4:  if( SendCommand(CCommand::PASS(), logonInfo.FwPassword(),                                                              Reply) ) break; else return false;
    case 5:  if( SendCommand(CCommand::SITE(), strHostnamePort,                                                                     Reply) ) break; else return false;
    case 6:  if( SendCommand(CCommand::USER(), logonInfo.Username() + _T("@") + strHostnamePort,                                    Reply) ) break; else return false;
    case 7:  if( SendCommand(CCommand::OPEN(), strHostnamePort,                                                                     Reply) ) break; else return false;
    case 8:  if( SendCommand(CCommand::USER(), logonInfo.FwUsername() + _T("@") + strHostnamePort,                                  Reply) ) break; else return false;
    case 9:  if( SendCommand(CCommand::USER(), logonInfo.Username() + _T("@") + strHostnamePort + _T(" ") + logonInfo.FwUsername(), Reply) ) break; else return false;
    case 10: if( SendCommand(CCommand::USER(), logonInfo.Username() + _T("@") + logonInfo.FwUsername() + _T("@") + strHostnamePort, Reply) ) break; else return false;
    case 11: if( SendCommand(CCommand::PASS(), logonInfo.Password() + _T("@") + logonInfo.FwPassword(),                             Reply) ) break; else return false;
    }

    if( !Reply.Code().IsPositiveCompletionReply() && !Reply.Code().IsPositiveIntermediateReply() )
      return false;

    const unsigned int uiFirstDigitOfReplyCode = CCnv::TStringToLong(Reply.Code().Value())/100;
    iLogonPoint=iLogonSeq[logonInfo.FwType().AsEnum()][iLogonPoint + uiFirstDigitOfReplyCode-1]; //get next command from array
    switch(iLogonPoint)
    {
    case ER: // ER means somewhat has gone wrong
      {
        ReportError(_T("Logon failed."), CCnv::ConvertToTString(__FILE__), __LINE__);
      }
      return false;
    case LO: // LO means we're fully logged on
      if( ChangeWorkingDirectory(mc_strRemoteDirectorySeparator)!=FTP_OK )
        return false;
      return true;
    }
  }
  //return false;
}

/// Rename a file on the FTP server.
/// @remarks Can be used for moving the file to another directory.
/// @param[in] strOldName Name of the file to rename.
/// @param[in] strNewName The new name for the file.
/// @return see return values of CFTPClient::SimpleErrorCheck
int CFTPClient::Rename(const tstring& strOldName, const tstring& strNewName) const
{
  CReply Reply;
  if( !SendCommand(CCommand::RNFR(), strOldName, Reply) )
    return FTP_ERROR;

  if( Reply.Code().IsNegativeReply() )
    return FTP_NOTOK;
  else if( !Reply.Code().IsPositiveIntermediateReply() )
  {
    ASSERT( Reply.Code().IsPositiveCompletionReply() || Reply.Code().IsPositivePreliminaryReply() );
    return FTP_ERROR;
  }

  if( !SendCommand(CCommand::RNTO(), strNewName, Reply) )
    return FTP_ERROR;

  return SimpleErrorCheck(Reply);
}

/// Moves a file within the FTP server.
/// @param[in] strFullSourceFilePath Name of the file which should be moved.
/// @param[in] strFullTargetFilePath The destination where the file should be moved to (file name must be also given).
/// @return see return values of CFTPClient::SimpleErrorCheck
int CFTPClient::Move(const tstring& strFullSourceFilePath, const tstring& strFullTargetFilePath) const
{
  return Rename(strFullSourceFilePath, strFullTargetFilePath);
}

/// Gets the directory listing of the FTP server. Sends the LIST command to
/// the FTP server.
/// @param[in] strPath Starting path for the list command.
/// @param[out] vstrFileList Returns a simple list of the files and folders of the specified directory.
/// @param[in] fPasv see documentation of CFTPClient::Passive
bool CFTPClient::List(const tstring& strPath, TStringVector& vstrFileList, bool fPasv) const
{
  COutputStream outputStream(mc_strEolCharacterSequence, CCommand::LIST().AsString());
  if( !ExecuteDatachannelCommand(CCommand::LIST(), strPath, CRepresentation(CType::ASCII()), fPasv, 0, outputStream) )
    return false;

  vstrFileList.clear();
  tstring strLine;
  outputStream.SetStartPosition();
  while( outputStream.GetNextLine(strLine) )
    vstrFileList.push_back(strPath + strLine.c_str());

  return true;
}

/// Gets the directory listing of the FTP server. Sends the NLST command to
/// the FTP server.
/// @param[in] strPath Starting path for the list command.
/// @param[out] vstrFileList Returns a simple list of the files and folders of the specified the directory.
/// @param[in] fPasv see documentation of CFTPClient::Passive
bool CFTPClient::NameList(const tstring& strPath, TStringVector& vstrFileList, bool fPasv) const
{
  COutputStream outputStream(mc_strEolCharacterSequence, CCommand::NLST().AsString());
  if( !ExecuteDatachannelCommand(CCommand::NLST(), strPath, CRepresentation(CType::ASCII()), fPasv, 0, outputStream) )
    return false;

  vstrFileList.clear();
  tstring strLine;
  outputStream.SetStartPosition();
  while( outputStream.GetNextLine(strLine) )
    vstrFileList.push_back(strPath + strLine.c_str());

  return true;
}

/// Gets the directory listing of the FTP server. Sends the LIST command to
/// the FTP server.
/// @param[in] strPath Starting path for the list command.
/// @param[out] vFileList Returns a detailed list of the files and folders of the specified directory.
///                       vFileList contains CFTPFileStatus-Objects. These Objects provide a lot of
///                       information about the file/folder.
/// @param[in] fPasv see documentation of CFTPClient::Passive
bool CFTPClient::List(const tstring& strPath, TFTPFileStatusShPtrVec& vFileList, bool fPasv) const
{
  ASSERT( m_apFileListParser.get() );

  COutputStream outputStream(mc_strEolCharacterSequence, CCommand::LIST().AsString());
  if( !ExecuteDatachannelCommand(CCommand::LIST(), strPath, CRepresentation(CType::ASCII()), fPasv, 0, outputStream) )
    return false;

  vFileList.clear();
  tstring strLine;
  outputStream.SetStartPosition();
  while( outputStream.GetNextLine(strLine) )
  {
    TFTPFileStatusShPtr spFtpFileStatus(new CFTPFileStatus());
    if( m_apFileListParser->Parse(*spFtpFileStatus, strLine) )
    {
      spFtpFileStatus->Path() = strPath;
      vFileList.push_back(spFtpFileStatus);
    }
  }

  return true;
}

/// Gets the directory listing of the FTP server. Sends the NLST command to
/// the FTP server.
/// @param[in] strPath Starting path for the list command.
/// @param[out] vFileList Returns a simple list of the files and folders of the specified directory.
///                       vFileList contains CFTPFileStatus-Objects. Normally these Objects provide
///                       a lot of information about the file/folder. But the NLST-command provide
///                       only a simple list of the directory content (no specific information).
/// @param[in] fPasv see documentation of CFTPClient::Passive
bool CFTPClient::NameList(const tstring& strPath, TFTPFileStatusShPtrVec& vFileList, bool fPasv) const
{
  COutputStream outputStream(mc_strEolCharacterSequence, CCommand::NLST().AsString());
  if( !ExecuteDatachannelCommand(CCommand::NLST(), strPath, CRepresentation(CType::ASCII()), fPasv, 0, outputStream) )
    return false;

  vFileList.clear();
  tstring strLine;
  outputStream.SetStartPosition();
  while( outputStream.GetNextLine(strLine) )
  {
    TFTPFileStatusShPtr spFtpFileStatus(new CFTPFileStatus());
    spFtpFileStatus->Path() = strPath;
    spFtpFileStatus->Name() = strLine;
    vFileList.push_back(spFtpFileStatus);
  }

  return true;
}

/// Gets a file from the FTP server.
/// Uses C functions for file access (very fast).
/// @param[in] strRemoteFile Filename of the sourcefile on the FTP server.
/// @param[in] strLocalFile Filename of the target file on the local computer.
/// @param[in] repType Representation Type (see documentation of CRepresentation)
/// @param[in] fPasv see documentation of CFTPClient::Passive
bool CFTPClient::DownloadFile(const tstring& strRemoteFile, const tstring& strLocalFile, const CRepresentation& repType, bool fPasv) const
{
  CFile file;
  if( !file.Open(strLocalFile, m_fResumeIfPossible?_T("ab"):_T("wb")) )
  {
    ReportError(CError::GetErrorDescription(), CCnv::ConvertToTString(__FILE__), __LINE__);
    return false;
  }
  file.Seek(0, CFile::orEnd);

  return DownloadFile(strRemoteFile, file, repType, fPasv);
}

/// Gets a file from the FTP server.
/// Uses C functions for file access (very fast).
/// @param[in] strRemoteFile Filename of the sourcefile on the FTP server.
/// @param[in] Observer Object which receives the transfer notifications.
/// @param[in] repType Representation Type (see documentation of CRepresentation)
/// @param[in] fPasv see documentation of CFTPClient::Passive
bool CFTPClient::DownloadFile(const tstring& strRemoteFile, ITransferNotification& Observer, const CRepresentation& repType, bool fPasv) const
{
  long lRemoteFileSize = 0;
  FileSize(strRemoteFile, lRemoteFileSize);

  for( TObserverSet::const_iterator it=m_setObserver.begin(); it!=m_setObserver.end(); it++ )
    (*it)->OnPreReceiveFile(strRemoteFile, Observer.GetLocalStreamName(), lRemoteFileSize);

  const bool fRet = ExecuteDatachannelCommand(CCommand::RETR(), strRemoteFile, repType, fPasv,
    m_fResumeIfPossible ? Observer.GetLocalStreamSize() : 0, Observer);

  for( TObserverSet::const_iterator it2=m_setObserver.begin(); it2!=m_setObserver.end(); it2++ )
    (*it2)->OnPostReceiveFile(strRemoteFile, Observer.GetLocalStreamName(), lRemoteFileSize);

  return fRet;
}

/// Gets a file from the FTP server.
/// The target file is on an other FTP server (FXP).
/// NOTICE: The file is directly transferred from one server to the other server.
/// @param[in] strSourceFile File which is on the source FTP server.
/// @param[in] TargetFtpServer The FTP server where the downloaded file will be stored.
/// @param[in] strTargetFile Filename of the target file on the target FTP server.
/// @param[in] repType Representation Type (see documentation of CRepresentation)
/// @param[in] fPasv see documentation of CFTPClient::Passive
bool CFTPClient::DownloadFile(const tstring& strSourceFile, const CFTPClient& TargetFtpServer,
                              const tstring& strTargetFile, const CRepresentation& repType/*=CRepresentation(CType::Image())*/,
                              bool fPasv/*=true*/) const
{
  return TransferFile(*this, strSourceFile, TargetFtpServer, strTargetFile, repType, fPasv);
}

/// Puts a file on the FTP server.
/// Uses C functions for file access (very fast).
/// @param[in] strLocalFile Filename of the the local sourcefile which to put on the FTP server.
/// @param[in] strRemoteFile Filename of the target file on the FTP server.
/// @param[in] fStoreUnique if true, the FTP command STOU is used for saving
///                         else the FTP command STOR is used.
/// @param[in] repType Representation Type (see documentation of CRepresentation)
/// @param[in] fPasv see documentation of CFTPClient::Passive
bool CFTPClient::UploadFile(const tstring& strLocalFile, const tstring& strRemoteFile, bool fStoreUnique, const CRepresentation& repType, bool fPasv) const
{
  CFile file;
  if( !file.Open(strLocalFile, _T("rb")) )
  {
    ReportError(CError::GetErrorDescription(), CCnv::ConvertToTString(__FILE__), __LINE__);
    return false;
  }

  return UploadFile(file, strRemoteFile, fStoreUnique, repType, fPasv);
}

/// Puts a file on the FTP server.
/// Uses C functions for file access (very fast).
/// @param[in] Observer Object which receives the transfer notifications for upload requests.
/// @param[in] strRemoteFile Filename of the target file on the FTP server.
/// @param[in] fStoreUnique if true, the FTP command STOU is used for saving
///                         else the FTP command STOR is used.
/// @param[in] repType Representation Type (see documentation of CRepresentation)
/// @param[in] fPasv see documentation of CFTPClient::Passive
bool CFTPClient::UploadFile(ITransferNotification& Observer, const tstring& strRemoteFile, bool fStoreUnique, const CRepresentation& repType, bool fPasv) const
{
  long lRemoteFileSize = 0;
  if( m_fResumeIfPossible )
    FileSize(strRemoteFile, lRemoteFileSize);

  CCommand cmd(CCommand::STOR());
  if( lRemoteFileSize > 0 )
    cmd = CCommand::APPE();
  else if( fStoreUnique )
    cmd = CCommand::STOU();

  const long lLocalFileSize = Observer.GetLocalStreamSize();
  Observer.SetLocalStreamOffset(lRemoteFileSize);

  TObserverSet::const_iterator it;
  for( it=m_setObserver.begin(); it!=m_setObserver.end(); it++ )
    (*it)->OnPreSendFile(Observer.GetLocalStreamName(), strRemoteFile, lLocalFileSize);

  const bool fRet = ExecuteDatachannelCommand(cmd, strRemoteFile, repType, fPasv, 0, Observer);

  for( it=m_setObserver.begin(); it!=m_setObserver.end(); it++ )
    (*it)->OnPostSendFile(Observer.GetLocalStreamName(), strRemoteFile, lLocalFileSize);

  return fRet;
}

/// Puts a file on the FTP server.
/// The source file is on an other FTP server (FXP).
/// NOTICE: The file is directly transferred from one server to the other server.
/// @param[in] SourceFtpServer A FTP server from which the file is taken for upload action.
/// @param[in] strSourceFile File which is on the source FTP server.
/// @param[in] strTargetFile Filename of the target file on the FTP server.
/// @param[in] repType Representation Type (see documentation of CRepresentation)
/// @param[in] fPasv see documentation of CFTPClient::Passive
bool CFTPClient::UploadFile(const CFTPClient& SourceFtpServer, const tstring& strSourceFile,
                            const tstring& strTargetFile, const CRepresentation& repType/*=CRepresentation(CType::Image())*/,
                            bool fPasv/*=true*/) const
{
  return TransferFile(SourceFtpServer, strSourceFile, *this, strTargetFile, repType, !fPasv);
}

/// Transfers a file from a FTP server to another FTP server.
/// The source file is on an other FTP server (FXP).
/// NOTICE: The file is directly transferred from one server to the other server.
/// @param[in] SourceFtpServer A FTP server from which the file which is copied.
/// @param[in] strSourceFile Name of the file which is on the source FTP server.
/// @param[in] TargetFtpServer A FTP server to which the file is copied.
/// @param[in] strTargetFile Name of the file on the target FTP server.
/// @param[in] repType Representation Type (see documentation of CRepresentation)
/// @param[in] fPasv see documentation of CFTPClient::Passive
/*static*/ bool CFTPClient::TransferFile(const CFTPClient& SourceFtpServer, const tstring& strSourceFile,
                                         const CFTPClient& TargetFtpServer, const tstring& strTargetFile,
                                         const CRepresentation& repType/*=CRepresentation(CType::Image())*/,
                                         bool fSourcePasv/*=false*/)
{
  // transmit representation to server
  if( SourceFtpServer.RepresentationType(repType)!=FTP_OK )
    return false;

  if( TargetFtpServer.RepresentationType(repType)!=FTP_OK )
    return false;

  const CFTPClient& PassiveServer = fSourcePasv ? SourceFtpServer : TargetFtpServer;
  const CFTPClient& ActiveServer  = fSourcePasv ? TargetFtpServer : SourceFtpServer;

  // set one FTP server in passive mode
  // the FTP server opens a port and tell us the socket (ip address + port)
  // this socket is used for opening the data connection
  ULONG  ulIP    = 0;
  USHORT ushSock = 0;
  if( PassiveServer.Passive(ulIP, ushSock)!=FTP_OK )
    return false;

  CSockAddr csaPassiveServer(ulIP, ushSock);

  // transmit the socket (ip address + port) of the first FTP server to the
  // second server
  // the second FTP server establishes then the data connection to the first
  if( ActiveServer.DataPort(csaPassiveServer.DottedDecimal(), ushSock)!=FTP_OK )
    return false;

  if( !SourceFtpServer.SendCommand(CCommand::RETR(), strSourceFile) )
    return false;

  CReply ReplyTarget;
  if( !TargetFtpServer.SendCommand(CCommand::STOR(), strTargetFile, ReplyTarget) ||
    !ReplyTarget.Code().IsPositivePreliminaryReply() )
    return false;

  CReply ReplySource;
  if( !SourceFtpServer.GetResponse(ReplySource) || !ReplySource.Code().IsPositivePreliminaryReply() )
    return false;

  // get response from FTP servers
  if( !SourceFtpServer.GetResponse(ReplySource) || !ReplySource.Code().IsPositiveCompletionReply() ||
    !TargetFtpServer.GetResponse(ReplyTarget) || !ReplyTarget.Code().IsPositiveCompletionReply() )
    return false;

  return true;
}

/// Executes a commando that result in a communication over the data port.
/// @param[in] crDatachannelCmd Command to be executeted.
/// @param[in] strPath Parameter for the command usually a path.
/// @param[in] representation see documentation of CFTPClient::CRepresentation
/// @param[in] fPasv see documentation of CFTPClient::Passive
/// @param[in] dwByteOffset Server marker at which file transfer is to be restarted.
/// @param[in] Observer Object for observing the execution of the command.
bool CFTPClient::ExecuteDatachannelCommand(const CCommand& crDatachannelCmd, const tstring& strPath, const CRepresentation& representation,
                                           bool fPasv, DWORD dwByteOffset, ITransferNotification& Observer) const
{
  if( !crDatachannelCmd.IsDatachannelCommand() )
  {
    ASSERT(false);
    return false;
  }

  if( m_fTransferInProgress )
    return false;

  if( !IsConnected() )
    return false;

  // transmit representation to server
  if( RepresentationType(representation)!=FTP_OK )
    return false;

  std::unique_ptr<IBlockingSocket> apSckDataConnection(m_apSckControlConnection->CreateInstance());

  if( fPasv )
  {
    if( !OpenPassiveDataConnection(*apSckDataConnection, crDatachannelCmd, strPath, dwByteOffset) )
      return false;
  }
  else
  {
    if( !OpenActiveDataConnection(*apSckDataConnection, crDatachannelCmd, strPath, dwByteOffset) )
      return false;
  }

  const bool fTransferOK = TransferData(crDatachannelCmd, Observer, *apSckDataConnection);

  apSckDataConnection->Close();

  // get response from FTP server
  CReply Reply;
  if( !fTransferOK || !GetResponse(Reply) || !Reply.Code().IsPositiveCompletionReply() )
    return false;

  return true;
}

/// Executes a commando that result in a communication over the data port.
/// @param[in] crDatachannelCmd Command to be executeted.
/// @param[in] Observer Object for observing the execution of the command.
/// @param[in] sckDataConnection Socket which is used for sending/receiving data.
bool CFTPClient::TransferData(const CCommand& crDatachannelCmd, ITransferNotification& Observer, IBlockingSocket& sckDataConnection) const
{
  if( crDatachannelCmd.IsDatachannelWriteCommand() )
  {
    if( !SendData(Observer, sckDataConnection) )
      return false;
  }
  else if( crDatachannelCmd.IsDatachannelReadCommand() )
  {
    if( !ReceiveData(Observer, sckDataConnection) )
      return false;
  }
  else
  {
    ASSERT( false );
    return false;
  }
  return true;
}

/// Opens an active data connection.
/// @param[out] sckDataConnection
/// @param[in] crDatachannelCmd Command to be executeted.
/// @param[in] strPath Parameter for the command usually a path.
/// @param[in] dwByteOffset Server marker at which file transfer is to be restarted.
bool CFTPClient::OpenActiveDataConnection(IBlockingSocket& sckDataConnection, const CCommand& crDatachannelCmd, const tstring& strPath, DWORD dwByteOffset) const
{
  if( !crDatachannelCmd.IsDatachannelCommand() )
  {
    ASSERT(false);
    return false;
  }

  std::unique_ptr<IBlockingSocket> apSckServer(m_apSckControlConnection->CreateInstance());

  USHORT ushLocalSock = 0;
  try
  {
    // INADDR_ANY = ip address of localhost
    // second parameter "0" means that the WINSOCKAPI ask for a port
    CSockAddr csaAddressTemp(INADDR_ANY, 0);
    apSckServer->Create(SOCK_STREAM);
    apSckServer->Bind(csaAddressTemp);
    apSckServer->GetSockAddr(csaAddressTemp);
    ushLocalSock=csaAddressTemp.Port();
    apSckServer->Listen();
  }
  catch(CBlockingSocketException& blockingException)
  {
    ReportError(blockingException.GetErrorMessage(), CCnv::ConvertToTString(__FILE__), __LINE__);
    apSckServer->Cleanup();
    return false;
  }

  // get own ip address
  CSockAddr csaLocalAddress;
  m_apSckControlConnection->GetSockAddr(csaLocalAddress);

  // transmit the socket (ip address + port) to the server
  // the FTP server establishes then the data connection
  if( DataPort(csaLocalAddress.DottedDecimal(), ushLocalSock)!=FTP_OK )
    return false;

  // if resuming is activated then set offset
  if( m_fResumeIfPossible &&
    (crDatachannelCmd==CCommand::STOR() || crDatachannelCmd==CCommand::RETR() || crDatachannelCmd==CCommand::APPE() ) &&
    (dwByteOffset!=0 && Restart(dwByteOffset)!=FTP_OK) )
    return false;

  // send FTP command RETR/STOR/NLST/LIST to the server
  CReply Reply;
  if( !SendCommand(crDatachannelCmd, strPath, Reply) ||
    !Reply.Code().IsPositivePreliminaryReply() )
    return false;

  // accept the data connection
  CSockAddr sockAddrTemp;
  if( !apSckServer->Accept(sckDataConnection, sockAddrTemp) )
    return false;

  return true;
}

/// Opens a passive data connection.
/// @param[out] sckDataConnection
/// @param[in] crDatachannelCmd Command to be executeted.
/// @param[in] strPath Parameter for the command usually a path.
/// @param[in] dwByteOffset Server marker at which file transfer is to be restarted.
bool CFTPClient::OpenPassiveDataConnection(IBlockingSocket& sckDataConnection, const CCommand& crDatachannelCmd, const tstring& strPath, DWORD dwByteOffset) const
{
  if( !crDatachannelCmd.IsDatachannelCommand() )
  {
    ASSERT(false);
    return false;
  }

  ULONG   ulRemoteHostIP = 0;
  USHORT  ushServerSock  = 0;

  // set passive mode
  // the FTP server opens a port and tell us the socket (ip address + port)
  // this socket is used for opening the data connection
  if( Passive(ulRemoteHostIP, ushServerSock)!=FTP_OK )
    return false;

  // establish connection
  CSockAddr sockAddrTemp;
  try
  {
    sckDataConnection.Create(SOCK_STREAM);
    CSockAddr csaAddress(ulRemoteHostIP, ushServerSock);
    sckDataConnection.Connect(csaAddress);
  }
  catch(CBlockingSocketException& blockingException)
  {
    ReportError(blockingException.GetErrorMessage(), CCnv::ConvertToTString(__FILE__), __LINE__);
    sckDataConnection.Cleanup();
    return false;
  }

  // if resuming is activated then set offset
  if( m_fResumeIfPossible &&
    (crDatachannelCmd==CCommand::STOR() || crDatachannelCmd==CCommand::RETR() || crDatachannelCmd==CCommand::APPE() ) &&
    (dwByteOffset!=0 && Restart(dwByteOffset)!=FTP_OK) )
    return false;

  // send FTP command RETR/STOR/NLST/LIST to the server
  CReply Reply;
  if( !SendCommand(crDatachannelCmd, strPath, Reply) ||
    !Reply.Code().IsPositivePreliminaryReply() )
    return false;

  return true;
}

/// Sends data over a socket to the server.
/// @param[in] Observer Object for observing the execution of the command.
/// @param[in] sckDataConnection Socket which is used for the send action.
bool CFTPClient::SendData(ITransferNotification& Observer, IBlockingSocket& sckDataConnection) const
{
  try
  {
    m_fTransferInProgress=true;

    int iNumWrite = 0;
    size_t bytesRead = 0;

    Observer.OnPreBytesSend(&m_vBuffer.front(), m_vBuffer.size(), bytesRead);

    while( !m_fAbortTransfer && bytesRead!=0 )
    {
      iNumWrite = sckDataConnection.Write(&(*m_vBuffer.begin()), static_cast<int>(bytesRead), mc_uiTimeout);
      ASSERT( iNumWrite == static_cast<int>(bytesRead) );

      for( TObserverSet::const_iterator it=m_setObserver.begin(); it!=m_setObserver.end(); it++ )
        (*it)->OnBytesSent(m_vBuffer, iNumWrite);

      Observer.OnPreBytesSend(&m_vBuffer.front(), m_vBuffer.size(), bytesRead);
    }

    m_fTransferInProgress=false;
    if( m_fAbortTransfer )
    {
      Abort();
      return false;
    }
  }
  catch(CBlockingSocketException& blockingException)
  {
    m_fTransferInProgress=false;
    ReportError(blockingException.GetErrorMessage(), CCnv::ConvertToTString(__FILE__), __LINE__);
    sckDataConnection.Cleanup();
    return false;
  }

  return true;
}

/// Receives data over a socket from the server.
/// @param[in] Observer Object for observing the execution of the command.
/// @param[in] sckDataConnection Socket which is used for receiving the data.
bool CFTPClient::ReceiveData(ITransferNotification& Observer, IBlockingSocket& sckDataConnection) const
{
  try
  {
    m_fTransferInProgress = true;

    for( TObserverSet::const_iterator it=m_setObserver.begin(); it!=m_setObserver.end(); it++ )
      (*it)->OnBeginReceivingData();

    int iNumRead=sckDataConnection.Receive(&(*m_vBuffer.begin()), static_cast<int>(m_vBuffer.size()), mc_uiTimeout);
    long lTotalBytes = iNumRead;
    while( !m_fAbortTransfer && iNumRead!=0 )
    {
      for( TObserverSet::const_iterator it=m_setObserver.begin(); it!=m_setObserver.end(); it++ )
        (*it)->OnBytesReceived(m_vBuffer, iNumRead);

      Observer.OnBytesReceived(m_vBuffer, iNumRead);

      iNumRead=sckDataConnection.Receive(&(*m_vBuffer.begin()), static_cast<int>(m_vBuffer.size()), mc_uiTimeout);
      lTotalBytes += iNumRead;
    }

    for( TObserverSet::const_iterator it2=m_setObserver.begin(); it2!=m_setObserver.end(); it2++ )
      (*it2)->OnEndReceivingData(lTotalBytes);

    m_fTransferInProgress=false;
    if( m_fAbortTransfer )
    {
      Abort();
      return false;
    }
  }
  catch(CBlockingSocketException& blockingException)
  {
    m_fTransferInProgress=false;
    ReportError(blockingException.GetErrorMessage(), CCnv::ConvertToTString(__FILE__), __LINE__);
    sckDataConnection.Cleanup();
    return false;
  }

  return true;
}

/// Sends a command to the server.
/// @param[in] Command Command to send.
bool CFTPClient::SendCommand(const CCommand& Command, const CArg& Arguments) const
{
  if( !IsConnected() )
    return false;

  try
  {
    for( TObserverSet::const_iterator it=m_setObserver.begin(); it!=m_setObserver.end(); it++ )
      (*it)->OnSendCommand(Command, Arguments);
    const std::string strCommand = CCnv::ConvertToString(Command.AsString(Arguments)) + "\r\n";
    m_apSckControlConnection->Write(strCommand.c_str(), static_cast<int>(strCommand.length()), mc_uiTimeout);
  }
  catch(CBlockingSocketException& blockingException)
  {
    ReportError(blockingException.GetErrorMessage(), CCnv::ConvertToTString(__FILE__), __LINE__);
    const_cast<CFTPClient*>(this)->m_apSckControlConnection->Cleanup();
    return false;
  }

  return true;
}

/// Sends a command to the server.
/// @param[in]  Command Command to send.
/// @param[out] Reply The Reply of the server to the sent command.
bool CFTPClient::SendCommand(const CCommand& Command, const CArg& Arguments, CReply& Reply) const
{
  if( !SendCommand(Command, Arguments) || !GetResponse(Reply) )
    return false;
  return true;
}

/// This function gets the server response.
/// A server response can exists of more than one line. This function
/// returns the full response (multiline).
/// @param[out] Reply Reply of the server to a command.
bool CFTPClient::GetResponse(CReply& Reply) const
{
  tstring strResponse;
  if( !GetSingleResponseLine(strResponse) )
    return false;

  if( strResponse.length()>3 && strResponse.at(3)==_T('-') )
  {
    tstring strSingleLine(strResponse);
    const int iRetCode=CCnv::TStringToLong(strResponse);
    // handle multi-line server responses
    while( !(strSingleLine.length()>3 &&
      strSingleLine.at(3)==_T(' ') &&
      CCnv::TStringToLong(strSingleLine)==iRetCode) )
    {
      if( !GetSingleResponseLine(strSingleLine) )
        return false;
      strResponse += mc_strEolCharacterSequence + strSingleLine;
    }
  }

  bool fRet = Reply.Set(strResponse);

  for( TObserverSet::const_iterator it=m_setObserver.begin(); it!=m_setObserver.end(); it++ )
    (*it)->OnResponse(Reply);

  return fRet;
}

/// Reads a single response line from the server control channel.
/// @param[out] strResponse Response of the server as string.
bool CFTPClient::GetSingleResponseLine(tstring& strResponse) const
{
  if( !IsConnected() )
    return false;

  try
  {
    if( m_qResponseBuffer.empty() )
    {
      // internal buffer is empty ==> get response from FTP server
      int iNum=0;
      std::string strTemp;

      do
      {
        iNum=m_apSckControlConnection->Receive(&(*m_vBuffer.begin()), static_cast<int>(m_vBuffer.size())-1, mc_uiTimeout);
        if( mc_uiResponseWait !=0 )
          Sleep(mc_uiResponseWait);
        m_vBuffer[iNum] = '\0';
        strTemp+=&(*m_vBuffer.begin());
      } while( iNum==static_cast<int>(m_vBuffer.size())-1 && m_apSckControlConnection->CheckReadability() );

      // each line in response is a separate entry in the internal buffer
      while( strTemp.length() )
      {
        size_t iCRLF=strTemp.find('\n');
        if( iCRLF != std::string::npos )
        {
          m_qResponseBuffer.push(strTemp.substr(0, iCRLF+1));
          strTemp.erase(0, iCRLF+1);
        }
        else
        {
          // this is not rfc standard; normally each command must end with CRLF
          // in this case it doesn't
          m_qResponseBuffer.push(strTemp);
          strTemp.clear();
        }
      }

      if( m_qResponseBuffer.empty() )
        return false;
    }

    // get first response-line from buffer
    strResponse = CCnv::ConvertToTString(m_qResponseBuffer.front().c_str());
    m_qResponseBuffer.pop();

    // remove CrLf if exists (don't use mc_strEolCharacterSequence here)
    if( strResponse.length()> 1 && strResponse.substr(strResponse.length()-2)==_T("\r\n") )
      strResponse.erase(strResponse.length()-2, 2);
  }
  catch(CBlockingSocketException& blockingException)
  {
    ReportError(blockingException.GetErrorMessage(), CCnv::ConvertToTString(__FILE__), __LINE__);
    const_cast<CFTPClient*>(this)->m_apSckControlConnection->Cleanup();
    return false;
  }

  return true;
}

/// Executes the FTP command CDUP (change to parent directory).
/// This command is a special case of CFTPClient::ChangeWorkingDirectory
/// (CWD), and is  included to simplify the implementation of programs for
/// transferring directory trees between operating systems having different
/// syntaxes for naming the parent directory.
/// @return see return values of CFTPClient::SimpleErrorCheck
int CFTPClient::ChangeToParentDirectory() const
{
  CReply Reply;
  if( !SendCommand(CCommand::CDUP(), CArg(), Reply) )
    return FTP_ERROR;
  return SimpleErrorCheck(Reply);
}

/// Executes the FTP command QUIT.
/// This command terminates a USER and if file transfer is not in progress,
/// the server closes the control connection. If file transfer is in progress,
/// the connection will remain open for result response and the server will
/// then close it.
/// If the user-process is transferring files for several USERs but does not
/// wish to close and then reopen connections for each, then the REIN command
/// should be used instead of QUIT.
/// An unexpected close on the control connection will cause the server to take
/// the effective action of an abort (ABOR) and a logout.
/// @return see return values of CFTPClient::SimpleErrorCheck
int CFTPClient::Logout()
{
  CReply Reply;
  if( !SendCommand(CCommand::QUIT(), CArg(), Reply) )
    return FTP_ERROR;

  CloseControlChannel();

  return SimpleErrorCheck(Reply);
}

/// Executes the FTP command PASV. Set the passive mode.
/// This command requests the server-DTP (data transfer process) on a data to
/// "listen"  port (which is not its default data port) and to wait for a
/// connection rather than initiate one upon receipt of a transfer command.
/// The response to this command includes the host and port address this
/// server is listening on.
/// @param[out] ulIpAddress IP address the server is listening on.
/// @param[out] ushPort Port the server is listening on.
/// @return see return values of CFTPClient::SimpleErrorCheck
int CFTPClient::Passive(ULONG& ulIpAddress, USHORT& ushPort) const
{
  CReply Reply;
  if( !SendCommand(CCommand::PASV(), CArg(), Reply) )
    return FTP_ERROR;

  if( Reply.Code().IsPositiveCompletionReply() )
  {
    if( !GetIpAddressFromResponse(Reply.Value(), ulIpAddress, ushPort) )
      return FTP_ERROR;
  }

  return SimpleErrorCheck(Reply);
}

/// Parses a response string and extracts the ip address and port information.
/// @param[in]  strResponse The response string of a FTP server which holds
///                         the ip address and port information.
/// @param[out] ulIpAddress Buffer for the ip address.
/// @param[out] ushPort     Buffer for the port information.
/// @retval true  Everything went ok.
/// @retval false An error occurred (invalid format).
bool CFTPClient::GetIpAddressFromResponse(const tstring& strResponse, ULONG& ulIpAddress, USHORT& ushPort) const
{
  // parsing of ip-address and port implemented with a finite state machine
  // ...(192,168,1,1,3,44)...
  enum T_enState { state0, state1, state2, state3, state4 } enState = state0;

  tstring strIpAddress, strPort;
  USHORT ushTempPort = 0;
  ULONG  ulTempIpAddress = 0;
  int iCommaCnt = 4;
  for( tstring::const_iterator it=strResponse.begin(); it!=strResponse.end(); ++it )
  {
    switch( enState )
    {
    case state0:
      if( *it == _T('(') )
        enState = state1;
      break;
    case state1:
      if( *it == _T(',') )
      {
        if( --iCommaCnt == 0 )
        {
          enState = state2;
          ulTempIpAddress += CCnv::TStringToLong(strIpAddress.c_str());
        }
        else
        {
          ulTempIpAddress += CCnv::TStringToLong(strIpAddress.c_str())<<8*iCommaCnt;
          strIpAddress.clear();
        }
      }
      else
      {
        if( !tisdigit(*it) )
          return false;
        strIpAddress += *it;
      }
      break;
    case state2:
      if( *it == _T(',') )
      {
        ushTempPort = static_cast<USHORT>(CCnv::TStringToLong(strPort.c_str())<<8);
        strPort.clear();
        enState = state3;
      }
      else
      {
        if( !tisdigit(*it) )
          return false;
        strPort += *it;
      }
      break;
    case state3:
      if( *it == _T(')') )
      {
        // compiler warning if using +=operator
        ushTempPort = ushTempPort + static_cast<USHORT>(CCnv::TStringToLong(strPort.c_str()));
        enState = state4;
      }
      else
      {
        if( !tisdigit(*it) )
          return false;
        strPort += *it;
      }
      break;
    case state4:
      break; // some compilers complain if not all enumeration values are listet
    }
  }

  if( enState==state4 )
  {
    ulIpAddress = ulTempIpAddress;
    ushPort = ushTempPort;
  }

  return enState==state4;
}

/// Executes the FTP command ABOR.
/// This command tells the server to abort the previous FTP service command
/// and any associated transfer of data.  The abort command may require
/// "special action", as discussed in the Section on FTP Commands, to force
/// recognition by the server. No action is to be taken if the previous
/// command has been completed (including data transfer). The control
/// connection is not to be closed by the server, but the data connection
/// must be closed.
/// There are two cases for the server upon receipt of this command:<BR>
/// (1) the FTP service command was already completed, or <BR>
/// (2) the FTP service command is still in progress.<BR>
/// In the first case, the server closes the data connection (if it is open)
/// and responds with a 226 reply, indicating that the abort command was
/// successfully processed.
/// In the second case, the server aborts the FTP service in progress and
/// closes the data connection, returning a 426 reply to indicate that the
/// service request terminated abnormally. The server then sends a 226 reply,
/// indicating that the abort command was successfully processed.
/// @return see return values of CFTPClient::SimpleErrorCheck
int CFTPClient::Abort() const
{
  if( m_fTransferInProgress )
  {
    m_fAbortTransfer = true;
    return FTP_OK;
  }

  m_fAbortTransfer = false;
  CReply Reply;
  if( !SendCommand(CCommand::ABOR(), CArg(), Reply) )
    return FTP_ERROR;
  return SimpleErrorCheck(Reply);
}

/// Executes the FTP command PWD (PRINT WORKING DIRECTORY)
/// This command causes the name of the current working directory
/// to be returned in the reply.
int CFTPClient::PrintWorkingDirectory() const
{
  CReply Reply;
  if( !SendCommand(CCommand::PWD(), CArg(), Reply) )
    return FTP_ERROR;
  return SimpleErrorCheck(Reply);
}

/// Executes the FTP command SYST (SYSTEM)
/// This command is used to find out the type of operating system at the server.
/// The reply shall have as its first word one of the system names listed in the
/// current version of the Assigned Numbers document [Reynolds, Joyce, and
/// Jon Postel, "Assigned Numbers", RFC 943, ISI, April 1985.].
/// @return see return values of CFTPClient::SimpleErrorCheck
int CFTPClient::System() const
{
  CReply Reply;
  if( !SendCommand(CCommand::SYST(), CArg(), Reply) )
    return FTP_ERROR;
  return SimpleErrorCheck(Reply);
}

/// Executes the FTP command NOOP
/// This command does not affect any parameters or previously entered commands.
/// It specifies no action other than that the server send an FTP_OK reply.
/// @return see return values of CFTPClient::SimpleErrorCheck
int CFTPClient::Noop() const
{
  CReply Reply;
  if( !SendCommand(CCommand::NOOP(), CArg(), Reply) )
    return FTP_ERROR;
  return SimpleErrorCheck(Reply);
}

/// Executes the FTP command PORT (DATA PORT)
/// The argument is a HOST-PORT specification for the data port to be used in data
/// connection. There are defaults for both the user and server data ports, and
/// under normal circumstances this command and its reply are not needed.  If
/// this command is used, the argument is the concatenation of a 32-bit internet
/// host address and a 16-bit TCP port address.
/// @param[in] strHostIP IP-address like xxx.xxx.xxx.xxx
/// @param[in] uiPort 16-bit TCP port address.
/// @return see return values of CFTPClient::SimpleErrorCheck
int CFTPClient::DataPort(const tstring& strHostIP, USHORT ushPort) const
{
  tstring strPortArguments;
  // convert the port number to 2 bytes + add to the local IP
  strPortArguments = CMakeString() << strHostIP << _T(",") << (ushPort>>8) << _T(",") << (ushPort&0xFF);

  ReplaceStr(strPortArguments, _T("."), _T(","));

  CReply Reply;
  if( !SendCommand(CCommand::PORT(), strPortArguments, Reply) )
    return FTP_ERROR;
  return SimpleErrorCheck(Reply);
}

/// Executes the FTP command TYPE (REPRESENTATION TYPE)
/// Caches the representation state if successful.
/// see Documentation of nsFTP::CRepresentation
/// @param[in] representation see Documentation of nsFTP::CRepresentation
/// @param[in] iSize Indicates Bytesize for type LocalByte.
/// @return see return values of CFTPClient::SimpleErrorCheck
int CFTPClient::RepresentationType(const CRepresentation& representation, DWORD dwSize) const
{
  // check representation
  if( m_apCurrentRepresentation.get()!=NULL && representation==*m_apCurrentRepresentation )
    return FTP_OK;

  const int iRet = _RepresentationType(representation, dwSize);

  if( iRet==FTP_OK )
  {
    if( m_apCurrentRepresentation.get()==NULL )
      m_apCurrentRepresentation.reset(new CRepresentation(representation));
    else
      *m_apCurrentRepresentation = representation;
  }
  else
    m_apCurrentRepresentation.reset(NULL);

  return iRet;
}

/// Executes the FTP command TYPE (REPRESENTATION TYPE)
/// see Documentation of nsFTP::CRepresentation
/// @param[in] representation see Documentation of nsFTP::CRepresentation
/// @param[in] dwSize Indicates Bytesize for type LocalByte.
/// @return see return values of CFTPClient::SimpleErrorCheck
int CFTPClient::_RepresentationType(const CRepresentation& representation, DWORD dwSize) const
{
  CArg Arguments(representation.Type().AsString());

  switch( representation.Type().AsEnum() )
  {
  case CType::tyLocalByte:
    Arguments.push_back(CMakeString() << dwSize);
    break;
  case CType::tyASCII:
  case CType::tyEBCDIC:
  case CType::tyImage:
    if( representation.Format().IsValid() )
      Arguments.push_back(representation.Format().AsString());
  }

  CReply Reply;
  if( !SendCommand(CCommand::TYPE(), Arguments, Reply) )
    return FTP_ERROR;
  return SimpleErrorCheck(Reply);
}

/// Executes the FTP command CWD (CHANGE WORKING DIRECTORY)
/// This command allows the user to work with a different directory or dataset
/// for file storage or retrieval without altering his login or accounting
/// information. Transfer parameters are similarly unchanged.
/// @param[in] strDirectory Pathname specifying a directory or other system
///                         dependent file group designator.
/// @return see return values of CFTPClient::SimpleErrorCheck
int CFTPClient::ChangeWorkingDirectory(const tstring& strDirectory) const
{
  ASSERT( !strDirectory.empty() );
  CReply Reply;
  if( !SendCommand(CCommand::CWD(), strDirectory, Reply) )
    return FTP_ERROR;
  return SimpleErrorCheck(Reply);
}

/// Executes the FTP command MKD (MAKE DIRECTORY)
/// This command causes the directory specified in the pathname to be created
/// as a directory (if the pathname is absolute) or as a subdirectory of the
/// current working directory (if the pathname is relative).
/// @pararm[in] strDirectory Pathname specifying a directory to be created.
/// @return see return values of CFTPClient::SimpleErrorCheck
int CFTPClient::MakeDirectory(const tstring& strDirectory) const
{
  ASSERT( !strDirectory.empty() );
  CReply Reply;
  if( !SendCommand(CCommand::MKD(), strDirectory, Reply) )
    return FTP_ERROR;
  return SimpleErrorCheck(Reply);
}

/// Executes the FTP command SITE (SITE PARAMETERS)
/// This command is used by the server to provide services specific to his
/// system that are essential to file transfer but not sufficiently universal
/// to be included as commands in the protocol.  The nature of these services
/// and the specification of their syntax can be stated in a reply to the HELP
/// SITE command.
/// @param[in] strCmd Command to be executed.
/// @return see return values of CFTPClient::SimpleErrorCheck
int CFTPClient::SiteParameters(const tstring& strCmd) const
{
  ASSERT( !strCmd.empty() );
  CReply Reply;
  if( !SendCommand(CCommand::SITE(), strCmd, Reply) )
    return FTP_ERROR;
  return SimpleErrorCheck(Reply);
}

/// Executes the FTP command HELP
/// This command shall cause the server to send helpful information regarding
/// its implementation status over the control connection to the user.
/// The command may take an argument (e.g., any command name) and return more
/// specific information as a response.  The reply is type 211 or 214.
/// It is suggested that HELP be allowed before entering a USER command. The
/// server may use this reply to specify site-dependent parameters, e.g., in
/// response to HELP SITE.
/// @param[in] strTopic Topic of the requested help.
/// @return see return values of CFTPClient::SimpleErrorCheck
int CFTPClient::Help(const tstring& strTopic) const
{
  CReply Reply;
  if( !SendCommand(CCommand::HELP(), strTopic, Reply) )
    return FTP_ERROR;
  return SimpleErrorCheck(Reply);
}

/// Executes the FTP command DELE (DELETE)
/// This command causes the file specified in the pathname to be deleted at the
/// server site.  If an extra level of protection is desired (such as the query,
/// "Do you really wish to delete?"), it should be provided by the user-FTP process.
/// @param[in] strFile Pathname of the file to delete.
/// @return see return values of CFTPClient::SimpleErrorCheck
int CFTPClient::Delete(const tstring& strFile) const
{
  ASSERT( !strFile.empty() );
  CReply Reply;
  if( !SendCommand(CCommand::DELE(), strFile, Reply) )
    return FTP_ERROR;
  return SimpleErrorCheck(Reply);
}

/// Executes the FTP command RMD (REMOVE DIRECTORY)
/// This command causes the directory specified in the pathname to be removed
/// as a directory (if the pathname is absolute) or as a subdirectory of the
/// current working directory (if the pathname is relative).
/// @param[in] strDirectory Pathname of the directory to delete.
/// @return see return values of CFTPClient::SimpleErrorCheck
int CFTPClient::RemoveDirectory(const tstring& strDirectory) const
{
  ASSERT( !strDirectory.empty() );
  CReply Reply;
  if( !SendCommand(CCommand::RMD(), strDirectory, Reply) )
    return FTP_ERROR;
  return SimpleErrorCheck(Reply);
}

/// Executes the FTP command STRU (FILE STRUCTURE)
/// see documentation of nsFTP::CStructure
/// The default structure is File.
/// @param[in] crStructure see Documentation of nsFTP::CStructure
/// @return see return values of CFTPClient::SimpleErrorCheck
int CFTPClient::FileStructure(const CStructure& crStructure) const
{
  CReply Reply;
  if( !SendCommand(CCommand::STRU(), crStructure.AsString(), Reply) )
    return FTP_ERROR;
  return SimpleErrorCheck(Reply);
}

/// Executes the FTP command MODE (TRANSFER MODE)
/// see documentation of nsFTP::CTransferMode
/// The default transfer mode is Stream.
/// @param[in] crTransferMode see Documentation of nsFTP::CTransferMode
/// @return see return values of CFTPClient::SimpleErrorCheck
int CFTPClient::TransferMode(const CTransferMode& crTransferMode) const
{
  CReply Reply;
  if( !SendCommand(CCommand::MODE(), crTransferMode.AsString(), Reply) )
    return FTP_ERROR;
  return SimpleErrorCheck(Reply);
}

/// Executes the FTP command STAT (STATUS)
/// This command shall cause a status response to be sent over the control
/// connection in the form of a reply. The command may be sent during a file
/// transfer (along with the Telnet IP and Synch signals--see the Section on
/// FTP Commands) in which case the server will respond with the status of the
/// operation in progress, or it may be sent between file transfers. In the
/// latter case, the command may have an argument field.
/// @param[in] strPath If the argument is a pathname, the command is analogous
///                    to the "list" command except that data shall be transferred
///                    over the control connection. If a partial pathname is
///                    given, the server may respond with a list of file names or
///                    attributes associated with that specification. If no argument
///                    is given, the server should return general status information
///                    about the server FTP process. This should include current
///                    values of all transfer parameters and the status of connections.
/// @return see return values of CFTPClient::SimpleErrorCheck
int CFTPClient::Status(const tstring& strPath) const
{
  CReply Reply;
  if( !SendCommand(CCommand::STAT(), strPath, Reply) )
    return FTP_ERROR;
  return SimpleErrorCheck(Reply);
}

/// Executes the FTP command ALLO (ALLOCATE)
/// This command may be required by some servers to reserve sufficient storage
/// to accommodate the new file to be transferred.
/// @param[in] iReserveBytes The argument shall be a decimal integer representing
///                          the number of bytes (using the logical byte size) of
///                          storage to be reserved for the file. For files sent
///                          with record or page structure a maximum record or page
///                          size (in logical bytes) might also be necessary; this
///                          is indicated by a decimal integer in a second argument
///                          field of the command.
/// @pararm[in] piMaxPageOrRecordSize This second argument is optional. This command
///                          shall be followed by a STORe or APPEnd command.
///                          The ALLO command should be treated as a NOOP (no operation)
///                          by those servers which do not require that the maximum
///                          size of the file be declared beforehand, and those servers
///                          interested in only the maximum record or page size should
///                          accept a dummy value in the first argument and ignore it.
/// @return see return values of CFTPClient::SimpleErrorCheck
int CFTPClient::Allocate(int iReserveBytes, const int* piMaxPageOrRecordSize/*=NULL*/) const
{
  CArg Arguments(CMakeString() << iReserveBytes);
  if( piMaxPageOrRecordSize!=NULL )
  {
    Arguments.push_back(_T("R"));
    Arguments.push_back(CMakeString() << *piMaxPageOrRecordSize);
  }

  CReply Reply;
  if( !SendCommand(CCommand::ALLO(), Arguments, Reply) )
    return FTP_ERROR;
  return SimpleErrorCheck(Reply);
}

/// Executes the FTP command SMNT ()
/// @return see return values of CFTPClient::SimpleErrorCheck
int CFTPClient::StructureMount(const tstring& strPath) const
{
  CReply Reply;
  if( !SendCommand(CCommand::SMNT(), strPath, Reply) )
    return FTP_ERROR;
  return SimpleErrorCheck(Reply);
}

/// Executes the FTP command (STRUCTURE MOUNT)
/// This command allows the user to mount a different file system data structure
/// without altering his login or accounting information. Transfer parameters
/// are similarly unchanged.  The argument is a pathname specifying a directory
/// or other system dependent file group designator.
/// @return see return values of CFTPClient::SimpleErrorCheck
int CFTPClient::Reinitialize() const
{
  CReply Reply;
  if( !SendCommand(CCommand::REIN(), CArg(), Reply) )
    return FTP_ERROR;

  if( Reply.Code().IsPositiveCompletionReply() )
    return FTP_OK;
  else if( Reply.Code().IsPositivePreliminaryReply() )
  {
    if( !GetResponse(Reply) || !Reply.Code().IsPositiveCompletionReply() )
      return FTP_ERROR;
  }
  else if( Reply.Code().IsNegativeReply() )
    return FTP_NOTOK;

  ASSERT( Reply.Code().IsPositiveIntermediateReply() );
  return FTP_ERROR;
}

/// Executes the FTP command REST (RESTART)
/// This command does not cause file transfer but skips over the file to the
/// specified data checkpoint. This command shall be immediately followed
/// by the appropriate FTP service command which shall cause file transfer
/// to resume.
/// @param[in] dwPosition Represents the server marker at which file transfer
///                       is to be restarted.
/// @return see return values of CFTPClient::SimpleErrorCheck
int CFTPClient::Restart(DWORD dwPosition) const
{
  CReply Reply;
  if( !SendCommand(CCommand::REST(), CArg(CMakeString() << dwPosition), Reply) )
    return FTP_ERROR;

  if( Reply.Code().IsPositiveIntermediateReply() )
    return FTP_OK;
  else if( Reply.Code().IsNegativeReply() )
    return FTP_NOTOK;

  ASSERT( Reply.Code().IsPositiveReply() );

  return FTP_ERROR;
}

/// Executes the FTP command SIZE
/// Return size of file.
/// SIZE is not specified in RFC 959.
/// @param[in] Pathname of a file.
/// @param[out] Size of the file specified in pathname.
/// @return see return values of CFTPClient::SimpleErrorCheck
int CFTPClient::FileSize(const tstring& strPath, long& lSize) const
{
  CReply Reply;
  if( !SendCommand(CCommand::SIZE(), strPath, Reply) )
    return FTP_ERROR;
  lSize = CCnv::TStringToLong(Reply.Value().substr(4).c_str());
  return SimpleErrorCheck(Reply);
}

/// Executes the FTP command MDTM
/// Show last modification time of file.
/// MDTM is not specified in RFC 959.
/// @param[in] strPath Pathname of a file.
/// @param[out] strModificationTime Modification time of the file specified in pathname.
/// @return see return values of CFTPClient::SimpleErrorCheck
int CFTPClient::FileModificationTime(const tstring& strPath, tstring& strModificationTime) const
{
  strModificationTime.erase();

  CReply Reply;
  if( !SendCommand(CCommand::MDTM(), strPath, Reply) )
    return FTP_ERROR;

  if( Reply.Value().length()>=18 )
  {
    tstring strTemp(Reply.Value().substr(4));
    size_t iPos=strTemp.find(_T('.'));
    if( iPos!=tstring::npos )
      strTemp = strTemp.substr(0, iPos);
    if( strTemp.length()==14 )
      strModificationTime=strTemp;
  }

  if( strModificationTime.empty() )
    return FTP_ERROR;

  return SimpleErrorCheck(Reply);
}

/// Show last modification time of file.
/// @param[in] strPath Pathname of a file.
/// @param[out] tmModificationTime Modification time of the file specified in pathname.
/// @return see return values of CFTPClient::SimpleErrorCheck
int CFTPClient::FileModificationTime(const tstring& strPath, tm& tmModificationTime) const
{
  tstring strTemp;
  const int iRet = FileModificationTime(strPath, strTemp);

  memset(&tmModificationTime, 0, sizeof(tmModificationTime));
  if( iRet==FTP_OK )
  {
    tmModificationTime.tm_year = CCnv::TStringToLong(strTemp.substr(0, 4).c_str());
    tmModificationTime.tm_mon  = CCnv::TStringToLong(strTemp.substr(4, 2).c_str());
    tmModificationTime.tm_mday = CCnv::TStringToLong(strTemp.substr(6, 2).c_str());
    tmModificationTime.tm_hour = CCnv::TStringToLong(strTemp.substr(8, 2).c_str());
    tmModificationTime.tm_min  = CCnv::TStringToLong(strTemp.substr(10, 2).c_str());
    tmModificationTime.tm_sec  = CCnv::TStringToLong(strTemp.substr(12).c_str());
  }
  return iRet;
}

/// Notifies all observers that an error occurred.
/// @param[in] strErrorMsg Error message which is reported to all observers.
/// @param[in] Name of the sourcecode file where the error occurred.
/// @param[in] Line number in th sourcecode file where the error occurred.
void CFTPClient::ReportError(const tstring& strErrorMsg, const tstring& strFile, DWORD dwLineNr) const
{
  for( TObserverSet::const_iterator it=m_setObserver.begin(); it!=m_setObserver.end(); it++ )
    (*it)->OnInternalError(strErrorMsg, strFile, dwLineNr);
}