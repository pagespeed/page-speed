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

/**
 * @constructor
 */
PAGESPEED.FullResultsBeacon = function(beaconUrlPref) {
  this.beaconUrlPref_ = beaconUrlPref;
};

PAGESPEED.FullResultsBeacon.prototype.buildBeacon = function(resultsContainer) {
  return resultsContainer.toString();
};

PAGESPEED.FullResultsBeacon.prototype.sendBeacon = function(resultsContainer) {

  var beaconUrl = PAGESPEED.Utils.getStringPref(this.beaconUrlPref_);

  if (!beaconUrl) {
    // TODO: If the user asked for a beacon to be sent, prompt
    // for a URL and set it in the pref.
    PS_LOG('No Url to send full beacon to.');
    return;
  }

  if (!PAGESPEED.Utils.urlFromString(beaconUrl)) {
    PS_LOG(['Can\'t send full beacon, because the beacon url is ',
            'not a valid URL: "', beaconUrl, '" .'].join(''));
    return;
  }

  var xhrFlow = new PAGESPEED.ParallelXhrFlow();
  xhrFlow.addRequest(beaconUrl,
                     ['content=',
                      encodeURIComponent(resultsContainer.toString())
                      ].join(''),
                     function() {PS_LOG('Beacon success.');},
                     function() {PS_LOG('Beacon fail.');}
                     );

  xhrFlow.sendRequests();
};

PAGESPEED.fullResultsBeacon =
    new PAGESPEED.FullResultsBeacon(FULL_RESULTS_BEACON_URL_PREF);

})();  // End closure
