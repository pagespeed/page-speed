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
 * @fileoverview Page Speed supports multiple beacons, which have
 * different formats.  Anything that all beacons do in the same way
 * should be implemented in this file.
 *
 * @author Sam Kerner
 */

(function() {  // Begin closure

/**
 * Given the type of beacon and a suffix, build the string name
 * of a preference.  See
 * http://code.google.com/p/page-speed/wiki/BeaconDocs
 * for a list of relevant preferences.
 * @param {string} beaconName The name of the beacon this pref will apply to.
 * @param {string} suffix The final part of the pref name.
 * @return {string} The name of a firefox preference controling a beacon.
 */
var buildPrefName = function(beaconName, suffix) {
  return ['extensions.PageSpeed.beacon.',
          beaconName, '.',
          suffix].join('');
};

/**
 * @constructor
 */
PAGESPEED.BeaconTraits = function(beaconName) {
  this.beaconName_ = beaconName;
  this.beaconUrlPref_ = buildPrefName(beaconName, 'url');
  this.beaconEnabledPref_ = buildPrefName(beaconName, 'enabled');
  this.beaconAutorunPref_ = buildPrefName(beaconName, 'autorun');
};

/**
 * Check prefs to see if a beacon should be sent.
 * If the beacon is not enabled, never return true.
 * @param {boolean} checkAutorunPref If true, test that the autorun pref
 *     is set.
 * @return {boolean} True if a beacon should be sent.  False otherwise.
 */
PAGESPEED.BeaconTraits.prototype.isBeaconEnabled = function(checkAutorunPref) {
  if (!PAGESPEED.Utils.getBoolPref(this.beaconEnabledPref_, false))
    return false;

  if (checkAutorunPref &&
      !PAGESPEED.Utils.getBoolPref(this.beaconAutorunPref_, false))
    return false;

  return true;
};

/**
 * Build the url to send a beacon to.  This should be done
 * when the beacon is about to be sent, so that the latest
 * preference values are used.
 * @return {string} The url to send a beacon to.  '' if the URL pref
 *     is unset or malformed.
 */
PAGESPEED.BeaconTraits.prototype.getBeaconUrl = function() {
  var beaconUrl = PAGESPEED.Utils.getStringPref(this.beaconUrlPref_);

  if (!beaconUrl) {
    // TODO: If the user asked for a beacon to be sent, prompt
    // for a URL and set it in the pref.
    PS_LOG('No url to send ' + this.beaconUrlPref_ + ' to.');
    return '';
  }

  if (!PAGESPEED.Utils.urlFromString(beaconUrl)) {
    PS_LOG(['Can\'t send ', this.beaconUrlPref_,
            ' because the beacon url is not well formed: "',
            beaconUrl, '" .'].join(''));
    return '';
  }

  return beaconUrl;
};

})();  // End closure
