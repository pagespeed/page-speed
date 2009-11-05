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
 * @fileoverview Build and send a beacon in the format requested by
 *     http://www.showslow.com .
 *
 * @author Sam Kerner
 *
 * TODO: The code in this file is similar to the code in fullResultsBeacon.js.
 * Both files may be changed substantially as we get user feedback, so they are
 * implemented separately for now.  Once they are mature, consider merging
 * common parts.
 */

(function() {  // Begin closure

var MINIMAL_BEACON_URL_PREF = 'extensions.PageSpeed.beacon.minimal.url';
var MINIMAL_BEACON_ENABLED_PREF = 'extensions.PageSpeed.beacon.minimal.enabled';
var MINIMAL_BEACON_AUTORUN_PREF = 'extensions.PageSpeed.beacon.minimal.autorun';

PAGESPEED.MinimalBeacon = {};

/**
 * Given a results container, build the param string which encodes
 * results.  This format is similar to YSlow's beacon.  It was requested
 * by the creator of showslow.com.  See the wiki page for info on
 * the beacon format: http://code.google.com/p/page-speed/wiki/BeaconDocs .
 *
 * @param {Object} resultsContainer The results of a page speed run.
 * @returns {string} The params string of the beacon to send.
 */
PAGESPEED.MinimalBeacon.buildBeacon = function(resultsContainer) {
  /**
   * Encode a key-value pair into the params portion of a URL.
   * @param {string} key The key.
   * @param {*} value The value.  Will be cast to a string.
   * @return {string} The string key=value, suitably escaped.
   */
  var encodeKeyValuePair = function(key, value) {
    if (typeof value == 'undefined') {
      value = 'err';
    }
    return [encodeURIComponent(key),
            '=',
            encodeURIComponent(value)].join('');
  }

  // Use short names for different parts of the results container.
  var versions = resultsContainer.results_.versions;
  var pageStats = resultsContainer.results_.pageStats;
  var rules = resultsContainer.results_.rules;

  var paramsArray = [
      // Try to keep the naming consistent with YSlow, as documented at:
      // http://tech.groups.yahoo.com/group/exceptional-performance/message/704
      encodeKeyValuePair('v', versions.pageSpeed),
      encodeKeyValuePair('u', pageStats.initialUrl),
      encodeKeyValuePair('r', pageStats.numRequests),
      encodeKeyValuePair('w', pageStats.pageSize),
      encodeKeyValuePair('o', pageStats.overallScore),

      // We add two new items:
      encodeKeyValuePair('l', pageStats.pageLoadTime),
      encodeKeyValuePair('t', pageStats.transferSize)
  ];

  // Next we add the rule scores.  The keys are the rule's short name with a
  // 'p' prefix.  To make parsing easy, ensure that no other key starts with
  // a 'p'.

  // Build a mapping from short names to scores, and a list of short names.
  var shortNamesToScores = {};
  var allShortNames = [];
  for (var i = 0, ie = rules.length; i < ie; ++i) {
    var rule = rules[i];
    allShortNames.push(rule.shortName);

    var score;
    switch (typeof(rule.score)) {
      case 'number':
        score = rule.score;
        break;
      case 'string':
        if (rule.score=='n/a') {
          // Drop the /, as it expands to several chars when escaped.
          score = 'na';
        } else {
          score = 'err';  // Other strings indicate errors.
        }
        break;
      default:
        score = 'err';
    }

    shortNamesToScores[rule.shortName] = score;
  }

  allShortNames.sort();  // Ensure deterministic order.

  // For each short name, add |shortName|=|score| to the array of data to send.
  for (var i = 0, ie = allShortNames.length; i < ie; ++i) {
    var shortName = allShortNames[i];
    paramsArray.push(
        encodeKeyValuePair('p' + shortName, shortNamesToScores[shortName]));
  }

  var params = paramsArray.join('&');
  return params;
};

/**
 * Send the beacon.
 * @param {Object} resultsContainer The object which holds all results
 *     for the tab we are scoring.
 * @param {boolean} opt_prefOverride If true, don't check the preference
 *     to see if the minimal beacon is enabled.  Used by autorun, which
 *     checks its own pref.
 * @return {boolean} False if the beacon can not be sent.
 */
PAGESPEED.MinimalBeacon.sendBeacon = function(resultsContainer,
                                              opt_prefOverride) {

  if (!PAGESPEED.Utils.getBoolPref(MINIMAL_BEACON_ENABLED_PREF, false)) {
    PS_LOG('Minimal beacon is not enabled.');
    return false;
  }

  var beaconUrl = PAGESPEED.Utils.getStringPref(MINIMAL_BEACON_URL_PREF);

  if (!beaconUrl) {
    PS_LOG('Can\'t send minimal beacon because the URL is not set.');
    return false;
  }

  if (!PAGESPEED.Utils.urlFromString(beaconUrl)) {
    PS_LOG(['Can\'t send minimal beacon, because the beacon url is ',
            'not a valid URL: "', beaconUrl, '" .'].join(''));
    return false;
  }

  var beaconParams = PAGESPEED.MinimalBeacon.buildBeacon(resultsContainer);

  // For now there is only one XHR to send.  We use the parallel XHR
  // class anyway.  The class hides some ugly XHR details.  If other
  // projects wish to recieve beacons, the capability to send more
  // than one will be useful.
  var xhrFlow = new PAGESPEED.ParallelXhrFlow();

  xhrFlow.addRequest('GET',
                     beaconUrl,
                     beaconParams,
                     '',  // No data (all data is in the params).
                     null,  // No action on success.
                     function() {PS_LOG('Minimal beacon fail.');}
                     );

  xhrFlow.sendRequests();

  return true;
};

if (PAGESPEED.PageSpeedContext) {
  PAGESPEED.PageSpeedContext.callbacks.postDisplay.addCallback(
      function(data) {
        var resultsContainer = data.resultsContainer;

        if (!PAGESPEED.Utils.getBoolPref(MINIMAL_BEACON_AUTORUN_PREF, false)) {
          return;
        }

        PAGESPEED.MinimalBeacon.sendBeacon(resultsContainer, true);
      });
}

})();  // End closure
