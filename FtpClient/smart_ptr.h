#ifndef INC_SMART_PTR_H
#define INC_SMART_PTR_H
#ifdef USE_STD_SMART_PTR
   #include <memory>
#elif defined(USE_BOOST_SMART_PTR)
   #include <boost/smart_ptr.hpp>
#endif

namespace nsSmartPointer
{
#ifdef USE_STD_SMART_PTR
   template <typename T>
   struct shared_ptr {
      typedef std::shared_ptr<T> type;
   };
#elif defined(USE_BOOST_SMART_PTR)
   template <typename T>
   struct shared_ptr {
      typedef boost::shared_ptr<T> type;
   };
#else
// base class for reference-counted objects
class RCObject
{
public:                                
   void AddReference() { ++m_iRefCount; }
   void RemoveReference()
   {
      if (--m_iRefCount == 0)
         delete this;
   }

   void MarkUnshareable() { m_fShareable = false; }
   bool IsShareable() const { return m_fShareable; }
   bool IsShared() const { return m_iRefCount > 1; }

protected:
   RCObject() : m_iRefCount(0), m_fShareable(true) {}
   RCObject(const RCObject& /*rhs*/) : m_iRefCount(0), m_fShareable(true) {}
   RCObject& operator=(const RCObject& /*rhs*/) { return *this; }

   virtual ~RCObject() {}

private:
   int  m_iRefCount;
   bool m_fShareable;
};

/******************************************************************************
*                 Template Class RCPtr 
******************************************************************************/
// template class for smart pointers-to-T objects; T must support the RCObject interface
template<class T>
class RCPtr
{
public:
   RCPtr(T* realPtr = 0) : m_Pointee(realPtr) { Init(); }
   RCPtr(const RCPtr& rhs) : m_Pointee(rhs.m_Pointee) { Init(); }
   ~RCPtr() { if (m_Pointee) m_Pointee->RemoveReference(); }
   RCPtr& operator=(const RCPtr& rhs)
   {
      if (m_Pointee != rhs.m_Pointee)
      {
         T* pOldPointee = m_Pointee;
         m_Pointee = rhs.m_Pointee;
         Init(); 

         if (pOldPointee)
            pOldPointee->RemoveReference();                
      }

      return *this;
   }
   T* operator->() const { return m_Pointee; };
   T& operator*() const { return *m_Pointee; };

   bool IsNull() const  { return m_Pointee==NULL; }
   bool IsValid() const { return m_Pointee!=NULL; }

private:
   T* m_Pointee;

   void Init()
   {
      if (m_Pointee == 0)
         return;

      if (m_Pointee->IsShareable() == false)
         m_Pointee = new T(*m_Pointee);
   
      m_Pointee->AddReference();
   }
};


template<class T>
class RCIPtr
{
public:
   RCIPtr(T* realPtr = 0) :
      m_pCounter(new CountHolder)
   { 
      m_pCounter->m_Pointee = realPtr;
      Init();
   }

   RCIPtr(const RCIPtr& rhs) : m_pCounter(rhs.m_pCounter) { Init(); }
   ~RCIPtr() { m_pCounter->RemoveReference(); }
   RCIPtr& operator=(const RCIPtr& rhs)
   {
      if (m_pCounter != rhs.m_pCounter)
      {
         m_pCounter->RemoveReference();     
         m_pCounter = rhs.m_pCounter;
         Init();
      }
      return *this;
   }

   T* operator->() const { return m_pCounter->m_Pointee; }
   T& operator*() const { return *(m_pCounter->m_Pointee); }

   RCObject& GetRCObject() { return *m_pCounter; }

   bool IsNull() const  { return m_pCounter==NULL?true:m_pCounter->m_Pointee==NULL; }
   bool IsValid() const { return m_pCounter==NULL?false:m_pCounter->m_Pointee!=NULL; }

private:
   struct CountHolder: public RCObject
   {
      ~CountHolder()
      { 
         delete m_Pointee;
      }
      T* m_Pointee;
   };

   CountHolder* m_pCounter;
   void Init()
   {
      if (m_pCounter->IsShareable() == false)
      {
         T* pOldValue = m_pCounter->m_Pointee;
         m_pCounter = new CountHolder;
         m_pCounter->m_Pointee = new T(*pOldValue);
      } 
      m_pCounter->AddReference();
   }
};
template <typename T>
struct shared_ptr {
   typedef RCIPtr<T> type;
};
#endif

} // nsSmartPointer

#endif // INC_SMART_PTR_H
