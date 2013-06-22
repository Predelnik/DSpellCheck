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

#ifndef INC_FTPDATATYPES_H
#define INC_FTPDATATYPES_H

#include <vector>
#include <map>
#include "Definements.h"
#include "time.h" // for clock()

namespace nsFTP
{
   typedef std::vector<char> TByteVector;
  
   class IInterface
   {
   protected:
      IInterface() {}
      virtual ~IInterface() {}
   
   private: // prevent copy action
      IInterface(const IInterface&);
      const IInterface& operator=(const IInterface&);
   };

   // constants
   const TCHAR ANONYMOUS_USER[] = _T("anonymous");
   enum T_enConstants {
      DEFAULT_FTP_PORT = 21, ///< The default port that an FTP service listens to on a remote host
      FTP_ERROR  = -1,
      FTP_OK     =  0,
      FTP_NOTOK  =  1, };

   /// Data Structure
   class CStructure
   {
   public:
      enum TFileStructureEnum { scFile, scRecord, scPage };

      CStructure(const CStructure& structure) :
         m_enStructure(structure.AsEnum()) {}

      bool operator==(const TFileStructureEnum& rhs) const { return m_enStructure==rhs; }
      bool operator!=(const TFileStructureEnum& rhs) const { return !operator==(rhs); }
      bool operator==(const CStructure& rhs) const { return m_enStructure==rhs.m_enStructure; }
      bool operator!=(const CStructure& rhs) const { return !operator==(rhs); }

      CStructure& operator=(const CStructure& rhs) { m_enStructure = rhs.AsEnum(); return *this; }

      TFileStructureEnum AsEnum() const { return m_enStructure; }
      tstring AsString() const;

      static const CStructure File()   { return scFile;   }
      static const CStructure Record() { return scRecord; }
      static const CStructure Page()   { return scPage;   }

   private:
      CStructure(TFileStructureEnum enStructure) : m_enStructure(enStructure) {}
      TFileStructureEnum m_enStructure;
   };

   /// Transmission Modes
   class CTransferMode
   {
   public:
      enum TTransferModeEnum { tmStream, tmBlock, tmCompressed };

      CTransferMode(const CTransferMode& transferMode) :
         m_enTransferMode(transferMode.AsEnum()) {}

      bool operator==(const TTransferModeEnum& rhs) const { return m_enTransferMode==rhs; }
      bool operator!=(const TTransferModeEnum& rhs) const { return !operator==(rhs); }
      bool operator==(const CTransferMode& rhs) const { return m_enTransferMode==rhs.m_enTransferMode; }
      bool operator!=(const CTransferMode& rhs) const { return !operator==(rhs); }

      CTransferMode& operator=(const CTransferMode& rhs) { m_enTransferMode = rhs.AsEnum(); return *this; }

      TTransferModeEnum AsEnum() const { return m_enTransferMode; }
      tstring AsString() const;

      static const CTransferMode Stream()     { return tmStream;     }
      static const CTransferMode Block()      { return tmBlock;      }
      static const CTransferMode Compressed() { return tmCompressed; }

   private:
      CTransferMode(TTransferModeEnum enTransferMode) : m_enTransferMode(enTransferMode) {}
      TTransferModeEnum m_enTransferMode;
   };

   class CFirewallType;
   typedef std::vector<CFirewallType> TFirewallTypeVector;

   /// Firewall Type
   class CFirewallType
   {
   public:
      // don't change order of enumeration
      enum TFirewallTypeEnum {
         ftNone, ftSiteHostName, ftUserAfterLogon, ftProxyOpen, ftTransparent,
         ftUserWithNoLogon, ftUserFireIDatRemotehost, ftUserRemoteIDatRemoteHostFireID, 
         ftUserRemoteIDatFireIDatRemoteHost };

      CFirewallType() : m_enFirewallType(ftNone) {}
      CFirewallType(const CFirewallType& firewallType) :
         m_enFirewallType(firewallType.AsEnum()) {}

      bool operator==(const TFirewallTypeEnum& rhs) const { return m_enFirewallType==rhs; }
      bool operator!=(const TFirewallTypeEnum& rhs) const { return !operator==(rhs); }
      bool operator==(const CFirewallType& rhs) const { return m_enFirewallType==rhs.m_enFirewallType; }
      bool operator!=(const CFirewallType& rhs) const { return !operator==(rhs); }

      CFirewallType& operator=(const CFirewallType& rhs) { m_enFirewallType = rhs.AsEnum(); return *this; }

      TFirewallTypeEnum AsEnum() const { return m_enFirewallType; }

      tstring AsDisplayString() const;
      tstring AsStorageString() const;
      static void GetAllTypes(TFirewallTypeVector& vTypes);

      static const CFirewallType None()                             { return ftNone;                             }
      static const CFirewallType SiteHostName()                     { return ftSiteHostName;                     }
      static const CFirewallType UserAfterLogon()                   { return ftUserAfterLogon;                   }
      static const CFirewallType ProxyOpen()                        { return ftProxyOpen;                        }
      static const CFirewallType Transparent()                      { return ftTransparent;                      }
      static const CFirewallType UserWithNoLogon()                  { return ftUserWithNoLogon;                  }
      static const CFirewallType UserFireIDatRemotehost()           { return ftUserFireIDatRemotehost;           }
      static const CFirewallType UserRemoteIDatRemoteHostFireID()   { return ftUserRemoteIDatRemoteHostFireID;   }
      static const CFirewallType UserRemoteIDatFireIDatRemoteHost() { return ftUserRemoteIDatFireIDatRemoteHost; }

   private:
      CFirewallType(TFirewallTypeEnum enFirewallType) : m_enFirewallType(enFirewallType) {}
      TFirewallTypeEnum m_enFirewallType;
   };

   /// @brief Representation Type - 1st param (see CRepresentation)
   class CType
   {
   public:
      enum TTypeEnum { tyASCII, tyEBCDIC, tyImage, tyLocalByte };

      CType(const CType& type) :
         m_enType(type.AsEnum()) {}

      bool operator==(const TTypeEnum& rhs) const { return m_enType==rhs; }
      bool operator!=(const TTypeEnum& rhs) const { return !operator==(rhs); }
      bool operator==(const CType& rhs) const { return m_enType==rhs.m_enType; }
      bool operator!=(const CType& rhs) const { return !operator==(rhs); }

      CType& operator=(const CType& rhs) { m_enType = rhs.AsEnum(); return *this; }

      TTypeEnum AsEnum() const { return m_enType; }
      tstring AsString() const;

      static const CType ASCII()     { return tyASCII;     }
      static const CType EBCDIC()    { return tyEBCDIC;    }
      static const CType Image()     { return tyImage;     }
      static const CType LocalByte() { return tyLocalByte; }

   private:
      CType(TTypeEnum enType) : m_enType(enType) {}
      TTypeEnum m_enType;
   };

   /// @brief Representation Type - 2nd param (see CRepresentation)
   class CTypeFormat
   {
   public:
      enum TTypeFormatEnum { tfInvalid, tfNonPrint, tfTelnetFormat, tfCarriageControl };

      CTypeFormat() : m_enTypeFormat(tfInvalid) {}
      CTypeFormat(const CTypeFormat& typeFormat) :
         m_enTypeFormat(typeFormat.AsEnum()) {}

      bool operator==(const TTypeFormatEnum& rhs) const { return m_enTypeFormat==rhs; }
      bool operator!=(const TTypeFormatEnum& rhs) const { return !operator==(rhs); }
      bool operator==(const CTypeFormat& rhs) const { return m_enTypeFormat==rhs.m_enTypeFormat; }
      bool operator!=(const CTypeFormat& rhs) const { return !operator==(rhs); }

      CTypeFormat& operator=(const CTypeFormat& rhs) { m_enTypeFormat = rhs.AsEnum(); return *this; }

      TTypeFormatEnum AsEnum() const { return m_enTypeFormat; }
      tstring AsString() const;
      bool IsValid() const { return m_enTypeFormat != tfInvalid; }

      static const CTypeFormat NonPrint()        { return tfNonPrint;        }
      static const CTypeFormat TelnetFormat()    { return tfTelnetFormat;    }
      static const CTypeFormat CarriageControl() { return tfCarriageControl; }

   private:
      CTypeFormat(TTypeFormatEnum enTypeFormat) : m_enTypeFormat(enTypeFormat) {}
      TTypeFormatEnum m_enTypeFormat;
   };

   /// Representation Type (see also CType and CTypeFormat)
   class CRepresentation
   {
   public:
      CRepresentation(CType Type) : m_Type(Type) {}
      CRepresentation(CType Type, CTypeFormat Format) : m_Type(Type), m_Format(Format) {}

      bool operator==(const CRepresentation& rhs) const { return rhs.m_Type == m_Type && rhs.m_Format == m_Format; }
      bool operator!=(const CRepresentation& rhs) const { return !operator==(rhs); }
      CRepresentation& operator=(const CRepresentation& rhs)
      { 
         m_Type = rhs.m_Type;
         m_Format = rhs.m_Format;
         return *this;
      }

      const CType&       Type()   const { return m_Type; }
      const CTypeFormat& Format() const { return m_Format; }

   private:
      CType       m_Type;
      CTypeFormat m_Format;
   };

   class CArg : public std::vector<tstring>
   {
   public:
      CArg() {}
      CArg(const tstring& strArgument) { push_back(strArgument); }
      CArg(const tstring& strFirstArgument, const tstring& strSecondArgument) { push_back(strFirstArgument); push_back(strSecondArgument); }
      CArg(const tstring& strFirstArgument, const tstring& strSecondArgument, const tstring& strThirdArgument) { push_back(strFirstArgument); push_back(strSecondArgument); push_back(strThirdArgument); }
   };

   class CCommand
   {
   public:
      enum TCommandEnum { cmdABOR, cmdACCT, cmdALLO, cmdAPPE, cmdCDUP, cmdCWD, cmdDELE, cmdHELP, cmdLIST, cmdMDTM, cmdMKD, cmdMODE, cmdNLST, cmdNOOP, cmdOPEN, cmdPASS, cmdPASV, cmdPORT, cmdPWD, cmdQUIT, cmdREIN, cmdREST, cmdRETR, cmdRMD, cmdRNFR, cmdRNTO, cmdSITE, cmdSIZE, cmdSMNT, cmdSTAT, cmdSTOR, cmdSTOU, cmdSTRU, cmdSYST, cmdTYPE, cmdUSER, };
      enum TSpecificationEnum { Unknown, RFC959, RFC3659, };
      enum TTypeEnum { DatachannelRead, DatachannelWrite, NonDatachannel, };

      class IExtendedInfo : public IInterface
      {
      public:
         virtual ~IExtendedInfo() {}
         virtual const tstring& GetServerString() const = 0;
         virtual const tstring& GetCompleteServerStringSyntax() const = 0;
         virtual UINT GetNumberOfParameters() const = 0;
         virtual UINT GetNumberOfOptionalParameters() const = 0;
         virtual TSpecificationEnum GetSpecification() const = 0;
         virtual TTypeEnum GetType() const = 0;
      };

      CCommand(const CCommand& datachannelCmd) :
         m_enCommand(datachannelCmd.AsEnum()) {}

      bool operator==(TCommandEnum rhs) const { return m_enCommand==rhs; }
      bool operator!=(TCommandEnum rhs) const { return !operator==(rhs); }

      bool operator==(const CCommand& rhs) const { return m_enCommand==rhs.m_enCommand; }
      bool operator!=(const CCommand& rhs) const { return !operator==(rhs); }

      CCommand& operator=(const CCommand& rhs)
      {
         m_enCommand = rhs.AsEnum();
         return *this;
      }

      TCommandEnum AsEnum() const { return m_enCommand; }
      tstring AsString() const;
      tstring AsString(const CArg& Arguments) const;
      const IExtendedInfo& GetExtendedInfo() const;

      bool IsDatachannelReadCommand() const;
      bool IsDatachannelWriteCommand() const;
      bool IsDatachannelCommand() const;

      static const CCommand ABOR() { return cmdABOR; }
      static const CCommand ACCT() { return cmdACCT; }
      static const CCommand ALLO() { return cmdALLO; }
      static const CCommand APPE() { return cmdAPPE; }
      static const CCommand CDUP() { return cmdCDUP; }
      static const CCommand CWD()  { return cmdCWD;  }
      static const CCommand DELE() { return cmdDELE; }
      static const CCommand HELP() { return cmdHELP; }
      static const CCommand LIST() { return cmdLIST; }
      static const CCommand MDTM() { return cmdMDTM; }
      static const CCommand MKD()  { return cmdMKD;  }
      static const CCommand MODE() { return cmdMODE; }
      static const CCommand NLST() { return cmdNLST; }
      static const CCommand NOOP() { return cmdNOOP; }
      static const CCommand OPEN() { return cmdOPEN; }
      static const CCommand PASS() { return cmdPASS; }
      static const CCommand PASV() { return cmdPASV; }
      static const CCommand PORT() { return cmdPORT; }
      static const CCommand PWD()  { return cmdPWD;  }
      static const CCommand QUIT() { return cmdQUIT; }
      static const CCommand REIN() { return cmdREIN; }
      static const CCommand REST() { return cmdREST; }
      static const CCommand RETR() { return cmdRETR; }
      static const CCommand RMD()  { return cmdRMD;  }
      static const CCommand RNFR() { return cmdRNFR; }
      static const CCommand RNTO() { return cmdRNTO; }
      static const CCommand SITE() { return cmdSITE; }
      static const CCommand SIZE() { return cmdSIZE; }
      static const CCommand SMNT() { return cmdSMNT; }
      static const CCommand STAT() { return cmdSTAT; }
      static const CCommand STOR() { return cmdSTOR; }
      static const CCommand STOU() { return cmdSTOU; }
      static const CCommand STRU() { return cmdSTRU; }
      static const CCommand SYST() { return cmdSYST; }
      static const CCommand TYPE() { return cmdTYPE; }
      static const CCommand USER() { return cmdUSER; }

   private:
      class CExtendedInfo;
      class CCmd2Info;
      CCommand(TCommandEnum enDatachannelCmd) : m_enCommand(enDatachannelCmd) {}
      TCommandEnum m_enCommand;
   };

   /// @brief Structure for logon information.
   ///
   /// Holds all necessary parameters for logging on a ftp-server.
   /// Includes also the parameters which are needed for firewall logon.
   class CLogonInfo
   {
   public:
      CLogonInfo();
      CLogonInfo(const tstring& strHostname, USHORT ushHostport=DEFAULT_FTP_PORT, const tstring& strUsername=ANONYMOUS_USER, 
                 const tstring& strPassword=_T("anonymous@user.com"), const tstring& strAccount=_T(""));
      CLogonInfo(const tstring& strHostname, USHORT ushHostport, const tstring& strUsername, const tstring& strPassword,
                 const tstring& strAccount, const tstring& strFwHostname, const tstring& strFwUsername, const tstring& strFwPassword,
                 USHORT ushFwPort, const CFirewallType& crFwType);

      void SetHost(const tstring& strHostname, USHORT ushHostport=DEFAULT_FTP_PORT, const tstring& strUsername=ANONYMOUS_USER, 
                   const tstring& strPassword=_T("anonymous@user.com"), const tstring& strAccount=_T(""));

      void SetFirewall(const tstring& strFwHostname, const tstring& strFwUsername, const tstring& strFwPassword,
                       USHORT ushFwPort, const CFirewallType& crFwType);

      void DisableFirewall() { m_FwType = CFirewallType::None(); }

      const tstring&       Hostname()   const  { return m_strHostname;    }
      USHORT               Hostport()   const  { return m_ushHostport;    }
      const tstring&       Username()   const  { return m_strUsername;    }
      const tstring&       Password()   const  { return m_strPassword;    }
      const tstring&       Account()    const  { return m_strAccount;     }
      const tstring&       FwHost()     const  { return m_strFwHostname;  }
      const tstring&       FwUsername() const  { return m_strFwUsername;  }
      const tstring&       FwPassword() const  { return m_strFwPassword;  }
      USHORT               FwPort()     const  { return m_ushFwPort;      }
      const CFirewallType& FwType()     const  { return m_FwType;         }
   
   private:
      tstring        m_strHostname;   ///< name or ip-address of the ftp-server
      USHORT         m_ushHostport;   ///< port of the ftp-server
      tstring        m_strUsername;   ///< username for ftp-server
      tstring        m_strPassword;   ///< password for ftp-server
      tstring        m_strAccount;    ///< account mostly needed on ftp-servers running on unix/linux
      tstring        m_strFwHostname; ///< name or ip-address of the firewall
      tstring        m_strFwUsername; ///< username for firewall
      tstring        m_strFwPassword; ///< password for firewall
      USHORT         m_ushFwPort;     ///< port of the firewall
      CFirewallType  m_FwType;        ///< type of firewall
   };

   /// Holds a response of a ftp-server.
   class CReply
   {
      tstring m_strResponse;

      /// Holds the reply code.
      class CCode
      {
         TCHAR m_szCode[4];
      public:
         CCode()
         {
            std::fill_n(m_szCode, sizeof(m_szCode)/sizeof(TCHAR), 0);
         }
         LPCTSTR Value() const { return m_szCode; }
         bool Set(const tstring& strCode)
         {
            if( strCode.length()!=3 ||
                strCode[0]<_T('1') || strCode[0]>_T('5') ||
                strCode[1]<_T('0') || strCode[1]>_T('5') )
            {
               std::fill_n(m_szCode, sizeof(m_szCode)/sizeof(TCHAR), 0);
               return false;
            }
            std::copy(strCode.begin(), strCode.end(), m_szCode);
            return true;
         }

         bool IsPositiveReply() const { return IsPositivePreliminaryReply() || IsPositiveCompletionReply() || IsPositiveIntermediateReply(); }
         bool IsNegativeReply() const { return IsTransientNegativeCompletionReply() || IsPermanentNegativeCompletionReply(); }

         bool IsPositivePreliminaryReply() const         { return m_szCode[0] == _T('1'); }
         bool IsPositiveCompletionReply() const          { return m_szCode[0] == _T('2'); }
         bool IsPositiveIntermediateReply() const        { return m_szCode[0] == _T('3'); }
         bool IsTransientNegativeCompletionReply() const { return m_szCode[0] == _T('4'); }
         bool IsPermanentNegativeCompletionReply() const { return m_szCode[0] == _T('5'); }

         bool IsRefferingToSyntax() const                      { return m_szCode[1] == _T('0'); }
         bool IsRefferingToInformation() const                 { return m_szCode[1] == _T('1'); }
         bool IsRefferingToConnections() const                 { return m_szCode[1] == _T('2'); }
         bool IsRefferingToAuthenticationAndAccounting() const { return m_szCode[1] == _T('3'); }
         bool IsRefferingToUnspecified() const                 { return m_szCode[1] == _T('4'); }
         bool IsRefferingToFileSystem() const                  { return m_szCode[1] == _T('5'); }
      } m_Code;
   public:
      bool Set(const tstring& strResponse)
      {
         m_strResponse = strResponse;
         if( m_strResponse.length()>2 )
            return m_Code.Set(m_strResponse.substr(0, 3));
         return false;
      }
      const tstring& Value() const { return m_strResponse; }
      const CCode& Code() const { return m_Code; }
   };
}

#endif // INC_FTPDATATYPES_H
