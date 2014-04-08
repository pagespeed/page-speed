/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM /builds/slave/aurora-lnx64-xr/build/netwerk/base/public/nsIChannelEventSink.idl
 */

#ifndef __gen_nsIChannelEventSink_h__
#define __gen_nsIChannelEventSink_h__


#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif
class nsIChannel; /* forward declaration */

class nsIAsyncVerifyRedirectCallback; /* forward declaration */


/* starting interface:    nsIChannelEventSink */
#define NS_ICHANNELEVENTSINK_IID_STR "a430d870-df77-4502-9570-d46a8de33154"

#define NS_ICHANNELEVENTSINK_IID \
  {0xa430d870, 0xdf77, 0x4502, \
    { 0x95, 0x70, 0xd4, 0x6a, 0x8d, 0xe3, 0x31, 0x54 }}

/**
 * Implement this interface to receive control over various channel events.
 * Channels will try to get this interface from a channel's
 * notificationCallbacks or, if not available there, from the loadGroup's
 * notificationCallbacks.
 *
 * These methods are called before onStartRequest.
 */
class NS_NO_VTABLE NS_SCRIPTABLE nsIChannelEventSink : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICHANNELEVENTSINK_IID)

  /**
     * This is a temporary redirect. New requests for this resource should
     * continue to use the URI of the old channel.
     *
     * The new URI may be identical to the old one.
     */
  enum { REDIRECT_TEMPORARY = 1U };

  /**
     * This is a permanent redirect. New requests for this resource should use
     * the URI of the new channel (This might be an HTTP 301 reponse).
     * If this flag is not set, this is a temporary redirect.
     *
     * The new URI may be identical to the old one.
     */
  enum { REDIRECT_PERMANENT = 2U };

  /**
     * This is an internal redirect, i.e. it was not initiated by the remote
     * server, but is specific to the channel implementation.
     *
     * The new URI may be identical to the old one.
     */
  enum { REDIRECT_INTERNAL = 4U };

  /**
     * Called when a redirect occurs. This may happen due to an HTTP 3xx status
     * code. The purpose of this method is to notify the sink that a redirect
     * is about to happen, but also to give the sink the right to veto the
     * redirect by throwing or passing a failure-code in the callback.
     *
     * Note that vetoing the redirect simply means that |newChannel| will not
     * be opened. It is important to understand that |oldChannel| will continue
     * loading as if it received a HTTP 200, which includes notifying observers
     * and possibly display or process content attached to the HTTP response.
     * If the sink wants to prevent this loading it must explicitly deal with
     * it, e.g. by calling |oldChannel->Cancel()|
     *
     * There is a certain freedom in implementing this method:
     *
     * If the return-value indicates success, a callback on |callback| is
     * required. This callback can be done from within asyncOnChannelRedirect
     * (effectively making the call synchronous) or at some point later
     * (making the call asynchronous). Repeat: A callback must be done
     * if this method returns successfully.
     *
     * If the return value indicates error (method throws an exception)
     * the redirect is vetoed and no callback must be done. Repeat: No
     * callback must be done if this method throws!
     *
     * @see nsIAsyncVerifyRedirectCallback::onRedirectVerifyCallback()
     *
     * @param oldChannel
     *        The channel that's being redirected.
     * @param newChannel
     *        The new channel. This channel is not opened yet.
     * @param flags
     *        Flags indicating the type of redirect. A bitmask consisting
     *        of flags from above.
     *        One of REDIRECT_TEMPORARY and REDIRECT_PERMANENT will always be
     *        set.
     * @param callback
     *        Object to inform about the async result of this method
     *
     * @throw <any> Throwing an exception will cause the redirect to be
     *        cancelled
     */
  /* void asyncOnChannelRedirect (in nsIChannel oldChannel, in nsIChannel newChannel, in unsigned long flags, in nsIAsyncVerifyRedirectCallback callback); */
  NS_SCRIPTABLE NS_IMETHOD AsyncOnChannelRedirect(nsIChannel *oldChannel, nsIChannel *newChannel, PRUint32 flags, nsIAsyncVerifyRedirectCallback *callback) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIChannelEventSink, NS_ICHANNELEVENTSINK_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSICHANNELEVENTSINK \
  NS_SCRIPTABLE NS_IMETHOD AsyncOnChannelRedirect(nsIChannel *oldChannel, nsIChannel *newChannel, PRUint32 flags, nsIAsyncVerifyRedirectCallback *callback); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSICHANNELEVENTSINK(_to) \
  NS_SCRIPTABLE NS_IMETHOD AsyncOnChannelRedirect(nsIChannel *oldChannel, nsIChannel *newChannel, PRUint32 flags, nsIAsyncVerifyRedirectCallback *callback) { return _to AsyncOnChannelRedirect(oldChannel, newChannel, flags, callback); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSICHANNELEVENTSINK(_to) \
  NS_SCRIPTABLE NS_IMETHOD AsyncOnChannelRedirect(nsIChannel *oldChannel, nsIChannel *newChannel, PRUint32 flags, nsIAsyncVerifyRedirectCallback *callback) { return !_to ? NS_ERROR_NULL_POINTER : _to->AsyncOnChannelRedirect(oldChannel, newChannel, flags, callback); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsChannelEventSink : public nsIChannelEventSink
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICHANNELEVENTSINK

  nsChannelEventSink();

private:
  ~nsChannelEventSink();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsChannelEventSink, nsIChannelEventSink)

nsChannelEventSink::nsChannelEventSink()
{
  /* member initializers and constructor code */
}

nsChannelEventSink::~nsChannelEventSink()
{
  /* destructor code */
}

/* void asyncOnChannelRedirect (in nsIChannel oldChannel, in nsIChannel newChannel, in unsigned long flags, in nsIAsyncVerifyRedirectCallback callback); */
NS_IMETHODIMP nsChannelEventSink::AsyncOnChannelRedirect(nsIChannel *oldChannel, nsIChannel *newChannel, PRUint32 flags, nsIAsyncVerifyRedirectCallback *callback)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_nsIChannelEventSink_h__ */
