// CSwishCtl.h : Declaration of the CCSwishCtl

#ifndef __CSWISHCTL_H_
#define __CSWISHCTL_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CCSwishCtl
class ATL_NO_VTABLE CCSwishCtl : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CCSwishCtl, &CLSID_CSwishCtl>,
	public IDispatchImpl<ICSwishCtl, &IID_ICSwishCtl, &LIBID_SWISHCTLLib>,
	public IObjectSafetyImpl<CCSwishCtl, INTERFACESAFE_FOR_UNTRUSTED_CALLER || INTERFACESAFE_FOR_UNTRUSTED_DATA> // ATL's version of IObjectSafety
{
public:
	CCSwishCtl();
 	~CCSwishCtl();



DECLARE_REGISTRY_RESOURCEID(IDR_CSWISHCTL)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CCSwishCtl)
	COM_INTERFACE_ENTRY(ICSwishCtl)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IObjectSafety) // Tie IObjectSafety to this COM map
END_COM_MAP()

// ICSwishCtl
public:
	STDMETHOD(SetSearchLimit)(/*[in]*/ BSTR propertyname, /*[in]*/ BSTR low, /*[in]*/ BSTR hi);
	STDMETHOD(Execute)(/*[in]*/ BSTR words, /*[out,retval]*/ int *errcode);
	STDMETHOD(NewSearch)(/*[in]*/ BSTR query, /*[out,retval]*/ int *errcode);
	STDMETHOD(SetSort)(/*[in]*/ BSTR sort);
	STDMETHOD(SeekResult)(/*[in]*/ int position, /*[out,retval]*/ int *result);
	STDMETHOD(Close)();
	STDMETHOD(ResultPropertyString)(/*[in]*/ BSTR propertyname, /*[out,retval]*/ BSTR *result);
	STDMETHOD(NextResult)(/*[out,retval]*/ int *more);
	STDMETHOD(Hits)(/*[out,retval]*/ int *hits);
	STDMETHOD(ErrorString)(/*[out,retval]*/ BSTR *errstring);
	STDMETHOD(Query)(/*[in]*/ BSTR words, /*[out,retval]*/ int *errcode);
	STDMETHOD(Error)(/*[out,retval]*/ int *errcode);
	STDMETHOD(Init)(BSTR IndexFiles);
	DWORD m_dwSafety;
	STDMETHOD (SetInterfaceSafetyOptions)(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions);
	STDMETHOD (GetInterfaceSafetyOptions)(REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions);
private:
	SW_SEARCH swish_search;
	SW_RESULT current_result;
	SW_RESULTS swish_results;
	SW_HANDLE swish_handle;
};

#endif //__CSWISHCTL_H_
