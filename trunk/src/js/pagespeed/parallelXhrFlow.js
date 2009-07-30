/**
 * Copyright 2009 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @fileoverview Creates a control flow that makes multiple XMLHttpRequests in
 * parallel with a callback after each is completed and a callback after all are
 * completed.
 *
 * Example usage:
 *
 * var onComplete = function(responseText) {
 *   // Handle responseText.
 * }
 * var onError = function(status) {
 *   // Handle error.
 * }
 * var onAllComplete = function() {
 *   // Handle all complete.
 * }
 * var flow = new ParallelXhrFlow(onAllComplete);
 * flow.addRequest('http://myurl.com/path', 'foo=bar1', onComplete, onError);
 * flow.addRequest('http://myurl.com/path', 'foo=bar2', onComplete, onError);
 * flow.sendRequests();
 *
 * @author Tony Gentilcore
 */

/**
 * Creates a new Parallel XHR Flow.
 *
 * @param {Function} onAllComplete Function to call after all requests have
 *     completed. Note that in the case no requests have been registered, this
 *     method will be called directly by sendRequests().
 * @constructor
 */
PAGESPEED.ParallelXhrFlow = function(onAllComplete) {
  /** @type {Array.<PAGESPEED.XhrRequest>} */ this.requests_ = [];
  /** @type {number} */ this.numOutstandingRequests_ = 0;
  this.onAllComplete_ = onAllComplete;
};

/**
 * Adds a new request that will be sent in parallel when sendRequests is
 * called.
 *
 * @param {string} url The URL to POST to.
 * @param {string} params Parameters to pass to the URL.
 * @param {Function} opt_onComplete Function to call when one request completes
 *     successfully with a 200 response code. The responseText of the response
 *     will be passed to this callback.
 * @param {Function} opt_onError Function to call when one request encounters
 *     an error or receives a non-200 status code. The status code of the
 *     response will be passed to this callback.
 */
PAGESPEED.ParallelXhrFlow.prototype.addRequest = function(url, params,
                                                          opt_onComplete,
                                                          opt_onError) {
  var self = this;
  var request = new PAGESPEED.XhrRequest(url, params,
                                         function(responseText) {
                                           self.onComplete_(
                                               opt_onComplete || null,
                                               responseText);
                                         },
                                         function(status) {
                                           self.onComplete_(
                                               opt_onError || null,
                                               status);
                                         });
  this.requests_.push(request);
};

/**
 * Clears all pending requests.
 */
PAGESPEED.ParallelXhrFlow.prototype.clearRequests = function() {
  this.requests_ = [];
  this.numOutstandingRequests_ = 0;
};

/**
 * Sends all requests in parallel.
 */
PAGESPEED.ParallelXhrFlow.prototype.sendRequests = function() {
  // Invoke the all complete handler and bail out early if there are no
  // requests to be made.
  if (this.getTotalRequests() == 0) {
    this.onAllComplete_();
    return;
  }

  for (var i = 0, len = this.requests_.length; i < len; ++i) {
    ++this.numOutstandingRequests_;
    this.requests_[i].send();
  }
};

/**
 * Gets the number of outstanding requests.
 * @return {number} The number of outstanding requests.
 */
PAGESPEED.ParallelXhrFlow.prototype.getNumOutstandingRequests = function() {
  return this.numOutstandingRequests_;
};

/**
 * Gets the total number of requets to be made.
 * @return {number} The number of outstanding requests.
 */
PAGESPEED.ParallelXhrFlow.prototype.getTotalRequests = function() {
  return this.requests_.length;
};

/**
 * Called internally when each requests completes.
 *
 * @param {Function} callback The callback function.
 * @param {string|number} callback_arg The argument to pass to the callback.
 * @private
 */
PAGESPEED.ParallelXhrFlow.prototype.onComplete_ = function(callback,
                                                           callback_arg) {
  // Call the appropriate callback for this completion.
  if (callback) {
    callback(callback_arg);
  }

  // If this is the final completion, call the onAllComplete callback.
  if (!--this.numOutstandingRequests_) {
    this.onAllComplete_();
  }
};


/**
 * Creates an XMLHttpRequest for asynchronous POST.
 *
 * @param {string} url The URL to POST to.
 * @param {string} params Parameters to pass to the URL.
 * @param {Function} onComplete This is called on a change in the state of the
 *     request.
 * @param {Function} onError The function to call in the event of an error.
 * @constructor
 */
PAGESPEED.XhrRequest = function(url, params, onComplete, onError) {
  this.url = url;
  this.params = params;
  this.onComplete = onComplete;
  this.onError = onError;
  this.didCallback = false;
};

/**
 * Creates and sends the XHR.
 */
PAGESPEED.XhrRequest.prototype.send = function() {
  var self = this;
  var oXhr = new XMLHttpRequest();
  oXhr.open('POST', this.url, true);
  oXhr.onerror = function() {
    if (!self.didCallback) {
      PS_LOG('ERROR making request:' +
             '\n  URL: ' + self.url +
             '\n  Params: ' + self.params);
      self.onError(0);
    }
  };
  oXhr.onreadystatechange = function() {
    if (oXhr.readyState == 4) {
      if (oXhr.status == 200) {
        self.onComplete(oXhr.responseText || '');
      } else {
        PS_LOG('ERROR making request:' +
               '\n  URL: ' + self.url +
               '\n  Params: ' + self.params);
        self.onError(oXhr.status);
      }
      self.didCallback = true;
    }
  };
  oXhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
  oXhr.setRequestHeader('Content-Length', this.params.length);
  oXhr.setRequestHeader('Connection', 'close');
  oXhr.send(this.params);
};
