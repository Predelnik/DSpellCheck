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

#ifndef INC_FTPFILESTATUS_H
#define INC_FTPFILESTATUS_H

#include <string>
#include <vector>
#include "smart_ptr.h"
#include "Definements.h"

namespace nsFTP
{

class CFTPFileStatus;
typedef nsSmartPointer::shared_ptr<CFTPFileStatus>::type TFTPFileStatusShPtr;
typedef std::vector<TFTPFileStatusShPtr> TFTPFileStatusShPtrVec;

/// Holds status information about a file on the ftp server.
class CFTPFileStatus
{
public:
   enum T_enSizeType {
      stUnknown,
      stBinary,        ///< size is the number of octets in TYPE I
      stASCII,         ///< size is the number of octets in TYPE A
   };

   /// When a time zone is unknown, it is assumed to be GMT. You may want
   /// to use localtime() for LOCAL times, along with an indication that the
   /// time is correct in the local time zone, and gmtime() for REMOTE* times.
   enum T_enMTimeType {
      mttUnknown,
      mttLocal,        ///< time is correct
      mttRemoteMinute, ///< time zone and secs are unknown
      mttRemoteDay,    ///< time zone and time of day are unknown
   };

   enum T_enIDType {
      idUnknown,
      idFull,          ///< unique identifier for files on this FTP server
   };

public:
   CFTPFileStatus();
   CFTPFileStatus(const CFTPFileStatus& src);
   ~CFTPFileStatus();

   CFTPFileStatus& operator=(const CFTPFileStatus& rhs);
   bool operator==(const CFTPFileStatus& rhs) const;
   bool operator!=(const CFTPFileStatus& rhs) const;

public:
   void Reset();

   const tstring& Name()           const { return m_strName;                };
   const tstring& Path()           const { return m_strPath;                };
   bool           IsCwdPossible()  const { return m_fTryCwd;                };
   bool           IsRetrPossible() const { return m_fTryRetr;               };
   T_enSizeType   SizeType()       const { return m_enSizeType;             };
   long           Size()           const { return m_lSize;                  };
   T_enMTimeType  MTimeType()      const { return m_enModificationTimeType; };
   time_t         MTime()          const { return m_mtime;                  };
   const tstring& Attributes()     const { return m_strAttributes;          };
   const tstring& UID()            const { return m_strUID;                 };
   const tstring& GID()            const { return m_strGID;                 };
   const tstring& Link()           const { return m_strLink;                };
   T_enIDType     IDType()         const { return m_enIDType;               };
   const tstring& ID()             const { return m_strID;                  };
   bool           IsDot()          const { return m_strName==_T(".") || m_strName==_T(".."); };
#ifdef _DEBUG
   const tstring& MTimeStr()       const { return m_strMTime;               };
#endif

   tstring&       Name()         { return m_strName;                };
   tstring&       Path()         { return m_strPath;                };
   bool&          CwdPossible()  { return m_fTryCwd;                };
   bool&          RetrPossible() { return m_fTryRetr;               };
   T_enSizeType&  SizeType()     { return m_enSizeType;             };
   long&          Size()         { return m_lSize;                  };
   T_enMTimeType& MTimeType()    { return m_enModificationTimeType; };
   time_t&        MTime()        { return m_mtime;                  };
   tstring&       Attributes()   { return m_strAttributes;          };
   tstring&       UID()          { return m_strUID;                 };
   tstring&       GID()          { return m_strGID;                 };
   tstring&       Link()         { return m_strLink;                };
   T_enIDType&    IDType()       { return m_enIDType;               };
   tstring&       ID()           { return m_strID;                  };
#ifdef _DEBUG
   tstring&       MTimeStr()     { return m_strMTime;               };
#endif

private:
   tstring        m_strName;
   tstring        m_strPath;
   bool           m_fTryCwd;                 ///< false if cwd is definitely pointless, true otherwise
   bool           m_fTryRetr;                ///< false if RETR is definitely pointless, true otherwise 
   T_enSizeType   m_enSizeType;
   long           m_lSize;                   ///< number of octets
   T_enMTimeType  m_enModificationTimeType;
   time_t         m_mtime;                   ///< modification time
   tstring        m_strAttributes;           ///< file attributes
   tstring        m_strUID;                  ///< user identifier (owner of file)
   tstring        m_strGID;                  ///< group identifier
   tstring        m_strLink;                 ///< number of links to file
   T_enIDType     m_enIDType;                ///< identifier type
   tstring        m_strID;                   ///< identifier
#ifdef _DEBUG
   tstring        m_strMTime;                ///< for debug only
#endif
};

/// Function object for sorting the filelists
class CFTPFileStatusContainerSort
{
   /// Baseclass for functionobjects
   class CSortCriteria
   {
   public:
      virtual ~CSortCriteria() {}
      virtual bool Compare(TFTPFileStatusShPtr x, TFTPFileStatusShPtr y) const = 0;
   };

   const CSortCriteria& m_rSortBase;  ///< sortkey
   const bool           m_fAscending; ///< sort order
   const bool           m_fDirFirst;  ///< if true, then directories are before the files
                                      ///< if false, directories and files are mixed up

   CFTPFileStatusContainerSort& operator=(const CFTPFileStatusContainerSort&); // no implementation for assignment operator

public:
   CFTPFileStatusContainerSort(const CSortCriteria& rSortBase, bool fAscending, bool fDirFirst) :
      m_rSortBase(rSortBase),
      m_fAscending(fAscending),
      m_fDirFirst(fDirFirst)
   {}

   /// entry function for STL sort algorithm
   bool operator()(TFTPFileStatusShPtr x, TFTPFileStatusShPtr y) const
   {
      if( m_fDirFirst )
      {
         if( x->Name().size()<3 || y->Name().size()<3 )
         {
            if( x->Name().at(0)==_T('.') && y->Name().at(0)==_T('.') )
               return m_fAscending && x->Name()==_T(".");
            else if( x->Name().at(0)==_T('.') )
               return m_fAscending && true;
            else if( y->Name().at(0)==_T('.') )
               return m_fAscending && false;
         }

         if( x->IsCwdPossible() != y->IsCwdPossible() )
         {
            if( x->IsCwdPossible() && !y->IsCwdPossible() )
               return true;
            return false;
         }
      }

      if( m_fAscending )
         return !m_rSortBase.Compare(x, y);
      else
         return m_rSortBase.Compare(x, y);
   }

   /// Order by name.
   class CName : public CSortCriteria
   {
   public:
      virtual bool Compare(TFTPFileStatusShPtr x, TFTPFileStatusShPtr y) const override
      {
         return x->Name() > y->Name();
      }
   };

   /// Order by size.
   class CSize : public CSortCriteria
   {
   public:
      virtual bool Compare(TFTPFileStatusShPtr x, TFTPFileStatusShPtr y) const override
      {
         return x->Size() > y->Size();
      }
   };

   /// Order by modification date.
   class CMTime : public CSortCriteria
   {
   public:
      virtual bool Compare(TFTPFileStatusShPtr x, TFTPFileStatusShPtr y) const override
      {
         return x->MTime() > y->MTime();
      }
   };

   /// Order by attributes.
   class CAttributes : public CSortCriteria
   {
   public:
      virtual bool Compare(TFTPFileStatusShPtr x, TFTPFileStatusShPtr y) const override
      {
         return x->Attributes() > y->Attributes();
      }
   };

   /// Order by user id.
   class CUID : public CSortCriteria
   {
   public:
      virtual bool Compare(TFTPFileStatusShPtr x, TFTPFileStatusShPtr y) const override
      {
         return x->UID() > y->UID();
      }
   };

   /// Order by group id.
   class CGID : public CSortCriteria
   {
   public:
      virtual bool Compare(TFTPFileStatusShPtr x, TFTPFileStatusShPtr y) const override
      {
         return x->GID() > y->GID();
      }
   };

   /// Order by link.
   class CLink : public CSortCriteria
   {
   public:
      virtual bool Compare(TFTPFileStatusShPtr x, TFTPFileStatusShPtr y) const override
      {
         return x->Link() > y->Link();
      }
   };

   /// Order by id.
   class CID : public CSortCriteria
   {
   public:
      virtual bool Compare(TFTPFileStatusShPtr x, TFTPFileStatusShPtr y) const override
      {
         return x->ID() > y->ID();
      }
   };
};

}
#endif // INC_FTPFILESTATUS_H
