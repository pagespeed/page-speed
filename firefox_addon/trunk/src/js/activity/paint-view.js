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
 * @fileoverview Class responsible for rendering screenshots to the
 * canvas pane.
 *
 * @author Bryan McQuade
 */

goog.provide('activity.PaintView');

goog.require('goog.Disposable');

/**
 * @param {nsIDOMElement} paintPaneRoot The root DOM object that
 *     contains the canvas elements.
 * @param {nsIDOMElement} paintPaneSplitter The XUL element that
 *     is used to expand/collapse the paint pane.
 * @param {nsIDOMDocument} xulElementFactory A factory for creating DOM
 *     elements (e.g. the document object).
 * @param {Object} tabBrowser The tabbrowser instance.
 * @param {number} canvasWidth The width of the canvases, in pixels.
 * @constructor
 * @extends {goog.Disposable}
 */
activity.PaintView = function(paintPaneRoot,
                              paintPaneSplitter,
                              xulElementFactory,
                              tabBrowser,
                              canvasWidth) {
  goog.Disposable.call(this);

  /**
   * The root XUL dom element for our canvases.
   * @type {nsIDOMElement}
   * @private
   */
  this.paintPaneRoot_ = paintPaneRoot;

  /**
   * The XUL dom element that contains the canvases.
   * @type {nsIDOMElement}
   * @private
   */
  this.paintPaneContainer_ = paintPaneRoot.firstChild;

  /**
   * The XUL dom element used to expand/collapse the paint pane.
   * @type {nsIDOMElement}
   * @private
   */
  this.paintPaneSplitter_ = paintPaneSplitter;

  /**
   * A factory for creating DOM elements.
   * @type {nsIDOMDocument}
   * @private
   */
  this.xulElementFactory_ = xulElementFactory;

  /**
   * The tabbrowser object.
   * @type {Object}
   * @private
   */
  this.tabBrowser_ = tabBrowser;

  /**
   * Width of the canvases, in pixels.
   * @type {number}
   * @private
   */
  this.canvasWidth_ = canvasWidth;
};
goog.inherits(activity.PaintView, goog.Disposable);

/**
 * Minimum number of pixels to highlight in a given dimension (x or
 * y). If the highlighted region is smaller than this, we expand
 * it. We do this because it's difficult to see very small highlighted
 * regions.
 * @type {number}
 * @private
 */
activity.PaintView.MIN_HIGHLIGHTED_PIXELS_ = 14;

/**
 * Maximum number of pixels to capture from screen to canvas, in
 * either dimension (x or y). We set this limit because performing
 * screen captures of very large pages causes the browser to come to a
 * halt. Capturing max 2500x2500 is a nice compromise.
 * @type {number}
 * @private
 */
activity.PaintView.MAX_DIMENSION_PIXELS_ = 2500;

/**
 * @type {string}
 * @private
 */
activity.PaintView.WHITE_ = 'rgb(255,255,255)';

/**
 * @type {string}
 * @private
 */
activity.PaintView.BLACK_ = 'rgb(0, 0, 0)';

/**
 * @type {string}
 * @private
 */
activity.PaintView.OFFSCREEN_REGION_STYLE_ = 'rgba(32, 32, 32, 0.5)';

/**
 * @type {string}
 * @private
 */
activity.PaintView.HIGHLIGHT_REGION_STYLE_ = 'rgba(255, 255, 128, 0.5)';

/** @inheritDoc */
activity.PaintView.prototype.disposeInternal = function() {
  activity.PaintView.superClass_.disposeInternal.call(this);

  this.removeAllChildren_();

  this.xulElementFactory_ = null;
  this.paintPaneRoot_ = null;
  this.paintPaneContainer_ = null;
  this.paintPaneSplitter_ = null;
};

/**
 * @param {MozAfterPaintEvent} event The MozAfterPaint event.
 */
activity.PaintView.prototype.onPaintEvent = function(event) {
  if (this.isDisposed()) {
    return;
  }

  var win = event.target;
  if (win != win.top) {
    // We would like to capture paint events in child frames, but
    // doing so requires translating coordinate spaces between the
    // frames, which is non-trivial. Until we have the ability to
    // perform these translations, we do not perform screen captures
    // for child frames. Closure's style utility
    // http://code.google.com/p/doctype/source/browse/trunk/goog/style/style.js
    // looks like it can perform the needed translations.
    return;
  }

  // Only capture screenshots for the foreground tab.
  if (!this.tabBrowser_ ||
      !this.tabBrowser_.selectedBrowser ||
      !this.tabBrowser_.selectedBrowser.webProgress ||
      !this.tabBrowser_.selectedBrowser.webProgress.DOMWindow ||
      this.tabBrowser_.selectedBrowser.webProgress.DOMWindow != win) {
    return;
  }

  var contentWidth = win.innerWidth + win.scrollMaxX;
  var contentHeight = win.innerHeight + win.scrollMaxY;

  if (contentHeight <= 0 || contentWidth <= 0) {
    return;
  }

  var xOffset = 0;
  var clampedWidth =
      Math.min(contentWidth, activity.PaintView.MAX_DIMENSION_PIXELS_);
  if (clampedWidth < contentWidth) {
    if (win.scrollMaxX == 0) {
      // If we're clamping the width, center the content if it
      // can't be scrolled horizontally. We do this because most web
      // pages center themselves left-to-right within the browser.
      xOffset = Math.round((contentWidth - clampedWidth) / 2);
    } else {
      // Otherwise take a snapshot that starts at the x scroll point.
      var maxScrollX = contentWidth - activity.PaintView.MAX_DIMENSION_PIXELS_;
      xOffset = Math.max(0, Math.min(maxScrollX, win.scrollX));
    }
  }

  var yOffset = 0;
  var clampedHeight =
      Math.min(contentHeight, activity.PaintView.MAX_DIMENSION_PIXELS_);
  if (clampedHeight < contentHeight) {
    // Start the snapshot at the y scroll point.
    var maxScrollY = contentHeight - activity.PaintView.MAX_DIMENSION_PIXELS_;
    yOffset = Math.max(0, Math.min(maxScrollY, win.scrollY));
  }

  // TODO: consider returning early if event.boundingClientRect falls
  // entirely outside of the clipped region.

  var ratio = clampedWidth / clampedHeight;
  var canvasHeight = Math.round(this.canvasWidth_ / ratio);

  var canvas = this.xulElementFactory_.createElementNS(
      'http://www.w3.org/1999/xhtml', 'canvas');

  // Specify the canvas coordinates. These are internal to the canvas
  // and have nothing to do with how the canvas is rendered to the
  // screen.
  canvas.width = this.canvasWidth_;
  canvas.height = canvasHeight;

  // Specify the properties of the canvas within the canvas pane.
  canvas.style.width = this.canvasWidth_ + 'px';
  canvas.style.height = canvasHeight + 'px';
  canvas.className = 'screenshot';

  var ctx = canvas.getContext('2d');
  ctx.clearRect(0, 0, this.canvasWidth_, canvasHeight);
  ctx.save();

  // Scale the canvas so we can use screen coordinates.
  ctx.scale(this.canvasWidth_ / clampedWidth, canvasHeight / clampedHeight);

  // Draw the window contents into the canvas.
  ctx.drawWindow(
      win,
      xOffset,
      yOffset,
      clampedWidth,
      clampedHeight,
      activity.PaintView.WHITE_);

  ctx.save();

  // Translate the canvas coordinates by the x and y offsets.
  ctx.translate(-xOffset, -yOffset);

  this.shadeOffScreenRegions_(ctx, win, contentWidth, contentHeight);
  this.highlightPaintedRegions_(ctx, win, event.clientRects);

  ctx.restore();
  ctx.restore();

  // Make sure the paint pane and its splitter are unhidden now that
  // we have captured a screen snapshot.
  this.paintPaneRoot_.setAttribute('collapsed', false);
  this.paintPaneSplitter_.setAttribute('collapsed', false);

  this.paintPaneContainer_.appendChild(canvas);
};

/**
 * Shade the offscreen regions of the window (those not visible to the
 * user because they are outside of the current scroll region).
 * @param {Object} ctx The canvas context.
 * @param {nsIDOMWindow} win The DOM window.
 * @param {number} contentWidth The width of the content.
 * @param {number} contentHeight The height of the content.
 * @private
 */
activity.PaintView.prototype.shadeOffScreenRegions_ = function(
    ctx, win, contentWidth, contentHeight) {
  ctx.save();
  ctx.fillStyle = activity.PaintView.OFFSCREEN_REGION_STYLE_;
  ctx.beginPath();
  if (win.scrollX > 0) {
    ctx.rect(0, 0, win.scrollX, contentHeight);
  }
  if (win.scrollY > 0) {
    ctx.rect(0, 0, contentWidth, win.scrollY);
  }
  if (win.scrollMaxY > win.scrollY) {
    var y = win.scrollY + win.innerHeight;
    ctx.rect(0, y, contentWidth, contentHeight - y);
  }
  if (win.scrollMaxX > win.scrollX) {
    var x = win.scrollX + win.innerWidth;
    ctx.rect(x, 0, contentWidth - x, contentHeight);
  }
  ctx.fill();
  ctx.closePath();
  ctx.restore();
};

/**
 * @param {number} val The number to offset.
 * @return {number} The amount to offset by.
 */
function getHighlightedPixelsOffset(val) {
  return Math.round((activity.PaintView.MIN_HIGHLIGHTED_PIXELS_ - val) / 2);
}

/**
 * Highlight the regions that were just painted to the screen.
 * @param {Object} ctx The canvas context.
 * @param {nsIDOMWindow} win The window.
 * @param {Array} rects The DOM window.
 * @private
 */
activity.PaintView.prototype.highlightPaintedRegions_ =
    function(ctx, win, rects) {
  ctx.save();

  // The highlighted region coordinates are relative to the window's
  // current scroll region, so we need to translate into page
  // coordinates.
  ctx.translate(win.scrollX, win.scrollY);
  ctx.fillStyle = activity.PaintView.HIGHLIGHT_REGION_STYLE_;
  ctx.strokeStyle = activity.PaintView.BLACK_;
  for (var i = 0, len = rects.length; i < len; i++) {
    var rect = rects[i];
    var x = Math.round(rect.left);
    var y = Math.round(rect.top);
    var width = Math.round(rect.right - rect.left);
    var height = Math.round(rect.bottom - rect.top);
    if (width < activity.PaintView.MIN_HIGHLIGHTED_PIXELS_) {
      x -= getHighlightedPixelsOffset(width);
      width = activity.PaintView.MIN_HIGHLIGHTED_PIXELS_;
    }
    if (height < activity.PaintView.MIN_HIGHLIGHTED_PIXELS_) {
      y -= getHighlightedPixelsOffset(height);
      height = activity.PaintView.MIN_HIGHLIGHTED_PIXELS_;
    }
    ctx.fillRect(x, y, width, height);
    ctx.strokeRect(x, y, width, height);
  }
  ctx.restore();
};

/**
 * Removes all children from the paint pane.
 * @private
 */
activity.PaintView.prototype.removeAllChildren_ = function() {
  var canvasParent = this.paintPaneContainer_;
  if (!canvasParent) {
    return;
  }
  while (canvasParent.hasChildNodes()) {
    canvasParent.removeChild(
        canvasParent.childNodes[canvasParent.childNodes.length - 1]);
  }
};
