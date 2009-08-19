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
 * @fileoverview Queries image compression component which uses
 * optipng and libjpeg to determine which images on the page can be
 * compressed further.  Also tests for images that are downsampled in
 * HTML.
 *
 * @author Tony Gentilcore
 * @author Bryan McQuade
 */

(function() {  // Begin closure

/**
 * Minimum number of bytes we expect to save when compressing an image
 * before we warn the user about it.
 * @type {number}
 */
var MIN_BYTES_SAVED = 25;

/**
 * Name of the directory where we store compressed images.
 * @type {string}
 */
var COMPRESSED_IMAGE_DIR_NAME = 'page-speed-images';

/**
 * Permissions of the directory where we store compressed images.
 * @type {number}
 */
var COMPRESSED_IMAGE_DIR_PERMISSIONS = 0755;

/**
 * Data structure to hold the result of an image compression attempt.
 * @constructor
 */
function ImageCompressionData(url, origSize) {
  /** @type {string} */
  this.url = url;

  /** @type {number} */
  this.origSize = origSize;

  /** @type {number} */
  this.compressedSize = origSize;
}

/** @type {string?} */
ImageCompressionData.prototype.compressedUrl = null;

/** @type {number} */
ImageCompressionData.prototype.naturalWidth = -1;

/** @type {number} */
ImageCompressionData.prototype.naturalHeight = -1;

/** @type {number} */
ImageCompressionData.prototype.clientWidth = -1;

/** @type {number} */
ImageCompressionData.prototype.clientHeight = -1;

/**
 * @param {string} url The url of the compressed version of the image.
 * @param {number} size the size, in bytes, of the compressed image.
 */
ImageCompressionData.prototype.setCompressible = function(url, size) {
  this.compressedUrl = url;
  this.compressedSize = size;
};

/**
 * @param {number} naturalWidth The actual width of the image.
 * @param {number} naturalHeight The actual height of the image.
 * @param {number} clientWidth The width of the image in the HTML.
 * @param {number} clientHeight The height of the image in the HTML.
 */
ImageCompressionData.prototype.setResizable = function(
    naturalWidth, naturalHeight, clientWidth, clientHeight) {
  if (this.isResizable()) {
    // The image is already known to be resizable. We want to keep
    // track of the largest instance of this image on the page, so
    // check and see if the new client size is larger than the
    // previously recorded client size.
    if (naturalHeight != this.naturalHeight ||
        naturalWidth != this.naturalWidth) {
      // Sanity check that the size is the same.
      throw new Error('Mismatched width/height parameters!');
    }
    var currentClientSizePx = this.clientWidth * this.clientHeight;
    var newClientSizePx = clientWidth * clientHeight;
    if (currentClientSizePx > newClientSizePx) {
      // Old size is larger, so don't update.
      return;
    }
  }
  this.naturalWidth = naturalWidth;
  this.naturalHeight = naturalHeight;
  this.clientWidth = clientWidth;
  this.clientHeight = clientHeight;
};

/**
 * @return {boolean} whether or not the image is losslessly compressible.
 */
ImageCompressionData.prototype.isCompressible = function() {
  return this.compressedSize > 0 &&
      this.origSize > 0 &&
      this.compressedSize < this.origSize;
};

/**
 * @return {boolean} whether or not the image is resizable.
 */
ImageCompressionData.prototype.isResizable = function() {
  var naturalSizePx = this.naturalWidth * this.naturalHeight;
  var clientSizePx = this.clientWidth * this.clientHeight;
  return clientSizePx >= 0 &&
      naturalSizePx >= 0 &&
      clientSizePx < naturalSizePx;
};

/**
 * @return {number} the number of bytes saved by compression and
 *     resizing.
 */
ImageCompressionData.prototype.getBytesSaved = function() {
  return this.origSize * this.getCompressionAmount();
};

/**
 * @return {number} the amount that this image can be compressed
 *     (0..1, where 0 indicates not at all, and 1 indicates full
 *     compression down to zero bytes).
 */
ImageCompressionData.prototype.getCompressionAmount = function() {
  var resizingMultiplier = 1;
  if (this.isResizable()) {
    var naturalSizePx = this.naturalWidth * this.naturalHeight;
    var clientSizePx = this.clientWidth * this.clientHeight;
    resizingMultiplier = clientSizePx / naturalSizePx;
  }

  var compressionMultiplier = 1;
  if (this.isCompressible()) {
    compressionMultiplier = this.compressedSize / this.origSize;
  }

  var savingsMultiplier = resizingMultiplier * compressionMultiplier;
  return 1 - savingsMultiplier;
};

/**
 * @return {string} A human-readable results string.
 */
ImageCompressionData.prototype.buildResultString = function() {
  var resizable = this.isResizable();
  var compressible = this.isCompressible();
  if (!resizable && !compressible) {
    return '';
  }
  var introString = '';
  var compressString = '';
  var resizeString = '';
  if (resizable && compressible) {
    introString = 'Compressing and resizing ';
  } else if (resizable) {
    introString = 'Resizing ';
  } else if (compressible) {
    introString = 'Compressing ';
  }
  if (resizable) {
    var naturalDimensions = [this.naturalWidth, this.naturalHeight].join('x');
    var clientDimensions = [this.clientWidth, this.clientHeight].join('x');
    resizeString = ['. The image is scaled in HTML from ',
                    naturalDimensions,
                    ' to ',
                    clientDimensions].join('');
  }
  if (compressible) {
    compressString = ['. See ', this.compressedUrl].join('');
  }
  return [introString,
          this.url,
          ' could save ',
          // If the image is resizable, the savings are an estimate.
          resizable ? '~' : '',
          PAGESPEED.Utils.formatBytes(this.getBytesSaved()),
          ' (',
          PAGESPEED.Utils.formatPercent(this.getCompressionAmount()),
          ' reduction)',
          resizeString,
          compressString,
          '.'].join('');
};

/**
 * Object that holds a collection of ImageCompressionData objects.
 * @constructor
 */
function ImageCompressionDataContainer() {
  this.data_ = {};
}

/**
 * @param {string} url The url of the image.
 * @param {number} size The size of the image, in bytes.
 * @return {ImageCompressionData} the ImageCompressionData for the
 *     given URL.
 */
ImageCompressionDataContainer.prototype.getData = function(url, size) {
  if (!this.data_[url]) {
    this.data_[url] = new ImageCompressionData(url, size);
  }
  return this.data_[url];
};

/**
 * @return {Array.<ImageCompressionData>} Array of all image
 *     compression results, sorted by bytes saved.
 */
ImageCompressionDataContainer.prototype.getDataArray = function() {
  var aData = [];
  for (var url in this.data_) {
    aData.push(this.data_[url]);
  }
  aData.sort(sortByBytesSaved);
  return aData;
};

var compressor = PAGESPEED.Utils.CCSV(
    '@code.google.com/p/page-speed/ImageCompressor;1', 'IImageCompressor');

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
  var hash = PAGESPEED.Utils.getMd5HashForInputStream(inputStream);
  inputStream.close();

  var fileName = [hash, extension].join('.');
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
 * @param {string} url The URL of the image we're trying to compress.
 * @param {ImageCompressionData} data the result of the compression
 *     attempt.
 */
function tryToCompressImage(url, data) {
  // Get the file extension for the file type we are going to compress
  // this resource to (png, jpg, or null).
  var extension = getCompressedFileExtension(url);
  if (!extension) {
    return;
  }
  var compressedFile = prepareFileForCompression(url, extension);
  if (!compressedFile) {
    return;
  }
  var compressedPath = compressedFile.path;

  var success = false;
  try {
    if (extension == 'png') {
      compressor.compressToPng(compressedPath, compressedPath);
      success = true;
    } else if (extension == 'jpg') {
      compressor.compressJpeg(compressedPath, compressedPath);
      success = true;
    }
  } finally {
    if (!success) {
      compressedFile.remove(true);
    }
  }

  if (!compressedFile.exists()) {
    return;
  }

  data.setCompressible(
      PAGESPEED.Utils.getUrlForFile(compressedFile), compressedFile.fileSize);

  // Make sure the temp file gets deleted when Firefox exits.
  PAGESPEED.Utils.deleteTemporaryFileOnExit(compressedFile);
}

/**
 * Find all downsampled images.
 * @param {Array.<string>} imgs Array of image URLs referenced on
 *     this page.
 * @param {ImageCompressionDataContainer} results A map from url to
 *     ImageCompressionData objects.
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
  var candidateMap = {};
  for (var img in imgs) {
    for (var i = 0, len = imgs[img].elements.length; i < len; ++i) {
      var elem = imgs[img].elements[i];
      if (elem.tagName != 'IMG') continue;

      if (elem.clientWidth && elem.clientHeight &&
          elem.naturalWidth && elem.naturalHeight) {
        var data = results.getData(img, PAGESPEED.Utils.getResourceSize(img));
        data.setResizable(elem.naturalWidth,
                          elem.naturalHeight,
                          elem.clientWidth,
                          elem.clientHeight);
      }
    }
  }
}

/**
 * Given a URL, return a function that tries to compress it.
 * @this PAGESPEED.LintRule
 * @param {string} url Url of the image to compress.
 * @return {Function} A function that will try to compress the image at
 *     the given Url.
 */
function buildImageCompressionCallback(url) {
  return function() {
    try {
      var data = this.results.getData(
          url, PAGESPEED.Utils.getResourceSize(url));
      tryToCompressImage(url, data);
    } catch (e) {
      // Unable to compress this image. Could be an animated gif,
      // which can't be converted to png. Ignore and try to compress
      // the next one.
      PS_LOG('Unable to compress image ' + url);
    }

    return url;
  };
}

/**
 * @this PAGESPEED.LintRule
 */
var imageCompressionLint = function() {
  if (!compressor) {
    this.score = 'error';
    var xulRuntime = PAGESPEED.Utils.CCSV(
        '@mozilla.org/xre/app-info;1', 'nsIXULRuntime');
    this.warnings = ['Unable to compress images on your platform: ',
                     xulRuntime.OS, xulRuntime.XPCOMABI].join(' ');
    return;
  }

  var imagesDir = PAGESPEED.Utils.getOutputDir(COMPRESSED_IMAGE_DIR_NAME);
  if (!imagesDir) {
    this.score = 'error';
    this.warnings = ['Unable to compress images because the directory where ',
                     'optimized images are saved can not be ',
                     'accessed.  Click the black triangle in the Page Speed ',
                     'tab and select a different path under "Save Optimized ',
                     'Files To:".'].join('');
    return;
  }

  var urls = PAGESPEED.Utils.getResources('image', 'cssimage', 'favicon');

  // Shortcut to get out early if the page doesn't have any images.
  if (urls.length == 0) {
    this.score = 'n/a';
    return;
  }

  this.results = new ImageCompressionDataContainer();

  // For each compressable image, add a function to do the compression.
  for (var i = 0, len = urls.length; i < len; i++) {
    // Add a continuation that will try to compress the image.
    this.addContinuation(buildImageCompressionCallback(urls[i]));
  }

  this.addContinuation(function() {
    findResizableImages(PAGESPEED.Utils.getComponents().image, this.results);

    var aResults = this.results.getDataArray();

    var aWarnings = [];
    var origBytes = 0;
    var totalWastedBytes = 0;
    for (var i = 0, len = aResults.length; i < len; i++) {
      var data = aResults[i];
      origBytes += data.origSize;
      if (data.getBytesSaved() > MIN_BYTES_SAVED) {
        totalWastedBytes += data.getBytesSaved();
        aWarnings.push(data.buildResultString());
      }
    }

    // Compute the score by looping through all image warnings and deducting
    // the proper amount based on the size of the reduction.
    if (aWarnings.length > 0) {
      if (totalWastedBytes && origBytes) {
        this.score -= 1.5 * parseInt(totalWastedBytes / origBytes * 100, 10);
        this.warnings += ['There are ', PAGESPEED.Utils.formatBytes(origBytes),
                          ' worth of images. Optimizing them could',
                          ' save ~',
                          PAGESPEED.Utils.formatBytes(totalWastedBytes),
                          ' (',
                          PAGESPEED.Utils.formatPercent(
                              totalWastedBytes / origBytes),
                          ' reduction).'].join('');
      }

      var messages = PAGESPEED.Utils.formatWarnings(aWarnings);

      // Remove the path from the text of the link to the compressed images, to
      // make it more readable.
      var imagesDir = PAGESPEED.Utils.getOutputDir(COMPRESSED_IMAGE_DIR_NAME);
      if (imagesDir) {
        var imageDirUrl = PAGESPEED.Utils.getUrlForFile(imagesDir);
        var imageDirRegexp = new RegExp(
            ['>', imageDirUrl, '[^<]*'].join(''), 'g');
        messages = messages.replace(imageDirRegexp, '>compressed version');
      }
      this.warnings += messages;
    }
  });

  this.doneAddingContinuations();
};

PAGESPEED.LintRules.registerLintRule(
  new PAGESPEED.LintRule(
    'Optimize images',
    PAGESPEED.PAYLOAD_GROUP,
    'payload.html#CompressImages',
    imageCompressionLint,
    3.0,
    'OptImgs'
  )
);

})();  // End closure
