/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Fri May 23 23:23:35 2003
 */
/* Compiler settings for C:\projects\swish\win32\swishctl\SwishCtl.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __SwishCtl_h__
#define __SwishCtl_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __ICSwishCtl_FWD_DEFINED__
#define __ICSwishCtl_FWD_DEFINED__
typedef interface ICSwishCtl ICSwishCtl;
#endif 	/* __ICSwishCtl_FWD_DEFINED__ */


#ifndef __CSwishCtl_FWD_DEFINED__
#define __CSwishCtl_FWD_DEFINED__

#ifdef __cplusplus
typedef class CSwishCtl CSwishCtl;
#else
typedef struct CSwishCtl CSwishCtl;
#endif /* __cplusplus */

#endif 	/* __CSwishCtl_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __ICSwishCtl_INTERFACE_DEFINED__
#define __ICSwishCtl_INTERFACE_DEFINED__

/* interface ICSwishCtl */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICSwishCtl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9E204F2D-4F04-11D7-858F-8CA08FF5860C")
    ICSwishCtl : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Init( 
            /* [in] */ BSTR IndexFiles) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Error( 
            /* [retval][out] */ int __RPC_FAR *errcode) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Query( 
            /* [in] */ BSTR words,
            /* [retval][out] */ int __RPC_FAR *errcode) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ErrorString( 
            /* [retval][out] */ BSTR __RPC_FAR *errstring) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Hits( 
            /* [retval][out] */ int __RPC_FAR *hits) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE NextResult( 
            /* [retval][out] */ int __RPC_FAR *more) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ResultPropertyString( 
            /* [in] */ BSTR propertyname,
            /* [retval][out] */ BSTR __RPC_FAR *result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Close( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SeekResult( 
            /* [in] */ int position,
            /* [retval][out] */ int __RPC_FAR *result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetSort( 
            /* [in] */ BSTR sort) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE NewSearch( 
            /* [in] */ BSTR query,
            /* [retval][out] */ int __RPC_FAR *errcode) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Execute( 
            /* [in] */ BSTR words,
            /* [retval][out] */ int __RPC_FAR *errcode) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetSearchLimit( 
            /* [in] */ BSTR propertyname,
            /* [in] */ BSTR low,
            /* [in] */ BSTR hi) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICSwishCtlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICSwishCtl __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICSwishCtl __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICSwishCtl __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ICSwishCtl __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ICSwishCtl __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ICSwishCtl __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ICSwishCtl __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Init )( 
            ICSwishCtl __RPC_FAR * This,
            /* [in] */ BSTR IndexFiles);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Error )( 
            ICSwishCtl __RPC_FAR * This,
            /* [retval][out] */ int __RPC_FAR *errcode);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Query )( 
            ICSwishCtl __RPC_FAR * This,
            /* [in] */ BSTR words,
            /* [retval][out] */ int __RPC_FAR *errcode);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ErrorString )( 
            ICSwishCtl __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *errstring);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Hits )( 
            ICSwishCtl __RPC_FAR * This,
            /* [retval][out] */ int __RPC_FAR *hits);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *NextResult )( 
            ICSwishCtl __RPC_FAR * This,
            /* [retval][out] */ int __RPC_FAR *more);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ResultPropertyString )( 
            ICSwishCtl __RPC_FAR * This,
            /* [in] */ BSTR propertyname,
            /* [retval][out] */ BSTR __RPC_FAR *result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Close )( 
            ICSwishCtl __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SeekResult )( 
            ICSwishCtl __RPC_FAR * This,
            /* [in] */ int position,
            /* [retval][out] */ int __RPC_FAR *result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetSort )( 
            ICSwishCtl __RPC_FAR * This,
            /* [in] */ BSTR sort);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *NewSearch )( 
            ICSwishCtl __RPC_FAR * This,
            /* [in] */ BSTR query,
            /* [retval][out] */ int __RPC_FAR *errcode);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Execute )( 
            ICSwishCtl __RPC_FAR * This,
            /* [in] */ BSTR words,
            /* [retval][out] */ int __RPC_FAR *errcode);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetSearchLimit )( 
            ICSwishCtl __RPC_FAR * This,
            /* [in] */ BSTR propertyname,
            /* [in] */ BSTR low,
            /* [in] */ BSTR hi);
        
        END_INTERFACE
    } ICSwishCtlVtbl;

    interface ICSwishCtl
    {
        CONST_VTBL struct ICSwishCtlVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICSwishCtl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICSwishCtl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICSwishCtl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICSwishCtl_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICSwishCtl_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICSwishCtl_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICSwishCtl_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICSwishCtl_Init(This,IndexFiles)	\
    (This)->lpVtbl -> Init(This,IndexFiles)

#define ICSwishCtl_Error(This,errcode)	\
    (This)->lpVtbl -> Error(This,errcode)

#define ICSwishCtl_Query(This,words,errcode)	\
    (This)->lpVtbl -> Query(This,words,errcode)

#define ICSwishCtl_ErrorString(This,errstring)	\
    (This)->lpVtbl -> ErrorString(This,errstring)

#define ICSwishCtl_Hits(This,hits)	\
    (This)->lpVtbl -> Hits(This,hits)

#define ICSwishCtl_NextResult(This,more)	\
    (This)->lpVtbl -> NextResult(This,more)

#define ICSwishCtl_ResultPropertyString(This,propertyname,result)	\
    (This)->lpVtbl -> ResultPropertyString(This,propertyname,result)

#define ICSwishCtl_Close(This)	\
    (This)->lpVtbl -> Close(This)

#define ICSwishCtl_SeekResult(This,position,result)	\
    (This)->lpVtbl -> SeekResult(This,position,result)

#define ICSwishCtl_SetSort(This,sort)	\
    (This)->lpVtbl -> SetSort(This,sort)

#define ICSwishCtl_NewSearch(This,query,errcode)	\
    (This)->lpVtbl -> NewSearch(This,query,errcode)

#define ICSwishCtl_Execute(This,words,errcode)	\
    (This)->lpVtbl -> Execute(This,words,errcode)

#define ICSwishCtl_SetSearchLimit(This,propertyname,low,hi)	\
    (This)->lpVtbl -> SetSearchLimit(This,propertyname,low,hi)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICSwishCtl_Init_Proxy( 
    ICSwishCtl __RPC_FAR * This,
    /* [in] */ BSTR IndexFiles);


void __RPC_STUB ICSwishCtl_Init_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICSwishCtl_Error_Proxy( 
    ICSwishCtl __RPC_FAR * This,
    /* [retval][out] */ int __RPC_FAR *errcode);


void __RPC_STUB ICSwishCtl_Error_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICSwishCtl_Query_Proxy( 
    ICSwishCtl __RPC_FAR * This,
    /* [in] */ BSTR words,
    /* [retval][out] */ int __RPC_FAR *errcode);


void __RPC_STUB ICSwishCtl_Query_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICSwishCtl_ErrorString_Proxy( 
    ICSwishCtl __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *errstring);


void __RPC_STUB ICSwishCtl_ErrorString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICSwishCtl_Hits_Proxy( 
    ICSwishCtl __RPC_FAR * This,
    /* [retval][out] */ int __RPC_FAR *hits);


void __RPC_STUB ICSwishCtl_Hits_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICSwishCtl_NextResult_Proxy( 
    ICSwishCtl __RPC_FAR * This,
    /* [retval][out] */ int __RPC_FAR *more);


void __RPC_STUB ICSwishCtl_NextResult_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICSwishCtl_ResultPropertyString_Proxy( 
    ICSwishCtl __RPC_FAR * This,
    /* [in] */ BSTR propertyname,
    /* [retval][out] */ BSTR __RPC_FAR *result);


void __RPC_STUB ICSwishCtl_ResultPropertyString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICSwishCtl_Close_Proxy( 
    ICSwishCtl __RPC_FAR * This);


void __RPC_STUB ICSwishCtl_Close_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICSwishCtl_SeekResult_Proxy( 
    ICSwishCtl __RPC_FAR * This,
    /* [in] */ int position,
    /* [retval][out] */ int __RPC_FAR *result);


void __RPC_STUB ICSwishCtl_SeekResult_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICSwishCtl_SetSort_Proxy( 
    ICSwishCtl __RPC_FAR * This,
    /* [in] */ BSTR sort);


void __RPC_STUB ICSwishCtl_SetSort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICSwishCtl_NewSearch_Proxy( 
    ICSwishCtl __RPC_FAR * This,
    /* [in] */ BSTR query,
    /* [retval][out] */ int __RPC_FAR *errcode);


void __RPC_STUB ICSwishCtl_NewSearch_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICSwishCtl_Execute_Proxy( 
    ICSwishCtl __RPC_FAR * This,
    /* [in] */ BSTR words,
    /* [retval][out] */ int __RPC_FAR *errcode);


void __RPC_STUB ICSwishCtl_Execute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICSwishCtl_SetSearchLimit_Proxy( 
    ICSwishCtl __RPC_FAR * This,
    /* [in] */ BSTR propertyname,
    /* [in] */ BSTR low,
    /* [in] */ BSTR hi);


void __RPC_STUB ICSwishCtl_SetSearchLimit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICSwishCtl_INTERFACE_DEFINED__ */



#ifndef __SWISHCTLLib_LIBRARY_DEFINED__
#define __SWISHCTLLib_LIBRARY_DEFINED__

/* library SWISHCTLLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_SWISHCTLLib;

EXTERN_C const CLSID CLSID_CSwishCtl;

#ifdef __cplusplus

class DECLSPEC_UUID("9E204F2E-4F04-11D7-858F-8CA08FF5860C")
CSwishCtl;
#endif
#endif /* __SWISHCTLLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
