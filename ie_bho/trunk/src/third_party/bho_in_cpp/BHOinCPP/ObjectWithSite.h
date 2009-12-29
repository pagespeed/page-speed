/*
 Copyright (C) 2009 Moutaz Haq <cefarix@gmail.com>
 This file is released under the Code Project Open License <http://www.codeproject.com/info/cpol10.aspx>

 This defines our implementation of the IObjectWithSite interface. The IObjectWithSite interface is how Internet Explorer accesses our BHO.
*/

#ifndef __OBJECTWITHSITE_H__
#define __OBJECTWITHSITE_H__

#include <Ocidl.h>
#include <Exdisp.h>
#include "Unknown.h"

class CObjectWithSite : public CUnknown<IObjectWithSite> {
public:
	// Constructor and destructor
	CObjectWithSite();
	virtual ~CObjectWithSite();
	// IObjectWithSite methods
	STDMETHODIMP SetSite(IUnknown *pUnkSite);
	STDMETHODIMP GetSite(REFIID riid,void **ppvSite);
protected:
	void ConnectEventSink(); // used to start handling events from IE
	void DisconnectEventSink(); // used to stop handling events from IE
	IWebBrowser2 *pSite; // the currently set site
	IConnectionPoint *pCP; // the active connection point interface
	DWORD adviseCookie; // used by ConnectEventSink() and DisconnectEventSink() in conjunction with pCP
	static const IID SupportedIIDs[2];
};

#endif // __OBJECTWITHSITE_H__
