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
 * @fileoverview Rule to find resources that could be parallelized
 * across hostnames to decrease page load time.
 *
 * @author Bryan McQuade
 */

(function() {  // Begin closure

// Internal tests show that balancing requests across 4 hostnames is
// optimal.
// TODO: revisit this value when FF3 and IE8 are more widely
// deployed, since they allow a greater number of connections per
// hostname.
var OPTIMAL_NUMBER_OF_HOSTNAMES_ = 4;

// Don't generate a score unless the busiest hostname has at least
// this many resources.
var MIN_REQUEST_THRESHOLD_ = 10;

// Don't penalize the site until their busiest host is 50% busier than
// the average of the top four hosts.
var MIN_BALANCE_THRESHOLD_ = 0.5;

/**
 * Rule to find resources that could be parallelized
 * across hostnames to decrease page load time.
 * @this PAGESPEED.LintRule
 * @param {PAGESPEED.ResourceAccessor} resourceAccessor An object that
 *     allows rules to fetch content by type.
 */
var parallelizeDownloadRule = function(resourceAccessor) {
  // Find all "static" resources, except for JS, since choice of
  // domain serving for JS downloads can't be parallelized in most
  // browsers.

  var urls = resourceAccessor.getResources(
      ['css', 'image', 'object', 'cssimage'],
      new PAGESPEED.ResourceFilters.FetchedByProtocol(['http', 'https']),
      true);  // Only return resources fetched before onload.

  // Create a mapping from hostname to an array of resources served by
  // that hostname.
  var hosts = [];
  var hostToResource = {};
  var numHosts = 0;
  for (var i = 0, len = urls.length; i < len; i++) {
    var hostname = PAGESPEED.Utils.getDomainFromUrl(urls[i]);
    if (!hostToResource[hostname]) {
      hostToResource[hostname] = [];
      hosts.push(hostname);
      numHosts++;
    }
    hostToResource[hostname].push(urls[i]);
  }

  var SortByResourceCounts = function(a, b) {
    var aCount = hostToResource[a].length;
    var bCount = hostToResource[b].length;
    return (aCount < bCount) ? 1 : (a == b) ? 0 : -1;
  };

  // Sort the hosts from those with most resources, to those with least.
  hosts.sort(SortByResourceCounts);

  // For the time being, we think that 4 hosts is the maximum number
  // that we should parallelize on. Thus, only evaluate the 4 hosts
  // with the most resources.
  if (hosts.length > OPTIMAL_NUMBER_OF_HOSTNAMES_) {
    hosts.splice(OPTIMAL_NUMBER_OF_HOSTNAMES_);
  }

  var numResourcesOnBusiestHost = 0;
  if (hostToResource[hosts[0]]) {
    numResourcesOnBusiestHost = hostToResource[hosts[0]].length;
  }

  var numResourcesAboveThreshold =
      numResourcesOnBusiestHost - MIN_REQUEST_THRESHOLD_;
  if (numResourcesAboveThreshold <= 0) {
    // If the host serving the most requests serves 10 or fewer
    // resources, then parallelization is probably overkill.
    this.score = 'n/a';
    return;
  }

  // Compute the average number of resources per host, over 4 hosts.
  var avgResourcesPerHost = 0;
  for (var i = 0, len = hosts.length; i < len; i++) {
    avgResourcesPerHost += hostToResource[hosts[i]].length;
  }

  // We intentionally hard-code 4 in the denominator, instead of
  // using hosts.length. If hosts.length is less than 4, the app
  // could benefit from parallelization. So we compute the average
  // based on the assumption that the resources should be
  // distributed over 4 hosts.
  avgResourcesPerHost /= OPTIMAL_NUMBER_OF_HOSTNAMES_;

  // We compute their score based on the percentage difference between
  // the number of resources on the busiest host and the average
  // number of resources per host.

  if (avgResourcesPerHost == 0) {
    // Round up to 1, so we don't divide by zero. javascript numbers
    // are floats so this should never happen, but just to be safe...
    avgResourcesPerHost = 1;
  }

  var percentageAboveAvg =
      (numResourcesAboveThreshold / avgResourcesPerHost) - 1.0;

  if (percentageAboveAvg < MIN_BALANCE_THRESHOLD_) {
    // If the busiest hostname serves less than 50% more than the
    // average, they are balanced enough. Give a 100%.
    this.score = 100;
    return;
  }

  this.score -= (percentageAboveAvg - MIN_BALANCE_THRESHOLD_) * 100.0;

  var aUrlsOnBusiestHost = hostToResource[hosts[0]];

  // Give the client some information to help them understand why
  // we are suggesting this rule. This is especially important for
  // teams that have already adopted parallelization. If their
  // parallelization is not well balanced, we want to call out
  // their busiest host so they know that they are sending too
  // many requests to that host.
  this.warnings = [
    'This page makes ', numResourcesOnBusiestHost, ' parallelizable ',
    'requests to ', hosts[0], '. Increase download parallelization by ',
    'distributing these requests across multiple hostnames:',
    PAGESPEED.Utils.formatWarnings(aUrlsOnBusiestHost)].join('');
};

// For now, this is an experimental rule. Experimental rules don't
// actually count against the overall score.
PAGESPEED.LintRules.registerLintRule(
  new PAGESPEED.LintRule(
    'Parallelize downloads across hostnames',
    PAGESPEED.RTT_GROUP,
    'rtt.html#ParallelizeDownloads',
    parallelizeDownloadRule,
    2.5,
    'ParallelDl'
  )
);

})();  // End closure
