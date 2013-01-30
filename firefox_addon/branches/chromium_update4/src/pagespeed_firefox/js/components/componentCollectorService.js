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
 *
 * @fileoverview A component that identifies resource requests and binds
 * to the source window.
 *
 * ComponentCollectorService implements a Content Policy manager to
 * listen for external page components. shouldLoad is called
 * synchronously before each resource is requested (whether from cache
 * or network).
 *
 * In order to collect redirects it also implements nsIObserverService and
 * listens for http-on-modify-request and http-on-examine-response.
 *
 * @author Kyle Scholz
 * @author Bryan McQuade
 */

Components.utils.import('resource://gre/modules/XPCOMUtils.jsm');

var IComponentCollectorIface = Components.interfaces.IComponentCollector;
var nsISupportsIface = Components.interfaces.nsISupports;
var nsIContentPolicyIface = Components.interfaces.nsIContentPolicy;
var nsIObserverServiceIface = Components.interfaces.nsIObserverService;
var nsIDOMDocumentIface = Components.interfaces.nsIDOMDocument;
var nsIDOMNodeIface = Components.interfaces.nsIDOMNode;
var nsIDOMWindowIface = Components.interfaces.nsIDOMWindow;
var nsIHttpChannelIface = Components.interfaces.nsIHttpChannel;
var nsIDocShellIface = Components.interfaces.nsIDocShell;
var nsIWebNavigationIface = Components.interfaces.nsIWebNavigation;
var nsIInterfaceRequestorIface = Components.interfaces.nsIInterfaceRequestor;

var CONTENT_POLICY = 'content-policy';
var HTTP_ON_MODIFY_REQUEST = 'http-on-modify-request';
var HTTP_ON_EXAMINE_RESPONSE = 'http-on-examine-response';
// The cached response callback was added in FF3.5.
var HTTP_ON_EXAMINE_CACHED_RESPONSE = 'http-on-examine-cached-response';
var HTTP_ON_EXAMINE_MERGED_RESPONSE = 'http-on-examine-merged-response';
var PROFILE_AFTER_CHANGE = 'profile-after-change';

var LOG_MSG_FOUND_CYCLE = 'Found cycle in linked list for ';
var LOG_MSG_NEG_DIFF = 'diff less than zero';

// We consider an entry in the pendingDocs_ map too old, and remove
// it, if it's older than 2 minutes. This allows us to catch most
// redirects, even if the user is on a slow connection with a very
// high RTT and/or the server is taking a long time to respond.
var MAX_AGE_MS = 120000;

function PS_LOG(msg) {
  // We set PS_LOG.enabled below.
  if (!PS_LOG.enabled) return;
  var consoleService = Components.classes['@mozilla.org/consoleservice;1']
      .getService(Components.interfaces.nsIConsoleService);
  consoleService.logStringMessage(msg);
}

/**
 * Check to see if logging is enabled in the Firefox preferences.
 */
function checkLoggingEnabled() {
  try {
    var prefClass = Components.classes['@mozilla.org/preferences-service;1'];
    if (prefClass) {
      var prefService = prefClass.getService(Components.interfaces.nsIPrefBranch);
      if (prefService) {
        PS_LOG.enabled = prefService.getBoolPref(
            'extensions.PageSpeed.enable_console_logging');
      }
    }
  } catch (x) {
    PS_LOG.enabled = false;
  }
}

/**
 * Our setup hook. Called at startup, via the profile-after-change hook.
 */
function initialize() {
  checkLoggingEnabled();
}

/**
 * Turns an nsIURI into a user friendly string with the fragment stripped.
 *
 * @param {nsIURI} uri The URI to format.
 * @return {string} The formatted URI.
 */
function formatURI(uri) {
  // Get a user friendly URL from an nsIURI.
  var formattedURI = [uri.scheme, '://', uri.hostPort, uri.path].join('');

  // Strip URL fragment, i.e. anything after the #.
  return formattedURI.replace(/#.*$/, '');
}

/**
 * @param {nsIDOMWindow} win The window to get a URI from.
 * @return {string} The formatted URI.
 */
function getURIForWindow(win) {
  return win.location.href.replace(/#.*$/, '');
}

/**
 * @param {nsIURI} uri The URI to check.
 * @return {boolean} True if HTTP or HTTPS, false otherwise.
 */
function isHttpUri(uri) {
  if (!uri || !uri.scheme) return false;
  return (uri.scheme == 'http' || uri.scheme == 'https');
}

/**
 * @param {nsIHttpChannel} httpChannel The HTTP channel to get headers
 *     from.
 * @param {boolean} bRequestHeaders If true, get request headers. If
 *     false, get response headers.
 * @return {Object} A map of HTTP headers.
 */
function getHeadersFromChannel(httpChannel, bRequestHeaders) {
  /** @type {nsIHttpHeaderVisitor} */
  var visitor = {
    headers: {},
    /** @this nsIHttpHeaderVisitor */
    visitHeader: function(a, b) {this.headers[a] = b;}
  };
  if (bRequestHeaders) {
    httpChannel.visitRequestHeaders(visitor);
  } else {
    httpChannel.visitResponseHeaders(visitor);
  }

  return visitor.headers;
}

/**
 * Iterate over a components collection, and invoke the specified
 * callback whenever a resource with the specified URI is encountered.
 * @param {Object} components The components object to iterate over.
 * @param {string} uri The URI to match against.
 * @param {Function} callback The callback to invoke.
 * @return {number} The number of components visited.
 */
function visitComponentsWithUri(components, uri, callback) {
  var cnt = 0;
  for (var type in components) {
    for (var u in components[type]) {
      if (u == uri) {
        callback(components, type, uri);
        cnt++;
      }
    }
  }
  return cnt;
}

/**
 * Function that verifies that entries are not too far apart.
 * @param {LinkedHashMap} map The hash map.
 * @param {string} prev The previous key.
 * @param {string} next The next key.
 * @return {boolean} False if too far apart, true otherwise.
 */
function entriesNotTooFarApart(map, prev, next) {
  if (!map.hasEntry(prev) || !map.hasEntry(next)) return false;
  prev = map.getEntry(prev);
  next = map.getEntry(next);
  if (typeof prev.initTime != 'number') return false;
  if (typeof next.initTime != 'number') return false;
  var diff = next.initTime - prev.initTime;
  if (diff < 0) {
    // Error case: bail.
    PS_LOG(LOG_MSG_NEG_DIFF);
    return false;
  }
  return (diff < MAX_AGE_MS);
}

/**
 * Function that verifies that entries are not too old.
 * @param {LinkedHashMap} map The hash map.
 * @param {string} key The key to check.
 * @return {boolean} True if recent enough, false otherwise.
 */
function isEntryRecentEnough(map, key) {
  if (!key || !map) return false;
  var tail = map.getTail(key);
  if (!tail) return false;
  tail = map.getEntry(tail);
  if (typeof tail.initTime != 'number') return false;
  var diff = new Date().getTime() - tail.initTime;
  if (diff < 0) {
    // Error case: bail.
    PS_LOG(LOG_MSG_NEG_DIFF);
    return false;
  }
  return (diff < MAX_AGE_MS);
}

/**
 * Dummy function, used by LinkedHashMap, that always returns true.
 * @return {boolean} True.
 */
function alwaysTrueFn() {
  return true;
}

/**
 * Is the given document in what looks like a friendly iframe? A
 * friendly iframe is an iframe on the same domain as the parent page,
 * without an actual 'src' URL. It can be used to allow the main page
 * to do work in a frame that does not block the main page (e.g. to
 * load blocking third-party ads).
 */
function isLikelyFriendlyIframe(doc) {
  // If the iframe has a 'src' attribute then it's not a friendly
  // iframe.
  if (!doc.defaultView ||
      !doc.defaultView.frameElement ||
      doc.defaultView.frameElement.src) {
    return false;
  }

  // A friendly iframe, that is one without a src URL, will have a
  // document that says its URL is that of the parent document's
  // URL. This isn't really the URL of the document but that's the
  // expected behavior for friendly iframes (documented as part of the
  // HTML5 spec).
  if (!doc.defaultView.parent ||
      !doc.defaultView.parent.document ||
      doc.defaultView.parent.document.URL != doc.URL) {
    return false;
  }
  return true;
}

/**
 * Doubly linked hash map. This class is declared inline in
 * componentCollectorService.js because it is not possible to import
 * other JS files from within a JS-based xpcom component.
 * @param {Function} opt_linkValidationFn The function to use to
 *     determine if links between entries are still valid, or undefined if
 *     links are always valid.
 * @param {Function} opt_keyValidationFn The function to use to
 *     determine if the key of a given entry is still valid, or
 *     undefined if keys are always valid.
 * @constructor
 */
function LinkedHashMap(opt_linkValidationFn, opt_keyValidationFn) {
  this.map_ = {};
  this.validating_ = false;
  this.linkValidationFn_ = opt_linkValidationFn || alwaysTrueFn;
  this.keyValidationFn_ = opt_keyValidationFn || alwaysTrueFn;
}

/**
 * @param {string} key The key to add an entry for.
 * @return {Object} The value object for the new entry.
 */
LinkedHashMap.prototype.addEntry = function(key) {
  if (!key) return null;

  // Make sure we break old links from any adjacent entries, since
  // they were for a different entry and should no longer be linked to
  // this key.
  this.unlinkPrev(key);
  this.unlinkNext(key);

  // Create the new entry, and add it to the map.
  var newEntry = {
    data: {}
  };
  this.map_[key] = newEntry;

  // Return the data object for the new entry.
  return newEntry.data;
};

/**
 * @param {string} key The key to get an entry for.
 * @return {Object} The value object for the entry.
 */
LinkedHashMap.prototype.getEntry = function(key) {
  if (!key) return null;
  if (!this.map_[key]) return null;
  if (!this.validating_) {
    // We're calling out to the validation function. We need to be
    // careful not to trigger an infinite validation loop (we call the
    // vaidation fcn, they call us, etc) so we indicate that we're in
    // the midst of validating to prevent infinite recursion.
    this.validating_ = true;
    if (!this.keyValidationFn_(this, key)) {
      this.removeEntry(key);
      this.validating_ = false;
      return null;
    }
    this.validating_ = false;
  }
  return this.map_[key].data;
};

/**
 * @param {string} key The key to check for.
 * @return {boolean} Whether or not an entry exists for the given key.
 */
LinkedHashMap.prototype.hasEntry = function(key) {
  return this.getEntry(key) != null;
};

/**
 * @param {string} key The key to remove.
 */
LinkedHashMap.prototype.removeEntry = function(key) {
  if (!key) return;
  if (!this.map_[key]) return;
  this.unlinkPrev(key);
  this.unlinkNext(key);
  delete this.map_[key];
};

/**
 * @param {Function} fn The predicate that each entry will be queried
 *     against, to determine if that entry should be removed.
 */
LinkedHashMap.prototype.removeEntriesNotMatching = function(fn) {
  var toRemove = [];
  for (var key in this.map_) {
    // If the key itself is no longer valid, don't bother asking the
    // validation function.
    if (!this.hasEntry(key)) continue;
    if (!fn(this, key)) {
      toRemove.push(key);
    }
  }

  for (var i = 0, len = toRemove.length; i < len; i++) {
    var key = toRemove[i];
    if (!this.hasEntry(key)) continue;
    this.removeEntry(key);
  }
};

/**
 * @param {string} key The key to get the next key for.
 * @return {string?} The next key, if any.
 */
LinkedHashMap.prototype.getNext = function(key) {
  if (!this.hasEntry(key)) return null;
  var next = this.map_[key].next;
  if (!next) return null;
  if (!this.linkValid_(key, next)) {
    this.unlinkNext(key);
    return null;
  }
  return next;
};

/**
 * @param {string} key The key to get the previous key for.
 * @return {string?} The previous key, if any.
 */
LinkedHashMap.prototype.getPrev = function(key) {
  if (!this.hasEntry(key)) return null;
  var prev = this.map_[key].prev;
  if (!prev) return null;
  if (!this.linkValid_(prev, key)) {
    this.unlinkPrev(key);
    return null;
  }
  return prev;
};

/**
 * @param {string} key The key to get the tail key for.
 * @return {string?} The last key in the chain for this key, or null
 *     if there is no entry for the key.
 */
LinkedHashMap.prototype.getTail = function(key) {
  if (!this.hasEntry(key)) return null;
  var orig = key;
  for (var next = this.getNext(key);
       next != null;
       key = next, next = this.getNext(next)) {
    if (next == orig) {
      PS_LOG(LOG_MSG_FOUND_CYCLE + orig);
      return null;
    }
  }
  return key;
};

/**
 * @param {string} key The key to get the head key for.
 * @return {string?} The first key in the chain for this key, or null
 *     if there is no entry for the key.
 */
LinkedHashMap.prototype.getHead = function(key) {
  if (!this.hasEntry(key)) return null;
  var orig = key;
  for (var prev = this.getPrev(key);
       prev != null;
       key = prev, prev = this.getPrev(prev)) {
    if (prev == orig) {
      PS_LOG(LOG_MSG_FOUND_CYCLE + orig);
      return null;
    }
  }
  return key;
};

/**
 * Link the two keys in the map.
 * @param {string} key The next key after prevKey in the map.
 * @param {string} prevKey The previous key after key in the map.
 */
LinkedHashMap.prototype.linkPrev = function(key, prevKey) {
  if (prevKey == key ||
      !this.hasEntry(key) ||
      !this.hasEntry(prevKey)) return;
  if (this.linkValid_(prevKey, key)) {
    this.map_[key].prev = prevKey;
    this.map_[prevKey].next = key;
  }
};

/**
 * Link the two keys in the map.
 * @param {string} key The previous key before nextKey in the map.
 * @param {string} nextKey The next key after key in the map.
 */
LinkedHashMap.prototype.linkNext = function(key, nextKey) {
  if (nextKey == key ||
      !this.hasEntry(key) ||
      !this.hasEntry(nextKey)) return;
  if (this.linkValid_(key, nextKey)) {
    this.map_[key].next = nextKey;
    this.map_[nextKey].prev = key;
  }
};

/**
 * @param {string} key The key to unlink from its previous key, if any.
 */
LinkedHashMap.prototype.unlinkPrev = function(key) {
  if (!key) return;
  var entry = this.map_[key];
  if (!entry) return;
  var prev = entry.prev;
  delete entry.prev;
  if (prev && this.map_[prev]) {
    delete this.map_[prev].next;
  }
};

/**
 * @param {string} key The key to unlink from its next key, if any.
 */
LinkedHashMap.prototype.unlinkNext = function(key) {
  if (!key) return;
  var entry = this.map_[key];
  if (!entry) return;
  var next = entry.next;
  delete entry.next;
  if (next && this.map_[next]) {
    delete this.map_[next].prev;
  }
};

/**
 * @param {string} key The key to get entries for.
 * @return {Array.<string>} All of the keys from this key to the tail
 *     of the chain for that key.
 */
LinkedHashMap.prototype.getAllToTail = function(key) {
  var entries = [];
  if (!this.hasEntry(key)) return entries;
  var orig = key;
  while (key) {
    entries.push(key);
    key = this.getNext(key);
    if (key == orig) {
      PS_LOG(LOG_MSG_FOUND_CYCLE + key);
      return entries;
    }
  }
  return entries;
};

/**
 * @param {string} key The key to get entries for.
 * @return {Array.<string>} All of the keys from this key to the head
 *     of the chain for that key.
 */
LinkedHashMap.prototype.getAllToHead = function(key) {
  var entries = [];
  if (!this.hasEntry(key)) return entries;
  var orig = key;
  while (key) {
    entries.push(key);
    key = this.getPrev(key);
    if (key == orig) {
      PS_LOG(LOG_MSG_FOUND_CYCLE + key);
      return entries;
    }
  }
  return entries;
};

/**
 * @param {string} prevKey The previous key to test for a valid link with.
 * @param {string} key The next key to check for a valid link with.
 * @return {boolean} True if the keys are valid to link together,
 *     false otherwise.
 * @private
 */
LinkedHashMap.prototype.linkValid_ = function(prevKey, key) {
  if (!this.validating_) {
    this.validating_ = true;
    var result = this.linkValidationFn_(this, prevKey, key);
    this.validating_ = false;
    return result;
  }

  return true;
};

/**
 * ComponentCollector Constructor.
 * @constructor
 */
function ComponentCollectorService() {
  // Exposing the wrappedJSObject property in this way allows callers
  // to access the native JS object, not just the xpconnect-wrapped
  // object. This allows access to the full ComponentCollectorService
  // API, not just the subset of the API exported via the
  // IComponentCollector interface.
  this.wrappedJSObject = this;

  // Register the observer.
  var observerService = Components.classes['@mozilla.org/observer-service;1']
                                  .getService(nsIObserverServiceIface);
  observerService.addObserver(this, HTTP_ON_MODIFY_REQUEST, false);
  observerService.addObserver(this, HTTP_ON_EXAMINE_RESPONSE, false);
  observerService.addObserver(this, HTTP_ON_EXAMINE_CACHED_RESPONSE, false);
  observerService.addObserver(this, HTTP_ON_EXAMINE_MERGED_RESPONSE, false);

  // Install our progress listener. We must hold a strong reference to
  // the progress listener, since it implements the
  // nsISupportsWeakReference interface.
  this.progressListener_ = this.installProgressListener();

  // pendingDocs_ is a map and doubly-linked list that tracks
  // documents that are pending being transitioned to, and any
  // HTTP/JS/meta redirects to those documents. The keys in the map
  // are the URLs of the documents and/or redirects, and the values
  // are Objects which contain information about the resources.
  //
  // NOTE: This is currently a process-global map where the key is the
  // URL. This makes it possible for different tabs to step on each
  // others' data if the same URL is loaded in different tabs. A
  // better solution would be to keep a tab-local map, or at least to
  // keep some per-tab context as part of the key, so we could
  // disambiguate requests for the same URL coming from different
  // tabs. This is possible by extracting the webProgress object from
  // the opt_context argument of TYPE_DOCUMENT requests. webProgress
  // is a tab-local object. It is also possible to get the webProgress
  // for a nsIHttpChannel instance (using the nsIInterfaceRequestor
  // API), and it's possible to get the DOM window from the
  // webProgress object (webProgress.DOMWindow), so a future
  // improvement would use the webProgress object to prevent map
  // collisions between tabs.
  this.pendingDocs_ = new LinkedHashMap(
      entriesNotTooFarApart, isEntryRecentEnough);

  // Our array of callbacks that gets invoked whenever a new window is
  // created.
  this.newWindowCallbacks_ = [];
}

// Component Types
// See nsIContentPolicy.idl
ComponentCollectorService.TYPE_OTHER = 1;
ComponentCollectorService.TYPE_SCRIPT = 2;
ComponentCollectorService.TYPE_IMAGE = 3;
ComponentCollectorService.TYPE_CSSIMAGE = 31;  // Custom type
ComponentCollectorService.TYPE_FAVICON = 32;  // Custom type
ComponentCollectorService.TYPE_STYLESHEET = 4;
ComponentCollectorService.TYPE_OBJECT = 5;
ComponentCollectorService.TYPE_DOCUMENT = 6;
ComponentCollectorService.TYPE_SUBDOCUMENT = 7;
ComponentCollectorService.TYPE_REDIRECT = 71;  // Custom type
ComponentCollectorService.TYPE_REFRESH = 8;  // Unused
ComponentCollectorService.TYPE_XBL = 9;  // Unused
ComponentCollectorService.TYPE_PING = 10;  // Unused
ComponentCollectorService.TYPE_XMLHTTPREQUEST = 11;
ComponentCollectorService.TYPE_OBJECT_SUBREQUEST = 12;

// Component Type Names
ComponentCollectorService.type = {
  1:  'other',  // Typically XHR.
  11: 'xhr',  // XHR.
  12: 'subobject',  // e.g. Flash request.
  2:  'js',
  3:  'image',
  31: 'cssimage',
  32: 'favicon',
  4:  'css',
  5:  'object',  // e.g. Flash.
  6:  'doc',
  7:  'iframe',
  71: 'redirect'
};

ComponentCollectorService.prototype.classID =
    Components.ID('{7bc5b600-1302-11dd-bd0b-0800200c9a66}');

ComponentCollectorService.prototype.classDescription =
    'Page Speed component collector';

ComponentCollectorService.prototype.contractID =
    '@code.google.com/p/page-speed/ComponentCollectorService;1',

// Mozilla 1.9.x requires _xpcom_categories to perform category
// registration (in Mozilla 2 this is handled in chrome.manifest).
ComponentCollectorService.prototype._xpcom_categories =
    [ { category: PROFILE_AFTER_CHANGE },
      { category: CONTENT_POLICY }
    ];

ComponentCollectorService.prototype.installProgressListener = function() {
  var progressService = Components.classes['@mozilla.org/docloaderservice;1'].
      getService(Components.interfaces.nsIWebProgress);
  var listener = new ProgressListenerHookInstaller(this);
  progressService.addProgressListener(
      listener, Components.interfaces.nsIWebProgress.NOTIFY_STATE_REQUEST);
  return listener;
};

// nsIPolicy
ComponentCollectorService.prototype.shouldProcess = function(contentType,
                                                             contentLocation,
                                                             requestOrigin,
                                                             context,
                                                             mimeType,
                                                             extra) {
  // NOTE: This method is no longer invoked by Firefox (as of FF3),
  // but we need to implement it to satisfy the nsIContentPolicy
  // interface.
  return nsIContentPolicyIface.ACCEPT;
};

/**
 * ShouldLoad will be called before loading the resource at contentLocation
 * to determine whether to start the load at all.
 *
 * @param {number} contentType The type of content being tested. This will be
 *     one of the TYPE_* constants.
 * @param {nsIURI} contentLocation The location of the
 *     content being checked; must not be null.
 * @param {nsIURI} opt_requestOrigin The location
 *     of the resource that initiated this load request; can be null if
 *     inapplicable.
 * @param {nsISupports} opt_context The nsIDOMNode
 *     that initiated the request, or something that can QI to
 *     one of those; can be null if inapplicable.
 * @param {string} opt_mimeTypeGuess A guess for the requested
 *     content's MIME type, based on information available to the request
 *     initiator (e.g., an OBJECT's type attribute); does not reliably reflect
 *     the actual MIME type of the requested content. NOT AVAILABLE IN FF1.5.
 * @param {Object} opt_extra An argument, pass-through for non-Gecko
 *     callers to pass extra data to callees. NOT AVAILABLE IN FF1.5.
 * @return {number} ACCEPT or REJECT_*.
 */
ComponentCollectorService.prototype.shouldLoad = function(contentType,
                                                          contentLocation,
                                                          opt_requestOrigin,
                                                          opt_context,
                                                          opt_mimeTypeGuess,
                                                          opt_extra) {
  try {
    this.shouldLoadImpl(
        contentType,
        contentLocation,
        opt_requestOrigin,
        opt_context,
        opt_mimeTypeGuess,
        opt_extra);
  } catch (e) {
    PS_LOG('ComponentCollectorService caught exception in shouldLoadImpl: ' +
           e);
  }
  return nsIContentPolicyIface.ACCEPT;
};

/**
 * This method identifies components and associates them with parent window.
 *
 * @param {number} contentType The type of content being tested. This will be
 *     one of the TYPE_* constants.
 * @param {nsIURI} contentLocation The location of the
 *     content being checked; must not be null.
 * @param {nsIURI} opt_requestOrigin The location
 *     of the resource that initiated this load request; can be null if
 *     inapplicable.
 * @param {nsISupports} opt_context The nsIDOMNode
 *     that initiated the request, or something that can QI to
 *     one of those; can be null if inapplicable.
 * @param {string} opt_mimeTypeGuess A guess for the requested
 *     content's MIME type, based on information available to the request
 *     initiator (e.g., an OBJECT's type attribute); does not reliably reflect
 *     the actual MIME type of the requested content. NOT AVAILABLE IN FF1.5.
 * @param {Object} opt_extra An argument, pass-through for non-Gecko
 *     callers to pass extra data to callees. NOT AVAILABLE IN FF1.5.
 */
ComponentCollectorService.prototype.shouldLoadImpl = function(contentType,
                                                              contentLocation,
                                                              opt_requestOrigin,
                                                              opt_context,
                                                              opt_mimeTypeGuess,
                                                              opt_extra) {
  if (!opt_context || !isHttpUri(contentLocation)) {
    return;
  }

  // Get a user friendly URL from an nsIURI.
  var componentLocation = formatURI(contentLocation);

  if (!(opt_context instanceof nsIDOMNodeIface)) {
    PS_LOG('Context is not an nsIDOMNode for ' + componentLocation);
    return;
  }
  var node = opt_context.QueryInterface(nsIDOMNodeIface);

  // 1. Get the actual content type for the resource.
  contentType = this.getActualContentType(contentType, node);

  if (contentType == ComponentCollectorService.TYPE_DOCUMENT) {
    // TYPE_DOCUMENT is a special case: we receive calls for
    // TYPE_DOCUMENT *before* the transition to that page has begun,
    // so the associated document and window refer to the document
    // that's currently loaded in the browser, not the document we're
    // transitioning to. Thus, we don't process TYPE_DOCUMENTs
    // here in the same way we process other resources. Instead, we
    // lazily populate the document field for the components structure
    // the first time we encounter a subresource on that page (see
    // getWindowComponents()).
    this.createDocumentEntry(componentLocation);
    return;
  }

  if (opt_requestOrigin &&
      opt_requestOrigin.scheme &&
      !isHttpUri(opt_requestOrigin)) {
    // Requests originating from non-http(s) schemes (e.g. chrome://)
    // should not be included. This happens in Firefox2, but not in
    // Firefox3.
    return;
  }

  this.handleResource(contentType, componentLocation, node);
};

/**
 * Handle a resource loaded by a page.
 * @param {number} contentType The type of content being tested. This
 *     will be one of the TYPE_* constants.
 * @param {string} componentLocation The location of the resource being
 *     checked.
 * @param {nsIDOMNode} node The nsIDOMNode that initiated the request.
 */
ComponentCollectorService.prototype.handleResource = function(
    contentType,
    componentLocation,
    node) {
  // Identify the containing document for the resource.
  var doc;
  try {
    doc = this.getDocumentForContentType(contentType, node);
  } catch (e) {
    PS_LOG('ERROR matching contentType ' + contentType +
        ' for ' + componentLocation);
    return;
  }

  if (!doc || !doc.defaultView) {
    PS_LOG('ERROR getting window for ' + componentLocation);
    return;
  }

  var win = doc.defaultView.top;

  // Bind Resource->Window
  if (win.wrappedJSObject) {
    // We store weak references to the DOM elements, so they can be
    // cleared if the node is removed from the DOM.
    var weakNode = Components.utils.getWeakReference(node);
    var typeUrlObj = this.addWindowComponent(
        win, contentType, componentLocation, weakNode);
    if (typeUrlObj) {
      if (!typeUrlObj.requestTime) {
        typeUrlObj.requestTime = (new Date()).getTime();
      }
    }
  } else {
    PS_LOG('ERROR binding resource to window: ' +
           contentType + ' ' + componentLocation);
  }
};

/**
 * Get the document associated with the given content type and node.
 * @param {number} contentType The type of content being tested. This will be
 *     one of the TYPE_* constants.
 * @param {!nsIDOMNode} node The nsIDOMNode that initiated the
 *     request.
 * @return {nsIDOMDocument} The document associated with the given
 *     content type and node.
 */
ComponentCollectorService.prototype.getDocumentForContentType = function(
    contentType, node) {
  if (node == null) {
    return null;
  }

  // First we handle some special cases for specific resource types.
  if (contentType == ComponentCollectorService.TYPE_DOCUMENT ||
      contentType == ComponentCollectorService.TYPE_SUBDOCUMENT) {
    if (node.contentDocument &&
        node.contentDocument.defaultView) {
      return node.contentDocument;
    }
  }

  if (contentType == ComponentCollectorService.TYPE_OTHER) {
    // Sent in FF2. TYPE_OTHER does not appear to be sent in FF3.
    PS_LOG('Received unexpected TYPE_OTHER.');
    if (!node.ownerDocument) {
      if (node.documentElement &&
          node.documentElement.ownerDocument &&
          node.documentElement.ownerDocument.defaultView) {
        return node.documentElement.ownerDocument;
      }
    }
  }

  // If none of the specific types matched, use the default cases. For
  // some reason it is necessary to perform the test for
  // node.ownerDocument before the test for
  // node.defaultView. Performing the tests in the other order causes
  // flash embedded using JavaScript (swfobject) to fail to load.
  if (node.ownerDocument &&
      node.ownerDocument.defaultView) {
    // Standard DOM Node.
    return node.ownerDocument;
  }

  if (node.defaultView) {
    // The node is itself a document.
    return node;
  }

  return null;
};

/**
 * Get the actual content type for the given content type and
 * node. In some cases, the content type passed to us from the
 * content policy manager doesn't reflect the content type that we
 * would assign to the resource. This method maps from the content
 * type presented by the content policy manager to the actual content
 * type that we assign to the resource.
 * @param {number} contentType The type of content being tested. This will be
 *     one of the TYPE_* constants.
 * @param {!nsIDOMNode} node The nsIDOMNode that initiated the
 *     request.
 * @return {number} the actual content type, as determined by the
 *     node.
 */
ComponentCollectorService.prototype.getActualContentType = function(
    contentType, node) {
  switch (contentType) {
    case ComponentCollectorService.TYPE_IMAGE:
      // Need to distinguish between favicon, CSS background-image,
      // and HTML img tag.
      if (node.nodeName == 'LINK' &&
          node.rel &&
          // Handles both 'icon' and 'shortcut icon'
          node.rel.search(/icon/i) != -1) {
        return ComponentCollectorService.TYPE_FAVICON;
      } else if (node.defaultView && node.defaultView.document) {
        return ComponentCollectorService.TYPE_CSSIMAGE;
      } else {
        return ComponentCollectorService.TYPE_IMAGE;
      }
    default:
      // All non-image content types are correct as received from the
      // content policy service.
      return contentType;
  }
};

/**
 * Create an entry for the given document in the pending documents
 * map.
 * @param {string} documentUrl The URL of the document.
 */
ComponentCollectorService.prototype.createDocumentEntry = function(
    documentUrl, opt_redirectFromUrl) {
  var entry = this.pendingDocs_.addEntry(documentUrl);
  entry.initTime = new Date().getTime();
  entry.resourceProperties = { requestTime: entry.initTime };
};

/**
 * Observe will be called when there is a notification for the topic.
 * This assumes that the object implementing this interface has been
 * registered with an observer service such as the nsIObserverService.
 *
 * If you expect multiple topics/subjects, the impl is responsible for
 * filtering.
 *
 * You should not modify, add, remove, or enumerate notifications in the
 * implemention of observe.
 *
 * @param {nsISupports} aSubject Notification specific
 *     interface pointer.
 * @param {string} aTopic The notification topic or subject.
 * @param {string?} aData Notification specific wide string. Subject event.
 */
ComponentCollectorService.prototype.observe = function(aSubject,
                                                       aTopic,
                                                       aData) {
  if (PROFILE_AFTER_CHANGE == aTopic) {
    // We receive this callback at startup.
    initialize();
    return;
  }
  if (!(aSubject instanceof nsIHttpChannelIface)) {
    PS_LOG('Got non-http channel in observer callback.');
    return;
  }

  var httpChannel = aSubject.QueryInterface(nsIHttpChannelIface);

  if (HTTP_ON_MODIFY_REQUEST == aTopic) {
    this.observeRequest(httpChannel);
  } else if (HTTP_ON_EXAMINE_RESPONSE == aTopic) {
    this.observeResponse(httpChannel, false, false);
  } else if (HTTP_ON_EXAMINE_CACHED_RESPONSE == aTopic) {
    this.observeResponse(httpChannel, true, false);
  } else if (HTTP_ON_EXAMINE_MERGED_RESPONSE == aTopic) {
    this.observeResponse(httpChannel, false, true);
  }
};

/**
 * Observe an HTTP request.
 *
 * @param {nsIHttpChannel} httpChannel The http channel that is being
 *     requested.
 */
ComponentCollectorService.prototype.observeRequest = function(httpChannel) {
  var win = this.getDomWindowForRequest(httpChannel);
  if (!win || !win.wrappedJSObject) {
    // This is not a request that originated from a DOM node (might be
    // a cert validation request, or a phishing protection request,
    // for instance). So we ignore it.
    return;
  }

  var prevWindowURI = getURIForWindow(win);
  var toURI = formatURI(httpChannel.URI);

  // TODO: consider handling JS/meta redirects for iframes as well, if
  // it's possible to redirect iframes in this way.
  if (this.isJSOrMetaRedirect(httpChannel, prevWindowURI)) {
    // This URL is associated with a document, and it appears that we
    // are in the midst of a JS or meta redirect. Update the pending
    // docs map to indicate there was a redirect.
    this.pendingDocs_.linkPrev(toURI, prevWindowURI);
  } else {
    var fromURI = formatURI(httpChannel.originalURI);
    if (fromURI && toURI && fromURI != toURI) {
      this.observeRedirect(httpChannel, win, fromURI, toURI);
    }
  }

  // Attach the request headers to the associated component entry.
  var srcObject = {
    requestHeaders: getHeadersFromChannel(httpChannel, true),
    requestMethod: httpChannel.requestMethod,
  };

  this.bindToPendingDocumentOrComponent(srcObject, win, toURI);
};

/**
 * Observe an HTTP redirect.
 *
 * @param {nsIHttpChannel} httpChannel The http channel that is being
 *     redirected.
 * @param {nsIDOMWindow} win The window containing the redirected
 *     resource.
 * @param {string} fromURI The URI being redirected from.
 * @param {string} toURI The URI being redirected to.
 */
ComponentCollectorService.prototype.observeRedirect = function(
    httpChannel, win, fromURI, toURI) {
  // NOTE: fromURI always points to the first URL in a chain of
  // redirects. If A redirects to B and B redirects to C, the
  // nsIHttpChannel with URI == C will have an originalURI == A. Thus,
  // we have to seek forward in our redirect tracking objects to find
  // the last known entry in the chain starting at fromURI.
  var finalURI = this.getFinalRedirectTarget_(win, fromURI);
  if (finalURI == null || finalURI == toURI) {
    PS_LOG('getFinalRedirectTarget_ returned bad URI: ' +
           fromURI + ' > ' + toURI);
    return;
  }

  if (this.pendingDocs_.hasEntry(fromURI)) {
    // The main document is being HTTP redirected. Update the
    // pending documents map to indicate this.
    this.createDocumentEntry(toURI);
    this.pendingDocs_.linkPrev(toURI, finalURI);
  } else {
    var typeUrlObj = this.addWindowComponent(
        win,
        ComponentCollectorService.TYPE_REDIRECT,
        finalURI,
        toURI,
        true);
    if (typeUrlObj) {
      if (!typeUrlObj.requestTime) {
        typeUrlObj.requestTime = (new Date()).getTime();
      }
    }
  }
};

/**
 * Get the last redirect in a chain of redirects. For instance, if
 * fromURI is known to redirect to A, and A is known to redirect to B,
 * then this method would return B. If fromURI does not redirect, this
 * method returns fromURI.
 *
 * @param {nsIDOMWindow} win The window containing the redirected
 *     resource.
 * @param {string} fromURI The URI being redirected from.
 * @return {string?} The last redirect in the chain of redirects, or
 * null if there was an error.
 * @private
 */
ComponentCollectorService.prototype.getFinalRedirectTarget_ = function(
    win, fromURI) {
  // First consult the pendingDocs_ map.
  if (this.pendingDocs_.hasEntry(fromURI)) {
    var tail = this.pendingDocs_.getTail(fromURI);
    if (tail == null) {
      // Not a redirect.
      return fromURI;
    }
    return tail;
  }

  // If the pendingDocs_ map didn't have a match, iterate through the
  // components object to follow the redirect chain.

  // Loop through the components map until we reach the end of a
  // redirect chain, or we encounter a redirect loop.
  var urisEncountered = {};
  while (true) {
    if (urisEncountered[fromURI]) {
      PS_LOG('Infinite redirect loop for ' + fromURI);
      return null;
    }
    urisEncountered[fromURI] = true;

    var component = this.getWindowComponentOfType(
      win,
      ComponentCollectorService.TYPE_REDIRECT,
      fromURI);
    if (component == null ||
        component.elements == null ||
        component.elements.length <= 0) {
      // There isn't a valid redirect entry for this URL, so stop
      // iterating.
      break;
    }
    if (component.elements.length > 1) {
      PS_LOG(fromURI +
             ' has multiple redirect destinations. Using ' +
             component.elements[0]);
    }

    fromURI = component.elements[0];
  }

  return fromURI;
};

/**
 * @param {number} responseCode The response code to check.
 * @return {boolean} Whether or not a response with the given response
 *     status code is allowed to have a response body, according to
 *     the HTTP/1.1 RFC, section 4.3.
 */
function responseCodeCanHaveBody(responseCode) {
  var responseCodeClass = Math.floor(responseCode / 100);
  if (responseCode == 204 ||
      responseCodeClass == 1) {
    // NOTE: we do not include code 304 in this computation because
    // the 304 doesn't have a body at the network level, but when a
    // 304 is generated, Firefox reads the resource contents from its
    // local cache. Thus, though the HTTP response on the wire does
    // not have a body when a 304 is sent, an nsIRequest will have a
    // body when a 304 is sent.
    return false;
  }

  if (responseCode == 301 || responseCode == 302) {
    // HTTP redirects are allowed to have bodies according to the HTTP
    // RFC. However, Firefox does not store the body of HTTP
    // redirects.
    return false;
  }

  return true;
}

/**
 * Observe an HTTP response.
 *
 * @param {nsIHttpChannel} httpChannel The http channel that is
 *     receiving a response.
 * @param {boolean} isCachedResponse True if the response is
 *     served from cache, false otherwise.
 * @param {boolean} isMergedResponse True if part of the response is
 *     merged from cache, false otherwise.
 */
ComponentCollectorService.prototype.observeResponse = function(
    httpChannel, isCachedResponse, isMergedResponse) {
  var win = this.getDomWindowForRequest(httpChannel);
  if (!win || !win.wrappedJSObject) {
    // This is not a response that originated from a DOM node (might be
    // a cert validation request, or a phishing protection request,
    // for instance). So we ignore it.
    return;
  }

  var responseCode = httpChannel.responseStatus;

  // In some cases, a network response needs to be merged with cached
  // data in order to generate a full response. In those cases, two
  // responses come through this codepath for a single response. The
  // initial (unmerged) response contains the actual response code,
  // and the merged response contains the full response headers, but
  // the wrong response code. Thus, we have to selectively extract the
  // appropriate data from the unmerged and merged responses in cases
  // where a merged response is generated.

  // A merged response is coming if this is not already the merged
  // response, and it has a 206 or 304 response code. See
  // nsHttpChannel.cpp for specific details about when a merged
  // response is generated.
  var mergedResponseComing =
      !isMergedResponse && (responseCode == 206 || responseCode == 304);

  // Attach the response headers and status code to the associated
  // component entry.
  var srcObject = {};
  srcObject.fromCache = isCachedResponse;
  if (!mergedResponseComing) {
    // Only use the responseHeaders if this response isn't going to be
    // merged with cached data. In that case, wait for the merged
    // response before recording headers.
    srcObject.responseHeaders = getHeadersFromChannel(httpChannel, false);
  }
  if (!isMergedResponse) {
    // The merged response's response code comes from the cache. We
    // want the response code for the actual HTTP response.
    srcObject.responseCode = responseCode;
  }

  if (!mergedResponseComing && responseCodeCanHaveBody(responseCode)) {
    if (httpChannel.isNoStoreResponse()) {
      // This resource isn't going to get written to the cache, so we
      // need to cache the contents in memory. The HTTP RFC says it is
      // ok to keep a 'no-store' response in volatile memory, as long as
      // it is not kept in memory too long. We release our handle
      // to no-store resources as soon as the user navigates away
      // from the page that contains that resource.
      var storageStream = this.observeChannelData(httpChannel);
      if (storageStream) {
        srcObject.responseStorageStream = storageStream;
      }
    }

    if (httpChannel.contentLength > 0) {
      srcObject.responseContentLength = httpChannel.contentLength;
    }
  }

  var toURI = formatURI(httpChannel.URI);
  this.bindToPendingDocumentOrComponent(srcObject, win, toURI);
};

/**
 * Hook into the response stream and copy the response data into a
 * local buffer that we can read from later.
 * @param {nsIHttpChannel} httpChannel The channel to get data from.
 * @return {nsIStorageStream} The storage stream that accumulates the
 *     stream's data.
 */
ComponentCollectorService.prototype.observeChannelData = function(httpChannel) {
  if (!Components.interfaces.nsITraceableChannel) {
    // nsITraceableChannel was added in FF3.0.4. Double check that
    // it's available. If it's not, don't try to bind to the channel.
    return null;
  }

  // Create a storage stream. A storage stream is basically a buffer
  // exposed as an output stream, which we can also create any number
  // of input streams for. We will write a copy of the channel data
  // to this object, which we can read from later.
  var ss = Components.classes['@mozilla.org/storagestream;1']
      .createInstance(Components.interfaces.nsIStorageStream);
  // Buffer in 16K chunks, up to 3MB. If we exceed the buffer size,
  // our in-memory copy just gets truncated. The client can check for
  // truncation by inspecting the writeInProgress field of the
  // nsIStorageStream.
  ss.init(0x4000, 0x300000, null);

  // Create a stream listener tee. This object writes all incoming
  // data to the nextListener and to the storage stream's output
  // stream.
  var tee = Components.classes['@mozilla.org/network/stream-listener-tee;1']
      .createInstance(Components.interfaces.nsIStreamListenerTee);

  // Wire the http channel up to our buffer.
  var traceableChannel = httpChannel.QueryInterface(
      Components.interfaces.nsITraceableChannel);
  var nextListener = traceableChannel.setNewListener(tee);
  tee.init(nextListener, ss.getOutputStream(0));
  return ss;
};

/**
 * Bind the properties in the srcObject to the appropriate entry in
 * the component map or the pending document map.
 * @param {Object} srcObject The object to copy properties from.
 * @param {nsIDOMWindow} win The window associated with the component.
 * @param {string} toURI The URI of the resource.
 */
ComponentCollectorService.prototype.bindToPendingDocumentOrComponent = function(
    srcObject, win, toURI) {
  // First check to see if the URI is associated with a document in
  // the pending documents map.
  if (this.pendingDocs_.hasEntry(toURI)) {
    var dest = this.pendingDocs_.getEntry(toURI).resourceProperties;
    mergeProperties(srcObject, dest, dest);
    return;
  }

  // If not in the pending docs map, we expect to find a matching
  // component.
  var components = this.getWindowComponents(win, true);
  if (!components) return;

  function bindResponseData(components, type, uri) {
    var dest = components[type][uri];
    mergeProperties(srcObject, dest, dest);
  }
  var numComponentsBound =
      visitComponentsWithUri(components, toURI, bindResponseData);
  if (!numComponentsBound) {
    PS_LOG('Unable to bind response data for ' + toURI);
  }
};

/**
 * Get the interface instance associated with the request.
 * @param {nsIRequest} request The request.
 * @param {Object} iface The interface instance to retrieve.
 * @return {nsIDOMWindow} The associated interface instance, or null.
 */
ComponentCollectorService.prototype.getInterfaceForRequest = function(
    request, iface) {
  /**
   * @param {nsISupports} supports The instance to retrieve the interface from.
   * @return {nsISupports} The associated interface instance, or null.
   */
  function getIface(supports) {
    if (supports == null) {
      return null;
    }

    if (!(supports instanceof nsIInterfaceRequestorIface)) {
      return null;
    }

    var callbacks = supports.QueryInterface(nsIInterfaceRequestorIface);

    try {
      return callbacks.getInterface(iface);
    } catch (e) {
      return null;
    }
  }

  var obj = getIface(request.notificationCallbacks);
  if (!obj && request.loadGroup != null) {
    // If we were unable to get the interface from the request
    // itself, try to get one from the request's load group (required
    // for XHRs).
    obj = getIface(request.loadGroup.groupObserver);
  }
  return obj;
};

/**
 * Get the DOM window associated with the request.
 * @param {nsIRequest} request The request.
 * @return {nsIDOMWindow} The DOM window associated with the request.
 */
ComponentCollectorService.prototype.getDomWindowForRequest = function(
    request) {
  return this.getInterfaceForRequest(request, nsIDOMWindowIface);
};

/**
 * Get the doc shell associated with the request.
 * @param {nsIRequest} request The request.
 * @return {nsIDocShell} The doc shell associated with the request.
 */
ComponentCollectorService.prototype.getDocShellForRequest = function(
    request) {
  return this.getInterfaceForRequest(request, nsIDocShellIface);
};

/**
 * Bind the entries in the pending documents map to the context on the
 *     current window.
 * @param {string} documentUrl the URL of the document to create an
 *     entry for.
 * @param {nsIDOMWindow} win The window to bind to.
 */
ComponentCollectorService.prototype.bindPendingDocumentEntries = function(
    documentUrl, win) {
  if (!this.pendingDocs_.hasEntry(documentUrl)) {
    return;
  }
  var components = this.getWindowComponents(win);

  var srcObject = this.pendingDocs_.getEntry(documentUrl).resourceProperties;
  var doc = components.doc[documentUrl];
  mergeProperties(srcObject, doc, doc);
  var earliestStartTime = doc.requestTime;
  if (earliestStartTime <= 0) {
    PS_LOG('Found invalid doc.requestTime ' + earliestStartTime);
  }

  var redirects = this.pendingDocs_.getAllToHead(documentUrl);
  // Walk the array in reverse order, since the array is ordered from
  // most recent to least recent, but we want to add them in the order
  // they were created.
  for (var i = redirects.length - 1; i >= 0; i--) {
    var toURL = redirects[i];
    var fromURL = this.pendingDocs_.getPrev(toURL);
    if (toURL && fromURL) {
      var fromEntry = this.pendingDocs_.getEntry(fromURL);
      var redirectStartTime = fromEntry.initTime;
      if (redirectStartTime > 0 && redirectStartTime < earliestStartTime) {
        earliestStartTime = redirectStartTime;
      } else {
        PS_LOG('Found out of order or invalid redirect time ' +
               redirectStartTime);
      }
      this.addWindowComponent(
          win,
          ComponentCollectorService.TYPE_REDIRECT,
          fromURL,
          toURL,
          true);

      // Bind each redirect's properties.
      var srcObject = fromEntry.resourceProperties;
      var redirect = components.redirect[fromURL];
      mergeProperties(srcObject, redirect, redirect);
    }
  }
  for (var i = 0, len = redirects.length; i < len; ++i) {
    this.pendingDocs_.removeEntry(redirects[i]);
  }
  if (earliestStartTime > 0) {
    doc.pageLoadStartTime = earliestStartTime;
  } else {
    PS_LOG('invalid earliestStartTime ' + earliestStartTime);
  }
};

/**
 * @param {nsIHttpChannel} httpChannel The http channel to inspect.
 * @param {string} possibleFromURI The URI that we might have been
 *     JS/meta redirected from.
 */
ComponentCollectorService.prototype.isJSOrMetaRedirect = function(
    httpChannel, possibleFromURI) {
  // The load flags for the doc shell will contain
  // LOAD_FLAGS_REPLACE_HISTORY when a page is navigated from before
  // the onload event has triggered, or when the navigation was due to
  // a meta refresh. Note that meta refreshes are asynchronous, so
  // checking the onload state to detect meta refresh does not work.
  var shell = this.getDocShellForRequest(httpChannel);
  if (!shell) return false;

  var possibleToURI = formatURI(httpChannel.URI);
  if (possibleToURI == possibleFromURI ||
      !this.pendingDocs_.hasEntry(possibleFromURI) ||
      !this.pendingDocs_.hasEntry(possibleToURI) ||
      !entriesNotTooFarApart(
          this.pendingDocs_, possibleFromURI, possibleToURI)) {
    return false;
  }

  // The loadType is encoded as 2 values: a load command (low 2
  // bytes) and load flags (high 2 bytes). We extract the values
  // from the loadType field below:
  var loadType = shell.loadType;
  var loadCommand = loadType & 0xffff;
  var loadFlags = loadType >> 16;

  if (loadCommand == nsIDocShellIface.LOAD_CMD_NORMAL &&
      (loadFlags & nsIWebNavigationIface.LOAD_FLAGS_REPLACE_HISTORY) != 0) {
    return true;
  }

  return false;
};

/**
 * Retrieves the components container for a window. Creates the initial value
 * if needed, including 'doc' member.
 *
 * @param {Object} win The window.
 * @param {boolean} opt_bDontCreate If true, don't create the
 *     components object if it doesn't exist (and return null).
 * @return {Object} The components object, or null if opt_bDontCreate
 *     is true and the components object doesn't already exist for
 *     this window.
 */
ComponentCollectorService.prototype.getWindowComponents = function(
    win, opt_bDontCreate) {
  // Make sure we use the topmost window in the window hierarchy.
  win = win.top;
  if (!win.wrappedJSObject) {
    // Make sure we received an xpconnect-wrapped instance, so we can
    // safely hang properties off of it without exposing those
    // properties to the HTML document running within the window.
    PS_LOG('getWindowComponents received unsafe window.');
    return null;
  }
  if (!win.__components__) {
    if (opt_bDontCreate) {
      // The components object doesn't exist and we've been told not
      // to create it, so return null.
      return null;
    }
    win.__components__ = {
      doc: {}
    };
    // We need to populate the TYPE_DOCUMENT field here, since we do
    // not do so from shouldLoad. See the comment in shouldLoad for
    // more information.
    var documentUrl = getURIForWindow(win);
    win.__components__.doc[documentUrl] = {
      elements: [Components.utils.getWeakReference(win.document)]
    };
    this.bindPendingDocumentEntries(documentUrl, win);
    this.pendingDocs_.removeEntriesNotMatching(isEntryRecentEnough);
  }
  return win.__components__;
};

/**
 * Adds a new component of the given type, key, and value.
 *
 * @param {Object} win The window.
 * @param {number} type The type as one of ComponentCollectorService.types.
 * @param {string} key The key to use for this component, typically the URL of
 *     the component.
 * @param {Object} value The value to use for this component, typically the
 *     DOM element associated with this component.
 * @param {boolean} opt_isUnique Iff true, the value will only be added if it
 *     does not already exist in the elements array.
 * @return {Object} The object holding information about the type of resource
 *     and key if the value was added.  Null otherwise.
 */
ComponentCollectorService.prototype.addWindowComponent = function(
    win, type, key, value, opt_isUnique) {
  var components = this.getWindowComponents(win);
  var typeStr = ComponentCollectorService.type[type];

  if (!components[typeStr]) components[typeStr] = {};
  if (!components[typeStr][key]) {
    components[typeStr][key] = {elements: []};
  }

  var elements = components[typeStr][key].elements;

  // Bail out early if this element should be unique and it is already in the
  // array.
  if (opt_isUnique && elements.indexOf(value) != -1) {
    return null;
  }

  elements.push(value);
  if (type == ComponentCollectorService.TYPE_REDIRECT) {
    if (typeof value != 'string') {
      // This should never happen, since TYPE_REDIRECT always has
      // string values. However, the function argument type is Object,
      // so to be correct, we verify that the value is a string.
      PS_LOG('Got non-string destination for redirect.');
    }
    this.updateRedirectedResource(components, key, String(value));
  }
  return components[typeStr][key];
};

/**
 * Gets the component of the given type and key.
 *
 * @param {Object} win The window.
 * @param {number} type The type as one of ComponentCollectorService.types.
 * @param {string} key The key to use for this component, typically the URL of
 *     the component.
 * @return {Object} The object holding information about the type of resource
 *     and key if the value was added.  Null otherwise.
 */
ComponentCollectorService.prototype.getWindowComponentOfType = function(
    win, type, key) {
  var components = this.getWindowComponents(win);
  var typeStr = ComponentCollectorService.type[type];

  if (!components[typeStr]) {
    return null;
  }

  return components[typeStr][key];
};

/**
 * A resource was redirected from fromURL to toURL, so update all
 * entries for a resource at the specified fromURL to instead exist at
 * the toURL.
 * @param {Object} components The components structure to update.
 * @param {string} fromURL The URL being redirected from.
 * @param {string} toURL The URL being redirected to.
 */
ComponentCollectorService.prototype.updateRedirectedResource = function(
    components, fromURL, toURL) {
  var redirect = components.redirect[fromURL];
  function updateResource(components, type, uri) {
    // We're updating the resource being redirected to, so make sure
    // to skip over all TYPE_REDIRECT entries. Also skip over
    // TYPE_DOCUMENT, since we already handle TYPE_DOCUMENT redirects
    // correctly.
    if (type == 'redirect' || type == 'doc') return;

    // We found a match. Move all of the properties from the
    // pre-redirect URL to the redirected URL.
    var src = components[type][uri];
    if (!components[type][toURL]) {
      components[type][toURL] = {};
    }

    mergeProperties(src, components[type][toURL], redirect);
    delete components[type][uri];
  }
  var numComponentsBound =
      visitComponentsWithUri(components, fromURL, updateResource);
  if (!numComponentsBound) {
    PS_LOG('Unable to update redirected resource data for ' + fromURL);
  }
};

/**
 * @param {Function} callback Callback to invoke when a new window
 * gets created.
 */
ComponentCollectorService.prototype.addNewWindowListener = function(callback) {
  this.newWindowCallbacks_.push(callback);
};

/**
 * Onload handler for the windows on the page.
 * @param {Object} event The onload event for the given window.
 */
ComponentCollectorService.prototype.onLoad = function(event) {
  var doc = event.target;
  if (!doc.defaultView) {
    PS_LOG('Unable to get defaultView for loaded document.');
    return;
  }
  var win = doc.defaultView.top;
  var components = this.getWindowComponents(win);
  if (!components) {
    PS_LOG('Unable to getWindowComponents for loaded document.');
    return;
  }
  function recordOnLoadTime(componentType) {
    if (!componentType) return false;
    for (var url in componentType) {
      var elts = componentType[url].elements;
      for (var i = 0, len = elts.length; i < len; ++i) {
        var elt = elts[i].get();
        // If the value is null, it indicates that the weak reference
        // has been cleared.
        if (!elt) {
          continue;
        }
        // Small hack. We reuse this logic for both iframe nodes and
        // document nodes. if it's an iframe node it will have a
        // content document which points to the actual document in the
        // frame. Otherwise it is a document node.
        if (elt.contentDocument) elt = elt.contentDocument;
        if (elt == doc) {
          componentType[url].onLoadTime = new Date().getTime();
          return true;
        }
      }
    }
    return false;
  }
  // Note: it is possible to embed a child document using the <object>
  // tag. We will fail to catch those cases here. This is sufficiently
  // rare and nonstandard that we do not need to support it.
  if (!recordOnLoadTime(components.doc) &&
      !recordOnLoadTime(components.iframe)) {
    if (!isLikelyFriendlyIframe(doc)) {
      PS_LOG('Unable to find document ' + doc.URL + '. onload not recorded.');
    }
  }
};

/**
 * Invokes all registered new window callbacks.
 * @param {Object} win The window that was just created.
 */
ComponentCollectorService.prototype.onNewWindow = function(win) {
  win.addEventListener('load', FunctionUtils.bind(this.onLoad, this), false);
  for (var i = 0, len = this.newWindowCallbacks_.length; i < len; ++i) {
    try {
      this.newWindowCallbacks_[i](win);
    } catch (e) {
      PS_LOG('newWindow callback threw exception: ' + e);
    }
  }
};

/**
 * @param {string} prop The name of the property.
 * @return {boolean} Whether the given property name is for a
 *     redirect, or for a component.
 */
function isRedirectProperty(prop) {
  return prop == 'requestHeaders' ||
    prop == 'responseHeaders' ||
    prop == 'responseCode' ||
    prop == 'requestMethod';
}

/**
 * mergeProperties will never overwrite properties whose names are
 * properties of this object.
 */
var KEEP_OLD_VALUE_WHEN_MERGING = {
  'requestTime': true
};

/**
 * Merge properties on the src object onto the destination object.
 * @param {Object} src The object to copy from.
 * @param {Object} component The component to copy to.
 * @param {Object} redirect The redirect to copy to.
 */
function mergeProperties(src, component, redirect) {
  for (var i in src) {
    // Some properties are moved to the redirect (headers, response
    // code); the rest are moved to the component's new URL.
    var dest;
    if (isRedirectProperty(i)) {
      dest = redirect;
    } else {
      dest = component;
    }
    if (dest[i] &&
        typeof src[i].length == 'number' &&
        typeof dest[i].length == 'number' &&
        typeof dest[i].push == 'function') {
      // Array property. Concatenate the arrays.
      Array.prototype.push.apply(dest[i], src[i]);
    } else if (!(i in dest)) {
      // dest has no such property, so copy without logging.
      dest[i] = src[i];
    } else if (dest[i] !== src[i] && !KEEP_OLD_VALUE_WHEN_MERGING[i]) {
      // Non-array property. If the property exists at the
      // destination, the best we can do is replace it.
      dest[i] = src[i];
    }
  }
}

// nsISupports
ComponentCollectorService.prototype.QueryInterface = function(aIID) {
  if (!aIID.equals(nsIObserverServiceIface) &&
      !aIID.equals(IComponentCollectorIface) &&
      !aIID.equals(nsIContentPolicyIface) &&
      !aIID.equals(nsISupportsIface)) {
    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
  return this;
};

var FunctionUtils = {
  // Returns a function object that invokes the specified function
  // using selfObj as the "this" object.
  bind: function(fn, selfObj) {
    return function() {
      return fn.apply(selfObj, arguments);
    };
  },

  // Returns a function object that invokes a function patch followed
  // by the original function. This allows us to hook JavaScript
  // functions.
  createPatch: function(target, orig) {
    return function(var_args) {
      target.apply(null, arguments);
      return orig.apply(this, arguments);
    };
  },
};

function ProgressListenerHookInstaller(componentCollector) {
  this.componentCollector_ = componentCollector;
}

ProgressListenerHookInstaller.prototype.EXPECTED_STATE =
    Components.interfaces.nsIWebProgressListener.STATE_IS_REQUEST |
    Components.interfaces.nsIWebProgressListener.STATE_START;

ProgressListenerHookInstaller.prototype.onStateChange = function(aWebProgress,
                                                                 aRequest,
                                                                 aStateFlags,
                                                                 aStatus) {
  if ((aStateFlags & this.EXPECTED_STATE) != this.EXPECTED_STATE) return;

  // We are not interested in about:blank, and allowing it through can
  // cause the Firefox Debugger to break. See
  // http://code.google.com/p/page-speed/issues/detail?id=983 for
  // more.
  if (aRequest.name == 'about:blank') return;
  var requestProtocolIdx = aRequest.name.indexOf(':');
  if (requestProtocolIdx < 0) {
    // The request name does not contain a protocol, so ignore it.
    return;
  }
  var requestProtocol = aRequest.name.substr(0, requestProtocolIdx);
  // The request should be http or https, *or* we allow through other
  // about: requests since there is e.g. about:document-onload-blocker
  // which we do actually need to allow through. In the event that
  // about:document-onload-blocker is renamed or other important
  // about: requests are added in the future, we whitelist all other
  // about: URLs to be safe.
  if (requestProtocol != 'http' &&
      requestProtocol != 'https' &&
      requestProtocol != 'about') {
    return;
  }

  var win = aWebProgress.DOMWindow;
  var doc = win.document;
  var protocol = doc.location.protocol;
  if (protocol != 'http:' && protocol != 'https:') return;
  if (!win._pageSpeedHookedWindow) {
    win._pageSpeedHookedWindow = true;
    this.componentCollector_.onNewWindow(win);
  }
};

ProgressListenerHookInstaller.prototype.QueryInterface =
    XPCOMUtils.generateQI([Components.interfaces.nsIWebProgressListener,
                           Components.interfaces.nsISupportsWeakReference]);

// XPCOMUtils.generateNSGetFactory was introduced in Mozilla 2 (Firefox 4).
// XPCOMUtils.generateNSGetModule is for Mozilla 1.9.x (Firefox 3).
if (XPCOMUtils.generateNSGetFactory) {
  var NSGetFactory =
      XPCOMUtils.generateNSGetFactory([ComponentCollectorService]);
} else {
  var NSGetModule =
      XPCOMUtils.generateNSGetModule([ComponentCollectorService]);
}
