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
 * @fileoverview Provide a class that returns resources that match
 * some defined criteria.  This is intended to replace the functions
 * in PAGESPEED.Utils, such as PAGESPEED.Utils.getResources().
 *
 * @author Sam Kerner
 */

(function() {  // Begin closure

/**
 * This class is used by lint rules to get resources.
 * @param {object} opt_resourceFilter An object which filters resources,
 *     to allow users of this class to see only some resources.
 * @constructor
 */
PAGESPEED.ResourceAccessor = function(opt_resourceFilter) {
  this.resourceFilter_ = opt_resourceFilter ||
                         new PAGESPEED.ResourceFilters.AllowAll();
};

/**
 * This function fetches all resources of a given type fetched by the
 * current page.  Lint rules that iterate over all resources of a given
 * set of types should use it to get resources.
 * @param {Array.<string>|string} types Return urls with these resource
 *     types.  'all' means resources of any type.
 * @param {object} opt_extraFilter An extra filter resources must pass,
 *     in addition to this.resourceFilter_.
 * @param {bool} opt_onlyBeforeOnload If true, only return resources
 *     fetched before onload.  See TODO below.
 * @return {Array.<string>} A list of urls that are of the types in |types|,
 *     and allowed by the resource filter set by this object's constructor.
 */
PAGESPEED.ResourceAccessor.prototype.getResources = function(
    types,
    opt_extraFilter,
    opt_onlyBeforeOnload) {
  // TODO: Until we update every lint rule to use this class instead of
  // PAGESPEED.Utils.* for resource access, we can't get rid of them. To
  // avoid duplicate code, use PAGESPEED.Utils.* here.  When all other
  // users are removed, move the code to this file.
  var unfilteredResources;

  // Find the set of types of resources we are interested in.  [] means
  // return all types.
  var argsToGetResources;
  if (types == 'all') {
    argsToGetResources = [];
  } else if (typeof types == 'string') {
    argsToGetResources = [types];
  } else {
    argsToGetResources = types;
  }

  // TODO: opt_onlyBeforeOnload filters by onload time.  This
  // functionality should be implemented as a filter, and this
  // will be trivial to do once the code in
  // PAGESPEED.Utils.getResourcesWithFilter() is moved into
  // this file.  Until that happens, it is painful to filter
  // by access time because we need access to the resource
  // object to get resource.requestTime.
  var unfilteredResources;
  if (opt_onlyBeforeOnload) {
    unfilteredResources = PAGESPEED.Utils.getResourcesBeforeOnload(
        argsToGetResources);
  } else {
    unfilteredResources = PAGESPEED.Utils.getResources.apply(
        PAGESPEED.Utils, argsToGetResources);
  }

  var filteredResults = [];
  for (var i = 0, ie = unfilteredResources.length; i < ie; ++i) {
    var url = unfilteredResources[i];

    var resourceData = {
      url: url
    };

    if (!this.resourceFilter_.isResourceAllowed(resourceData))
      continue;

    if (opt_extraFilter && !opt_extraFilter.isResourceAllowed(resourceData))
      continue;

    filteredResults.push(url);
  }

  return filteredResults;
};

/**
 * Test a resource to see if it would be returned by |getResources|.
 * @param {string} url The url of the resource to test.
 * @param {Array.<string>} types Only resources of these types will be
 *     considered.
 * @param {object} opt_extraFilter An extra filter resources must pass,
 *     in addition to this.resourceFilter_.
 * @return {boolean} True if the resource should be tested by Page Speed
 *     rules.  False if it should be ignored.
 */
PAGESPEED.ResourceAccessor.prototype.isResourceUnderTest = function(
    url,
    types,
    opt_extraFilter) {
  var resourceUrls = this.getResources(types, opt_extraFilter);
  return (resourceUrls.indexOf(url) != -1);
};

/**
 * Returns the contents of all scripts or styles including both inline and
 * external.
 *
 * @param {string} type Must be either 'script' or 'style'.
 * @return {Array.<Object>} An array of objects containing 'name' and
 *     'content'.
 */
PAGESPEED.ResourceAccessor.prototype.getContentsOfAllScriptsOrStyles =
    function(type) {

  if ('script' != type && 'style' != type) {
    throw new Error(
        'PAGESPEED.ResourceAccessor.getContentsOfAllScriptsOrStyles():  ' +
        'Invalid paramter type = ' + type);
  }

  var allTags = [];

  // Get inline content:
  var docUrls = this.getResources(['doc', 'iframe']);

  for (var i = 0, len = docUrls.length; i < len; ++i) {
    var sHTML = PAGESPEED.Utils.getResourceContent(docUrls[i]);
    var inline = PAGESPEED.Utils.getContentsOfAllTags(sHTML, type);
    var iNextInlineNumber = 1;
    for (var j = 0, jlen = inline.length; j < jlen; ++j) {
      if (inline[j]) {
        allTags.push({
            name: [
                docUrls[i],
                ' (inline block #', iNextInlineNumber++, ')'
                ].join(''),
            content: inline[j]
        });
      }
    }
  }

  // Get external resources:
  var urls = this.getResources([type == 'script' ? 'js' : 'css']);
  for (var i = 0, len = urls.length; i < len; ++i) {
    allTags.push({
        name: urls[i],
        content: PAGESPEED.Utils.getResourceContent(urls[i])
    });
  }

  return allTags;
};

})();  // End closure
