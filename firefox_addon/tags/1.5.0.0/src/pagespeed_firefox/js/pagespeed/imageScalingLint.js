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
 * @fileoverview Tests for images that are downsampled in HTML.
 *
 * @author Tony Gentilcore
 * @author Bryan McQuade
 */

(function() {  // Begin closure

/**
 * Find all downsampled images.
 * @param {Array.<string>} imgs Array of image URLs referenced on
 *     this page.
 * @param {ImageDataContainer} results A map from url to ImageData objects.
 */
function findResizableImages(imgs, results) {
  if (!imgs) {
    return;
  }

  // Keep track of the least-scaled version of each image on the
  // page. For instance, if an image appears twice, once unscaled and
  // another time scaled down, we should not penalize the page, since
  // they are using the unscaled version somewhere. However, if an
  // image is used more than once, and is scaled each time, we should
  // penalize them based on the largest scaled size.
  for (var img in imgs) {
    for (var i = 0, len = imgs[img].elements.length; i < len; ++i) {
      var elem = imgs[img].elements[i];
      if (elem.tagName != 'IMG') continue;

      if (elem.clientWidth && elem.clientHeight &&
          elem.naturalWidth && elem.naturalHeight) {
        var data = results.getData(img);
        data.setResizable(elem.naturalWidth,
                          elem.naturalHeight,
                          elem.clientWidth,
                          elem.clientHeight);
      }
    }
  }
}

/**
 * Data structure to hold image natural size and scaling information.
 * @constructor
 */
function ImageData(url) {
  /** @type {string} */
  this.url = url;

  /** @type {number} */
  this.origSize = PAGESPEED.Utils.getResourceSize(url);

  /** @type {number} */
  this.naturalWidth = -1;

  /** @type {number} */
  this.naturalHeight = -1;

  /** @type {number} */
  this.clientWidth = -1;

  /** @type {number} */
  this.clientHeight = -1;

  /** @type {boolean} */
  this.sizeMismatchFound = false;
}

/**
 * @return {boolean} whether or not the image is resizable.
 */
ImageData.prototype.isResizable = function() {
  return (!this.sizeMismatchFound && 
	  this.clientWidth >= 0 && this.clientHeight >= 0 && 
          (this.clientWidth < this.naturalWidth || 
           this.clientHeight < this.naturalHeight));
};

/**
 * @param {number} naturalWidth The actual width of the image.
 * @param {number} naturalHeight The actual height of the image.
 * @param {number} clientWidth The width of the image in the HTML.
 * @param {number} clientHeight The height of the image in the HTML.
 */
ImageData.prototype.setResizable = function(
    naturalWidth, naturalHeight, clientWidth, clientHeight) {
  if (this.naturalHeight != -1 || this.naturalWidth != -1) {
    // The image is already known to be resizable.  Make sure that the
    // natural dimensions match what we recorded last time.  If they
    // do not, flag the mismatch as an error condition to mark the
    // image as not-resizable.
    if (naturalHeight != this.naturalHeight ||
        naturalWidth != this.naturalWidth) {
      // Log size mismatches.
      PS_LOG('Mismatched width/height parameters while processing ',
	     this.url, '. Got ', naturalWidth, 'x', naturalHeight,
	     ' Expected ', this.naturalWidth, 'x', this.naturalHeight);
      this.sizeMismatchFound = true;
      return;
    }
  }

  this.naturalWidth = naturalWidth;
  this.naturalHeight = naturalHeight;
  this.clientWidth = Math.min(Math.max(this.clientWidth, clientWidth),
                              naturalWidth);
  this.clientHeight = Math.min(Math.max(this.clientHeight, clientHeight),
                               naturalHeight);
};

/**
 * @return {number} the number of bytes saved by compression and
 *     resizing.
 */
ImageData.prototype.getBytesSaved = function() {
  return this.origSize * this.getCompressionAmount();
};

/**
 * @return {number} the amount that this image can be compressed
 *     (0..1, where 0 indicates not at all, and 1 indicates full
 *     compression down to zero bytes).
 */
ImageData.prototype.getCompressionAmount = function() {
  var resizingMultiplier = 1;
  if (this && this.isResizable()) {
    if (this.clientWidth < this.naturalWidth) {
      resizingMultiplier *= this.clientWidth / this.naturalWidth;
    }
    if (this.clientHeight < this.naturalHeight) {
      resizingMultiplier *= this.clientHeight / this.naturalHeight;
    }
  }
  return 1 - resizingMultiplier;
};

/**
 * @return {string} A human-readable results string.
 */
ImageData.prototype.buildResultString = function() {
  var resizable = this.isResizable();
  if (!resizable) {
    return '';
  }

  return [this.url,
	  ' is scaled in HTML or CSS from ',
	  this.naturalWidth,
	  'x',
          this.naturalHeight,
          ' to ',
	  this.clientWidth,
	  'x',
          this.clientHeight,
          '.  Serving a resized image could save ~',
          PAGESPEED.Utils.formatBytes(this.getBytesSaved()),
          ' (',
          PAGESPEED.Utils.formatPercent(this.getCompressionAmount()),
          ' reduction).'].join('');
};

/**
 * Object that holds a collection of ImageData objects.
 * @constructor
 */
function ImageDataContainer() {
  this.data_ = {};
}

/**
 * @param {string} url The url of the image.
 * @param {number} size The size of the image, in bytes.
 * @return {ImageData} the ImageData for the given URL.
 */
ImageDataContainer.prototype.getData = function(url) {
  if (!this.data_[url]) {
    this.data_[url] = new ImageData(url);
  }
  return this.data_[url];
};

/**
 * @return {Array.<ImageData>} Array of all image
 *     compression results, sorted by bytes saved.
 */
ImageDataContainer.prototype.getDataArray = function() {
  var aData = [];
  for (var url in this.data_) {
    aData.push(this.data_[url]);
  }
  aData.sort(sortByBytesSaved);
  return aData;
};

// Sorts the compressed images by most bytes saved.
function sortByBytesSaved(a, b) {
  var aSaved = a.getBytesSaved();
  var bSaved = b.getBytesSaved();
  return (aSaved > bSaved) ? -1 : (aSaved == bSaved) ? 0 : 1;
}

/**
 * @param {string} url the URL of the resource.
 * @return {string?} the default file extension for the image at the
 *     given URL, based on the resource's Content-Type, or null.
 */
function getFileExtension(url) {
  var contentType = getContentType(url);
  if (contentType == 'image/png') {
    return 'png';
  }

  if (contentType == 'image/gif') {
    return 'gif';
  }

  if (contentType == 'image/jpg' || contentType == 'image/jpeg') {
    return 'jpg';
  }

  return null;
}

/**
 * @param {string} url the URL of the resource.
 * @return {string?} the default file extension for the type that we would
 *     compress the image at the given URL to, based on the resource's
 *     Content-Type, or null.
 */
function getCompressedFileExtension(url) {
  var extension = getFileExtension(url);
  if (extension == 'png' || extension == 'gif') {
    // png and gif get compressed to png
    return 'png';
  }
  if (extension == 'jpg') {
    // jpg gets losslessly compressed to jpg
    return 'jpg';
  }
  return null;
}

/**
 * @param {string} url The URL to get the content-type for.
 * @return {string?} the content-type header, or null.
 */
function getContentType(url) {
  // TODO: handle data: URIs here (extract content type
  // from the URL).
  var responseHeaders = PAGESPEED.Utils.getResponseHeaders(url);
  if (!responseHeaders) {
    return null;
  }
  var contentType;
  for (var header in responseHeaders) {
    // TODO: move case-insensitive header search
    // functionality into a common location like util.js.
    if (header.toLowerCase() == 'content-type') {
      return responseHeaders[header];
    }
  }

  return null;
}

/**
 * @param {string} url The URL of the resource to prepare.
 * @param {string} extension the file extension to use.
 * @return {nsIFile} a file that contains the contents of the image we
 *     will attempt to compress.
 */
function prepareFileForCompression(url, extension) {
  var compressedFile = PAGESPEED.Utils.getOutputDir(COMPRESSED_IMAGE_DIR_NAME);
  if (!compressedFile) {
    return null;
  }
  var inputStream = PAGESPEED.Utils.getResourceInputStream(url);
  var humanFileName = PAGESPEED.Utils.getHumanFileName(url);
  var hash = PAGESPEED.Utils.getMd5HashForInputStream(inputStream);
  inputStream.close();

  var fileName = [[humanFileName, hash].join('_'), extension].join('.');
  compressedFile.append(fileName);
  if (compressedFile.exists()) {
    // If the file exists, remove it.
    compressedFile.remove(true);
  }
  inputStream = PAGESPEED.Utils.getResourceInputStream(url);
  PAGESPEED.Utils.copyCompleteInputToOutput(
      inputStream,
      PAGESPEED.Utils.openFileForWriting(compressedFile.path));

  return compressedFile;
}

/**
 * @this PAGESPEED.LintRule
 */
var imageScaleLint = function() {
  var urls = PAGESPEED.Utils.getResources('image', 'cssimage', 'favicon');

  // Shortcut to get out early if the page doesn't have any images.
  if (urls.length == 0) {
    this.score = 'n/a';
    return;
  }

  this.results = new ImageDataContainer();

  this.addContinuation(function() {
    findResizableImages(PAGESPEED.Utils.getComponents().image, this.results);

    var aResults = this.results.getDataArray();

    var aWarnings = [];
    var origBytes = 0;
    var totalWastedBytes = 0;
    for (var i = 0, len = aResults.length; i < len; i++) {
      var data = aResults[i];
      origBytes += data.origSize;
      totalWastedBytes += data.getBytesSaved();
      var result = data.buildResultString();
      if (result) {
        aWarnings.push(result);
      }
    }

    // Compute the score by looping through all image warnings and deducting
    // the proper amount based on the size of the reduction.
    if (aWarnings.length > 0) {
      if (totalWastedBytes && origBytes) {
        this.score -= 1.5 * parseInt(totalWastedBytes / origBytes * 100, 10);
        this.warnings += [
          'The following images are scaled in HTML or CSS.  ',
          'Serving resized images could save ~',
          PAGESPEED.Utils.formatBytes(totalWastedBytes),
          ' (',
          PAGESPEED.Utils.formatPercent(
            totalWastedBytes / origBytes),
          ' reduction).'].join('');
      }

      var messages = PAGESPEED.Utils.formatWarnings(aWarnings);
      this.warnings += messages;
    }
  });

  this.doneAddingContinuations();
};

PAGESPEED.LintRules.registerLintRule(
  new PAGESPEED.LintRule(
    'Serve scaled images',
    PAGESPEED.PAYLOAD_GROUP,
    'payload.html#ScaleImages',
    imageScaleLint,
    3.0,
    'ScaleImgs'
  )
);

})();  // End closure
