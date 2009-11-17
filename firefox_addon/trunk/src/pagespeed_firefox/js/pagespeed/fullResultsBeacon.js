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
 * @fileoverview Post results to a service that stores and aggregates them.
 *
 * @author Sam Kerner
 */

(function() {  // Begin closure

/**
 * @constructor
 */
PAGESPEED.FullResultsBeacon = function() {
  this.beaconBase_ = new PAGESPEED.BeaconBase('full_results');
};

/**
 * Build the string representation of the results container,
 * which holds all page speed results.
 * @param {Object} resultsContainer Object holding resilts data.
 * @return {string} JSON encoded results.
 */
PAGESPEED.FullResultsBeacon.prototype.buildBeacon = function(resultsContainer) {
  return resultsContainer.toString();
};

/**
 * Send a beacon to a service.
 * @param {Object} resultsContainer The object which holds all results
 *     for the tab we are scoring.
 * @param {boolean} checkAutorunPref If true, only run if the autorun pref
 *     is set.
 * @return {boolean} False if the beacon can not be sent.
 */
PAGESPEED.FullResultsBeacon.prototype.sendBeacon = function(
    resultsContainer, checkAutorunPref) {

  if (!this.beaconBase_.isBeaconEnabled(checkAutorunPref)) {
    PS_LOG('Full beacon is not enabled.');
    return false;
  }

  var beaconUrl = this.beaconBase_.getBeaconUrl();
  if (!beaconUrl) {
    // Error already logged by getBeaconUrl().
    return false;
  }

  var xhrFlow = new PAGESPEED.ParallelXhrFlow();
  xhrFlow.addRequest('POST',  // Contents are too large for a GET.
                     beaconUrl,
                     '',  // No params.
                     ['content=',
                      encodeURIComponent(resultsContainer.toString())
                      ].join(''),
                     null,  // no action on success.
                     function() {PS_LOG('Full beacon fail.');}
                     );

  xhrFlow.sendRequests();

  return true;
};

PAGESPEED.fullResultsBeacon = new PAGESPEED.FullResultsBeacon();

if (PAGESPEED.PageSpeedContext) {
  PAGESPEED.PageSpeedContext.callbacks.postDisplay.addCallback(
      function(data) {
        var resultsContainer = data.resultsContainer;
        PAGESPEED.fullResultsBeacon.sendBeacon(resultsContainer, true);
      });
}

})();  // End closure
