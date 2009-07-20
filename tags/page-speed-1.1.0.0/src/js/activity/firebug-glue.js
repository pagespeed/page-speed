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
 * @fileoverview Code that hooks the activity profiler into the Firebug UI.
 *
 * @author Bryan McQuade
 */

FBL.ns(function() { with (FBL) {

function ActivityPanel() {}

ActivityPanel.prototype = domplate(Firebug.Panel, {
  // Unique identifier for this panel.
  name: 'activity',

  // Text to display in the panel's tab.
  title: 'Page Speed Activity',

  // Indicates that the content of this panel is not editable.
  editable: false,

  /**
   * Initialize the node that this panel should draw within. Called
   * immediately after creating the node (every time a new page is
   * navigated to).
   * @override
   * @param {nsIDOMNode} myPanelNode The root node for this panel to
   *     render within.
   */
  initializeNode: function(myPanelNode) {
    var doc = myPanelNode.ownerDocument;

    // A new myPanelNode is created by Firebug every time a new window
    // object is constructed in the main browser window. But we want
    // more control over the lifetime of the activity profiler. So we
    // instead create our own node that is a peer of myPanelNode,
    // which Firebug will not destroy each time a new window is
    // created.
    var rootNode = myPanelNode.parentNode;
    doc.body.removeChild(myPanelNode);

    // We cache the iframe on the document object to indicate that
    // we've already created it and do not need to create it again.
    if (doc.iframe) return;

    // Create an iframe to host our activity.xul.
    var iframe = doc.createElement('iframe');
    iframe.frameBorder = '0';
    iframe.width = '100%';
    iframe.height = '100%';
    iframe.style.position = 'absolute';
    iframe.style.top = 0;
    iframe.style.bottom = 0;
    iframe.style.left = 0;
    iframe.style.right = 0;
    iframe.src = 'chrome://activity/content/activity.xul';
    doc.iframe = iframe;
    iframe.id = 'activity-iframe';
    rootNode.appendChild(iframe);

    // Finally, tell the activity profiler code about the new iframe, so it can
    // populate the timeline. The activity profiler code is loaded as an overlay
    // in browser.xul, which is the topmost window context.
    activity.ui.setTimelineWindow(iframe.contentWindow);
  },

  /**
   * Get a list of items for the 'options' menu in the upper right corner of
   * the firebug pane.
   *
   * @override
   *
   * @return {Array} List of objects whose properties are used by firebug to
   *    build the options menu.
   */
  getOptionsMenuItems: function() {
    var menuOptions = [];

    /**
     * Given a menu item name and a function, add a menu item which calls that
     * function when selected.
     *
     * @param {string} label The text of the menu item.
     * @param {Function} onSelectMenuItem Called when the menu item is selected.
     * @param {boolean} checked Whether the menu item should be checked.
     * @param {boolean?} opt_disabled Whether the menu item should be disabled.
     */
    var addMenuOption = function(
        label, onSelectMenuItem, checked, opt_disabled) {
      var menuItemObj = {
        label: label,
        nol10n: true,  // Use the label as-is, rather than looking in a
                       // properties file.
        command: onSelectMenuItem,
        type: 'checkbox',
        checked: checked,
        disabled: Boolean(opt_disabled)
      };
      menuOptions.push(menuItemObj);
    };

    if (activity.Profiler.isCompatibleJsd()) {
      var profileJavaScriptEnabled =
          activity.preference.getBool(
              activity.ui.PREF_ENABLE_JS_PROFILING_, true);
      addMenuOption(
          'Show JavaScript Events',
          function() {activity.ui.performCommand('toggleProfileJavaScript'); },
          profileJavaScriptEnabled);
      if (profileJavaScriptEnabled) {
        addMenuOption(
            'Full Call Graphs (slow)',
            function() {
              activity.ui.performCommand('toggleCompleteCallGraphs');
            },
            activity.preference.getBool(
                activity.ui.PREF_COLLECT_COMPLETE_CALL_GRAPHS_, false));
      }
    }

    // Firebug 1.4 betas show a menu even when there are no items, and it looks odd.
    // Bryan filed a bug (http://code.google.com/p/fbug/issues/detail?id=1896).
    // Until it is resolved, add a disabled menu item.
    if (!menuOptions.length) {
      addMenuOption('No Options',
                    function() {},
                    false,
                    true);
    }

    return menuOptions;
  },

  /** @override */
  show: function(state) {
    if (this.showToolbarButtons) {
      this.showToolbarButtons('fbActivityButtons', true);
    }
  },

  /** @override */
  hide: function() {
    if (this.showToolbarButtons) {
      this.showToolbarButtons('fbActivityButtons', false);
    }
  }
});  // ActivityPanel

/**
 * Create a Firebug Module so that we can use Firebug Hooks.
 * @extends {Firebug.Module}
 */
Firebug.ActivityModule = extend(Firebug.Module, {
  /**
   * @param {Object} browser The browser object we are rendering a
   *     panel for.
   * @param {Object} panel The panel we are displaying.
   * @override
   */
  showPanel: function(browser, panel) {
    if (!panel ||
        !panel.panelBrowser ||
        !panel.panelBrowser.contentDocument) return;

    var isActivityProfiler = 'activity' == panel.name;
    if (!isActivityProfiler) activity.ui.performCommand('stopProfiler');

    var iframe =
        panel.panelBrowser.contentDocument.getElementById('activity-iframe');
    if (!iframe) return;
    iframe.style.display = isActivityProfiler ? '' : 'none';
  },

  /**
   * Called when we need to collapse the UI.
   * @override
   */
  hideUI: function() {
    activity.ui.performCommand('reset');
  }
});  // ActivityModule

Firebug.registerPanel(ActivityPanel);
Firebug.registerModule(Firebug.ActivityModule);

}});  // FBL.ns
