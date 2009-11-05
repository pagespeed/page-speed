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
 * @fileoverview XPCom service, implemented as a wrappedJSObject, that runs
 *               Closure Compiler on a javascript source string.
 */

var IClosureCompilerIface = Components.interfaces.IClosureCompiler;
var nsISupportsIface = Components.interfaces.nsISupports;

var CLASS_ID = Components.ID('{5b47742c-b63d-493d-be8c-bab834d10704}');
var CLASS_NAME = 'ClosureCompilerService';
var CONTRACT_ID = '@code.google.com/p/page-speed/ClosureCompilerService;1';
var CLOSURE_COMPILER_GUID = '{70a9aa80-d283-4eae-8a87-ee7b769edf53}';

/**
 * ClosureCompiler Constructor.
 * @constructor
 */
function ClosureCompilerService() {
  // Exposing the wrappedJSObject property in this way allows callers
  // to access the native JS object, not just the xpconnect-wrapped
  // object. This allows access to the full ClosureCompilerService
  // API, not just the subset of the API exported via the
  // IClosureCompiler interface.
  this.wrappedJSObject = this;
}

/**
 * nsISupports implementation
 * Return "this" if the interface is supported, otherwise throw an exception.
 * @param {Object} aIID XPCOM interface.
 * @return {ClosureCompilerService} this if the interface is supported.
 */
ClosureCompilerService.prototype.QueryInterface = function(aIID) {
  if (!aIID.equals(IClosureCompilerIface) &&
      !aIID.equals(nsISupportsIface)) {
    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
  return this;
};

/**
 * Returns the directory where an extension is installed expressed as a file:
 * URL terminated by a slash.
 * @return {string} slash terminated file: url.
 */
ClosureCompilerService.prototype.getExtensionFileUrl = function() {
  var mgr = Components.
      classes['@mozilla.org/extensions/manager;1'].
      getService(Components.interfaces.nsIExtensionManager);
  var loc = mgr.getInstallLocation(CLOSURE_COMPILER_GUID);
  if (!loc) {
    return undefined;
  }
  var file = loc.getItemLocation(CLOSURE_COMPILER_GUID);
  if (!file) {
    return undefined;
  }
  var uri = Components.classes['@mozilla.org/network/protocol;1?name=file'].
      getService(Components.interfaces.nsIFileProtocolHandler).
      newFileURI(file);
  return uri.spec;
};

/**
 * Compile the provided js source.
 * @param {Object} pagespeed PAGESPEED namespace object.
 * @param {string} source javascript source to compile.
 * @return {string} compiled source.
 */
ClosureCompilerService.prototype.compile = function(pagespeed, source) {
  var jarFileNames = [];
  var jarNames = ['simple_compiler.jar', 'compiler.jar'];
  for (var i = 0; i < jarNames.length; i++) {
    jarFileNames.push(this.getExtensionFileUrl() + 'java/' + jarNames[i]);
  }

  var extension = new pagespeed.JavaExtension(
    'Closure Compiler',
    jarFileNames,
    'com.google.code.pagespeed.SimpleCompiler',
    function(compiler) { return compiler.doCompile(source); });
  return pagespeed.javaExtensionRunner(extension);
};

/**
 * Factory method for creating an ClosureCompilerService instance.
 */
var ClosureCompilerFactory = {
  /**
   * @return {ClosureCompilerService} closure compiler instance.
   */
  createInstance: function(aOuter, aIID) {
    if (aOuter != null)
      throw Components.results.NS_ERROR_NO_AGGREGATION;
    return (new ClosureCompilerService()).QueryInterface(aIID);
  }
};

// nsIModule
var ClosureCompilerModule = {
  registerSelf: function(aCompMgr, aFileSpec, aLocation, aType) {
    aCompMgr = aCompMgr.
        QueryInterface(Components.interfaces.nsIComponentRegistrar);
    aCompMgr.registerFactoryLocation(CLASS_ID, CLASS_NAME,
        CONTRACT_ID, aFileSpec, aLocation, aType);

    var aCatMgr = Components.classes['@mozilla.org/categorymanager;1']
                  .getService(Components.interfaces.nsICategoryManager);
    aCatMgr.addCategoryEntry('closure-compiler', CONTRACT_ID,
                             CONTRACT_ID, true, true);
  },

  unregisterSelf: function(aCompMgr, aLocation, aType) {
    aCompMgr = aCompMgr.
        QueryInterface(Components.interfaces.nsIComponentRegistrar);
    aCompMgr.unregisterFactoryLocation(CLASS_ID, aLocation);

    var aCatMgr = Components.classes['@mozilla.org/categorymanager;1']
                  .getService(Components.interfaces.nsICategoryManager);
    aCatMgr.deleteCategoryEntry('closure-compiler', CONTRACT_ID, true);
  },

  getClassObject: function(aCompMgr, aCID, aIID) {
    if (!aIID.equals(Components.interfaces.nsIFactory))
      throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

    if (aCID.equals(CLASS_ID))
      return ClosureCompilerFactory;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  canUnload: function(aCompMgr) { return true; }
};

function NSGetModule(aCompMgr, aFileSpec) {
  return ClosureCompilerModule;
}
