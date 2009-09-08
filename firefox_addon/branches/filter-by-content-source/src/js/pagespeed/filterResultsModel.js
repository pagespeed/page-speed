/**
 * Copyright 2009 Google Inc.
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
 * @fileoverview Lint rules can run on a subset of the contents of a page.
 * This file defines an object that holds settings that define what subset
 * of the page rules should run on.
 *
 * @author Sam Kerner
 */

(function() {  // Begin closure

/**
 * Create an object that stores the subset of the page that should be scored.
 * @constructor
 */
PAGESPEED.FilterResultsModel = function() {
  // Build the default state of the filter.
  // TODO: Write filter state to prefs so that menu settings
  // persist across browser restarts.
  this.filterState_ = {
    Ads: 'noFilter'  // By default, lint rules score all content.
  };
};

/**
 * Change the filter rules.
 * @param {string} category The category of content that can be filtered.
 * @param {string} setting The way content in |category| should be filtered.
 */
PAGESPEED.FilterResultsModel.prototype.setFilterState = function(
    category, setting) {
  this.filterState_[category] = setting;
};

/**
 * Create a resource accessor object that lint rules can use to
 * get content by type.
 * @return {object} An object which can fetch content matching
 *     the current filter state.
 */
PAGESPEED.FilterResultsModel.prototype.createFilter = function() {
  // TODO: Implement this.
  return null;
};

PAGESPEED.filterResultsModel = new PAGESPEED.FilterResultsModel();

})();  // End closure
