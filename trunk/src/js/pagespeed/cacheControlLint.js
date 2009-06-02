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
 * @fileoverview A lint rule for determining if Cache-Control headers are
 * appropriately set.
 *
 * @author Kyle Scholz
 */

(function() {  // Begin closure

PAGESPEED.CacheControlLint = {};

var kInformational = 0;
var kWarning = 1;
var kSevere = 2;
var kFailRule = 3;
var msInAMonth = 1000 * 60 * 60 * 24 * 30;
var msInElevenMonths = msInAMonth * 11;

/**
 * Constructor for a CacheRule.
 * @param {string} message A message to display for files violating this rule.
 * @param {number} severity The severity of the rule (kInformational,
 *     kWarning, kSevere, kFailRule).
 * @param {Function} exec A function which returns true if the given component
 *     and header are in violation of the rule.
 * @constructor
 */
var CacheRule = function(message, severity, exec) {
  this.message = message;
  this.severity = severity;
  this.exec = exec;
  this.violations = [];
};

/**
 * Left rotates x by the specified number of bits in a 32 bit field.
 * @param {number} x The 32 bit unsigned int to rotate.
 * @param {number} bits The number of bits to rotate.
 * @return {number} The rotated number.
 */
function rotateLeft(x, bits) {
  return ((x << bits) & 0xffffffff) | ((x >>> (32 - bits)) & 0xffffffff);
}

/**
 * Returns the hash key that Mozilla uses for the given URL.
 * See original implementations:
 * http://mxr.mozilla.org/seamonkey/source/netwerk/cache/src/nsDiskCacheDevice.cpp#240
 * http://mxr.mozilla.org/mozilla1.8/source/netwerk/cache/src/nsDiskCacheDevice.cpp#270
 * @param {string} url The URL for which to generate the cache key.
 * @return {number} The hash key for the given URL.
 */
PAGESPEED.CacheControlLint.generateMozillaHashKey = function(url) {
  var h = 0;
  for (var i = 0, len = url.length; i < len; ++i) {
    h = rotateLeft(h, 4) ^ url.charCodeAt(i);
  }
  return h;
};

/**
 * Determines if the given headers contain a header.
 *
 * The http spec says header names are case insensitive.  Firefox rewrites
 * the header names into a canonical capitalization.  targetHeader needs to
 * be in this canonical form.  To see the headers for a resource in canonical
 * form, click 'Show Resources' in the pagespeed UI and look at any object
 * which has the headers you are interested in.  The capitalization shown
 * is the canonical capitalization.
 *
 * @param {Object} headers An object with a key for each header and a value
 *     containing the contents of that header.
 * @param {string} targetHeader The header to match.
 * @return {boolean} true iff the headers contain the given header.
 */
function hasHeader(headers, targetHeader) {
  return headers.hasOwnProperty(targetHeader);
}

/**
 * Determines if the given headers contain a header that contains the target
 * string (case insensitive).
 * @param {Object} headers An object with a key for each header and a value
 *     containing the contents of that header.
 * @param {string} targetHeader The header to match.
 * @param {string} targetString The string to search for in the header.
 * @return {boolean} true iff the headers contain the given header and it
 *     contains the target string.
 */
function headerContains(headers, targetHeader, targetString) {
  if (!hasHeader(headers, targetHeader)) return false;

  var re = new RegExp(targetString, 'im');
  return headers[targetHeader].match(re);
}

/**
 * @param {Object} headers An object with a key for each header and a value
 *     containing the contents of that header.
 * @return {boolean} true iff the headers contain an expiration date
 *     for this resource.
 */
function hasExplicitExpiration(headers) {
  // HTTP/1.1 RFC says: HTTP/1.1 clients and caches MUST treat
  // invalid date formats, especially including the value "0", as in the
  // past (i.e., "already expired") so we do not need to validate the
  // contents of these headers. We only need to check that they are
  // present.
  return hasHeader(headers, 'Date') &&
       (hasHeader(headers, 'Expires') ||
        headerContains(headers, 'Cache-Control', 'max-age'));
}

/**
 * @param  {Object} headers An object with a key for each header and a value
 *     containing the contents of that header.
 * @param  {number} timeMs The freshness lifetime to compare with.
 * @return {boolean} true iff the headers indicate that this resource
 *     has a freshness lifetime greater than the specified time.
 */
function freshnessLifetimeGreaterThan(headers, timeMs) {
  if (!hasHeader(headers, 'Date')) {
    // HTTP RFC says the date header is required. If not present, we
    // have no reference point to compute the freshness lifetime from,
    // so we assume it has no freshness lifetime.
    return false;
  }

  var dateHdrMs = Date.parse(headers['Date']);
  if (isNaN(dateHdrMs)) {
    return false;
  }

  var freshnessLifetimeMs;

  // The max-age overrides Expires in most modern browsers.
  if (headerContains(headers, 'Cache-Control', 'max-age')) {
    var maxAgeMatch = headers['Cache-Control'].match(/max-age=(\d+)/);
    freshnessLifetimeMs =
        (maxAgeMatch && maxAgeMatch[1]) ? 1000 * maxAgeMatch[1] : 0;
  } else if (hasHeader(headers, 'Expires')) {
    var expDate = Date.parse(headers['Expires']);
    if (!isNaN(expDate)) {
      freshnessLifetimeMs = expDate - dateHdrMs;
    }
  }

  // Non-numeric freshness lifetime is considered a zero freshness
  // lifetime.
  if (isNaN(freshnessLifetimeMs)) return false;

  return freshnessLifetimeMs > timeMs;
}

/**
 * @param {string} url The URL of the resource.
 * @param {Object} headers An object with a key for each header and a value
 *     containing the contents of that header.
 * @return {boolean} true iff the headers indicate that this resource may ever
 *     be publicly cacheable.
 */
function isPubliclyCacheable(url, headers) {
  if (isExplicitlyNonCacheable(url)) {
    return false;
  }

  if (headerContains(headers, 'Cache-Control', 'public')) {
    return true;
  }

  // A response that isn't explicitly marked as private that does not
  // have a query string is cached by most proxies.
  if (url.indexOf('?') == -1 &&
      !headerContains(headers, 'Cache-Control', 'private')) {
    return true;
  }

  return false;
}

/**
 * @param {string} type The type of resource.
 * @return {boolean} whether the resource type is known to be
 *     cacheable.
 */
function isCacheableResourceType(type) {
  return (type == 'image' || type == 'js' ||
          type == 'cssimage' ||
          type == 'css' || type == 'object');
}

/**
 * @param {string} type The type of resource.
 * @return {boolean} whether the resource type is known to be
 *     compressible.
 */
function isCompressibleResourceType(type) {
  return (type == 'css' || type == 'js');
}

/**
 * @param {string} type The type of resource.
 * @return {boolean} whether the resource type is known to be
 *     uncacheable.
 */
function isNonCacheableResourceType(type) {
  // TODO: Figure out some good rules around caching XHRs.
  // TODO: Figure out if there is any way to cache a redirect.
  return (type == 'doc' || type == 'iframe' ||
          type == 'redirect' || type == 'other');
}

/**
 * @param {string} url The URL of the resource.
 * @return {boolean} whether the resource type is explicitly
 *     uncacheable.
 */
function isExplicitlyNonCacheable(url) {
  // Don't run any rules on URLs that explicitly do not want to be cached
  // (e.g. beacons).
  var headers = PAGESPEED.Utils.getResponseHeaders(url);
  var responseCode = PAGESPEED.Utils.getResponseCode(url);
  var hasExplicitExp = hasExplicitExpiration(headers);
  return (headerContains(headers, 'Cache-Control', 'no-cache') ||
          headerContains(headers, 'Cache-Control', 'no-store') ||
          headerContains(headers, 'Cache-Control', 'must-revalidate') ||
          headerContains(headers, 'Pragma', 'no-cache') ||
          // Explicit expiration in the past is the HTTP/1.0 equivalent
          // of Cache-Control: no-cache.
          (hasExplicitExp && !freshnessLifetimeGreaterThan(headers, 0)) ||
          // According to the HTTP RFC, responses with query strings
          // and no explicit caching headers must not be cached.
          (!hasExplicitExp && url.indexOf('?') >= 0) ||
          // According to the HTTP RFC, only responses with certain
          // response codes can be cached in the absence of caching
          // headers.
          (!hasExplicitExp &&
           !PAGESPEED.Utils.isCacheableResponseCode(responseCode)));
}

/**
 * Execute the given rules for the given resource.
 * @param {Array.<CacheRule>} rules The array of rules to execute.
 * @param {string} url The URL of the resource to be evaluated.
 */
function execRulesForResource(rules, url) {
  var headers = PAGESPEED.Utils.getResponseHeaders(url);
  var type = PAGESPEED.Utils.getResourceTypes(url)[0];
  for (var j = 0; j < rules.length; j++) {
    if (rules[j].exec.call(rules[j], url, headers, type)) {
      rules[j].violations.push(url);
    }
  }
}

/**
 * @param {Array.string} aMessages array of messages to format.
 * @return {string} The formatted messages.
 */
function sortAndFormatWarnings(aMessages) {
  aMessages.sort();
  return PAGESPEED.Utils.formatWarnings(aMessages);
}

/**
 * Format the violations for the given linter and rules.
 * @param {PAGESPEED.LintRule} linter The lint rule function object.
 * @param {Array.<CacheRule>} rules The array of rules to execute.
 * @param {number} numUrls The number of resource URLs that were
 *     evaluated.
 */
function formatViolations(linter, rules, numUrls) {
  if (numUrls == 0) {
    linter.score = 'n/a';
    return;
  }
  for (var i = 0, len = rules.length; i < len; i++) {
    if (rules[i].violations.length) {
      if (rules[i].severity == kInformational) {
        linter.information += rules[i].message +
          sortAndFormatWarnings(rules[i].violations);
      } else if (rules[i].severity == kFailRule) {
        linter.warnings += rules[i].message +
          sortAndFormatWarnings(rules[i].violations);
        linter.score = 0;
      } else if (rules[i].severity == kWarning) {
        linter.warnings += rules[i].message +
          sortAndFormatWarnings(rules[i].violations);
        linter.score -= 50 * rules[i].violations.length / numUrls;
      } else if (rules[i].severity == kSevere) {
        linter.warnings += rules[i].message +
          sortAndFormatWarnings(rules[i].violations);
        linter.score -= 100 * rules[i].violations.length / numUrls;
      }
    }
  }
}

/**
 * Given a list of URLs, divide them into groups such that
 * all members of each group have the same hash in firefox's
 * cache, and each group has more than one member.
 * @param {Array.string} urls The urls to group by hash.
 * @param {Function} hashFn This function will be run on each url to
 *    determine its key in a cache.
 * @return {Array.Array.string} Each element of this array is a set
 *    of urls which have the same hash.
 */
PAGESPEED.CacheControlLint.findCacheCollisions = function(
    urls, hashFn) {

  var urlHashes = urls.map(hashFn);
  var hashToUrl = {};

  for (var i = 0, len = urls.length; i < len; ++i) {
    var hash = urlHashes[i];

    if (hash in hashToUrl) {
      hashToUrl[hash].push(urls[i]);
    } else {
      hashToUrl[hash] = [urls[i]];
    }
  }

  var urlsGroupedByHash = [];

  // For each hash that maps to more than one Url, add
  // those urls to the array of url groups that collide.
  for (var hash in hashToUrl) {
    if (hashToUrl[hash].length <= 1)
      continue;

    urlsGroupedByHash.push(hashToUrl[hash]);
  }

  return urlsGroupedByHash;
};

/**
 * A Cache-Control Lint Rule
 * @this PAGESPEED.LintRule
 */
var browserCachingRule = function() {
  var rules = [
    new CacheRule('The following resources are missing a cache expiration.' +
                  ' Resources that do not specify an expiration may not be' +
                  ' cached by browsers.' +
                  ' Specify an expiration at least one month in the' +
                  ' future for resources that should be cached, and an' +
                  ' expiration in the past' +
                  ' for resources that should not be cached:',
                  kSevere,
                  function(url, headers, type) {
                    return isCacheableResourceType(type) &&
                        !hasHeader(headers, 'Set-Cookie') &&
                        !hasExplicitExpiration(headers);
                  }),
    new CacheRule('The following resources specify a "Vary" header that' +
                  ' disables caching in most versions of Internet Explorer.' +
                  ' Fix or remove the "Vary" header for the' +
                  ' following resources:',
                  kSevere,
                  function(url, headers, type) {
                    var varyHeader = headers['Vary'];
                    if (varyHeader) {
                      // MS documentation indicates that IE will cache
                      // responses with Vary Accept-Encoding or
                      // User-Agent, but not anything else. So we
                      // strip these strings from the header, as well
                      // as separator characters (comma and space),
                      // and trigger a warning if there is anything
                      // left in the header.
                      varyHeader = varyHeader.replace(/User-Agent/gi, '');
                      varyHeader = varyHeader.replace(/Accept-Encoding/gi, '');
                      varyHeader = varyHeader.replace(/[, ]*/g, '');
                    }
                    return isCacheableResourceType(type) &&
                        varyHeader &&
                        varyHeader.length > 0 &&
                        freshnessLifetimeGreaterThan(headers, 0);
                  }),
    new CacheRule('The following cacheable resources have a short' +
                  ' freshness lifetime. Specify an expiration at least one' +
                  ' month in the future for the following resources:',
                  kWarning,
                  function(url, headers, type) {
                    // Add an Expires header. Use at least one month in the
                    // future.
                    return isCacheableResourceType(type) &&
                        !hasHeader(headers, 'Set-Cookie') &&
                        !freshnessLifetimeGreaterThan(headers, msInAMonth) &&
                        freshnessLifetimeGreaterThan(headers, 0);
                  }),
    new CacheRule('Favicons should have an expiration at least one month' +
                  ' in the future:',
                  kWarning,
                  function(url, headers, type) {
                    // Its not reasonable to suggest that the favicon be a year
                    // in the future because sometimes the path cannot be
                    // controlled. However, it is very reasonable to suggest a
                    // month.
                    return type == 'favicon' &&
                        !hasHeader(headers, 'Set-Cookie') &&
                        !freshnessLifetimeGreaterThan(headers, msInAMonth);
                  }),
    new CacheRule('To further improve cache hit rate, specify an expiration' +
                  ' one year in the future for the following cacheable' +
                  ' resources:',
                  kInformational,
                  function(url, headers, type) {
                    // Add an Expires header. Use at least one year in the
                    // future.
                    return isCacheableResourceType(type) &&
                        !hasHeader(headers, 'Set-Cookie') &&
                        !freshnessLifetimeGreaterThan(
                            headers, msInElevenMonths) &&
                        freshnessLifetimeGreaterThan(headers, msInAMonth);
                  })
  ];

  // Keep track of all resources marked as explicitly non-cacheable by
  // the server.
  var aNonCacheableResources = [];
  var numCacheableResources = 0;

  var urls = PAGESPEED.Utils.filterProtocols(PAGESPEED.Utils.getResources());

  // Firefox has a feeble hash algorithm that is prone to collisions. Check
  // for collisions among resources on the same page here.
  //
  // TODO: If possible, it would be nice to determine if IE has the
  // same problem. However, it could be impossible to figure out its hashing
  // algorithm.
  var urlCacheCollisions = PAGESPEED.CacheControlLint.findCacheCollisions(
      urls, PAGESPEED.CacheControlLint.generateMozillaHashKey);

  if (urlCacheCollisions.length) {
    var addToWarnings = [];

    var kCacheCollisionWarning = [
        'Due to a URL conflict, the Firefox browser cache can store only ',
        'one of these resources at a time. Changing the URLs of some ',
        'resources can fix this problem. Consult the Page Speed ',
        'documentation for information on how to disambiguate these URLs.'
        ].join('');

    for (var i = 0, len = urlCacheCollisions.length; i < len; ++i) {
      addToWarnings.push(kCacheCollisionWarning);

      addToWarnings.push(
          sortAndFormatWarnings(urlCacheCollisions[i]));
      this.score -= 11;
    }

    this.warnings += addToWarnings.join('');
  }

  for (var i = 0, len = urls.length; i < len; i++) {
    var type = PAGESPEED.Utils.getResourceTypes(urls[i])[0];
    if (isNonCacheableResourceType(type)) {
      continue;
    }

    if (isExplicitlyNonCacheable(urls[i])) {
      aNonCacheableResources.push(urls[i]);
      continue;
    }

    numCacheableResources++;
    execRulesForResource(rules, urls[i]);
  }

  formatViolations(this, rules, numCacheableResources);

  if (aNonCacheableResources.length > 0) {
    this.information +=
        ['The following resources are explicitly non-cacheable. Consider ',
         'making them cacheable if possible:',
         sortAndFormatWarnings(aNonCacheableResources)].join('');
  }
};

/**
 * A Cache-Control Lint Rule
 * @this PAGESPEED.LintRule
 */
var proxyCachingRule = function() {
  var rules = [
    new CacheRule('Due to a bug in some proxy caching servers,' +
                  ' the following publicly' +
                  ' cacheable, compressible resources should use' +
                  ' "Cache-Control: private" or "Vary: Accept-Encoding":',
                  kWarning,
                  function(url, headers, type) {
                    // Support for compressed resources is broken on
                    // some proxies. The HTTP RFC does not call out
                    // that Content-Encoding should be a part of the
                    // cache key, which causes clients to break if a
                    // compressed response is served to a client that
                    // doesn't support compressed content. Most HTTP
                    // proxies work around this bug in the spec, but
                    // some don't. This function detects resources
                    // that are not properly configured for caching by
                    // these proxies. We do not check for the presence
                    // of Content-Encoding here because we recommend
                    // gzipping these resources elsewhere, so we
                    // assume that the client is going to enable
                    // compression for these resources if they haven't
                    // already.
                    return !hasHeader(headers, 'Set-Cookie') &&
                        isCompressibleResourceType(type) &&
                        isPubliclyCacheable(url, headers) &&
                        !headerContains(headers, 'Vary', 'Accept-Encoding');
                  }),
    new CacheRule('Resources with a "?" in the URL are not cached by most' +
                  ' proxy caching servers. Remove the query string and' +
                  ' encode the parameters into the URL for the following' +
                  ' resources:',
                  kWarning,
                  function(url, headers, type) {
                    // Static files should be publicly cacheable unless
                    // responses contain a Set-Cookie header.
                    return url.indexOf('?') >= 0 &&
                        !hasHeader(headers, 'Set-Cookie') &&
                        isPubliclyCacheable(url, headers);
                  }),
    new CacheRule('Consider adding a "Cache-Control: public" header to the' +
                  ' following resources:',
                  // We do not know for certain which if any HTTP
                  // proxies require CC: public in order to cache
                  // content, so we make this informational for now.
                  kInformational,
                  function(url, headers, type) {
                    // Static files should be publicly cacheable unless
                    // responses contain a Set-Cookie header.
                    return isCacheableResourceType(type) &&
                        !isCompressibleResourceType(type) &&
                        !headerContains(headers, 'Cache-Control', 'public') &&
                        !hasHeader(headers, 'Set-Cookie');
                  }),
    new CacheRule('The following publicly cacheable resources contain' +
                  ' a Set-Cookie header. This security vulnerability' +
                  ' can cause cookies to be shared by multiple users.',
                  kFailRule,
                  function(url, headers, type) {
                    // Files with Cookie headers should never be publicly
                    // cached.
                    return hasHeader(headers, 'Set-Cookie') &&
                        isPubliclyCacheable(url, headers);
                  })
  ];

  var numCacheableResources = 0;

  // Loop through all files finding suboptimal caching headers.
  var urls = PAGESPEED.Utils.filterProtocols(PAGESPEED.Utils.getResources());
  for (var i = 0, len = urls.length; i < len; i++) {
    var type = PAGESPEED.Utils.getResourceTypes(urls[i])[0];
    if (isNonCacheableResourceType(type)) {
      continue;
    }

    if (isExplicitlyNonCacheable(urls[i])) {
      continue;
    }

    numCacheableResources++;
    execRulesForResource(rules, urls[i]);
  }

  formatViolations(this, rules, numCacheableResources);
};

PAGESPEED.LintRules.registerLintRule(
  new PAGESPEED.LintRule(
    'Leverage browser caching',
    PAGESPEED.CACHE_GROUP,
    'caching.html#LeverageBrowserCaching',
    browserCachingRule,
    4.0
  )
);

PAGESPEED.LintRules.registerLintRule(
  new PAGESPEED.LintRule(
    'Leverage proxy caching',
    PAGESPEED.CACHE_GROUP,
    'caching.html#LeverageProxyCaching',
    proxyCachingRule,
    3.0
  )
);

})();  // End closure
