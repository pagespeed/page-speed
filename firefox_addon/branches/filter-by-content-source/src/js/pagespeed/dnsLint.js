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
 * @fileoverview Checks for two types of wasteful DNS lookups:
 *     1. Javascript files issued early during the page load. These lookups
 *        completely block progress rendering the page in most browsers.
 *     2. More than three DNS requests that each only serve one resource.
 *        A DNS request is considered wasteful if it doesn't serve at least 2
 *        resources. But because there are cases where a single resource needs
 *        to be served from a different domain (third-party JS library, ads,
 *        etc.), 3 "wasteful" DNS requests are allowed before this rule will
 *        complain.
 *
 * @author Bryan McQuade
 *         Tony Gentilcore
 */

(function() {  // Begin closure

/**
 * TreeWalker filter that visits all nodes.
 * @type {nsIDOMTreeWalker}
 */
var acceptAllNodes = {
  acceptNode: function() { return NodeFilter.FILTER_ACCEPT; }
};

/**
 * @return {number} The number of HTML nodes under the given node.
 */
function countAllNodesUnder(doc, node) {
  var numNodes = 0;
  var walker = doc.createTreeWalker(
      node, NodeFilter.SHOW_ELEMENT, acceptAllNodes, false);
  while (walker.nextNode()) numNodes++;
  return numNodes;
}

/**
 * @param {nsIDOMDocument} doc The parent document.
 * @param {nsIDOMNode} node The node to traverse.
 * @param {number} maxNodesToVisit The maximum number of nodes to
 *     visit.
 * @return {nsIDOMTreeWalker} A TreeWalker that will visit external
 *     script tags.
 */
function createExternalScriptTagVisitor(doc, node, maxNodesToVisit) {
  var filter = {
    visitCount_: 0,
    maxNodesToVisit_: maxNodesToVisit,
    acceptNode: function(node) {
      if (this.maxNodesToVisit_ >= 0 &&
          this.visitCount_++ > this.maxNodesToVisit_) {
        // We've already visited more than maxNodesToVisit_ nodes, so
        // we can safely return FILTER_REJECT instead of
        // FILTER_SKIP. FILTER_REJECT will skip a node and all of its
        // children, while FILTER_SKIP skips a node but visits its
        // children.
        return NodeFilter.FILTER_REJECT;
      }
      if (node.nodeName.toLowerCase() != 'script') {
        // Must skip, not reject, since the first clause above wants
        // to count all nodes, and rejection would cause us to skip
        // child nodes.
        return NodeFilter.FILTER_SKIP;
      }
      if (!node.src || node.src.length == 0) {
        // Must skip, not reject, since the first clause above wants
        // to count all nodes, and rejection would cause us to skip
        // child nodes.
        return NodeFilter.FILTER_SKIP;
      }

      // Found an external script tag. Accept it.
      return NodeFilter.FILTER_ACCEPT;
    }
  };

  return doc.createTreeWalker(node, NodeFilter.SHOW_ELEMENT, filter, false);
}

/**
 * @return {Array.<nsIDOMNode>} All external script nodes in the
 *     beginning of the document (<head> plus first 10% of <body>).
 */
function getEarlyExternalScriptNodes() {
  var aNodes = [];
  var aDocs = PAGESPEED.Utils.getElementsByType('doc');
  if (!aDocs || aDocs.length == 0) {
    return aNodes;
  }

  var doc = aDocs[0];

  // This is somewhat inefficient since it traverses the entire
  // document, but we only want to traverse a few levels
  // deep (html->head is 2 levels under the document). Consider
  // improving performance if this is turns out to be a
  // perf bottleneck.
  var headNodes = doc.getElementsByTagName('head');

  // Find all external script tags in the head.
  for (var i = 0, len = headNodes.length; i < len; i++) {
    var walker = createExternalScriptTagVisitor(doc, headNodes[i], -1);
    while (walker.nextNode()) aNodes.push(walker.currentNode);
  }

  // Find all external script tags in first 10% of the nodes in the
  // body.
  var body = doc.body;
  if (body) {
    var numBodyNodes = countAllNodesUnder(doc, body);
    var walker = createExternalScriptTagVisitor(doc, body, numBodyNodes / 10);
    while (walker.nextNode()) aNodes.push(walker.currentNode);
  }

  return aNodes;
}

/**
 * @param {string} documentHostname The hostname of the root document.
 * @param {string} docContents The contents of the root document.
 * @return {Array.<string>} An array of warnings about blocking DNS lookups.
 */
function getBlockingDnsLookupWarnings(documentHostname, docContents) {
  var aScriptNodes = getEarlyExternalScriptNodes();

  var urls = [];
  for (var i = 0, len = aScriptNodes.length; i < len; i++) {
    var src = aScriptNodes[i].src;
    // Sometimes, script tags are inserted into the document using
    // JavaScript late in the page load, but they end up in the head
    // of the DOM tree. We only want to penalize for
    // script nodes that were actually in the document when it was
    // initially parsed, so we search for a script tag fragment in the
    // actual document contents. This may miss script tags that are
    // written into the document early during the page load
    // (e.g. using document.write). It might be better to record the
    // time each request is initiated in the ComponentCollectorService
    // and user some time-based heuristics instead.
    var tagRegexp = new RegExp('src=["\']?' + src, 'i');

    // We didn't find a script fragment for the given URL, so we
    // do not include it in our analysis.
    if (docContents.search(tagRegexp) == -1) continue;

    urls.push(src);
  }

  var map = PAGESPEED.Utils.getHostToResourceMap(urls);

  var aWarnings = [];
  for (var hostname in map) {
    // Don't penalize for resources served from the same hostname as
    // the main document.
    if (hostname == documentHostname) continue;
    aWarnings.push(PAGESPEED.Utils.formatWarnings(map[hostname]));
  }
  return aWarnings;
}

/**
 * @param {string} documentHostname The hostname of the root document.
 * @param {Array.<string>} resourceUrls Urls of resources to test for wasteful
 *     DNS lookups.
 * @return {Array.<string>} An array of warnings about blocking DNS lookups.
 */
function getWastefulDnsLookupWarnings(documentHostname, resourceUrls) {
  var map = PAGESPEED.Utils.getHostToResourceMap(resourceUrls);

  var aWarnings = [];
  for (var hostname in map) {
    // Don't penalize for resources served from the same hostname as
    // the main document.
    if (hostname == documentHostname) continue;
    // A lookup is only wasteful if it only serves one resource.
    if (map[hostname].length > 1) continue;
    aWarnings.push(hostname);
  }
  return aWarnings;
}

/**
 * @this PAGESPEED.LintRule
 * @param {PAGESPEED.ResourceAccessor} resourceAccessor An object that
 *     allows rules to fetch content by type.
 */
var dnsRule = function(resourceAccessor) {
  var docUri = PAGESPEED.Utils.getElementsByType('doc')[0].documentURIObject;
  var docContents = PAGESPEED.Utils.getResourceContent(
      docUri.spec.replace(/#.*$/, ''));
  var documentHostname = docUri.host;

  var aBlockingWarnings = getBlockingDnsLookupWarnings(
      documentHostname, docContents);
  if (aBlockingWarnings.length > 0) {
    this.score -= aBlockingWarnings.length * 11;
    this.warnings =
        ['Serve the following JavaScript resources from the same',
         ' host as the main document (', documentHostname,
         '), or defer loading of these resources if possible:',
         aBlockingWarnings.join('')].join('');
  }

  var resourceUrls = resourceAccessor.getResources(
      'all',
      null,  // No extra filter.
      true); // Only return resources fetched before onload.

  var aWastefulWarnings = getWastefulDnsLookupWarnings(
      documentHostname, resourceUrls);

  if (aWastefulWarnings.length > 3) {
    this.score -= aWastefulWarnings.length * 6;
    this.warnings +=
        ['The following domains only serve one resource each. ',
         'If possible, avoid the extra DNS lookups by serving these resources ',
         'from existing domains.',
         PAGESPEED.Utils.formatWarnings(aWastefulWarnings)].join('');
  }
};

PAGESPEED.LintRules.registerLintRule(
  new PAGESPEED.LintRule(
    'Minimize DNS lookups',
    PAGESPEED.RTT_GROUP,
    'rtt.html#MinimizeDNSLookups',
    dnsRule,
    3.5,
    'MinDns'
  )
);

})();  // End closure
