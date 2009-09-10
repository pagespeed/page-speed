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
 * @fileoverview Provide classes that test resources against several
 * criteria.  Possible criteria include hostname of a resource url
 * and DOM position.
 *
 * @author Sam Kerner
 */

(function() {  // Begin closure

// Resource class names are short and simple (such as And, Not, IsAd).
// To keep the PAGESPEED namespace from having these names, declare a
// new namespece for resource filters.
PAGESPEED.ResourceFilters = {};

/**
 * Allow all resources.
 */
PAGESPEED.ResourceFilters.AllowAll = function() {};

/**
 * @return {boolean} True, so that no resources are filtered.
 */
PAGESPEED.ResourceFilters.AllowAll.prototype.isResourceAllowed = function() {
  return true;
};

/**
 * Create a resource filter that inverts the test of another filter.
 * @param {Object} resourceFilter The resource filter to invert.
 * @constructor
 */
PAGESPEED.ResourceFilters.Not = function(resourceFilter) {
  this.resourceFilterToInvert_ = resourceFilter;
};

/**
 * @param {object} data An object holding information about a resource.
 * @return {boolean} The inverse of the filter passed to the constructor.
 */
PAGESPEED.ResourceFilters.Not.prototype.isResourceAllowed = function(data) {
  return !this.resourceFilterToInvert_.isResourceAllowed(data);
};

/**
 * Create a resource filter that ANDs the test of other filters.
 * An empty array of resource filters produces a filter that allows
 * all resources.
 * @param {Array.<object>} resourceFilters List of filters to AND.
 * @constructor
 */
PAGESPEED.ResourceFilters.And = function(resourceFilters) {
  this.resourceFiltersToAnd_ = resourceFilters;
};

/**
 * @param {object} data An object holding information about a resource.
 * @return {boolean} The inverse of the filter passed to the constructor.
 */
PAGESPEED.ResourceFilters.And.prototype.isResourceAllowed = function(data) {
  for (var i = 0, ie = this.resourceFiltersToAnd_.length; i < ie; ++i) {
    if (!this.resourceFiltersToAnd_[i].isResourceAllowed(data))
      return false;
  }

  return true;
};

/**
 * This filter tests to see if a page is an ad.
 * @constructor
 */
PAGESPEED.ResourceFilters.IsAd = function() {};

/**
 * Test to see if a resource is an ad.
 * TODO: For now we use a very crude heuristic.  This is clearly not a
 * real solution, and will be replaced very soon.
 * @param {object} data An object holding information about a resource.
 * @return {boolean} The inverse of the filter passed to the constructor.
 */
PAGESPEED.ResourceFilters.IsAd.prototype.isResourceAllowed = function(data) {
  var url = data.url;

  if (!url)
    return true;

  return /ad[^/]*\.(com|net)/.test(url);
};

/**
 * This filter tests that a resource's url uses one of a given set of
 * protocols.
 * @param {Array.<string>} protocols A list of protocols to accept.
 * @constructor
 */
PAGESPEED.ResourceFilters.FetchedByProtocol = function(protocols) {
  if (!protocols.length) {
    // If there are no protocols that could match, do not build a regexp.
    return;
  }

  // Build a regexp that looks like this:
  // /^(http|https|gopher):/
  var regexpStr = [
      '^(',  // Protocols must start at the beginning of the string.
      protocols.join('|'),
      '):'].join('');  // They must be followed by ':'.

  this.protocolMatcher_ = new RegExp(regexpStr);
};

/**
 * @param {object} data An object holding information about a resource.
 * @return {boolean} True if the resource url starts with 'http:' or 'https:'.
 */
PAGESPEED.ResourceFilters.FetchedByProtocol.prototype.isResourceAllowed =
    function(data) {
  var url = data.url;

  if (!url || !this.protocolMatcher_)
    return false;

  return this.protocolMatcher_.test(url);
};

})();  // End closure
