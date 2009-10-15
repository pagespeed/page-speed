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

var FULL_RESULTS_BEACON_URL_PREF =
    'extensions.PageSpeed.beacon.full_results.url';

var FULL_RESULTS_BEACON_ENABLED_PREF =
    'extensions.PageSpeed.beacon.full_results.enabled';

/**
 * @constructor
 */
PAGESPEED.FullResultsBeacon = function(beaconUrlPref) {
  this.beaconUrlPref_ = beaconUrlPref;
};

/**
 * Build the string representation of the results container,
 * which holds all page spped results.
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
 * @return {boolean} False if the beacon can not be sent.
 */
PAGESPEED.FullResultsBeacon.prototype.sendBeacon = function(resultsContainer) {

  if (!PAGESPEED.Utils.getBoolPref(FULL_RESULTS_BEACON_ENABLED_PREF, false)) {
    PS_LOG('Full beacon is not enabled.');
    return false;
  }

  var beaconUrl = PAGESPEED.Utils.getStringPref(this.beaconUrlPref_);

  if (!beaconUrl) {
    // TODO: If the user asked for a beacon to be sent, prompt
    // for a URL and set it in the pref.
    PS_LOG('No url to send full beacon to.');
    return false;
  }

  if (!PAGESPEED.Utils.urlFromString(beaconUrl)) {
    PS_LOG(['Can\'t send full beacon, because the beacon url is ',
            'not well formed: "', beaconUrl, '" .'].join(''));
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

PAGESPEED.fullResultsBeacon =
    new PAGESPEED.FullResultsBeacon(FULL_RESULTS_BEACON_URL_PREF);

})();  // End closure
