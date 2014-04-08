/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM /builds/slave/aurora-lnx64-xr/build/netwerk/base/public/nsIURIWithPrincipal.idl
 */

#ifndef __gen_nsIURIWithPrincipal_h__
#define __gen_nsIURIWithPrincipal_h__


#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif
class nsIPrincipal; /* forward declaration */

class nsIURI; /* forward declaration */


/* starting interface:    nsIURIWithPrincipal */
#define NS_IURIWITHPRINCIPAL_IID_STR "626a5c0c-bfd8-4531-8b47-a8451b0daa33"

#define NS_IURIWITHPRINCIPAL_IID \
  {0x626a5c0c, 0xbfd8, 0x4531, \
    { 0x8b, 0x47, 0xa8, 0x45, 0x1b, 0x0d, 0xaa, 0x33 }}

/**
 * nsIURIWithPrincipal is implemented by URIs which are associated with a
 * specific principal.
 */
class NS_NO_VTABLE NS_SCRIPTABLE nsIURIWithPrincipal : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IURIWITHPRINCIPAL_IID)

  /**
     * The principal associated with the resource returned when loading this
     * uri.
     */
  /* readonly attribute nsIPrincipal principal; */
  NS_SCRIPTABLE NS_IMETHOD GetPrincipal(nsIPrincipal **aPrincipal) = 0;

  /**
     * The uri for the principal.
     */
  /* readonly attribute nsIURI principalUri; */
  NS_SCRIPTABLE NS_IMETHOD GetPrincipalUri(nsIURI **aPrincipalUri) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIURIWithPrincipal, NS_IURIWITHPRINCIPAL_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIURIWITHPRINCIPAL \
  NS_SCRIPTABLE NS_IMETHOD GetPrincipal(nsIPrincipal **aPrincipal); \
  NS_SCRIPTABLE NS_IMETHOD GetPrincipalUri(nsIURI **aPrincipalUri); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIURIWITHPRINCIPAL(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetPrincipal(nsIPrincipal **aPrincipal) { return _to GetPrincipal(aPrincipal); } \
  NS_SCRIPTABLE NS_IMETHOD GetPrincipalUri(nsIURI **aPrincipalUri) { return _to GetPrincipalUri(aPrincipalUri); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIURIWITHPRINCIPAL(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetPrincipal(nsIPrincipal **aPrincipal) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetPrincipal(aPrincipal); } \
  NS_SCRIPTABLE NS_IMETHOD GetPrincipalUri(nsIURI **aPrincipalUri) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetPrincipalUri(aPrincipalUri); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsURIWithPrincipal : public nsIURIWithPrincipal
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIURIWITHPRINCIPAL

  nsURIWithPrincipal();

private:
  ~nsURIWithPrincipal();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsURIWithPrincipal, nsIURIWithPrincipal)

nsURIWithPrincipal::nsURIWithPrincipal()
{
  /* member initializers and constructor code */
}

nsURIWithPrincipal::~nsURIWithPrincipal()
{
  /* destructor code */
}

/* readonly attribute nsIPrincipal principal; */
NS_IMETHODIMP nsURIWithPrincipal::GetPrincipal(nsIPrincipal **aPrincipal)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute nsIURI principalUri; */
NS_IMETHODIMP nsURIWithPrincipal::GetPrincipalUri(nsIURI **aPrincipalUri)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_nsIURIWithPrincipal_h__ */
