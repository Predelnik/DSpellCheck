////////////////////////////////////////////////////////////////////////////////
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
////////////////////////////////////////////////////////////////////////////////

//#include "stdafx.h"
#include "FTPDataTypes.h"
#include "smart_ptr.h"
#include <assert.h>

#ifdef __AFX_H__ // MFC only
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
#endif

using namespace nsFTP;

tstring CStructure::AsString() const
{
   switch( m_enStructure )
   {
   case scFile:   return _T("F");
   case scRecord: return _T("R");
   case scPage:   return _T("P");
   }
   ASSERT( false );
   return _T("");
}

tstring CTransferMode::AsString() const
{
   tstring strMode;
   switch( m_enTransferMode )
   {
   case tmStream:      return _T("S");
   case tmBlock:       return _T("B");
   case tmCompressed:  return _T("C");
   }
   ASSERT( false );
   return _T("");
}

tstring CType::AsString() const
{
   switch( m_enType )
   {
   case tyASCII:     return _T("A");
   case tyEBCDIC:    return _T("E");
   case tyImage:     return _T("I");
   case tyLocalByte: return _T("L");
   }
   ASSERT( false );
   return _T("");
}

tstring CTypeFormat::AsString() const
{
   switch( m_enTypeFormat )
   {
   case tfNonPrint:        return _T("N");
   case tfTelnetFormat:    return _T("T");
   case tfCarriageControl: return _T("C");
   case tfInvalid:         break;
   }
   ASSERT( false );
   return _T("");
}

/// returns the string which is used for display
tstring CFirewallType::AsDisplayString() const
{
   switch( m_enFirewallType )
   {
   case ftNone:                              return _T("no firewall");
   case ftSiteHostName:                      return _T("SITE hostname");
   case ftUserAfterLogon:                    return _T("USER after logon");
   case ftProxyOpen:                         return _T("proxy OPEN");
   case ftTransparent:                       return _T("Transparent");
   case ftUserWithNoLogon:                   return _T("USER with no logon");
   case ftUserFireIDatRemotehost:            return _T("USER fireID@remotehost");
   case ftUserRemoteIDatRemoteHostFireID:    return _T("USER remoteID@remotehost fireID");
   case ftUserRemoteIDatFireIDatRemoteHost:  return _T("USER remoteID@fireID@remotehost");
   }
   ASSERT( false );
   return _T("");
}

/// return the string which is used for storage (e.g. in an XML- or INI-file)
tstring CFirewallType::AsStorageString() const
{
   switch( m_enFirewallType )
   {
   case ftNone:                              return _T("NO_FIREWALL");
   case ftSiteHostName:                      return _T("SITE_HOSTNAME");
   case ftUserAfterLogon:                    return _T("USER_AFTER_LOGON");
   case ftProxyOpen:                         return _T("PROXY_OPEN");
   case ftTransparent:                       return _T("TRANSPARENT");
   case ftUserWithNoLogon:                   return _T("USER_WITH_NO_LOGON");
   case ftUserFireIDatRemotehost:            return _T("USER_FIREID@REMOTEHOST");
   case ftUserRemoteIDatRemoteHostFireID:    return _T("USER_REMOTEID@REMOTEHOST_FIREID");
   case ftUserRemoteIDatFireIDatRemoteHost:  return _T("USER_REMOTEID@FIREID@REMOTEHOST");
   }
   ASSERT( false );
   return _T("");
}

/// returns all available firewall types
void CFirewallType::GetAllTypes(TFirewallTypeVector& vTypes)
{
   vTypes.resize(9);
   vTypes[0] = ftNone;
   vTypes[1] = ftSiteHostName;
   vTypes[2] = ftUserAfterLogon;
   vTypes[3] = ftProxyOpen;
   vTypes[4] = ftTransparent;
   vTypes[5] = ftUserWithNoLogon;
   vTypes[6] = ftUserFireIDatRemotehost;
   vTypes[7] = ftUserRemoteIDatRemoteHostFireID;
   vTypes[8] = ftUserRemoteIDatFireIDatRemoteHost;
}

CLogonInfo::CLogonInfo() :
   m_ushHostport(DEFAULT_FTP_PORT),
   m_strUsername(ANONYMOUS_USER),
   m_ushFwPort(DEFAULT_FTP_PORT),
   m_FwType(CFirewallType::None())
{
}

CLogonInfo::CLogonInfo(const tstring& strHostname, USHORT ushHostport, const tstring& strUsername,
                       const tstring& strPassword, const tstring& strAccount) :
   m_strHostname(strHostname),
   m_ushHostport(ushHostport),
   m_strUsername(strUsername),
   m_strPassword(strPassword),
   m_strAccount(strAccount),
   m_ushFwPort(DEFAULT_FTP_PORT),
   m_FwType(CFirewallType::None())
{
}

CLogonInfo::CLogonInfo(const tstring& strHostname, USHORT ushHostport, const tstring& strUsername, const tstring& strPassword,
                       const tstring& strAccount, const tstring& strFwHostname, const tstring& strFwUsername,
                       const tstring& strFwPassword, USHORT ushFwPort, const CFirewallType& crFwType) :
   m_strHostname(strHostname),
   m_ushHostport(ushHostport),
   m_strUsername(strUsername),
   m_strPassword(strPassword),
   m_strAccount(strAccount),
   m_strFwHostname(strFwHostname),
   m_strFwUsername(strFwUsername),
   m_strFwPassword(strFwPassword),
   m_ushFwPort(ushFwPort),
   m_FwType(crFwType)
{
}

void CLogonInfo::SetHost(const tstring& strHostname, USHORT ushHostport, const tstring& strUsername,
                         const tstring& strPassword, const tstring& strAccount)
{
   m_strHostname  = strHostname;
   m_ushHostport  = ushHostport;
   m_strUsername  = strUsername;
   m_strPassword  = strPassword;
   m_strAccount   = strAccount;
}

void CLogonInfo::SetFirewall(const tstring& strFwHostname, const tstring& strFwUsername, const tstring& strFwPassword,
                             USHORT ushFwPort, const CFirewallType& crFwType)
{
   m_strFwHostname   = strFwHostname;
   m_strFwUsername   = strFwUsername;
   m_strFwPassword   = strFwPassword;
   m_ushFwPort       = ushFwPort;
   m_FwType          = crFwType;
}

class CCommand::CExtendedInfo : public CCommand::IExtendedInfo
{
   typedef CCommand::TSpecificationEnum TSpecificationEnum;
   typedef CCommand::TTypeEnum TTypeEnum;
public:
   CExtendedInfo(const tstring& strServerString, const tstring& strCompleteServerStringSyntax, UINT uiNumberOfParameters,
                 UINT uiNumberOfOptionalParameters, TSpecificationEnum enSpecification, TTypeEnum enType) :
      m_strServerString(strServerString),
      m_strCompleteServerStringSyntax(strCompleteServerStringSyntax),
      m_uiNumberOfParameters(uiNumberOfParameters),
      m_uiNumberOfOptionalParameters(uiNumberOfOptionalParameters),
      m_enSpecification(enSpecification),
      m_enType(enType)
   {}

   CExtendedInfo(const CExtendedInfo& src) :
      m_strServerString(src.m_strServerString),
      m_strCompleteServerStringSyntax(src.m_strCompleteServerStringSyntax),
      m_uiNumberOfParameters(src.m_uiNumberOfParameters),
      m_uiNumberOfOptionalParameters(src.m_uiNumberOfOptionalParameters),
      m_enSpecification(src.m_enSpecification),
      m_enType(src.m_enType)
   {
   }

   virtual const tstring& GetServerString() const override { return m_strServerString; }
   virtual const tstring& GetCompleteServerStringSyntax() const override { return m_strCompleteServerStringSyntax; }
   virtual UINT GetNumberOfParameters() const override { return m_uiNumberOfParameters; }
   virtual UINT GetNumberOfOptionalParameters() const override { return m_uiNumberOfOptionalParameters; }
   virtual TSpecificationEnum GetSpecification() const override  { return m_enSpecification; }
   virtual TTypeEnum GetType() const override { return m_enType; }

   const tstring            m_strServerString;
   const tstring            m_strCompleteServerStringSyntax;
   const UINT               m_uiNumberOfParameters;
   const UINT               m_uiNumberOfOptionalParameters;
   const TSpecificationEnum m_enSpecification;
   const TTypeEnum          m_enType;
};

class CCommand::CCmd2Info : private std::map<TCommandEnum, nsSmartPointer::shared_ptr<CExtendedInfo>::type >
{
   CCmd2Info();
   void Insert(TCommandEnum enCommand, CExtendedInfo* pExtendedInfo) { insert(std::make_pair(enCommand, nsSmartPointer::shared_ptr<CExtendedInfo>::type(pExtendedInfo))); }
   void Insert(TCommandEnum enCommand, const tstring& strServerString, const tstring& strCompleteServerStringSyntax, UINT uiNumberOfParameters,
               UINT uiNumberOfOptionalParameters, TSpecificationEnum enSpecification, TTypeEnum enType)
   {
      insert(std::make_pair(enCommand, nsSmartPointer::shared_ptr<CExtendedInfo>::type(new CExtendedInfo(strServerString, strCompleteServerStringSyntax, uiNumberOfParameters,
                                                                                                         uiNumberOfOptionalParameters, enSpecification, enType))));
   }

   static CCmd2Info& GetInstance() { static CCmd2Info TheOneAndOnly; return TheOneAndOnly; }
public:
   static const IExtendedInfo& Get(TCommandEnum enCommand);
};

CCommand::CCmd2Info::CCmd2Info()
{
   Insert(cmdABOR, _T("ABOR"), _T("ABOR <CRLF>"),                                                        0, 0, RFC959,   NonDatachannel);
   Insert(cmdACCT, _T("ACCT"), _T("ACCT <SP> <account-information> <CRLF>"),                             1, 0, RFC959,   NonDatachannel);
   Insert(cmdALLO, _T("ALLO"), _T("ALLO <SP> <decimal-integer> [<SP> R <SP> <decimal-integer>] <CRLF>"), 3, 2, RFC959,   NonDatachannel);
   Insert(cmdAPPE, _T("APPE"), _T("APPE <SP> <pathname> <CRLF>"),                                        1, 0, RFC959,   DatachannelWrite);
   Insert(cmdCDUP, _T("CDUP"), _T("CDUP <CRLF>"),                                                        0, 0, RFC959,   NonDatachannel);
   Insert(cmdCWD,  _T("CWD"),  _T("CWD <SP> <pathname> <CRLF>"),                                         1, 0, RFC959,   NonDatachannel);
   Insert(cmdDELE, _T("DELE"), _T("DELE <SP> <pathname> <CRLF>"),                                        1, 0, RFC959,   NonDatachannel);
   Insert(cmdHELP, _T("HELP"), _T("HELP [<SP> <string>] <CRLF>"),                                        1, 1, RFC959,   NonDatachannel);
   Insert(cmdLIST, _T("LIST"), _T("LIST [<SP> <pathname>] <CRLF>"),                                      1, 1, RFC959,   DatachannelRead);
   Insert(cmdMDTM, _T("MDTM"), _T("MDTM <SP> <pathname> <CRLF>"),                                        1, 0, RFC3659,  NonDatachannel);
   Insert(cmdMKD,  _T("MKD"),  _T("MKD <SP> <pathname> <CRLF>"),                                         1, 0, RFC959,   NonDatachannel);
   Insert(cmdMODE, _T("MODE"), _T("MODE <SP> <mode-code> <CRLF>"),                                       1, 0, RFC959,   NonDatachannel);
   Insert(cmdNLST, _T("NLST"), _T("NLST [<SP> <pathname>] <CRLF>"),                                      1, 1, RFC959,   DatachannelRead);
   Insert(cmdNOOP, _T("NOOP"), _T("NOOP <CRLF>"),                                                        0, 0, RFC959,   NonDatachannel);
   Insert(cmdOPEN, _T("OPEN"), _T("OPEN <SP> <string> <CRLF>"),                                          1, 0, Unknown,  NonDatachannel);
   Insert(cmdPASS, _T("PASS"), _T("PASS <SP> <password> <CRLF>"),                                        1, 0, RFC959,   NonDatachannel);
   Insert(cmdPASV, _T("PASV"), _T("PASV <CRLF>"),                                                        0, 0, RFC959,   NonDatachannel);
   Insert(cmdPORT, _T("PORT"), _T("PORT <SP> <host-port> <CRLF>"),                                       1, 0, RFC959,   NonDatachannel);
   Insert(cmdPWD,  _T("PWD"),  _T("PWD <CRLF>"),                                                         0, 0, RFC959,   NonDatachannel);
   Insert(cmdQUIT, _T("QUIT"), _T("QUIT <CRLF>"),                                                        0, 0, RFC959,   NonDatachannel);
   Insert(cmdREIN, _T("REIN"), _T("REIN <CRLF>"),                                                        0, 0, RFC959,   NonDatachannel);
   Insert(cmdREST, _T("REST"), _T("REST <SP> <marker> <CRLF>"),                                          1, 0, RFC959,   NonDatachannel);
   Insert(cmdRETR, _T("RETR"), _T("RETR <SP> <pathname> <CRLF>"),                                        1, 0, RFC959,   DatachannelRead);
   Insert(cmdRMD,  _T("RMD"),  _T("RMD <SP> <pathname> <CRLF>"),                                         1, 0, RFC959,   NonDatachannel);
   Insert(cmdRNFR, _T("RNFR"), _T("RNFR <SP> <pathname> <CRLF>"),                                        1, 0, RFC959,   NonDatachannel);
   Insert(cmdRNTO, _T("RNTO"), _T("RNTO <SP> <pathname> <CRLF>"),                                        1, 0, RFC959,   NonDatachannel);
   Insert(cmdSITE, _T("SITE"), _T("SITE <SP> <string> <CRLF>"),                                          1, 0, RFC959,   NonDatachannel);
   Insert(cmdSIZE, _T("SIZE"), _T("SIZE <SP> <pathname> <CRLF>"),                                        1, 0, RFC3659,  NonDatachannel);
   Insert(cmdSMNT, _T("SMNT"), _T("SMNT <SP> <pathname> <CRLF>"),                                        1, 0, RFC959,   NonDatachannel);
   Insert(cmdSTAT, _T("STAT"), _T("STAT [<SP> <pathname>] <CRLF>"),                                      1, 1, RFC959,   NonDatachannel);
   Insert(cmdSTOR, _T("STOR"), _T("STOR <SP> <pathname> <CRLF>"),                                        1, 0, RFC959,   DatachannelWrite);
   Insert(cmdSTOU, _T("STOU"), _T("STOU <CRLF>"),                                                        0, 0, RFC959,   DatachannelWrite);
   Insert(cmdSTRU, _T("STRU"), _T("STRU <SP> <structure-code> <CRLF>"),                                  1, 0, RFC959,   NonDatachannel);
   Insert(cmdSYST, _T("SYST"), _T("SYST <CRLF>"),                                                        0, 0, RFC959,   NonDatachannel);
   Insert(cmdTYPE, _T("TYPE"), _T("TYPE <SP> <type-code> <CRLF>"),                                       1, 0, RFC959,   NonDatachannel);
   Insert(cmdUSER, _T("USER"), _T("USER <SP> <username> <CRLF>"),                                        1, 0, RFC959,   NonDatachannel);
}

const CCommand::IExtendedInfo& CCommand::CCmd2Info::Get(TCommandEnum enCommand)
{
   const_iterator it = GetInstance().find(enCommand);
   ASSERT( it!=GetInstance().end() );
   return *it->second;
}

bool CCommand::IsDatachannelReadCommand() const
{
   return CCmd2Info::Get(m_enCommand).GetType()==DatachannelRead;
}

bool CCommand::IsDatachannelWriteCommand() const
{
   return CCmd2Info::Get(m_enCommand).GetType()==DatachannelWrite;
}

bool CCommand::IsDatachannelCommand() const
{
   return IsDatachannelReadCommand() || IsDatachannelWriteCommand();
}

tstring CCommand::AsString() const
{
   return CCmd2Info::Get(m_enCommand).GetServerString();
}

/// Returns the command string.
/// @param[in] strArgument Parameter which have to be added to the command.
tstring CCommand::AsString(const CArg& Arguments) const
{
   if( Arguments.empty() )
      return AsString();

   tstring strArgument;
   for( CArg::const_iterator itArg=Arguments.begin(); itArg!=Arguments.end(); ++itArg )
   {
      if( !itArg->empty() )
         strArgument += _T(" ") + *itArg;
   }

   return AsString() + strArgument;
}

const CCommand::IExtendedInfo& CCommand::GetExtendedInfo() const
{
   return CCmd2Info::Get(m_enCommand);
}
