/**
 * Copyright 2008-2009 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Author: Bryan McQuade

#include "http_activity_distributor.h"

#include "clock.h"
#include "check.h"
#include "timer.h"

#include "nsDebug.h"
#include "nsIObserverService.h"
#include "nsISimpleEnumerator.h"
#include "nsIThread.h"
#include "nsIThreadManager.h"
#include "nsServiceManagerUtils.h"  // for do_GetService

#ifdef MOZILLA_1_8_BRANCH
// In gecko 1.8, NS_OBSERVERSERVICE_CONTRACTID is defined in
// nsObserverService.h.
#include "nsObserverService.h"
#endif

// Declare that HttpActivityDistributor implements the mozilla
// nsIHttpActivityObserver interface.
NS_IMPL_ISUPPORTS1(
    activity::HttpActivityDistributor, nsIHttpActivityObserver)

// Declare that MainThreadDistributor implements the mozilla
// nsIRunnable interface.
NS_IMPL_THREADSAFE_ISUPPORTS1(activity::MainThreadDistributor, nsIRunnable)

namespace {

// Contract id for the nsIThreadManager service. Used to get a handle
// to the nsIThreadManager service.
const char *kThreadManagerContactStr = "@mozilla.org/thread-manager;1";

// Helper that returns an enumerator (as an 'out' parameter) of the
// observers subscribed to our topic.
nsresult GetObservers(nsISimpleEnumerator **observers) {
  // Get a handle to the nsIObserverService.
  nsresult rv;
  nsCOMPtr<nsIObserverService> observer_service(
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv));
  if (NS_FAILED(rv)) {
    return rv;
  }

  // Get all observers subscribed to our topic.
  rv = observer_service->EnumerateObservers(
      NS_HTTPACTIVITYOBSERVER_TOPIC,
      observers);
  if (NS_FAILED(rv)) {
    return rv;
  }

  return NS_OK;
}

// Is the specified thread the currently executing thread?
bool IsCurrentThread(nsIThread *thread) {
  PRThread *prthread = NULL;
  thread->GetPRThread(&prthread);
  return (PR_GetCurrentThread() == prthread);
}

// Distribute the specified event to all of the observers subscribed
// to the HTTP activity observer topic.
nsresult DistributeToObservers(
    nsISupports *http_channel,
    PRUint32 activity_type,
    PRUint32 activity_subtype,
    PRTime timestamp,
    PRUint64 extra_size_data,
    const nsACString &extra_string_data) {
  nsCOMPtr<nsISimpleEnumerator> observers;
  nsresult rv = GetObservers(getter_AddRefs(observers));
  if (NS_FAILED(rv)) {
    return rv;
  }

  // Loop over the registered observers.
  PRBool more = PR_FALSE;
  while (observers->HasMoreElements(&more) == NS_OK && more == PR_TRUE) {
    // nsIObserverService only guarantees that observers implement
    // nsISupports. In the event that a registered observer does not
    // implement the expected nsIHttpActivityObserver interface, we
    // first get the observer as an nsISupports instance, and then
    // attempt to QI to the expected interface. If the QI fails, we
    // skip the observer.
    nsCOMPtr<nsISupports> observer_supports;
    rv = observers->GetNext(getter_AddRefs(observer_supports));
    if (NS_FAILED(rv)) {
      return rv;
    }
    nsCOMPtr<nsIHttpActivityObserver> observer(
        do_QueryInterface(observer_supports, &rv));
    if (NS_FAILED(rv)) {
      // Observer does not implement the expected interface. Skip it.
      NS_WARNING("Observer does not implement nsIHttpActivityObserver.");
      continue;
    }

    PRBool active = PR_FALSE;
    rv = observer->GetIsActive(&active);
    if (NS_FAILED(rv)) {
      continue;
    }
    if (active == PR_TRUE) {
      observer->ObserveActivity(
          http_channel,
          activity_type,
          activity_subtype,
          timestamp,
          extra_size_data,
          extra_string_data);
    }
  }

  return NS_OK;
}

}  // namespace

namespace activity {

HttpActivityDistributor::HttpActivityDistributor()
    : clock_(new Clock()),
      // Instantiate a Timer instance, with a start time of 0. This
      // effectively gives us a Clock() instance that is guaranteed to
      // be monotonically increasing.
      timer_(new Timer(clock_.get(), 0LL)) {
  // Get a handle to the main thread, which we'll use to post events
  // to our observers.
  nsresult rv;
  nsCOMPtr<nsIThreadManager> thread_manager(
      do_GetService(kThreadManagerContactStr, &rv));
  if (NS_SUCCEEDED(rv)) {
    thread_manager->GetMainThread(getter_AddRefs(main_thread_));
  }
}

HttpActivityDistributor::~HttpActivityDistributor() {
}

NS_IMETHODIMP HttpActivityDistributor::ObserveActivity(
    nsISupports *http_channel,
    PRUint32 activity_type,
    PRUint32 activity_subtype,
    PRTime timestamp,
    PRUint64 extra_size_data,
    const nsACString &extra_string_data) {
  if (main_thread_ == NULL) {
    NS_WARNING("Main thread unavailable. Not distributing events.");
    return NS_ERROR_NOT_AVAILABLE;
  }

  // Verify that the only event generated on the main thread is
  // NS_HTTP_ACTIVITY_SUBTYPE_REQUEST_HEADER (the first event). The
  // clients assume that all events are delivered in order, but if
  // there is ever a case where some events are generated on the main
  // thread *after* events are generated on the network thread, then
  // the queuing of the network events could cause out-of-order
  // delivery. So, if we ever have to change this logic s.t. other
  // events are delivered on the main thread, we might consider
  // queuing *all* events, even those generated on the main thread, to
  // guarantee in-order delivery.
  const bool is_main_thread = IsCurrentThread(main_thread_);
  if (activity_type == NS_HTTP_ACTIVITY_TYPE_HTTP_TRANSACTION) {
    switch (activity_subtype) {
      case NS_HTTP_ACTIVITY_SUBTYPE_REQUEST_HEADER:
        // The request is initiated on the main thread.
        GCHECK(is_main_thread);
        break;
      case NS_HTTP_ACTIVITY_SUBTYPE_REQUEST_BODY_SENT:
      case NS_HTTP_ACTIVITY_SUBTYPE_RESPONSE_START:
      case NS_HTTP_ACTIVITY_SUBTYPE_RESPONSE_HEADER:
      case NS_HTTP_ACTIVITY_SUBTYPE_RESPONSE_COMPLETE:
      case NS_HTTP_ACTIVITY_SUBTYPE_TRANSACTION_CLOSE:
        // All other events happen on the socket thread.
        GCHECK(!is_main_thread);
        break;
      default:
        GCHECK(false);
        break;
    }
  } else if (activity_type == NS_HTTP_ACTIVITY_TYPE_SOCKET_TRANSPORT) {
    // All socket events arrive on the socket thread.
    GCHECK(!is_main_thread);
  } else {
    GCHECK(false);
  }

  // The caller doesn't actually specify a timestamp. It's up to us to
  // generate one.
  GCHECK_EQ(0, timestamp);
  timestamp = timer_->GetElapsedTimeUsec();

  if (!is_main_thread) {
    // Proxy the event over to the main thread.
    main_thread_->Dispatch(
        new MainThreadDistributor(http_channel,
                                  activity_type,
                                  activity_subtype,
                                  timestamp,
                                  extra_size_data,
                                  extra_string_data),
        nsIEventTarget::DISPATCH_NORMAL);
  } else {
    // We're already on the main thread, so dispatch the event
    // synchronously.
    DistributeToObservers(http_channel,
                          activity_type,
                          activity_subtype,
                          timestamp,
                          extra_size_data,
                          extra_string_data);
  }

  return NS_OK;
}

NS_IMETHODIMP HttpActivityDistributor::GetIsActive(PRBool *is_active) {
  if (!IsCurrentThread(main_thread_)) {
    // We don't want to handle any events that aren't generated on the
    // main thread. TODO: change this fcn's name from
    // GetIsActive() to GetShouldHandle() and pass it an
    // nsIHttpChannel instance.
    return false;
  }

  nsCOMPtr<nsISimpleEnumerator> observers;
  nsresult rv = GetObservers(getter_AddRefs(observers));
  if (NS_FAILED(rv)) {
    *is_active = PR_FALSE;
    return rv;
  }
  PRBool has_elements = PR_FALSE;
  rv = observers->HasMoreElements(&has_elements);
  if (NS_FAILED(rv)) {
    *is_active = PR_FALSE;
    return rv;
  }

  // We're active if we have at least one observer subscribed to our
  // topic.
  *is_active = has_elements;
  return NS_OK;
}

MainThreadDistributor::MainThreadDistributor(
    nsISupports *http_channel,
    PRUint32 activity_type,
    PRUint32 activity_subtype,
    PRTime timestamp,
    PRUint64 extra_size_data,
    const nsACString &extra_string_data)
    : http_channel_(http_channel),
      activity_type_(activity_type),
      activity_subtype_(activity_subtype),
      timestamp_(timestamp),
      extra_size_data_(extra_size_data),
      extra_string_data_(extra_string_data) {
}

MainThreadDistributor::~MainThreadDistributor() {
}

/**
 * This method gets invoked in the main thread. It distributes the
 * event data specified in the constructor to all observers subscribed
 * to our topic.
 */
NS_IMETHODIMP MainThreadDistributor::Run() {
  return DistributeToObservers(
      http_channel_,
      activity_type_,
      activity_subtype_,
      timestamp_,
      extra_size_data_,
      extra_string_data_);
}

}  // namespace activity
