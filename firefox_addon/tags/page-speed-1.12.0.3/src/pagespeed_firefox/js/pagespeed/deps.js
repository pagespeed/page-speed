/**
 * Copyright 2007-2009 Google Inc.
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
 * @fileoverview Performs preliminary dependency check for Page Speed.
 *
 * @author Kyle Scholz
 */

(function() {  // Begin closure

PAGESPEED.Utils.fetchPageSpeedVersion();

PAGESPEED.Utils.updateDependencyVersions(function() {
  /**
   * If requried extensions are missing, alert and go unhealthy.
   */
  var missingExtensions = [];
  for (var addonName in PAGESPEED.DEPENDENCIES) {
    if (!PAGESPEED.DEPENDENCIES[addonName].installedVersion) {
      PAGESPEED.isHealthy = false;
      missingExtensions.push(addonName);
    }
  }

  if (missingExtensions.length > 0) {
    var message = 'The following extension(s) are required by Page Speed\n ' +
      'and must be installed before it can run:\n\n';
    for (var i = 0; i < missingExtensions.length; i++) {
      var addonName = missingExtensions[i];
      message += '\t - ' + addonName
          + ' (' + PAGESPEED.DEPENDENCIES[addonName].url + ')\n';
    }
    alert(message);
  }
});

})();  // End closure
