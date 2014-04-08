/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM /builds/slave/aurora-lnx64-xr/build/netwerk/base/public/nsIRedirectChannelRegistrar.idl
 */

#ifndef __gen_nsIRedirectChannelRegistrar_h__
#define __gen_nsIRedirectChannelRegistrar_h__


#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif
class nsIChannel; /* forward declaration */

class nsIParentChannel; /* forward declaration */


/* starting interface:    nsIRedirectChannelRegistrar */
#define NS_IREDIRECTCHANNELREGISTRAR_IID_STR "efa36ea2-5b07-46fc-9534-a5acb8b77b72"

#define NS_IREDIRECTCHANNELREGISTRAR_IID \
  {0xefa36ea2, 0x5b07, 0x46fc, \
    { 0x95, 0x34, 0xa5, 0xac, 0xb8, 0xb7, 0x7b, 0x72 }}

/**
 * Used on the chrome process as a service to join channel implementation
 * and parent IPC protocol side under a unique id.  Provides this way a generic
 * communication while redirecting to various protocols.
 *
 * See also nsIChildChannel and nsIParentChannel.
 */
class NS_NO_VTABLE NS_SCRIPTABLE nsIRedirectChannelRegistrar : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IREDIRECTCHANNELREGISTRAR_IID)

  /**
   * Register the redirect target channel and obtain a unique ID for that
   * channel.
   *
   * Primarily used in HttpChannelParentListener::AsyncOnChannelRedirect to get
   * a channel id sent to the HttpChannelChild being redirected.
   */
  /* PRUint32 registerChannel (in nsIChannel channel); */
  NS_SCRIPTABLE NS_IMETHOD RegisterChannel(nsIChannel *channel, PRUint32 *_retval NS_OUTPARAM) = 0;

  /**
   * First, search for the channel registered under the id.  If found return
   * it.  Then, register under the same id the parent side of IPC protocol
   * to let it be later grabbed back by the originator of the redirect and
   * notifications from the real channel could be forwarded to this parent
   * channel.
   *
   * Primarily used in parent side of an IPC protocol implementation
   * in reaction to nsIChildChannel.connectParent(id) called from the child
   * process.
   */
  /* nsIChannel linkChannels (in PRUint32 id, in nsIParentChannel channel); */
  NS_SCRIPTABLE NS_IMETHOD LinkChannels(PRUint32 id, nsIParentChannel *channel, nsIChannel **_retval NS_OUTPARAM) = 0;

  /**
   * Returns back the channel previously registered under the ID with
   * registerChannel method.
   *
   * Primarilly used in chrome IPC side of protocols when attaching a redirect
   * target channel to an existing 'real' channel implementation.
   */
  /* nsIChannel getRegisteredChannel (in PRUint32 id); */
  NS_SCRIPTABLE NS_IMETHOD GetRegisteredChannel(PRUint32 id, nsIChannel **_retval NS_OUTPARAM) = 0;

  /**
   * Returns the stream listener that shall be attached to the redirect target
   * channel, all notification from the redirect target channel will be
   * forwarded to this stream listener.
   *
   * Primarilly used in HttpChannelParentListener::OnRedirectResult callback
   * to grab the created parent side of the channel and forward notifications
   * to it.
   */
  /* nsIParentChannel getParentChannel (in PRUint32 id); */
  NS_SCRIPTABLE NS_IMETHOD GetParentChannel(PRUint32 id, nsIParentChannel **_retval NS_OUTPARAM) = 0;

  /**
   * To not force all channel implementations to support weak reference
   * consumers of this service must ensure release of registered channels them
   * self.  This releases both the real and parent channel registered under
   * the id.
   *
   * Primarilly used in HttpChannelParentListener::OnRedirectResult callback.
   */
  /* void deregisterChannels (in PRUint32 id); */
  NS_SCRIPTABLE NS_IMETHOD DeregisterChannels(PRUint32 id) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIRedirectChannelRegistrar, NS_IREDIRECTCHANNELREGISTRAR_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIREDIRECTCHANNELREGISTRAR \
  NS_SCRIPTABLE NS_IMETHOD RegisterChannel(nsIChannel *channel, PRUint32 *_retval NS_OUTPARAM); \
  NS_SCRIPTABLE NS_IMETHOD LinkChannels(PRUint32 id, nsIParentChannel *channel, nsIChannel **_retval NS_OUTPARAM); \
  NS_SCRIPTABLE NS_IMETHOD GetRegisteredChannel(PRUint32 id, nsIChannel **_retval NS_OUTPARAM); \
  NS_SCRIPTABLE NS_IMETHOD GetParentChannel(PRUint32 id, nsIParentChannel **_retval NS_OUTPARAM); \
  NS_SCRIPTABLE NS_IMETHOD DeregisterChannels(PRUint32 id); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIREDIRECTCHANNELREGISTRAR(_to) \
  NS_SCRIPTABLE NS_IMETHOD RegisterChannel(nsIChannel *channel, PRUint32 *_retval NS_OUTPARAM) { return _to RegisterChannel(channel, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD LinkChannels(PRUint32 id, nsIParentChannel *channel, nsIChannel **_retval NS_OUTPARAM) { return _to LinkChannels(id, channel, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD GetRegisteredChannel(PRUint32 id, nsIChannel **_retval NS_OUTPARAM) { return _to GetRegisteredChannel(id, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD GetParentChannel(PRUint32 id, nsIParentChannel **_retval NS_OUTPARAM) { return _to GetParentChannel(id, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD DeregisterChannels(PRUint32 id) { return _to DeregisterChannels(id); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIREDIRECTCHANNELREGISTRAR(_to) \
  NS_SCRIPTABLE NS_IMETHOD RegisterChannel(nsIChannel *channel, PRUint32 *_retval NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->RegisterChannel(channel, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD LinkChannels(PRUint32 id, nsIParentChannel *channel, nsIChannel **_retval NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->LinkChannels(id, channel, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD GetRegisteredChannel(PRUint32 id, nsIChannel **_retval NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetRegisteredChannel(id, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD GetParentChannel(PRUint32 id, nsIParentChannel **_retval NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetParentChannel(id, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD DeregisterChannels(PRUint32 id) { return !_to ? NS_ERROR_NULL_POINTER : _to->DeregisterChannels(id); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsRedirectChannelRegistrar : public nsIRedirectChannelRegistrar
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREDIRECTCHANNELREGISTRAR

  nsRedirectChannelRegistrar();

private:
  ~nsRedirectChannelRegistrar();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsRedirectChannelRegistrar, nsIRedirectChannelRegistrar)

nsRedirectChannelRegistrar::nsRedirectChannelRegistrar()
{
  /* member initializers and constructor code */
}

nsRedirectChannelRegistrar::~nsRedirectChannelRegistrar()
{
  /* destructor code */
}

/* PRUint32 registerChannel (in nsIChannel channel); */
NS_IMETHODIMP nsRedirectChannelRegistrar::RegisterChannel(nsIChannel *channel, PRUint32 *_retval NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIChannel linkChannels (in PRUint32 id, in nsIParentChannel channel); */
NS_IMETHODIMP nsRedirectChannelRegistrar::LinkChannels(PRUint32 id, nsIParentChannel *channel, nsIChannel **_retval NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIChannel getRegisteredChannel (in PRUint32 id); */
NS_IMETHODIMP nsRedirectChannelRegistrar::GetRegisteredChannel(PRUint32 id, nsIChannel **_retval NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIParentChannel getParentChannel (in PRUint32 id); */
NS_IMETHODIMP nsRedirectChannelRegistrar::GetParentChannel(PRUint32 id, nsIParentChannel **_retval NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void deregisterChannels (in PRUint32 id); */
NS_IMETHODIMP nsRedirectChannelRegistrar::DeregisterChannels(PRUint32 id)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_nsIRedirectChannelRegistrar_h__ */
