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
 * @fileoverview Provides functions to read and set firefox user
 * preference values.
 *
 * @author Sam Kerner
 */

goog.provide('activity.preference');

goog.require('activity.xpcom');

/**
 * Activity profiler preferences start with 'extensions.Activity.'.  Having
 * a function to expand them avoids the clutter of repeating this
 * string throughout the code.
 *
 * @param {string} prefName Name of an activity profiler preference.
 * @return {string} Full name of the preference.
 * @private
 */
activity.preference.fullPrefName_ = function(prefName) {
  return 'extensions.Activity.' + prefName;
};

/**
 * Store the xpcom object used to access preferences on the preference object.
 * @private
 */
activity.preference.prefClass_ = activity.xpcom.CCSV(
    '@mozilla.org/preferences-service;1', 'nsIPrefBranch');

/**
 * Mozilla prefs have three possible types: int, string, and boolean.
 * Copy constants representing each type onto the activity.preference object,
 * so that users can see them without accessing the private preferences-service
 * class.
 * @enum {number}
 */
activity.preference.PrefType = {
  PREF_INT: activity.preference.prefClass_.PREF_INT,
  PREF_STRING: activity.preference.prefClass_.PREF_STRING,
  PREF_BOOL: activity.preference.prefClass_.PREF_BOOL
};

/**
 * Map pref types to string names, to make error messages easy to read.
 * @param {activity.preference.PrefType} prefType A preference type.
 * @return {string} Human readable pref type string.
 */
activity.preference.prefTypeToString = function(prefType) {
  switch (prefType) {
    case activity.preference.prefClass_.PREF_INT:
      return 'integer';
    case activity.preference.prefClass_.PREF_STRING:
      return 'string';
    case activity.preference.prefClass_.PREF_BOOL:
      return 'boolean';
    default:
      return 'invalid';
  }
};

/**
 * Reset a preference to the default value.
 * @param {string} prefName The name of the pref to clear.
 * @this {activity.preference}
 */
activity.preference.clear = function(prefName) {
  try {
    activity.preference.prefClass_.clearUserPref(
        activity.preference.fullPrefName_(prefName));
  } catch (e) {
    // clearUserPref throws an exception if there is nothing to clear.
    // Do nothing in this case.
  }
};

/**
 * Set a preference to a given integer value.
 *
 * @param {string} prefName The name of the pref, without the standard prefix.
 * @param {number} value The value to set.
 */
activity.preference.setInt = function(prefName, value) {
  // TODO: If the value is not close to an int, warn.
  activity.preference.prefClass_.setIntPref(
      activity.preference.fullPrefName_(prefName),
      value);
};

/**
 * Set a preference to a given string value.
 *
 * @param {string} prefName The name of the pref, without the standard prefix.
 * @param {*} value The value to set.  Will be converted to a string.
 */
activity.preference.setString = function(prefName, value) {
  activity.preference.prefClass_.setCharPref(
      activity.preference.fullPrefName_(prefName),
      value.toString());
};

/**
 * Set a preference to a given boolean value.
 *
 * @param {string} prefName The name of the pref, without the standard prefix.
 * @param {boolean} value The value to set.
 */
activity.preference.setBool = function(prefName, value) {
  activity.preference.prefClass_.setBoolPref(
      activity.preference.fullPrefName_(prefName),
      value);
};

/**
 * Get the value of a preference.  The preference can be a string,
 * int, or bool.  Return undefined if there is no such preference.
 * @param {string} prefName The name of the preference to fetch.
 * @param {string|number|boolean|undefined} opt_default The value to
 *    return if the preference is not set.
 * @param {activity.preference.PrefType} opt_expectedType The expected
 *    preference type.
 * @return {string|number|boolean|undefined} The preference value.
 *    Returns undefined if there is no such preference.
 * @private
 */
activity.preference.get_ = function(prefName,
                                  opt_default,
                                  opt_expectedType) {
  var fullPrefName = activity.preference.fullPrefName_(prefName);
  var result = opt_default;

  var prefType = activity.preference.prefClass_.getPrefType(fullPrefName);

  if (typeof opt_expectedType != 'undefined' &&
      prefType &&  // prefType is false if the pref does not exist.
      prefType != opt_expectedType) {
    throw new Error('Read preference ' + fullPrefName + ' expecting a ' +
                    activity.preference.prefTypeToString(opt_expectedType) +
                    ', but the pref is set to a ' +
                    activity.preference.prefTypeToString(prefType) + '.');
  }

  try {
    switch (prefType) {
      case activity.preference.PrefType.PREF_STRING:
        result = activity.preference.prefClass_.getCharPref(fullPrefName);
        break;
      case activity.preference.PrefType.PREF_INT:
        result = activity.preference.prefClass_.getIntPref(fullPrefName);
        break;
      case activity.preference.PrefType.PREF_BOOL:
        result = activity.preference.prefClass_.getBoolPref(fullPrefName);
        break;
    }
  } catch (e) {
    // Reading a pref with no value throws an exception.
    // result was set to the default value above, so do nothing.
  }

  return result;
};

/**
 * Get the value of an integer preference.
 *
 * @param {string} prefName Name of an activity profiler preference.
 * @param {number} opt_default Value returned if the preference is not set.
 * @return {number} Value of the preference.
 */
activity.preference.getInt = function(prefName, opt_default) {
  return Number(activity.preference.get_(
      prefName, opt_default, activity.preference.PrefType.PREF_INT));
};

/**
 * Get the value of a string preference.
 *
 * @param {string} prefName Name of an activity profiler preference.
 * @param {string} opt_default Value returned if the preference is not set.
 * @return {string} Value of the preference.
 */
activity.preference.getString = function(prefName, opt_default) {
  return String(activity.preference.get_(
      prefName, opt_default, activity.preference.PrefType.PREF_STRING));
};

/**
 * Get the value of a boolean preference.
 *
 * @param {string} prefName Name of an activity profiler preference.
 * @param {boolean} opt_default Value returned if the preference is not set.
 * @return {boolean} Value of the preference.
 */
activity.preference.getBool = function(prefName, opt_default) {
  return Boolean(activity.preference.get_(
      prefName, opt_default, activity.preference.PrefType.PREF_BOOL));
};
