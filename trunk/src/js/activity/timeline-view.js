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
 * @fileoverview Classes responsible for rendering the timeline to the screen.
 *
 * @author Bryan McQuade
 */

goog.provide('activity.TimelineView');

goog.require('activity.preference');
goog.require('goog.Disposable');
goog.require('goog.dispose');

/**
 * The TimelineView object is responsible for building and maintaining the
 * rows contained inside of the XUL grid element. Each row is represented
 * by a Row helper object.
 * @param {nsIDOMElement} xulRowsElement The DOM object we should build
 *     the grid's rows under.
 * @param {nsIDOMDocument} xulElementFactory A factory for creating DOM
 *     elements (e.g. the document object).
 * @param {number} resolutionUsec The resolution of this view, in
 *     microseconds.
 * @constructor
 * @extends {goog.Disposable}
 */
activity.TimelineView = function(xulRowsElement,
                                 xulElementFactory,
                                 resolutionUsec) {
  goog.Disposable.call(this);

  /**
   * A factory for creating DOM elements.
   * @type {nsIDOMDocument}
   * @private
   */
  this.xulElementFactory_ = xulElementFactory;

  /*
   * The resolution of this view, in microseconds.
   * @type {number}
   * @private
   */
  this.resolutionUsec_ = resolutionUsec;

  /**
   * The root XUL dom element for our rows.
   * @type {nsIDOMElement}
   * @private
   */
  this.xulRowsElement_ = xulRowsElement;

  /**
   * The label that shows the start time.
   * @type {nsIDOMElement}
   * @private
   */
  this.startTimeLabel_ = this.xulElementFactory_.getElementById(
      'activity-startTimeLabel');

  /**
   * The label that shows the end time.
   * @type {nsIDOMElement}
   * @private
   */
  this.endTimeLabel_ = this.xulElementFactory_.getElementById(
      'activity-endTimeLabel');

  /**
   * The label that separates HTTP from non-HTTP resources.
   * @type {nsIDOMElement}
   * @private
   */
  this.nonHttpResourcesLabel_ = null;

  /**
   * The index of the first non-HTTP resource in the rows_ array.
   * @type {number}
   * @private
   */
  this.firstNonHttpResourceIndex_ = 0;

  /**
   * The number of resolutions to display before scrolling off the
   * left side of the screen.
   * @type {number}
   * @private
   */
  this.maxDisplayableResolutions_ = activity.preference.getInt(
      activity.TimelineView.PREF_MAX_DISPLAYABLE_RESOLUTIONS_,
      activity.TimelineView.DEFAULT_MAX_DISPLAYABLE_RESOLUTIONS_);

  if (this.maxDisplayableResolutions_ <
      activity.TimelineView.MIN_DISPLAYABLE_RESOLUTIONS_) {
    // Clamp to a reasonable value.
    this.maxDisplayableResolutions_ =
        activity.TimelineView.MIN_DISPLAYABLE_RESOLUTIONS_;
  }

  /**
   * The Rows of this timeline.
   * @type {Array.<!activity.Row>}
   * @private
   */
  this.rows_ = [];
};
goog.inherits(activity.TimelineView, goog.Disposable);

/**
 * The start time for this timeline view, in microseconds.
 * @type {number}
 * @private
 */
activity.TimelineView.prototype.startTimeUsec_ = 0;

/**
 * The preference name for the max displayable resolutions in the UI.
 * @type {string}
 * @private
 */
activity.TimelineView.PREF_MAX_DISPLAYABLE_RESOLUTIONS_ =
    'ui_max_displayable_resolutions';

/**
 * The maximum number of resolutions to display. If more than this number
 * of resolutions have passed, they will be scrolled off the left of the
 * timeline view. At the default of 20ms resolution, this means that up
 * to 20 seconds of activity will be visible at any time. We do this because
 * having too many spacer elements on the screen at once causes the browser
 * to slow down considerably. Scrolling old spacers off the left edge of the
 * timeline puts a limit on the number of elements being rendered, which
 * prevents the slowdown.
 * @type {number}
 * @private
 */
activity.TimelineView.DEFAULT_MAX_DISPLAYABLE_RESOLUTIONS_ = 1000;

/**
 * Minimum number of resolutions the UI must display before scrolling.
 * @type {number}
 * @private
 */
activity.TimelineView.MIN_DISPLAYABLE_RESOLUTIONS_ = 100;

/** @inheritDoc */
activity.TimelineView.prototype.disposeInternal = function() {
  activity.TimelineView.superClass_.disposeInternal.call(this);

  this.xulElementFactory_ = null;
  this.xulRowsElement_ = null;
  this.startTimeLabel_ = null;
  this.endTimeLabel_ = null;
  this.nonHttpResourcesLabel_ = null;

  for (var i = 0; i < this.rows_.length; i++) {
    goog.dispose(this.rows_[i]);
  }

  this.rows_ = null;
};

/**
 * Gets the resolution for this view, in microseconds.
 * @return {number} The resolution, in microseconds.
 */
activity.TimelineView.prototype.getResolutionUsec = function() {
  return this.resolutionUsec_;
};

/**
 * Called by the model to indicate that the end time has changed.
 * @param {number} endTimeUsec the new end time, in microseconds.
 */
activity.TimelineView.prototype.onEndTimeChanged = function(endTimeUsec) {
  this.throwIfDisposed_();

  // Convert from usec to number of resolutions.
  var endTimeInResolutions = Math.ceil(endTimeUsec / this.resolutionUsec_);
  var startTimeInResolutions =
      Math.floor(this.startTimeUsec_ / this.resolutionUsec_);

  // Clamp the start time, if necessary.
  var numResolutions = endTimeInResolutions - startTimeInResolutions;
  if (numResolutions > this.maxDisplayableResolutions_) {
    startTimeInResolutions =
        endTimeInResolutions - this.maxDisplayableResolutions_;
    this.startTimeUsec_ = startTimeInResolutions * this.resolutionUsec_;
  }

  // Update each row's start and end time.
  this.clampRowStartAndEndTimes_(startTimeInResolutions, endTimeInResolutions);

  // Show only those rows that have visible events.
  this.updateRowVisibility_();

  // Make sure every other visible row has a gray background.
  this.updateRowCss_();

  // Update the UI's start and end time indicators.
  this.updateTimeIndicators_(this.startTimeUsec_, endTimeUsec);
};

/**
 * Called by the model to indicate that a new timeline has been created.
 * @param {goog.Timeline} timeline The newly created timeline.
 */
activity.TimelineView.prototype.onTimelineCreated = function(timeline) {
  this.throwIfDisposed_();

  var rowElement = this.xulElementFactory_.createElement('row');
  var row = new activity.TimelineView.Row_(timeline,
                                         this.xulElementFactory_,
                                         rowElement,
                                         this.resolutionUsec_,
                                         this.maxDisplayableResolutions_);
  var startTimeInResolutions =
      Math.floor(this.startTimeUsec_ / this.resolutionUsec_);
  row.clampToStartTime(startTimeInResolutions);

  if (timeline.getIdentifier().lastIndexOf('http', 0) == 0) {
    this.xulRowsElement_.insertBefore(rowElement, this.nonHttpResourcesLabel_);
    this.rows_.splice(this.firstNonHttpResourceIndex_++, 0, row);
  } else {
    if (!this.nonHttpResourcesLabel_) {
      this.firstNonHttpResourceIndex_ = this.rows_.length;
      this.nonHttpResourcesLabel_ =
          this.xulElementFactory_.createElement('label');
      this.nonHttpResourcesLabel_.setAttribute('value', 'Non-HTTP resources:');
      this.nonHttpResourcesLabel_.setAttribute(
          'class', 'nonHttpResourcesLabel');
      this.xulRowsElement_.appendChild(this.nonHttpResourcesLabel_);
    }
    this.xulRowsElement_.appendChild(rowElement);
    this.rows_.push(row);
  }
};

/**
 * Update each row's start and end time.
 * @param {number} startTime The new start time, in resolutions.
 * @param {number} endTime The new end time, in resolutions.
 * @private
 */
activity.TimelineView.prototype.clampRowStartAndEndTimes_ = function(
    startTime, endTime) {
  for (var i = 0; i < this.rows_.length; i++) {
    this.rows_[i].clampToStartTime(startTime);
    this.rows_[i].padToEndTime(endTime);
  }
};

/**
 * Update the rows so that any rows without visible events are hidden.
 * @private
 */
activity.TimelineView.prototype.updateRowVisibility_ = function() {
  var hasVisibleHttpRow = false;
  var hasVisibleNonHttpRow = false;
  for (var i = 0; i < this.rows_.length; i++) {
    var row = this.rows_[i];
    row.updateVisibility();
    if (row.isVisible()) {
      if (i >= this.firstNonHttpResourceIndex_) {
        hasVisibleNonHttpRow = true;
      } else {
        hasVisibleHttpRow = true;
      }
    }
  }

  // Update the visibility of the non-HTTP resources separator based
  // on whether or not both HTTP and non-HTTP resources are visible.
  if (this.nonHttpResourcesLabel_) {
    var shouldShowNonHttpResourcesLabel =
        hasVisibleNonHttpRow && hasVisibleHttpRow;
    this.nonHttpResourcesLabel_.setAttribute(
        'collapsed', !shouldShowNonHttpResourcesLabel);
  }
};

/**
 * Update the rows so that every other visible row has a gray background.
 * @private
 */
activity.TimelineView.prototype.updateRowCss_ = function() {
  var primary = true;
  for (var i = 0; i < this.rows_.length; i++) {
    if (i == this.firstNonHttpResourceIndex_) {
      primary = true;
    }
    var row = this.rows_[i];
    if (row.isVisible()) {
      row.updateRowCss(primary);
      primary = !primary;
    }
  }
};

/**
 * Update the start and end time indicators.
 * @param {number} startTimeUsec The start time to display, in microseconds.
 * @param {number} endTimeUsec The end time to display, in microseconds.
 * @private
 */
activity.TimelineView.prototype.updateTimeIndicators_ = function(
    startTimeUsec, endTimeUsec) {
  this.startTimeLabel_.setAttribute(
      'value', activity.TimelineView.formatTime_(startTimeUsec));

  this.endTimeLabel_.setAttribute(
      'value', activity.TimelineView.formatTime_(endTimeUsec));
};


/**
 * Throws an Error if this TimelineView is disposed.
 * @private
 */
activity.TimelineView.prototype.throwIfDisposed_ = function() {
  if (this.isDisposed()) {
    throw new Error('TimelineView already disposed.');
  }
};

/**
 * Format the given time as a human-readable string.
 * @param {number} timeUsec The time, in microseconds.
 * @return {string} A human-readable representation of the timestamp.
 * @private
 */
activity.TimelineView.formatTime_ = function(timeUsec) {
  var timeMillis = timeUsec / 1000;
  var prettyTime;
  if (timeMillis >= 10000) {
    prettyTime = Math.round(timeMillis / 1000) + ' seconds';
  } else if (timeMillis < 100) {
    prettyTime = Math.round(timeMillis) + 'ms';
  } else {
    // Round to the nearest 100 milliseconds.
    timeMillis = Math.round(timeMillis / 100) * 100;
    prettyTime = Math.round(timeMillis) + 'ms';
  }
  return prettyTime;
};


/**
 * A Row is responsible for managing the XUL elements used to render a
 * single row in the JavaScript execution timeline.
 *
 * @param {!activity.Timeline} timeline The timeline.
 * @param {!nsIDOMDocument} xulElementFactory A factory for creating DOM
 *     elements (e.g. the document object).
 * @param {!nsIDOMElement} root The root row node.
 * @param {number} resolutionUsec The resolution for this view.
 * @param {number} maxDisplayableResolutions The maximum number of
 *     resolutions to display before scrolling.
 * @constructor
 * @extends {goog.Disposable}
 * @private
 */
activity.TimelineView.Row_ = function(
    timeline,
    xulElementFactory,
    root,
    resolutionUsec,
    maxDisplayableResolutions) {
  goog.Disposable.call(this);

  /**
   * The Timeline for this Row.
   * @type {activity.Timeline}
   * @private
   */
  this.timeline_ = timeline;

  /**
   * A factory for creating DOM elements.
   * @type {nsIDOMDocument}
   * @private
   */
  this.xulElementFactory_ = xulElementFactory;

  /**
   * The resolution for this view.
   * @type {number}
   * @private
   */
  this.resolutionUsec_ = resolutionUsec;

  /**
   * The number of resolutions to display before scrolling off the
   * left side of the screen.
   * @type {number}
   * @private
   */
  this.maxDisplayableResolutions_ = maxDisplayableResolutions;

  /**
   * The XUL row element.
   * @type {nsIDOMElement}
   * @private
   */
  this.row_ = root;

  var label = this.xulElementFactory_.createElement('label');

  /*
   * Truncate the label to not include the hostname (because we have
   * limited space to display the label), but allow the user to mouse
   * over the label to see the full URL.
   */
  var identifierWithoutHostname =
      this.timeline_.getIdentifier().replace(/^[^\/]+:\/\/[^\/]*/, '');
  label.setAttribute('value', identifierWithoutHostname);
  label.setAttribute('tooltiptext', this.timeline_.getIdentifier());
  // Make sure the label's contents get ellipsized if it's too big to
  // fit.
  label.setAttribute('crop', 'end');
  label.setAttribute('class', 'identifier');

  // We put the label inside a vbox, since we want the label to be
  // vertically centered within the allocated space.
  var box = this.xulElementFactory_.createElement('vbox');
  box.setAttribute('class', 'identifier');
  box.setAttribute('pack', 'center');
  box.appendChild(label);
  this.row_.appendChild(box);

  /**
   * The root node for this row. All BarCharts are parented under this node.
   * @type {!nsIDOMElement}
   * @private
   */
  this.rootTimelineElement_ = this.xulElementFactory_.createElement('vbox');
  this.rootTimelineElement_.setAttribute('class', 'timelineBarContainer');
  this.row_.appendChild(this.rootTimelineElement_);

  /**
   * The BarCharts that show JS parse time, JS execution time, etc.
   * @type {Array.<!activity.TimelineView.BarChart_>}
   * @private
   */
  this.barCharts_ = [];

  /**
   * The BarChartsStacks, which contain a stack of overlayed bar charts.
   * @type {Array.<!activity.TimelineView.BarChartStack_>}
   * @private
   */
  this.barChartStacks_ = [];

  this.addBarCharts_();

  this.updateVisibility();

  this.timeline_.addListener(this);
};
goog.inherits(activity.TimelineView.Row_, goog.Disposable);

/**
 * Add all the BarCharts for this row.
 * @private
 */
activity.TimelineView.Row_.prototype.addBarCharts_ = function() {
  // All events associated with a request are stacked in the same
  // BarChartStack_.
  var requestStack = this.createBarChartStack_();
  this.barChartStacks_.push(requestStack);

  for (var ordinal = 1,
           len = activity.TimelineEventType.getNumberOfEventTypes();
       ordinal <= len;
       ordinal++) {
    var event = activity.TimelineEventType.getEventTypeForOrdinal(ordinal);
    var rootBarElement = this.xulElementFactory_.createElement('hbox');
    var barChart = new activity.TimelineView.BarChart_(
        rootBarElement,
        this.xulElementFactory_,
        this.getCssClassNameForEvent_(event));
    // By default, all charts are parented under the root timeline
    // element.
    var parent = this.rootTimelineElement_;
    if (activity.TimelineEventType.isRequestEventType(event)) {
      // Special case: the request charts are parented under the
      // request BarChartStack's stack element.
      parent = requestStack.getStackElement();
      requestStack.addChild(barChart);
    }
    parent.appendChild(rootBarElement);
    this.barCharts_.push(barChart);
  }
};

/**
 * Construct a new BarChartStack_.
 * @return {activity.TimelineView.BarChartStack_} the new BarChartStack.
 * @private
 */
activity.TimelineView.Row_.prototype.createBarChartStack_ = function() {
  var hbox = this.xulElementFactory_.createElement('hbox');
  var stack = new activity.TimelineView.BarChartStack_(
      hbox, this.xulElementFactory_);
  this.rootTimelineElement_.appendChild(hbox);
  return stack;
};

/**
 * Processes the given event. Adds up to three entries to the chart,
 * one for the portion that spans a partial resolution at the
 * beginning of the event, one for the portion that spans some number
 * of whole resolutions in the middle of the event, and one for the
 * portion that spans a partial resolution at the end of the
 * event. All three entries are optional.
 * @param {activity.TimelineModel.Event} event the event.
 */
activity.TimelineView.Row_.prototype.onEvent = function(event) {
  this.throwIfDisposed_();

  // Ordinals are one-indexed, but our array is zero-indexed, so we
  // need to subtract one from the ordinal value.
  var index =
      activity.TimelineEventType.getOrdinalForEventType(event.getType()) - 1;
  var chart = this.barCharts_[index];

  var eventEndTimeUsec = event.getStartTimeUsec() + event.getDurationUsec();
  var eventEndTimeInResolutions =
      Math.ceil(eventEndTimeUsec / this.resolutionUsec_);

  // The chart's new end time will be the max of its current end time
  // and the end time of the new event.
  var chartEndTime = Math.max(chart.getEndTime(), eventEndTimeInResolutions);
  var chartStartTime = chartEndTime - this.maxDisplayableResolutions_;

  // Clamp the event's start and end times to the chart's start and
  // end times.
  var clampedStartTimeUsec = Math.max(
      event.getStartTimeUsec(), chartStartTime * this.resolutionUsec_);
  var clampedEndTimeUsec = Math.min(
      eventEndTimeUsec, chartEndTime * this.resolutionUsec_);

  if (clampedEndTimeUsec <= clampedStartTimeUsec) {
    return;
  }

  var eventStartInResolutions =
      Math.floor(clampedStartTimeUsec / this.resolutionUsec_);
  if (chart.getEndTime() > eventStartInResolutions) {
    // The chart's start time exceeds the start time for this
    // event. This happens because we have different event sources
    // that may arrive out of order relative to each other. We need to
    // temporarily remove the data up to the event's start time, then
    // restore that data after adding the new event.
    chart.unwindToEndTime(eventStartInResolutions);
  }
  chart.padToEndTime(eventStartInResolutions);

  if (Math.floor(clampedStartTimeUsec / this.resolutionUsec_) ==
      Math.floor(clampedEndTimeUsec / this.resolutionUsec_)) {
    // The entire event fits in a single entry.
    var intensity = clampedEndTimeUsec - clampedStartTimeUsec;
    intensity *= event.getIntensity();
    chart.addEntry(
        activity.TimelineView.Row_.sanitizeIntensity_(
            intensity / this.resolutionUsec_),
        1);
  } else {
    // Generate up to three events, one for the partial resolution at
    // the beginning, another with the full resolutions in the middle,
    // and a final with the partial resolution at the end. Note that
    // all three events are optional.
    this.maybeAddStartEvent_(
        chart, clampedStartTimeUsec, clampedEndTimeUsec, event.getIntensity());
    this.maybeAddMiddleEvent_(
        chart, clampedStartTimeUsec, clampedEndTimeUsec, event.getIntensity());
    this.maybeAddEndEvent_(
        chart, clampedStartTimeUsec, clampedEndTimeUsec, event.getIntensity());
  }

  if (chartEndTime > chart.getEndTime()) {
    // Restore the previous end time.
    chart.padToEndTime(chartEndTime);
  }
};

/**
 * @param {activity.TimelineView.BarChart_} chart the chart to update.
 * @param {number} startTimeUsec the start time.
 * @param {number} endTimeUsec the end time.
 * @param {number} intensity the intensity of the event. Note that the
 *     actual intensity value drawn to the screen is a function of
 *     both the event intensity, and the amount of time that this
 *     event was active within the given resolution.
 * @private
 */
activity.TimelineView.Row_.prototype.maybeAddStartEvent_ = function(
    chart, startTimeUsec, endTimeUsec, intensity) {
  var firstEventIntensity =
      this.resolutionUsec_ - (startTimeUsec % this.resolutionUsec_);
  if (firstEventIntensity == this.resolutionUsec_) {
    return;
  }
  firstEventIntensity *= intensity;
  chart.addEntry(
      activity.TimelineView.Row_.sanitizeIntensity_(
          firstEventIntensity / this.resolutionUsec_),
      1);
};

/**
 * @param {activity.TimelineView.BarChart_} chart the chart to update.
 * @param {number} startTimeUsec the start time.
 * @param {number} endTimeUsec the end time.
 * @param {number} intensity the intensity of the event.
 * @private
 */
activity.TimelineView.Row_.prototype.maybeAddMiddleEvent_ = function(
    chart, startTimeUsec, endTimeUsec, intensity) {
  var startTimeInResolutions = Math.ceil(startTimeUsec / this.resolutionUsec_);
  var endTimeInResolutions = Math.floor(endTimeUsec / this.resolutionUsec_);
  var numFullResolutions = endTimeInResolutions - startTimeInResolutions;
  if (numFullResolutions > 0) {
    chart.addEntry(
        activity.TimelineView.Row_.sanitizeIntensity_(intensity),
        numFullResolutions);
  }
};

/**
 * @param {activity.TimelineView.BarChart_} chart the chart to update.
 * @param {number} startTimeUsec the start time.
 * @param {number} endTimeUsec the end time.
 * @param {number} intensity the intensity of the event. Note that the
 *     actual intensity value drawn to the screen is a function of
 *     both the event intensity, and the amount of time that this
 *     event was active within the given resolution.
 * @private
 */
activity.TimelineView.Row_.prototype.maybeAddEndEvent_ = function(
    chart, startTimeUsec, endTimeUsec, intensity) {
  var lastEventIntensity = endTimeUsec % this.resolutionUsec_;
  if (lastEventIntensity == 0) {
    return;
  }
  lastEventIntensity *= intensity;
  chart.addEntry(
      activity.TimelineView.Row_.sanitizeIntensity_(
          lastEventIntensity / this.resolutionUsec_),
      1);
};


/** @inheritDoc */
activity.TimelineView.Row_.prototype.disposeInternal = function() {
  activity.TimelineView.Row_.superClass_.disposeInternal.call(this);

  this.timeline_.removeListener(this);
  this.row_ = null;
  this.timeline_ = null;
  this.xulElementFactory_ = null;
  for (var i = 0, len = this.barCharts_.length; i < len; i++) {
    goog.dispose(this.barCharts_[i]);
  }
  for (var i = 0, len = this.barChartStacks_.length; i < len; i++) {
    goog.dispose(this.barChartStacks_[i]);
  }
  this.barCharts_ = null;
  this.barChartStacks_ = null;
};

function isBarChartEmpty(barChart) {
  return barChart.isEmpty();
}

/**
 * @return {boolean} Whether or not this row is empty.
 */
activity.TimelineView.Row_.prototype.isEmpty = function() {
  this.throwIfDisposed_();
  // The Row is empty if all of its BarCharts are empty.
  return this.barCharts_.every(isBarChartEmpty);
};

/**
 * @return {boolean} Whether or not this row is visible.
 */
activity.TimelineView.Row_.prototype.isVisible = function() {
  this.throwIfDisposed_();

  return this.row_.getAttribute('collapsed') != 'true';
};

/**
 * Update the visibility of this row based on the state of its bar charts.
 */
activity.TimelineView.Row_.prototype.updateVisibility = function() {
  this.throwIfDisposed_();

  this.row_.setAttribute('collapsed', this.isEmpty());
  for (var i = 0, len = this.barCharts_.length; i < len; i++) {
    this.barCharts_[i].updateVisibility();
  }
  for (var i = 0, len = this.barChartStacks_.length; i < len; i++) {
    this.barChartStacks_[i].updateVisibility();
  }
};

/**
 * Specify the CSS class for this row.
 * @param {boolean} primary Whether the row should use the primary or
 *     secondary CSS class.
 */
activity.TimelineView.Row_.prototype.updateRowCss = function(primary) {
  this.throwIfDisposed_();

  if (primary) {
    this.row_.setAttribute('class', 'primaryTimelineRow');
  } else {
    this.row_.setAttribute('class', 'secondaryTimelineRow');
  }
};

/**
 * Pad this row's BarCharts to the specified end time.
 * @param {number} endTime The time the row should be padded to.
 */
activity.TimelineView.Row_.prototype.padToEndTime = function(endTime) {
  this.throwIfDisposed_();

  for (var i = 0, len = this.barCharts_.length; i < len; i++) {
    this.barCharts_[i].padToEndTime(endTime);
  }
};

/**
 * Clamp this row's BarCharts to the specified start time.
 * @param {number} startTime The time the row's start time should be clamped to.
 */
activity.TimelineView.Row_.prototype.clampToStartTime = function(startTime) {
  this.throwIfDisposed_();

  for (var i = 0, len = this.barCharts_.length; i < len; i++) {
    this.barCharts_[i].clampToStartTime(startTime);
  }
};

/**
 * Get the CSS class name for the given event type.
 * @param {activity.TimelineEventType} event the type of event.
 * @return {string} the CSS class name.
 * @private
 */
activity.TimelineView.Row_.prototype.getCssClassNameForEvent_ =
    function(event) {
  switch (event) {
    case activity.TimelineEventType.JS_PARSE:
      return 'timelineParseColor';
    case activity.TimelineEventType.JS_EXECUTE:
      return 'timelineExecuteColor';
    case activity.TimelineEventType.NETWORK_WAIT:
      return 'networkWaitColor';
    case activity.TimelineEventType.DNS_LOOKUP:
      return 'dnsLookupColor';
    case activity.TimelineEventType.TCP_CONNECTING:
      return 'tcpConnectingColor';
    case activity.TimelineEventType.TCP_CONNECTED:
      return 'tcpConnectedColor';
    case activity.TimelineEventType.REQUEST_SENT:
      return 'requestSentColor';
    case activity.TimelineEventType.CACHE_HIT:
      return 'cacheHitColor';
    case activity.TimelineEventType.DATA_AVAILABLE:
      return 'dataAvailableColor';
    case activity.TimelineEventType.PAINT:
      return 'paintColor';
    case activity.TimelineEventType.SOCKET_DATA:
      return 'socketDataColor';
  }
  throw new Error('Unrecognized event type ' + event);
};

/**
 * Throws an Error if this Row_ is disposed.
 * @private
 */
activity.TimelineView.Row_.prototype.throwIfDisposed_ = function() {
  if (this.isDisposed()) {
    throw new Error('Row already disposed.');
  }
};

/**
 * Sanitize the intensity value: scale it and clamp it accordingly.
 * @param {number} intensity The intensity to sanitize.
 * @return {number} The sanitized intensity.
 * @private
 */
activity.TimelineView.Row_.sanitizeIntensity_ = function(intensity) {
  if (intensity == 0.0){
    // Special case: if we get a zero intensity value, it indicates
    // that some activity happened for this row at this time, but it
    // was too small to measure. We want to show some small value, so
    // we choose 0.3, which is smaller than the range that we clamp to
    // below (0.4..1.0).
    return 0.3;
  }

  if (intensity >= 1.0) {
    return 1.0;
  }

  /*
   * Take the square of the intensity. We want to make the difference
   * between 'executed non-stop' and 'only executed for part of the
   * time' very clear visually. Squaring helps to do this. If
   * execution was non-stop, 1.0 squared stays as 1.0. As code
   * executes less and less, we want the intensity to fall off more
   * quickly. For instance, when code executes 80% of the time, we
   * show intensity 0.64, which is visually clearer to be less than
   * 1.0, as compared to 0.8.
   */
  intensity *= intensity;

  /*
   * Otherwise, scale the intensity up into the range 0.4..1.0. We do this
   * because a very low intensity becomes impossible to see on most
   * monitors. Scaling up to 0.4 makes sure that even the smallest
   * intensity is visible to the user.
   */
   intensity *= 0.6;
   intensity += 0.4;
   return Math.min(1.0, intensity);
};


/**
 * A BarChartStack contains a stack of overlayed BarCharts.
 * @param {nsIDOMElement} rootXulElement The root element for this BarChart.
 * @param {nsIDOMDocument} xulElementFactory A factory for creating xul
 *     elements.
 * @constructor
 * @extends {goog.Disposable}
 * @private
 */
activity.TimelineView.BarChartStack_ = function(
    rootXulElement, xulElementFactory) {
  goog.Disposable.call(this);

  /**
   * A factory for creating DOM elements.
   * @type {nsIDOMDocument}
   * @private
   */
  this.xulElementFactory_ = xulElementFactory;

  /**
   * The root XUL element for this bar chart.
   * @type {nsIDOMElement}
   * @private
   */
  this.bar_ = rootXulElement;

  this.bar_.setAttribute('flex', 1);
  this.bar_.setAttribute('class', 'timelineBar');

  this.stack_ = xulElementFactory.createElement('stack');
  this.stack_.setAttribute('flex', 1);
  this.bar_.appendChild(this.stack_);

  /**
   * @type {Array.<!activity.TimelineView.BarChart_>}
   * @private
   */
  this.children_ = [];
};
goog.inherits(activity.TimelineView.BarChartStack_, goog.Disposable);

/** @inheritDoc */
activity.TimelineView.BarChartStack_.prototype.disposeInternal = function() {
  this.xulElementFactory_ = null;
  this.bar_ = null;
  this.stack_ = null;
  this.children_ = null;
};

/**
 * @return {nsIDOMElement} the XUL stack element for this
 * BarChartStack.
 */
activity.TimelineView.BarChartStack_.prototype.getStackElement = function() {
  return this.stack_;
};

/**
 * @param {activity.TimelineView.BarChart_} chart the child to add to
 *     this stack.
 */
activity.TimelineView.BarChartStack_.prototype.addChild = function(chart) {
  this.children_.push(chart);
};

/**
 * @return {boolean} Whether or not this bar chart is empty.
 */
activity.TimelineView.BarChartStack_.prototype.isEmpty = function() {
  return this.children_.every(isBarChartEmpty);
};

/**
 * Update the visibility of this bar chart based on whether it has any
 * visible elements.
 */
activity.TimelineView.BarChartStack_.prototype.updateVisibility = function() {
  this.bar_.setAttribute('collapsed', this.isEmpty());
};


/**
 * A BarChart renders a single timeline of events as a bar chart.
 * @param {nsIDOMElement} rootXulElement The root element for this BarChart.
 * @param {nsIDOMDocument} xulElementFactory A factory for creating xul
 *     elements.
 * @param {string} cssClassName The CSS class name to use for this BarChart.
 * @constructor
 * @extends {goog.Disposable}
 * @private
 */
activity.TimelineView.BarChart_ = function(
    rootXulElement, xulElementFactory, cssClassName) {
  goog.Disposable.call(this);

  /**
   * A factory for creating DOM elements.
   * @type {nsIDOMDocument}
   * @private
   */
  this.xulElementFactory_ = xulElementFactory;

  /**
   * The CSS class name to use for this BarChart.
   * @type {string}
   * @private
   */
  this.cssClassName_ = cssClassName;

  /**
   * The root XUL element for this bar chart.
   * @type {nsIDOMElement}
   * @private
   */
  this.bar_ = rootXulElement;

  this.bar_.setAttribute('flex', 1);
  this.bar_.setAttribute('class', 'timelineBar');
};
goog.inherits(activity.TimelineView.BarChart_, goog.Disposable);

/**
 * The start time for this bar chart, in resolutions.
 * @type {number}
 * @private
 */
activity.TimelineView.BarChart_.prototype.startTime_ = 0;

/**
 * The end time for this bar chart, in resolutions.
 * @type {number}
 * @private
 */
activity.TimelineView.BarChart_.prototype.endTime_ = 0;

/**
 * The last time an event was drawn in this BarChart.
 * @type {number}
 * @private
 */
activity.TimelineView.BarChart_.prototype.lastEventEndTime_ = 0;

/** @inheritDoc */
activity.TimelineView.BarChart_.prototype.disposeInternal = function() {
  activity.TimelineView.BarChart_.superClass_.disposeInternal.call(this);

  this.xulElementFactory_ = null;
  this.bar_ = null;
};

/**
 * @return {number} The end time, in resolutions.
 */
activity.TimelineView.BarChart_.prototype.getEndTime = function() {
  return this.endTime_;
};

/**
 * @param {number} endTime The end time to unwind to, in resolutions.
 */
activity.TimelineView.BarChart_.prototype.unwindToEndTime = function(endTime) {
  this.throwIfDisposed_();

  var amountToUnwind = this.endTime_ - endTime;
  if (amountToUnwind < 0) {
    throw new Error(['Current BarChart end time',
                     this.endTime_,
                     'preceeds requested end time',
                     endTime].join(' '));
  }
  while (amountToUnwind > 0) {
    var spacer = this.bar_.lastChild;
    if (!spacer) {
      throw new Error('Ran out of spacers!');
    }
    var duration = spacer.getAttribute('flex');
    if (!duration) {
      throw new Error('unwindToEndTime encountered zero-width child.');
    }
    if (duration <= amountToUnwind) {
      // The duration of this spacer is less than the amount to unwind,
      // so we can safely remove the entire spacer.
      this.bar_.removeChild(spacer);
      amountToUnwind -= duration;
    } else {
      // The spacer's duration exceeds that of the amount to unwind,
      // so we need to reduce the spacer's duration by the amount to
      // unwind.
      spacer.setAttribute('flex', duration - amountToUnwind);
      amountToUnwind = 0;
    }
  }
  this.endTime_ = endTime;
};

/**
 * Pad the given BarChart with empty entries, up to the specified end time.
 * @param {number} endTime The end time to pad to, in resolutions.
 */
activity.TimelineView.BarChart_.prototype.padToEndTime = function(endTime) {
  this.throwIfDisposed_();

  var duration = endTime - this.endTime_;
  if (duration < 0) {
    throw new Error(['Current BarChart end time',
                     this.endTime_,
                     'exceeds requested end time',
                     endTime].join(' '));
  }
  if (duration == 0) {
    // We're already padded to the specified end time, so return.
    return;
  }

  this.appendSpacer_(duration, null);
  this.endTime_ = endTime;
};

/**
 * Remove all entries before the given time.
 * @param {number} startTime The start time to clamp to, in resolutions.
 */
activity.TimelineView.BarChart_.prototype.clampToStartTime =
    function(startTime) {
  this.throwIfDisposed_();

  if (this.isEmpty() || startTime >= this.endTime_) {
    // If there are no visible events, update the start and end times
    // and remove any existing children.
    this.startTime_ = startTime;
    this.endTime_ = startTime;
    this.lastEventEndTime_ = 0;
    this.removeAllChildren_();
    return;
  }

  var amountToTrim = startTime - this.startTime_;
  if (amountToTrim < 0) {
    throw new Error(['Current BarChart start time',
                     this.startTime_,
                     'exceeds requested start time',
                     startTime].join(' '));
  }

  while (amountToTrim > 0) {
    var spacer = this.bar_.firstChild;
    if (!spacer) {
      throw new Error('Ran out of spacers!');
    }
    var duration = spacer.getAttribute('flex');
    if (!duration) {
      throw new Error('clampToStartTime encountered zero-width child.');
    }
    if (duration <= amountToTrim) {
      // The duration of this spacer is less than the amount to trim,
      // so we can safely remove the entire spacer.
      this.bar_.removeChild(spacer);
      amountToTrim -= duration;
    } else {
      // The spacer's duration exceeds that of the amount to trim, so
      // we need to reduce the spacer's duration by the amount to
      // trim.
      spacer.setAttribute('flex', duration - amountToTrim);
      amountToTrim = 0;
    }
  }
  this.startTime_ = startTime;
};

/**
 * Add an entry to the given BarChart.
 * @param {number} opacity The opacity of the entry (from 0 to 1).
 * @param {number} duration The number of resolutions this entry spans.
 */
activity.TimelineView.BarChart_.prototype.addEntry =
    function(opacity, duration) {
  this.throwIfDisposed_();

  if (opacity == 0.0) {
    this.padToEndTime(this.endTime_ + duration);
    return;
  }

  /*
   * Now build the complete CSS style string.
   */
  var style = [
      'opacity:',
      opacity.toFixed(2)
      ].join('');

  this.appendSpacer_(duration, style);
  this.endTime_ += duration;
  this.lastEventEndTime_ = this.endTime_;
};

/**
 * @return {boolean} Whether or not this bar chart is empty.
 */
activity.TimelineView.BarChart_.prototype.isEmpty = function() {
  this.throwIfDisposed_();

  return this.lastEventEndTime_ <= this.startTime_;
};

/**
 * Update the visibility of this bar chart based on whether it has any
 * visible elements.
 */
activity.TimelineView.BarChart_.prototype.updateVisibility = function() {
  this.bar_.setAttribute('collapsed', this.isEmpty());
};

/**
 * Helper that determines if two style strings are the same. We treat
 * null and empty string as the same style.
 * @param {string?} a first style to check.
 * @param {string?} b second style to check.
 * @return {boolean} whether the styles are the same.
 */
function stylesAreSame(a, b) {
  if (a == b) return true;
  if (a == null && b == '') return true;
  if (a == '' && b == null) return true;
  return false;
}

/**
 * Adds a single XUL spacer element to the row.
 * @param {number} duration Duration of the event, in resolutions.
 * @param {string?} style The CSS style to apply to the generated node.
 * @private
 */
activity.TimelineView.BarChart_.prototype.appendSpacer_ = function(
    duration, style) {
  var lastSpacer = this.bar_.lastChild;
  if (lastSpacer && stylesAreSame(lastSpacer.getAttribute('style'), style)) {
    // Optimization: instead of appending a new spacer, just add our
    // duration on to the last spacer, since its attributes are
    // identical.
    var lastSpacerDuration = lastSpacer.getAttribute('flex');
    lastSpacer.setAttribute('flex', Number(lastSpacerDuration) + duration);
  } else {
    var spacer = this.xulElementFactory_.createElement('spacer');

    // NOTE: The XUL rendering engine does not render variable-width
    // spacers with perfect accuracy. Specifically, spacers in different
    // rows may not be perfectly aligned with each other. In practice,
    // the alignment is off by only a few pixels, which is hardly
    // detectable to the human eye. The alternative, single-width
    // spacers, can cause significant slowdowns in Firefox when
    // rendering very large timelines, so we choose slightly reduced
    // accuracy over reduced performance.
    spacer.setAttribute('flex', duration);

    if (style != null) {
      spacer.setAttribute('class', this.cssClassName_);
      spacer.setAttribute('style', style);
    }
    this.bar_.appendChild(spacer);
  }
};

/**
 * Removes all XUL children from this bar.
 * @private
 */
activity.TimelineView.BarChart_.prototype.removeAllChildren_ = function() {
  while (this.bar_.lastChild != null) {
    this.bar_.removeChild(this.bar_.lastChild);
  }
};

/**
 * Throws an Error if this BarChart_ is disposed.
 * @private
 */
activity.TimelineView.BarChart_.prototype.throwIfDisposed_ = function() {
  if (this.isDisposed()) {
    throw new Error('BarChart already disposed.');
  }
};
