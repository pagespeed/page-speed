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
 * @param {string} type Return urls with this resource type.  'all' means
 *     resources of any type.
 * @param {object} opt_extraFilter An extra filter resources must pass,
 *     in addition to this.resourceFilter_.
 * @return {Array.<string>} A list of urls that are of type |type|,
 *     and allowed by the resource filter set by this object's constructor.
 */
PAGESPEED.ResourceAccessor.prototype.getResources = function(type,
                                                             opt_extraFilter) {
  // TODO: Until we update every lint rule to use this class instead of
  // PAGESPEED.Utils.* for resource access, we can't get rid of them. To
  // avoid duplicate code, use PAGESPEED.Utils.* here.  When all other
  // users are removed, move the code to this file.
  var unfilteredResources;

  if (type == 'all') {
    unfilteredResources = PAGESPEED.Utils.getResources();
  } else {
    unfilteredResources = PAGESPEED.Utils.getResources(type);
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

})();  // End closure
