/*
 Copyright (C) 2009 Moutaz Haq <cefarix@gmail.com>
 This file is released under the Code Project Open License <http://www.codeproject.com/info/cpol10.aspx>

 This defines our implementation of the DWebBrowserEvents2 dispatch interface.
 We give a reference to an object of this class to Internet Explorer, and IE calls into that object when an event occurs.
*/

#include <Exdisp.h>
#include <Exdispid.h>

// Note we don't derive from our implementation of CUnknown
// This is because CEventSink always has only one instance, so we write a custom implementation of the IUnknown methods
class CEventSink : public DWebBrowserEvents2 {
public:
	// No constructor or destructor is needed
	// IUnknown methods
	STDMETHODIMP QueryInterface(REFIID riid,void **ppvObject);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();
	// IDispatch methods
	STDMETHODIMP GetTypeInfoCount(UINT *pctinfo);
	STDMETHODIMP GetTypeInfo(UINT iTInfo,LCID lcid,ITypeInfo **ppTInfo);
	STDMETHODIMP GetIDsOfNames(REFIID riid,LPOLESTR *rgszNames,UINT cNames,LCID lcid,DISPID *rgDispId);
	STDMETHODIMP Invoke(DISPID dispIdMember,REFIID riid,LCID lcid,WORD wFlags,DISPPARAMS *pDispParams,VARIANT *pVarResult,EXCEPINFO *pExcepInfo,UINT *puArgErr);
	// DWebBrowserEvents2 does not have any methods, IE calls our Invoke() method to notify us of events
protected:
	// Event handling methods
	bool Event_BeforeNavigate2(LPOLESTR url,LONG Flags,LPOLESTR TargetFrameName,PUCHAR PostData,LONG PostDataSize,LPOLESTR Headers,bool Cancel);
};

// We only have one global object of this
extern CEventSink EventSink;
