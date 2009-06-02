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
 * @fileoverview Simple file utilities used to create temporary
 * directories and files.
 * @author David Bau
 * @author Bryan McQuade
 */

goog.provide('activity.fileutil');

/**
 * Create a new directory in the system temp dir. Guaranteed to be unique.
 * For instance, if you pass in 'foo' and the system temp dir is '/tmp/',
 * this function will attempt to create a directory '/tmp/foo/'. If that
 * directory exists, it will attempt to create '/tmp/foo-1/', then
 * '/tmp/foo-2/', etc.
 * @param {string} name Name of the subdirectory to create
 *     within the system temp directory.
 * @return {nsIFile} handle to the created directory.
 */
activity.fileutil.createUniqueTempDir = function(name) {
  return activity.fileutil.createUniqueTempHelper_(name, true, 0755);
};

/**
 * Create a new file in the system temp dir. Guaranteed to be unique.
 * For instance, if you pass in 'foo' and the system temp dir is '/tmp/',
 * this function will attempt to create a file '/tmp/foo'. If that
 * directory exists, it will attempt to create '/tmp/foo-1', then
 * '/tmp/foo-2', etc.
 * @param {string} name Name of the subdirectory to create
 *     within the system temp directory.
 * @return {nsIFile} the created file.
 */
activity.fileutil.createUniqueTempFile = function(name) {
  return activity.fileutil.createUniqueTempHelper_(name, false, 0644);
};

/**
 * @return {nsIFile} the system temp dir.
 */
activity.fileutil.getTempDir = function() {
  /** @type nsIProperties */
  var directoryService = activity.xpcom.CCSV(
     '@mozilla.org/file/directory_service;1', 'nsIProperties');

  /** @type nsIFile */
  return directoryService.get('TmpD', activity.xpcom.CI('nsIFile'));
};

/**
 * @return {nsIFile} the user's home dir.
 */
activity.fileutil.getHomeDir = function() {
  /** @type nsIProperties */
  var directoryService = activity.xpcom.CCSV(
     '@mozilla.org/file/directory_service;1', 'nsIProperties');

  /** @type nsIFile */
  return directoryService.get('Home', activity.xpcom.CI('nsIFile'));
};

/**
 * Helper that creates a new file or directory in the system temp dir.
 * @param {string} name Name of the file/directory to create
 *     within the system temp directory.
 * @param {boolean} isDir whether or not to create a directory.
 * @param {number} perms permissions on the created file.
 * @private
 * @return {nsIFile} the created file.
 */
activity.fileutil.createUniqueTempHelper_ = function(name, isDir, perms) {
  var file = activity.fileutil.getTempDir();
  file.append(name);
  var type = isDir ? file.DIRECTORY_TYPE : file.NORMAL_FILE_TYPE;
  file.createUnique(type, perms);
  return file;
};

/**
 * Create the specified file in the specified directory.
 * @param {nsIFile} dir the directory to create the file in.
 * @param {string} name the name of the file.
 * @return {nsIFile} the created file.
 */
activity.fileutil.createFile = function(dir, name) {
  /** @type nsIComponentManager */
  var manager = Components.classes['@mozilla.org/file/local;1'];

  /** @type nsIFile */
  var file = manager.createInstance(Components.interfaces.nsILocalFile);
  file.initWithFile(dir);
  file.append(name);
  file.create(file.NORMAL_FILE_TYPE, 0644);
  return file;
};
