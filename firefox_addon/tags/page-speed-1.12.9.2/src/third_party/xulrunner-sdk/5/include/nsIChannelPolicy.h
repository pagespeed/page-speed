/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM /builds/slave/aurora-lnx64-xr/build/netwerk/base/public/nsIChannelPolicy.idl
 */

#ifndef __gen_nsIChannelPolicy_h__
#define __gen_nsIChannelPolicy_h__


#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif

/* starting interface:    nsIChannelPolicy */
#define NS_ICHANNELPOLICY_IID_STR "18045e96-1afe-4162-837a-04691267158c"

#define NS_ICHANNELPOLICY_IID \
  {0x18045e96, 0x1afe, 0x4162, \
    { 0x83, 0x7a, 0x04, 0x69, 0x12, 0x67, 0x15, 0x8c }}

/**
 * A container for policy information to be used during channel creation.
 *
 * This interface exists to allow the content policy mechanism to function
 * properly during channel redirects.  Channels can be created with this
 * interface placed in the property bag and upon redirect, the interface can
 * be transferred from the old channel to the new channel.
 */
class NS_NO_VTABLE NS_SCRIPTABLE nsIChannelPolicy : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICHANNELPOLICY_IID)

  /**
   * Indicates what type of content is being loaded, e.g.
   * nsIContentPolicy::TYPE_IMAGE
   */
  /* attribute unsigned long loadType; */
  NS_SCRIPTABLE NS_IMETHOD GetLoadType(PRUint32 *aLoadType) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetLoadType(PRUint32 aLoadType) = 0;

  /**
   * A nsIContentSecurityPolicy object to determine if the load should
   * be allowed.
   */
  /* attribute nsISupports contentSecurityPolicy; */
  NS_SCRIPTABLE NS_IMETHOD GetContentSecurityPolicy(nsISupports **aContentSecurityPolicy) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetContentSecurityPolicy(nsISupports *aContentSecurityPolicy) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIChannelPolicy, NS_ICHANNELPOLICY_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSICHANNELPOLICY \
  NS_SCRIPTABLE NS_IMETHOD GetLoadType(PRUint32 *aLoadType); \
  NS_SCRIPTABLE NS_IMETHOD SetLoadType(PRUint32 aLoadType); \
  NS_SCRIPTABLE NS_IMETHOD GetContentSecurityPolicy(nsISupports **aContentSecurityPolicy); \
  NS_SCRIPTABLE NS_IMETHOD SetContentSecurityPolicy(nsISupports *aContentSecurityPolicy); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSICHANNELPOLICY(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetLoadType(PRUint32 *aLoadType) { return _to GetLoadType(aLoadType); } \
  NS_SCRIPTABLE NS_IMETHOD SetLoadType(PRUint32 aLoadType) { return _to SetLoadType(aLoadType); } \
  NS_SCRIPTABLE NS_IMETHOD GetContentSecurityPolicy(nsISupports **aContentSecurityPolicy) { return _to GetContentSecurityPolicy(aContentSecurityPolicy); } \
  NS_SCRIPTABLE NS_IMETHOD SetContentSecurityPolicy(nsISupports *aContentSecurityPolicy) { return _to SetContentSecurityPolicy(aContentSecurityPolicy); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSICHANNELPOLICY(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetLoadType(PRUint32 *aLoadType) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetLoadType(aLoadType); } \
  NS_SCRIPTABLE NS_IMETHOD SetLoadType(PRUint32 aLoadType) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetLoadType(aLoadType); } \
  NS_SCRIPTABLE NS_IMETHOD GetContentSecurityPolicy(nsISupports **aContentSecurityPolicy) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetContentSecurityPolicy(aContentSecurityPolicy); } \
  NS_SCRIPTABLE NS_IMETHOD SetContentSecurityPolicy(nsISupports *aContentSecurityPolicy) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetContentSecurityPolicy(aContentSecurityPolicy); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsChannelPolicy : public nsIChannelPolicy
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICHANNELPOLICY

  nsChannelPolicy();

private:
  ~nsChannelPolicy();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsChannelPolicy, nsIChannelPolicy)

nsChannelPolicy::nsChannelPolicy()
{
  /* member initializers and constructor code */
}

nsChannelPolicy::~nsChannelPolicy()
{
  /* destructor code */
}

/* attribute unsigned long loadType; */
NS_IMETHODIMP nsChannelPolicy::GetLoadType(PRUint32 *aLoadType)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsChannelPolicy::SetLoadType(PRUint32 aLoadType)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute nsISupports contentSecurityPolicy; */
NS_IMETHODIMP nsChannelPolicy::GetContentSecurityPolicy(nsISupports **aContentSecurityPolicy)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsChannelPolicy::SetContentSecurityPolicy(nsISupports *aContentSecurityPolicy)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_nsIChannelPolicy_h__ */
