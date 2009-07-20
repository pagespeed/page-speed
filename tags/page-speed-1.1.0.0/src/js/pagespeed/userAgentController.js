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
 * @fileoverview Add the ability to change user agent settings.  This allows
 * Page Speed to check pages that a web server sends to specific (non-firefox)
 * browsers.
 *
 * @author Sam Kerner
 */

(function() {  // Begin closure

/**
 * Constructor for object that hold the data needed to make firefox mimic
 * the user agent settings of another browser.
 *
 * @constructor
 */
PAGESPEED.UserAgentSettings = function() {
};

/**
 * Populate the UAS structure.
 *
 * @param {string} guiName A human readable browser name.
 * @param {string} appName The value navigator.appName should be set to.
 * @param {string} appVersion The value navigator.appVersion should be set to.
 * @param {string} platform The value navigator.platform should be set to.
 * @param {string} userAgent The value navigator.userAgent should be set to.
 * @param {string} opt_vendor The value navigator.vendor should be set to.
 * @param {string} opt_vendorSub The value navigator.vendorSub should be set to.
 */
PAGESPEED.UserAgentSettings.prototype.init = function(
    guiName, appName, appVersion, platform,
    userAgent, opt_vendor, opt_vendorSub) {
  this.guiName = guiName;
  this.appName = appName;
  this.appVersion = appVersion;
  this.platform = platform;
  this.userAgent = userAgent;
  this.vendor = opt_vendor;
  this.vendorSub = opt_vendorSub;
};

/**
 * Use the GUI name as the string representation of a UserAgentSettings object.
 * @return {string} A human readable name of the browser this object represents.
 */
PAGESPEED.UserAgentSettings.prototype.toString = function() {
  return this.guiName;
};

/**
 * Is this UAS equal to the given UAS?
 * @param {Object} other The object to compare for equality.
 * @return {boolean} Whether or not the UASs are equal.
 */
PAGESPEED.UserAgentSettings.prototype.equals = function(other) {
  if (this == other) return true;
  if (!other) return false;

  // We treat 'undefined' as equal to the empty string, so we need a
  // custom comparison function.
  var stringsAreEquivalent = function(a, b) {
    return (!a && !b) ? true : (a == b);
  };
  return stringsAreEquivalent(this.appName, other.appName) &&
         stringsAreEquivalent(this.appVersion, other.appVersion) &&
         stringsAreEquivalent(this.platform, other.platform) &&
         stringsAreEquivalent(this.userAgent, other.userAgent) &&
         stringsAreEquivalent(this.vendor, other.vendor) &&
         stringsAreEquivalent(this.vendorSub, other.vendorSub);
};

/**
 * Is this UAS the default UAS?
 * @return {boolean} Whether or not the UAS is the default.
 */
PAGESPEED.UserAgentSettings.prototype.isDefault = function() {
  return !this.appName &&
         !this.appVersion &&
         !this.platform &&
         !this.userAgent &&
         !this.vendor &&
         !this.vendorSub;
};

/**
 * PAGESPEED.UserAgentController will be instantiated on PAGESPEED.  It sets
 * preferences that control the user agent firefox reports, and holds the
 * initial values of those preferences so that changes to the user agent can
 * be undone.
 * @constructor
 */
PAGESPEED.UserAgentController = function() {
  // Map the name of the property of the navigator object to the pref that
  // alters it.
  this.SETTINGS_TO_PREF_NAMES_ = {
    appName: 'general.appname.override',
    appVersion: 'general.appversion.override',
    platform: 'general.platform.override',
    userAgent: 'general.useragent.override',
    vendor: 'general.useragent.vendor',
    vendorSub: 'general.useragent.vendorSub'
  };

  // Hold the settings for several browsers, to make common settings easy.
  // Because we must set a firefox string pref to override any of these values,
  // it is not possible to make any of the six properties on the navigator
  // object undefined.  Some browsers do not define some of the properties, and
  // the best we can do is to set the pref to the empty string.  So that
  // COMMON_AGENT_SETTINGS_  accurately documents the settings of browsers,
  // these properties are set to undefined.  The code that uses these objects
  // sets the corresponding preference to the empty string when it encounters
  // undefined.
  this.COMMON_AGENT_SETTINGS_ = {};

  // Make this.COMMON_AGENT_SETTINGS_ accessable to function
  // addUserAgentSettingsObj().
  var commonAgentSettings = this.COMMON_AGENT_SETTINGS_;

  /**
   * Add a UserAgentSettings object to as a property of the
   * COMMON_AGENT_SETTINGS_ object, such that each UserAgentSettings object
   * can be efficiently located by its name.
   *
   * @param {string} guiName A human readable browser name.
   * @param {string} appName The value navigator.appName should be set to.
   * @param {string} appVersion The value navigator.appVersion should be set to.
   * @param {string} platform The value navigator.platform should be set to.
   * @param {string} userAgent The value navigator.userAgent should be set to.
   * @param {string} opt_vendor The value navigator.vendor should be set to.
   * @param {string} opt_vendorSub The value navigator.vendorSub
   *     should be set to.
   */
  var addUserAgentSettingsObj = function(
      guiName, appName, appVersion, platform,
      userAgent, opt_vendor, opt_vendorSub) {
    var uas = new PAGESPEED.UserAgentSettings();
    uas.init(guiName, appName, appVersion, platform,
             userAgent, opt_vendor, opt_vendorSub);
    commonAgentSettings[uas.guiName] = uas;
  };

  addUserAgentSettingsObj(
      'Firefox 3 on Linux',
      'Netscape',
      '5.0 (X11; en-US)',
      'Linux i686 (x86_64)',
      'Mozilla/5.0 (X11; U; Linux i686 (x86_64); en-US; ' +
      'rv:1.9.0.1) Gecko/2008070206 Firefox/3.0.1',
      '',
      '');

  addUserAgentSettingsObj(
      'Firefox 3 on Windows',
      'Netscape',
      '5.0 (Windows; en-US)',
      'Win32',
      'Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; ' +
      'rv:1.9.0.1) Gecko/2008070206 Firefox/3.0.1',
      '',
      '');

  addUserAgentSettingsObj(
      'Firefox 2 on Linux',
      'Netscape',
      '5.0 (X11; en-US)',
      'Linux i686 (x86_64)',
      'Mozilla/5.0 (X11; U; Linux i686 (x86_64); en-US; ' +
      'rv:1.8.1.16) Gecko/20080716 Firefox/2.0.0.16',
      '',
      '');

  addUserAgentSettingsObj(
      'Firefox 2 on Windows',
      'Netscape',
      '5.0 (Windows; en-US)',
      'Win32',
      'Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; ' +
      'rv:1.8.1.13) Gecko/20080311 Firefox/2.0.0.13',
      '',
      '');

  addUserAgentSettingsObj(
      'Internet Explorer 7.0',
      'Microsoft Internet Explorer',
      '4.0 (compatible; MSIE 7.0; Windows NT 5.1; ' +
      '.NET CLR 2.0.50727)',
      'Win32',
      'Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1; ' +
      '.NET CLR 2.0.50727)',
      undefined,
      undefined);

  addUserAgentSettingsObj(
      'Internet Explorer 6.0',
      'Microsoft Internet Explorer',
      '4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1)',
      'Win32',
      'Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1)',
      undefined,
      undefined);

  addUserAgentSettingsObj(
      'Internet Explorer 5.0',
      'Microsoft Internet Explorer',
      '4.0 (compatible; MSIE 5.01; Windows NT 5.0; ' +
      '.NET CLR 1.1.4322 .NET CLR 2.0.50727)',
      'Win32',
      'Mozilla/4.0 (compatible; MSIE 5.01; Windows NT 5.0; ' +
      '.NET CLR 1.1.4322; .NET CLR 2.0.50727)',
      undefined,
      undefined);

  addUserAgentSettingsObj(
      'Safari 3',
      'Netscape',
      '5.0 (Macintosh; U; Intel Mac OS X 10_5_3; en-us) ' +
      'AppleWebKit/525.13 (KHTML, like Gecko) Version/3.1 ' +
      'Safari/525.13',
      'MacIntel',
      'Mozilla/5.0 (Macintosh; U; Intel Mac OS X 10_5_3; en-us) ' +
      'AppleWebKit/525.13 (KHTML, like Gecko) Version/525.13 ' +
      'Safari/525.13',
      'Apple Computer, Inc.',
      undefined);

  addUserAgentSettingsObj(
      'Safari 2',
      'Netscape',
      '5.0 (Macintosh; U; PPC Mac OS X; en) AppleWebKit/418.8 ' +
      '(KHTML, like Gecko) Safari/419.3',
      'MacPPC',
      'Mozilla/5.0 (Macintosh; U; PPC Mac OS X; en) ' +
      'AppleWebKit/418.8 (KHTML, like Gecko) Safari/419.3',
      'Apple Computer, Inc.',
      undefined);

  addUserAgentSettingsObj(
      'Opera 9 on Windows',
      'Opera',
      '9.24 (Windows NT 5.1; U; en)',
      'Win32',
      'Opera/9.24 (Windows NT 5.1; U; en)',
      undefined,
      undefined);

  addUserAgentSettingsObj(
      'Chrome 1.0 on Windows',
      'Netscape',
      '5.0 (Windows; U; Windows NT 5.1; en-US) AppleWebKit/525.19 ' +
      '(KHTML, like Gecko) Chrome/1.0.154.48 Safari/525.19',
      'Win32',
      'Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US) ' +
      'AppleWebKit/525.19 (KHTML, like Gecko) Chrome/1.0.154.48 ' +
      'Safari/525.19',
      'Google Inc.',
      '');

  addUserAgentSettingsObj(
      'Chrome 2.0 Beta on Windows',
      'Netscape',
      '5.0 (Windows; U; Windows NT 5.1; en-US) AppleWebKit/530.5 ' +
      '(KHTML, like Gecko) Chrome/2.0.172.8 Safari/530.5',
      'Win32',
      'Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US) ' +
      'AppleWebKit/530.5 (KHTML, like Gecko) Chrome/2.0.172.8 ' +
      'Safari/530.5',
      'Google Inc.',
      '');

  // The Google Wireless Transcoder renders pages to be easy to read on a
  // mobile device.  See http://www.google.com/gwt/n .  Some google properties
  // automatically redirect to a page processed by this service if the Android
  // browser's user agent is set.
  addUserAgentSettingsObj(
      'Google Wireless Transcoder',
      'Microsoft Internet Explorer',
      '5.0 (Windows; U; Windows NT 5.1; en-US) AppleWebKit/525.19 ' +
      '(KHTML, like Gecko) Chrome/0.3.154.9 Safari/525.19',
      'Win32',
      'Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US) ' +
      'AppleWebKit/525.19 (KHTML, like Gecko) Chrome/0.3.154.9 ' +
      'Safari/525.19',
      'Google Inc.',
      '');

  addUserAgentSettingsObj(
      'Android G1',
      'Netscape',
      '5.0 (Linux; U; Android 1.1; en-us; dream) AppleWebKit/525.10+ ' +
      '(KHTML, like Gecko) Version/3.0.4 Mobile Safari/523.12.2',
      '',
      'Mozilla/5.0 (Linux; U; Android 1.1; en-us; dream) ' +
      'AppleWebKit/525.10+ (KHTML, like Gecko) Version/3.0.4 ' +
      'Mobile Safari/523.12.2',
      'Apple Computer, Inc.',
      '');

  addUserAgentSettingsObj(
      'iPhone',
      'AppleWebKit/420+ (KHTML, like Gecko)',
      'Version/3.0',
      'Mobile/1A542a Safari/419.3',
      'Mozilla/5.0 (iPhone; U; CPU like Mac OS X; en)',
      undefined,
      undefined
      );

  addUserAgentSettingsObj(
      'Internet Explorer 8.0',
      'Microsoft Internet Explorer',
      '4.0 (compatible; MSIE 8.0; Windows NT 5.1; ' +
      'Trident/4.0; GoogleT5; .NET CLR 1.1.4322; .NET CLR 2.0.50727)',
      'Win32',
      'Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 5.1; ' +
      'Trident/4.0; GoogleT5; .NET CLR 1.1.4322; .NET CLR 2.0.50727)',
      undefined,
      undefined);

  addUserAgentSettingsObj(
      'Internet Explorer 8.0, Compatibility View',
      'Microsoft Internet Explorer',
      '4.0 (compatible; MSIE 7.0; Windows NT 5.1; ' +
      'Trident/4.0; GoogleT5; .NET CLR 1.1.4322; .NET CLR 2.0.50727)',
      'Win32',
      'Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1; ' +
      'Trident/4.0; GoogleT5; .NET CLR 1.1.4322; .NET CLR 2.0.50727)',
      undefined,
      undefined);

  // initialUserAgentSettings_ holds user agent settings, so that initial
  // settings can be restored.  Set to null to indicate that no change
  // to the user agent has been made yet.
  this.initialUserAgentSettings_ = null;
};

/**
 * Fetch a list of the names of the user agent objects in object
 * COMMON_AGENT_SETTINGS_.  Used by the GUI to get a list of browsers
 * whose user agent settings can be emulated.  The array is sorted by
 * the name of the object as shown in the GUI.
 *
 * @return {Array} Array of objects, each of which holds the user agent
 *    settings of a specific browser.
 */
PAGESPEED.UserAgentController.prototype.getUserAgentNames = function() {
  var userAgentObjects = [];
  for (var name in this.COMMON_AGENT_SETTINGS_) {
    userAgentObjects.push(this.COMMON_AGENT_SETTINGS_[name]);
  }

  userAgentObjects.sort()

  return userAgentObjects.map(function(uao) {return uao.guiName;});
};

/**
 * Set the user agent.  Takes an object whose properties hold the user agent
 * values.  For examples of objects with the properties to set, see the object
 * PAGESPEED.UserAgentController.COMMON_AGENT_SETTINGS_.firefox3linux .
 *
 * @param {Object} userAgentSettings Properties of this object are used to set
 *    user agent data.
 */
PAGESPEED.UserAgentController.prototype.setUserAgentByObj_ =
    function(userAgentSettings) {
  // If this is the first time the user agent is set by this function, then
  // store the settings so that they can be restored on exit.

  if (!this.initialUserAgentSettings_) {
    this.initialUserAgentSettings_ = this.getUserAgent();
  }

  var prefs = PAGESPEED.Utils.getPrefs();
  for (var setting in this.SETTINGS_TO_PREF_NAMES_) {
    var prefName = this.SETTINGS_TO_PREF_NAMES_[setting];

    if (!userAgentSettings.hasOwnProperty(setting)) {
      // If the field isn't present in the UAS, it means that the
      // browser should use its default value, so we clear the
      // corresponding preference, which causes the browser to use its
      // default value.
      PAGESPEED.Utils.clearPref(prefName);
    } else {
      // If the field is a non-string value, we map it to the empty
      // string (this includes undefined). Otherwise, we use the
      // specified string value.
      var prefValue = '';
      if (typeof userAgentSettings[setting] == 'string') {
        prefValue = userAgentSettings[setting];
      }
      prefs.setCharPref(prefName, prefValue);
    }
  }
};

/**
 * Set the user agent.  Takes the properties of the window.navigator object.
 *
 * @param {string} appName The setting desired for navigatior.appName .
 * @param {string} appVersion The setting desired for navigatior.appVersion .
 * @param {string} platform The setting desired for navigatior.platform .
 * @param {string} userAgent The setting desired for navigatior.userAgent .
 * @param {string} opt_vendor The setting desired for navigatior.vendor .
 * @param {string} opt_vendorSub The setting desired for navigatior.vendorSub .
 */
PAGESPEED.UserAgentController.prototype.setUserAgent = function(appName,
                                                            appVersion,
                                                            platform,
                                                            userAgent,
                                                            opt_vendor,
                                                            opt_vendorSub) {
  var userAgentSettings = new PAGESPEED.UserAgentSettings();
  userAgentSettings.init(
      'custom', appName, appVersion, platform,
      userAgent, opt_vendor, opt_vendorSub);
  return this.setUserAgentByObj_(userAgentSettings);
};

/**
 * Get an object holding the current user agent settings, so that they
 * can be restored later.
 *
 * @return {Object} Properties of this object are the properties on the
 *    window.navigator object that specify the browser type.
 */
PAGESPEED.UserAgentController.prototype.getUserAgent = function() {
  var userAgentSettings = new PAGESPEED.UserAgentSettings();
  var prefs = PAGESPEED.Utils.getPrefs();
  for (var setting in this.SETTINGS_TO_PREF_NAMES_) {
    var prefName = this.SETTINGS_TO_PREF_NAMES_[setting];
    // Only populate the field if it's been overridden. It's important
    // to do this, and not to set it to empty string or undefined,
    // because we want to record the difference between unset (which
    // means use the default value for the browser) and empty string
    // (which means that the field should be overridden to an empty
    // string).
    if (prefs.prefHasUserValue(prefName)) {
      userAgentSettings[setting] = PAGESPEED.Utils.getPrefValue(prefName);
    }
  }

  return userAgentSettings;
};

/**
 * Is the given UA name the currently active UA?
 * @param {string} agentName The name of the browser to check for.
 * @return {boolean} Whether or not the specified UA is the current UA.
 */
PAGESPEED.UserAgentController.prototype.isCurrentUserAgent = function(
    agentName) {
  return this.getUserAgent().equals(this.COMMON_AGENT_SETTINGS_[agentName]);
};

/**
 * Is the currently active UA the default UA for the system?
 * @return {boolean} Whether or not the current UA is the default UA.
 */
PAGESPEED.UserAgentController.prototype.isDefaultUserAgent = function() {
  return this.getUserAgent().isDefault();
};

/**
 * Set a user agent from a predefined list of agent names.  This list
 * is the list of properties of this.COMMON_AGENT_SETTINGS_.
 * @param {string} agentName The name of the browser who we want to masquerade
 *    as.
 * @return {boolean} False if the agentName is not known, true otherwise.
 */
PAGESPEED.UserAgentController.prototype.setUserAgentByName = function(
    agentName) {
  var settings = this.COMMON_AGENT_SETTINGS_[agentName];

  if (!settings)  // Name is not known.
    return false;

  this.setUserAgentByObj_(settings);
  return true;
};

/**
 * Restore the user agent settings to what they were before PAGESPEED.UserAgent
 * first changed them.  You can call setUserAgent() more than once, and this
 * function will still reset the user agent to what it was before the first
 * call to setUserAgent().
 */
PAGESPEED.UserAgentController.prototype.restoreUserAgent = function() {
  if (!this.initialUserAgentSettings_) {
    // A null value means that there is nothing to restore, because the user
    // agent has not been changed.
    return;
  }

  // Set user agent to the state that was saved the first time setUserAgent()
  // ran.  Then set initialUserAgentSettings_ to null, indicationg that the
  // user agent is in its initial, unaltered state.
  this.setUserAgentByObj_(this.initialUserAgentSettings_);
  this.initialUserAgentSettings_ = null;
};

/**
 * Reset the user agent, so that it is the true user agent of the browser.
 */
PAGESPEED.UserAgentController.prototype.resetUserAgent = function() {
  var prefs = PAGESPEED.Utils.getPrefs();

  for (var setting in this.SETTINGS_TO_PREF_NAMES_) {
    PAGESPEED.Utils.clearPref(this.SETTINGS_TO_PREF_NAMES_[setting]);
  }
};

// Set up UserAgentController as a Singleton
PAGESPEED.UserAgentController = new PAGESPEED.UserAgentController();

})();  // End closure
