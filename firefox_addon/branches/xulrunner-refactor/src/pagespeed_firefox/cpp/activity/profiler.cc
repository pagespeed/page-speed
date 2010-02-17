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

// Profiler implementation

#include "profiler.h"

#include "basic_tree_view.h"
#include "call_graph.h"
#include "call_graph_profile.h"
#include "call_graph_profile_snapshot.h"
#include "call_graph_util.h"
#include "clock.h"
#include "delayable_function_tree_view_delegate.h"
#include "find_first_invocations_visitor.h"
#include "jsd_call_hook.h"
#include "jsd_script_hook.h"
#include "output_stream_interface.h"
#include "profiler_runnables.h"
#include "uncalled_function_tree_view_delegate.h"

#include "jsdIDebuggerService.h"
#include "nsIEventTarget.h"
#include "nsILocalFile.h"
#include "nsIThread.h"
#include "nsIThreadManager.h"
#include "nsNetUtil.h"
#include "nsServiceManagerUtils.h"
#include "nsStringAPI.h"

#include "base/logging.h"

NS_IMPL_ISUPPORTS1(activity::Profiler, IActivityProfiler)

namespace {

const char* kJsdContractStr = "@mozilla.org/js/jsd/debugger-service;1";
const char* kThreadManagerContactStr = "@mozilla.org/thread-manager;1";

// Implementation of OutputStreamInterface that writes to an
// nsILocalFile instance.
class FileOutputStream : public activity::OutputStreamInterface {
 public:
  virtual ~FileOutputStream() {}

  bool Init(nsILocalFile *target);
  bool Close();

  // Implement OutputStreamInterface
  virtual bool Write(const void *buffer, size_t size);

 private:
  nsCOMPtr<nsIOutputStream> out_;
};

bool FileOutputStream::Init(nsILocalFile *target) {
  nsresult rv = NS_NewLocalFileOutputStream(getter_AddRefs(out_), target);
  if (NS_FAILED(rv)) {
    out_ = NULL;
    return false;
  }

  return true;
}

bool FileOutputStream::Write(const void *buffer, size_t size) {
  PRUint32 num_written = 0;
  nsresult rv = out_->Write(static_cast<const char*>(buffer),
                            size,
                            &num_written);
  if (NS_FAILED(rv) || num_written != size) {
    return false;
  }

  return true;
}

bool FileOutputStream::Close() {
  nsresult rv = out_->Close();
  if (NS_FAILED(rv)) {
    return false;
  }

  return true;
}

}  // namespace

namespace activity {

Profiler::Profiler()
    : clock_(new Clock()),
      profile_(new CallGraphProfile(clock_.get())),
      call_hook_(new JsdCallHook(profile_.get())),
      script_hook_(new JsdScriptHook(profile_.get())),
      state_(IActivityProfiler::NOT_STARTED),
      error_(false) {
}

Profiler::~Profiler() {
  // Make sure we drain the background thread before getting deleted,
  // since tasks running in that thread depend on our datastructures.
  if (background_thread_ != NULL) {
    background_thread_->Shutdown();
  }
}

NS_IMETHODIMP Profiler::Register(
    PRInt64 start_time_usec, PRBool collect_full_call_trees) {
  if (error_ || state_ != IActivityProfiler::NOT_STARTED) {
    LOG(ERROR) << "In error state or re-entrant start.  Error:"
               << error_
               << " state: "
               << state_;
    return NS_ERROR_FAILURE;
  }

  if (start_time_usec > clock_->GetCurrentTimeUsec()) {
    // We require that the specified start time is in the
    // past. Otherwise, the profile could end up containing negative
    // timestamps.
    LOG(ERROR) << "Time reference error";
    return NS_ERROR_INVALID_ARG;
  }

  nsresult rv = NS_OK;
  nsCOMPtr<jsdIDebuggerService> jsd(do_GetService(kJsdContractStr, &rv));
  if (NS_FAILED(rv)) {
    LOG(ERROR) << "Failed to get jsdIDebuggerService";
    error_ = true;
    return rv;
  }

  nsCOMPtr<nsIThreadManager> thread_manager =
      do_GetService(kThreadManagerContactStr, &rv);
  if (NS_FAILED(rv)) {
    LOG(ERROR) << "Error getting thread manager";
    error_ = true;
    return rv;
  }

  rv = thread_manager->NewThread(0, getter_AddRefs(background_thread_));
  if (NS_FAILED(rv)) {
    LOG(ERROR) << "Error starting thread";
    error_ = true;
    return rv;
  }

  rv = thread_manager->GetMainThread(getter_AddRefs(main_thread_));
  if (NS_FAILED(rv)) {
    LOG(ERROR) << "Error getting main thread";
    error_ = true;
    return rv;
  }

  profile_->Start(start_time_usec);
  state_ = IActivityProfiler::PROFILING;

  call_hook_->set_collect_full_call_trees(collect_full_call_trees == PR_TRUE);
  rv = jsd->SetFunctionHook(call_hook_);
  if (NS_FAILED(rv)) {
    LOG(ERROR) << "Error setting function hook";
    Unregister();
    error_ = true;
    return rv;
  }

  rv = jsd->SetTopLevelHook(call_hook_);
  if (NS_FAILED(rv)) {
    LOG(ERROR) << "Error setting toplevel hook";
    Unregister();
    error_ = true;
    return rv;
  }

  script_hook_->set_collect_full_call_trees(collect_full_call_trees == PR_TRUE);
  rv = jsd->SetScriptHook(script_hook_);
  if (NS_FAILED(rv)) {
    LOG(ERROR) << "Error setting script hook";
    Unregister();
    error_ = true;
    return rv;
  }

  return NS_OK;
}

NS_IMETHODIMP Profiler::Unregister() {
  if (error_ || state_ != IActivityProfiler::PROFILING) {
    return NS_ERROR_FAILURE;
  }

  nsresult rv = NS_OK;
  nsCOMPtr<jsdIDebuggerService> jsd(do_GetService(kJsdContractStr, &rv));
  if (NS_FAILED(rv)) {
    LOG(ERROR) << "Failed to get jsdIDebuggerService";
    return rv;
  }
  nsresult function_hook_rv = NS_OK;
  nsresult top_level_hook_rv = NS_OK;
  nsresult script_hook_rv = NS_OK;
  function_hook_rv = jsd->SetFunctionHook(NULL);
  top_level_hook_rv = jsd->SetTopLevelHook(NULL);
  script_hook_rv = jsd->SetScriptHook(NULL);

  profile_->Stop();
  state_ = IActivityProfiler::FINISHED;

  if (NS_FAILED(function_hook_rv)) {
    return function_hook_rv;
  }
  if (NS_FAILED(top_level_hook_rv)) {
    return top_level_hook_rv;
  }
  if (NS_FAILED(script_hook_rv)) {
    return script_hook_rv;
  }

  return NS_OK;
}

NS_IMETHODIMP Profiler::GetState(PRInt16 *state) {
  if (error_) {
    return NS_ERROR_FAILURE;
  }

  *state = state_;
  return NS_OK;
}

NS_IMETHODIMP Profiler::HasError(PRBool *is_error) {
  *is_error = error_ ? PR_TRUE : PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP Profiler::Dump(nsILocalFile *target) {
  if (error_ || state_ != IActivityProfiler::FINISHED) {
    return NS_ERROR_FAILURE;
  }

  if (target == NULL) {
    return NS_ERROR_INVALID_ARG;
  }

  FileOutputStream output_stream;
  if (!output_stream.Init(target)) {
    return NS_ERROR_FAILURE;
  }

  nsresult rv = NS_OK;
  if (!profile_->SerializeToOutputStream(&output_stream)) {
    rv = NS_ERROR_FAILURE;
  }
  if (!output_stream.Close()) {
    rv = NS_ERROR_FAILURE;
  }
  return rv;
}

NS_IMETHODIMP Profiler::GetTimelineEvents(
    PRInt64 start_time_usec,
    PRInt64 end_time_usec,
    PRInt64 resolution_usec,
    IActivityProfilerTimelineEventCallback *callback) {
  if (error_ ||
      (state_ != IActivityProfiler::PROFILING &&
       state_ != IActivityProfiler::FINISHED)) {
    return NS_ERROR_FAILURE;
  }

  // Check the arguments, and abort if they are invalid.
  if (start_time_usec < 0LL ||
      start_time_usec % resolution_usec != 0LL ||
      resolution_usec <= 0LL) {
    return NS_ERROR_INVALID_ARG;
  }
  if (end_time_usec > 0LL) {
    // If end_time_usec is negative, it means there is no end time. If
    // it is positive, however, we must verify that the specified end
    // time is valid.
    if (end_time_usec % resolution_usec != 0LL ||
        end_time_usec < start_time_usec) {
      return NS_ERROR_INVALID_ARG;
    }
  }
  if (background_thread_ == NULL || main_thread_ == NULL) {
    // We were unable to allocate the threads we need to perform the
    // background operation. Abort.
    return NS_ERROR_NOT_AVAILABLE;
  }

  int max_callgraph_time_usec =
      util::GetMaxFullyConstructedCallGraphTimeUsec(*profile_->call_graph());
  if (end_time_usec < 0 ||
      end_time_usec > max_callgraph_time_usec) {
    end_time_usec = util::RoundDownToNearestWholeMultiple(
        max_callgraph_time_usec, resolution_usec);
  }

  scoped_ptr<CallGraphProfileSnapshot> snapshot(
      profile_->CreateSnapshot());

  // Create the runnable that builds the timeline on the background
  // thread, and dispatch it to the background thread. Don't wrap it
  // in an nsCOMPtr, because nsCOMPtr is not thread-safe, and the
  // background thread will manage its lifetime.
  GetTimelineEventsRunnable *get_timeline_events_runnable =
      new GetTimelineEventsRunnable(
          main_thread_,
          callback,
          snapshot.release(),
          start_time_usec,
          end_time_usec,
          resolution_usec);

  background_thread_->Dispatch(
      get_timeline_events_runnable, nsIEventTarget::DISPATCH_NORMAL);

  return NS_OK;
}

NS_IMETHODIMP Profiler::GetDelayableFunctionsTreeView(nsITreeView **retval) {
  FindFirstInvocationsVisitor visitor;
  profile_->call_graph()->Traverse(&visitor);
  DelayableFunctionTreeViewDelegate *delegate =
      new DelayableFunctionTreeViewDelegate(*profile_.get());
  delegate->Initialize(visitor);
  BasicTreeView *view = new BasicTreeView(delegate, this);
  *retval = view;
  NS_ADDREF(*retval);
  return NS_OK;
}

NS_IMETHODIMP Profiler::GetUncalledFunctionsTreeView(nsITreeView **retval) {
  FindFirstInvocationsVisitor visitor;
  profile_->call_graph()->Traverse(&visitor);
  UncalledFunctionTreeViewDelegate *delegate =
      new UncalledFunctionTreeViewDelegate(*profile_.get());
  delegate->Initialize(visitor);
  BasicTreeView *view = new BasicTreeView(delegate, this);
  *retval = view;
  NS_ADDREF(*retval);
  return NS_OK;
}

NS_IMETHODIMP Profiler::GetCurrentTimeUsec(PRInt64 *time) {
  *time = clock_->GetCurrentTimeUsec();
  return NS_OK;
}

}  // namespace activity
