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
 * @fileoverview Provides the ability to call out to java binaries from
 *               within Pagespeed.
 *
 * @author Michael Bolin
 * @author Antonio Vicente
 */

(function() {  // Begin closure

/**
 * Returns true if Java is enabled; otherwise, returns false.
 * @return {boolean}
 */
function isJavaEnabled() {
  var navigator = window.navigator;
  try {
    return navigator.javaEnabled && navigator.javaEnabled();
  } catch (e) {
    return false;
  }
}

/**
 * Returns true if a recent enough Sun Java plugin w/ live connect support is
 * installed; otherwise, returns false.
 * @return {boolean}
 */
function hasValidJavaPluginVersion() {
  var required_major = 6;
  var required_minor = 12;
  for(var i = 0; i < navigator.plugins.length; i++) {
    var pluginName = navigator.plugins[i] ? navigator.plugins[i].name : '';
    if (!pluginName) {
      continue;
    }
    var match = pluginName.match(/Java\(TM\) Platform SE (\d+) U(\d+)/);
    if (match) {
      return ((match[1] > required_major) ||
	      ((match[1] == required_major) && (match[2] >= required_minor)));
    }
  }
  return false;
}

/**
 * Returns a subclass of java.security.Policy that whitelists Java code
 * loaded from jars at a specified URLs.
 * @return {Object} edu.mit.simile.javaFirefoxExtensionUtils.URLSetPolicy
 */
function createPolicy(urls) {
  var policy = null;
  var jarUrl = new Packages.java.net.URL(
    PAGESPEED.Utils.getExtensionRoot() + 'java/policy.jar');
  var policyClassLoader = new Packages.java.net.URLClassLoader([jarUrl]);
  var policyClass = Packages.java.lang.Class.forName(
      'edu.mit.simile.javaFirefoxExtensionUtils.URLSetPolicy',
      true, /* initialize */
      policyClassLoader);
  // The Java plugin for Mac does not appear to be able to load URLSetPolicy,
  // so policyClass is tested to make sure it is not null.
  if (policyClass) {
    var policy = policyClass.newInstance();
    policy.setOuterPolicy(Packages.java.security.Policy.getPolicy());
    policy.addPermission(new Packages.java.security.AllPermission());
    for each (var url in urls) {
      policy.addURL(url);
    }
    return policy;
  } else {
    return null;
  }
}

/**
 * Java extension wrapper.  Holds a description of a Java extension runnable
 * by calling PAGESPEED.javaExtensionRunner(extension)
 *
 * @param {string} appName Extension name
 * @param {Array} jarFileName Array of java jars to load.
 * @param {string} className Name of the java class to instantiate.
 * @param {Function} closure Function to run on the java extension object.
 * @constructor
 */
PAGESPEED.JavaExtension = function(appName, jarFileNames, className, closure) {
  this.appName = appName;
  this.jarFileNames = jarFileNames;
  this.className = className;
  this.closure = closure;
};

/**
 * Java extension wrapper.  Creates an object of the extension type and
 * calls the "closure" function with that object as an argument.
 *
 * @param {PAGESPEED.JavaExtension} extension Extension to run.
 */
PAGESPEED.javaExtensionRunner = function(extension) {
  if (!isJavaEnabled()) {
    throw new Error('Java support must be enabled to run ' +
                    extension.appName + '.');
  }

  if (!hasValidJavaPluginVersion()) {
    throw new Error(extension.appName + ' supports requires Sun Java ' +
                    'plug-in version 6 U12 or later, which is ' +
                    'available at http://www.java.com/.');
  }

  var urls = [];
  for each (var jarFileName in extension.jarFileNames) {
    var url = new Packages.java.net.URL(jarFileName);
    urls.push(url);
  }

  var originalPolicy = Packages.java.security.Policy.getPolicy();
  var policy = createPolicy(urls);
  if (originalPolicy && policy) {
    try {
      // TODO Figure out how to write extensions so modifying the global
      // policy is not necessary.
      Packages.java.security.Policy.setPolicy(policy);

      var classLoader = new Packages.java.net.URLClassLoader(urls);
      if (classLoader) {
	var instance = Packages.java.lang.Class.forName(
          extension.className,
	  true, /* initialize */
	  classLoader).newInstance();
	if (instance) {
          return extension.closure(instance);
	}
      }
    } finally {
      Packages.java.security.Policy.setPolicy(originalPolicy);
    }
  }

  throw new Error('Error initializing ' + extension.appName + '.');
};

})();  // End closure
