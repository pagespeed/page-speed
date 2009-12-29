/*
 Copyright (C) 2009 Moutaz Haq <cefarix@gmail.com>
 This file is released under the Code Project Open License <http://www.codeproject.com/info/cpol10.aspx>

 This defines our implementation of the IClassFactory interface. The IClassFactory interface is used by COM to create objects of the DLL's main COM class.
*/

#ifndef __CLASSFACTORY_H__
#define __CLASSFACTORY_H__

#include "Unknown.h"

class CClassFactory : public CUnknown<IClassFactory> {
public:
	// Constructor and destructor
	CClassFactory();
	virtual ~CClassFactory();
	// IClassFactory methods
	STDMETHODIMP CreateInstance(IUnknown *pUnkOuter,REFIID riid,void **ppvObject);
	STDMETHODIMP LockServer(BOOL fLock);
private:
	static const IID SupportedIIDs[2];
};

#endif // __CLASSFACTORY_H__