package com.googlecode.page_speed.autotester;

import com.googlecode.page_speed.autotester.util.DataUtils;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;

import java.net.URI;
import java.net.URISyntaxException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

// Copyright 2011 Google Inc. All Rights Reserved.

/**
 * Generates a HAR file from network data gathered from Chrome Debugger Tools.
 * The produced HAR is valid and good enough for use with Page Speed, but could
 * be improved to be more accurate and complete.
 * 
 * See WebKit/Source/WebCore/inspector/front-end/HAREntry.js
 * 
 * @author azlatin@google.com (Alexander Zlatin)
 * 
 */
@SuppressWarnings("unchecked")
class HARCreator {
  
  private static final String HAR_VERSION = "1.2";
  private TestResult result;

  /**
   * Create a new HAR builder object.
   * 
   * @param aResult the test result object to generate a har for.
   */
  public HARCreator(TestResult aResult) {
    this.result = aResult;
  }

  /**
   * Build the HAR.
   * 
   * @return The JSON HAR object.
   */
  public JSONObject build() {
    JSONObject obj = new JSONObject();
    obj.put("log", generateLog());
    return obj;
  }

  /**
   * Generates the log object.
   * @return The log object.
   */
  protected JSONObject generateLog() {
    JSONObject obj = new JSONObject();
    obj.put("version", HAR_VERSION);
    obj.put("creator", generateCreator());
    // obj.put("browser", generateBrowser());
    JSONArray entries = generateEntries();
    obj.put("pages", generatePages(DataUtils
      .getByPath((JSONObject) entries.get(0), "startedDateTime")));
    obj.put("entries", entries);
    // obj.put("comment", "");
    return obj;
  }

  /**
   * Generates the creator object. This should describe this script.
   * @return The creator object.
   */
  protected JSONObject generateCreator() {
    JSONObject obj = new JSONObject();
    obj.put("name", "HARCreator.java");
    obj.put("version", "0.2");
    // obj.put("comment", "");
    return obj;
  }

  /**
   * Generates the browser descriptor object.
   * TODO(azlatin): Make this contain the real browser version.
   * @return the browser object.
   */
  protected JSONObject generateBrowser() {
    JSONObject obj = new JSONObject();
    obj.put("name", "Chrome");
    obj.put("version", "");
    // obj.put("comment", "");
    return obj;
  }

  /**
   * Generates the list of pages loaded.
   * Currently only generates the main page.
   * 
   * @param startTime The time the page started loading in ISO8601.
   * @return an array of loaded pages.
   */
  protected JSONArray generatePages(Object startTime) {
    JSONArray arr = new JSONArray();
    JSONObject mainPage = new JSONObject();
    mainPage.put("startedDateTime", startTime);
    mainPage.put("id", "main_page");
    mainPage.put("title", "");
    mainPage.put("pageTimings", generatePageTimings());
    // mainPage.put("comment", "");
    arr.add(mainPage);
    return arr;
  }
  
  /**
   * Generates the pageTimings object.
   * This uses getMetrics() in TestResult.
   * @return the pageTimings object.
   */
  protected JSONObject generatePageTimings() {
    JSONObject obj = new JSONObject();
    JSONObject metrics = result.getMetrics();
    obj.put("onContentLoad", metrics.get("time_to_base_page_complete_ms"));
    obj.put("onLoad", metrics.get("load_time_ms"));
    // obj.put("comment", "");
    return obj;
  }

  /**
   * Generates the list of resources loaded by the page, including the page itself.
   * All network data from dev tools is grouped together by resource. Each resource
   * is then separately processed.
   * @return the entries object.
   */
  protected JSONArray generateEntries() {
    JSONArray arr = new JSONArray();
    Map<Object, Resource> resources = new HashMap<Object, Resource>();
    for (JSONObject obj : result.getData()) {
      if (obj == null || !obj.containsKey("method")) {
        continue;
      }
      // All network events corresponding to a resource are grouped.
      Object id = DataUtils.getId(obj);
      if (id == null || id.equals("")) {
        continue;
      }
      if (!resources.containsKey(id)) {
        resources.put(id, new Resource(id));
      }
      String method = (String) obj.get("method");
      Resource resource = resources.get(id);
      if (method.equals("Network.dataReceived")) {
        resource.addDataChunk((JSONObject) obj.get("params"));
      } else if (method.equals("Network.loadingFailed")) {
        resource.setDataComplete((JSONObject) obj.get("params"));
      } else if (method.equals("Network.loadingFinished")) {
        resource.setDataComplete((JSONObject) obj.get("params"));
      } else if (method.equals("Network.requestWillBeSent")) {
        resource.addRequest((JSONObject) obj.get("params"));
      } else if (method.equals("Network.responseReceived")) {
        resource.setResponse((JSONObject) obj.get("params"));
        Object fromCache = DataUtils.getByPath(obj, "params.response.fromDiskCache");
        if (fromCache != null && (Boolean) fromCache) {
          resource.setCached();
        }
      } else if (method.equals("__Internal.resourceContent")) {
        resource.setContent((JSONObject) obj.get("result"));
      } else if (method.equals("Network.resourceMarkedAsCached")) {
        resource.setCached();
      } else if (method.equals("Network.resourceLoadedFromMemoryCache")) {
        //This includes a CachedResource with a cached ResourceResponse.
        JSONObject params = (JSONObject) obj.get("params");
        // We make it look similar to a regular resource completion.
        JSONObject cachedResource = (JSONObject) params.get("resource");
        for (Object key : cachedResource.keySet()) {
          params.put(key, cachedResource.get(key));
        }
        resource.setResponse(params);
        resource.setCached();
        
        // We never get a request notification, so lets make one.
        JSONObject requestRequest = new JSONObject();
        requestRequest.put("headers", new JSONObject());
        requestRequest.put("url", params.get("url"));
        requestRequest.put("method", "GET");  // Post requests can't be cached AFAIK.
        JSONObject request = new JSONObject();
        request.put("documentURL", params.get("documentURL"));
        request.put("initiator", params.get("initiator"));
        request.put("loaderId", params.get("loaderId"));
        request.put("request", requestRequest);
        request.put("requestId", params.get("requestId"));
        request.put("timestamp", params.get("timestamp"));
        
        resource.addRequest(request);
      }
    }
    
    for (Resource res : resources.values()) {
      processResource(arr, res);
    }
    
    // HAR Viewer wants these in order.
    // http://www.softwareishard.com/har/viewer/
    Collections.sort(arr, new Comparator<JSONObject>() {
      @Override
      public int compare(JSONObject o1, JSONObject o2) {
        return ((String) o1.get("startedDateTime"))
            .compareTo(((String) o2.get("startedDateTime")));
      }
    });
    
    return arr;
  }
  
  /**
   * Process a resource and generate an entry for it.
   * @param entries The list of entries to add it to.
   * @param res The resource to process.
   */
  protected void processResource(JSONArray entries, Resource res) {
    JSONObject next = null;
    JSONObject request = null;
    JSONObject response = null;
    if (res.getRequests().isEmpty()) {
      System.out.println("No requests for resource #" + res.id);
    }
    for (JSONObject sentReq : res.getRequests()) {
      request = (JSONObject) sentReq.get("request");
      if (res.isLastRequest(sentReq)) {
        // This is the only or last request, meaning it can't be a redirect.
        // Lets grab the final resource response.
        next = res.getFinalResponse();
        if (next == null) {
          // We never got a response before we stopped recording.
          continue;
        }
        response = (JSONObject) next.get("response");
      } else {
        // This is a redirect so we have to grab the redirect response.
        next = res.getNextRequest(sentReq);
        if (next == null) {
          // This should *never* happen since this can't be the last request.
          System.err.println("Could not get next request for #" + res.id);
          continue;
        }
        response = (JSONObject) next.get("redirectResponse");
        if (response == null) {
          // This should also *never* happen.
          System.err.println("Next request was not a redirect for #" + res.id);
          continue;
        }
      }
      Object status = response.get("status");
      if (res.getFinalResponse() == null
          || response.get("timing") == null) {
        // These are cases we can't handle.
        // No final response likely means it was an async load.
        // No timing means about:blank
        return;
      }
      try {
        entries.add(generateEntry(request, response, res, sentReq, next));
      } catch (Exception e) {
        e.printStackTrace();
        //System.err.println("Request: " + request);
        //System.err.println("Response: " + response);
      }
    }
  }

  /**
   * Generates an entry from resource requests and responses.
   * 
   * @param request The ResourceRequest object (from dev tools).
   * @param response The ResourceResponse object (from dev tools).
   * @param res The resource object.
   * @param rawRequest The requestWillBeSent notification params..
   * @param rawResponse The responseReceived notification params.
   * @return The entry object.
   */
  protected JSONObject generateEntry(JSONObject request, JSONObject response,
      Resource res, JSONObject rawRequest, JSONObject rawResponse) {
    JSONObject timing = (JSONObject) response.get("timing");
    JSONObject timings;
    
    // Handle cached resources.
    // We might get back cached timings and headers, so we will use the best guess
    // we have, which is the timestamp of the notification.
    if (res.isCached()) {
      timings = new JSONObject();
      Double timestamp = (Double) rawRequest.get("timestamp");
      timing.put("requestTime", timestamp);
      timings.put("send", 0);
      timings.put("wait", 0);
      timings.put("receive", 0);
      if (res.isLastRequest(request)) {
        Object endTime = res.getDataCompleteTime();
        if (endTime != null) {
          timings.put("receive", interval(timing.get("requestTime"), endTime, 1000));
        }
      }
    } else {
      timings = generateTimings(request, response, timing, res);
    }    
    
    JSONObject obj = new JSONObject();
    obj.put("pageref", "main_page");
    obj.put("startedDateTime", generateStartedDateTime(timing));
    obj.put("time", generateTime(timings));
    obj.put("request", generateRequest(request, response));
    obj.put("response", generateResponse(request, response, res));
    obj.put("cache", generateCache(response));
    obj.put("timings", timings);
    // obj.put("serverIPAddress", "1.2.3.4");
    obj.put("connection", generateConnectionId(response));
    // obj.put("comment", "");
    return obj;
  }

  /**
   * Generate the start time in ISO8601 format.
   * @param timing The timing object (from dev tools).
   * @return the startedDateTime for the resource request.
   */
  protected String generateStartedDateTime(JSONObject timing) {
    Object reqTime = timing.get("requestTime");
    Long date = null;
    try {
      date = new Double((((Double) reqTime) * 1000.0)).longValue();
    } catch (ClassCastException e) {
      // In very rare cases, this is a Long (maybe a whole value double?).
      date = ((Long) reqTime) * 1000;
    }
    return toISO8601(new Date(date));
  }

  /**
   * Generates the total time used for this request in ms.
   * @param timings The resource timings;
   * @return the time for the resource request.
   */
  protected Integer generateTime(JSONObject timings) {
    int sum = 0;
    Integer intval;
    for (Object val : timings.values()) {
      intval = (Integer) val;
      if (intval != -1) {
        // -1 means N/A
        sum += intval;
      }
    }
    return sum;
  }

  /**
   * Generates the timings for each part of the request for this resource.
   * @param request The ResourceRequest (from dev tools).
   * @param response The ResourceResponse (from dev tools).
   * @param timing The timing object (from dev tools).
   * @param res The Resource object.
   * @return the resource timings
   */
  protected JSONObject generateTimings(JSONObject request, JSONObject response,
      JSONObject timing, Resource res) {

    JSONObject timings = new JSONObject();

    int connectWait = interval(timing.get("connectStart"), timing.get("connectEnd"));
    int blocked;
    int connect;
    int dns = interval(timing.get("dnsStart"), timing.get("dnsEnd"));
    int send = interval(timing.get("sendStart"), timing.get("sendEnd"));
    int ssl = interval(timing.get("sslStart"), timing.get("sslEnd"));
    int wait = interval(timing.get("sendEnd"), timing.get("receiveHeadersEnd"));

    Double responseReceivedTime = (Double) timing
        .get("requestTime") + (Long) timing.get("receiveHeadersEnd") / 1000.0;

    int receive = 0;
    Double endTime = null;
    if (res.isLastRequest(request)) {
      // If it is the last request, we can use the time the resource finished downloading.
      endTime = (Double) res.getDataCompleteTime();
    } else {
      // Otherwise we just use the time until we requested the resource we redirected to.
      endTime = (Double) res.getNextRequest(request).get("timestamp");
    }
    // For some reason, sometimes the resource finishes downloading
    // before responseReceivedTime.
    if (endTime != null && responseReceivedTime < endTime) {
      receive = interval(responseReceivedTime, endTime, 1000);
    } else {
      receive = 0;
    }

    if (ssl != -1 && send != -1) {
      // HAR spec says this should be subtracted from connect, but dev tools
      // HAREntry.js subtracts from send.
      send -= ssl;
    }

    if ((Boolean) response.get("connectionReused")) {
      connect = -1;
      blocked = connectWait;
    } else {
      blocked = 0;
      connect = connectWait;
      if (dns != -1) {
        connect -= dns;
      }
    }

    timings.put("blocked", blocked);
    timings.put("dns", dns);
    timings.put("connect", connect);
    timings.put("send", send);
    timings.put("wait", wait);
    timings.put("receive", receive);
    timings.put("ssl", ssl);
    return timings;
  }

  /**
   * Generates the connection id.
   * @param response The ResourceResponse (from dev tools).
   * @return the connection id.
   */
  protected String generateConnectionId(JSONObject response) {
    return response.get("connectionId").toString();
  }

  /**
   * Generates the request object.
   * @param request The ResourceRequest (from dev tools).
   * @param response The ResourceResponse (from dev tools).
   * @return the request object.
   */
  protected JSONObject generateRequest(JSONObject request, JSONObject response) {
    JSONObject requestObj = new JSONObject();

    String headers = (String) response.get("requestHeadersText");
    // This is a guess since there is no way to know for sure.
    String[] httpRequest = {"", "", "HTTP/1.1"};
    if (headers != null) {
      httpRequest = headers.split("\r\n")[0].split(" ");
    } else {
      headers = "";
    }

    String url = (String) request.get("url");
    String postData = (String) request.get("postData");
    JSONObject headerMap = (JSONObject) response.get("requestHeaders");

    requestObj.put("method", request.get("method"));
    requestObj.put("url", url);
    requestObj.put("httpVersion", httpRequest[2]);
    requestObj.put("cookies", generateRequestCookies(headerMap));
    requestObj.put("headers", generateRequestHeaders(headerMap));
    requestObj.put("queryString", generateQueryString(url));
    requestObj.put("headersSize", headers.length());
    if (postData == null) {
      requestObj.put("bodySize", -1);
    } else {
      requestObj.put("postData", generatePostData(postData));
      requestObj.put("bodySize", postData.length());
    }
    // request.put("comment", "");
    return requestObj;
  }

  /**
   * Generates a query string object from a path.
   * 
   * @param path The path with the query string.
   * @return A JSONArray of HAR formatted name value pairs.
   */
  protected JSONArray generateQueryString(String path) {
    String qs;
    try {
      qs = (new URI(path)).getRawQuery();
    } catch (URISyntaxException e) {
      qs = null;
    }

    if (qs == null) {
      return new JSONArray();
    }

    String[] params = qs.split("&");
    JSONObject map = new JSONObject();
    for (String param : params) {
      String[] pair = param.split("=", 2);
      String name = pair[0];
      String value = pair.length > 1 ? pair[1] : "";
      map.put(name, value);
    }
    return parseNVPairs(map);
  }

  /**
   * Generates posted data info.
   * 
   * @param postData The content body of the post request.
   * @return A postData object.
   */
  protected JSONObject generatePostData(String postData) {
    JSONObject post = new JSONObject();
    
    String[] params = postData.split("&");
    JSONObject map = new JSONObject();
    for (String param : params) {
      String[] pair = param.split("=", 2);
      String name = pair[0];
      String value = pair.length > 1 ? pair[1] : "";
      map.put(name, value);
    }
    
    post.put("mimeType", "multipart/form-data");
    post.put("params", parseNVPairs(map));
    post.put("text", postData);
    // post.put("comment", "");
    return post;
  }
  
  /**
   * Generates the request header array from the request header JSON object.
   * @param headers the JSONObject with a mapping of header names to values.
   * @return An array of name value pairs of headers.
   */
  protected JSONArray generateRequestHeaders(JSONObject headers) {
    return parseNVPairs(headers);
  }

  /**
   * Parses the cookies that were sent to the server.
   * @param headers The JSONObject of headers.
   * @return An array of name value pairs of cookies.
   */
  protected JSONArray generateRequestCookies(JSONObject headers) {
    String cookies = (String) DataUtils.getByPath(headers, "Cookie");
    if (cookies == null) {
      return new JSONArray();
    }
    return parseCookieHeader(cookies.split(";\\s*"));
  }

  /**
   * Generates the response object.
   * @param request The ResourceRequest (from dev tools).
   * @param response The ResourceResponse (from dev tools).
   * @param res The Resource object.
   * @return the response object.
   */
  protected JSONObject generateResponse(JSONObject request, JSONObject response, Resource res) {
    JSONObject responseObj = new JSONObject();

    String headers = (String) response.get("headersText");
    if (headers == null) {
      headers = "";
    }
    JSONObject headerMap = (JSONObject) response.get("headers");

    Object redirect = headerMap.get("Location");
    if (redirect == null) {
      redirect = "";
    }

    responseObj.put("status", response.get("status"));
    responseObj.put("statusText", response.get("statusText"));
    responseObj.put("httpVersion", headers.split("\r\n")[0].split(" ")[0]);
    responseObj.put("cookies", generateResponseCookies(headerMap));
    responseObj.put("headers", generateResponseHeaders(headerMap));
    responseObj.put("content", generateResponseContent(request, response, res));
    responseObj.put("redirectURL", redirect);
    responseObj.put("headersSize", headers.length());
    responseObj.put("bodySize", res.getDataSize("encodedDataLength"));
    // response.put("comment", "");
    return responseObj;
  }

  /**
   * Generates the response content object.
   * @param request The ResourceRequest (from dev tools).
   * @param response The ResourceResponse (from dev tools).
   * @param res The Resource object.
   * @return The content object for the resource.
   */
  protected JSONObject generateResponseContent(JSONObject request,
      JSONObject response, Resource res) {
    JSONObject content = new JSONObject();

    Object mimeType = response.get("mimeType");
    if (mimeType == null) {
      // Fallback
      mimeType = DataUtils.getByPath(response, "headers.Content-Type");
    }
    content.put("mimeType", mimeType);

    Long size = null;
    if (res.isLastRequest(request)) {
      size = res.getDataSize("dataLength");
      JSONObject contentResponse = res.getContent();
      if (contentResponse != null) {
        String value = (String) contentResponse.get("content");
        content.put("text", value);
        Object base64 = contentResponse.get("base64Encoded");
        if ((Boolean) base64) {
          content.put("encoding", "base64");
        }
      }
    }

    if (size == null) {
      // Fallback
      size = res.getResponseContentLength(response);
      if (size == null) {
        size = 0L;
      }
    }

    content.put("size", size);
    // content.put("comment", "");

    return content;
  }

  /**
   * Generates the response header array from the response header JSON object.
   * @param headers the JSONObject with a mapping of header names to values.
   * @return An array of name value pairs of headers.
   */
  protected JSONArray generateResponseHeaders(JSONObject headers) {
    return parseNVPairs(headers);
  }

  /**
   * Parses the cookies that were received from the server.
   * @param headers The JSONObject of headers.
   * @return An array of name value (+ extra options) objects describing cookies.
   */
  protected JSONArray generateResponseCookies(JSONObject headers) {
    String cookies = (String)  DataUtils.getByPath(headers, "Set-Cookie");
    if (cookies == null) {
      return new JSONArray();
    }
    return parseCookieHeader(cookies.split("\n"));
  }

  /**
   * Generates information about the caching of the resource.
   * @param response The response from the server
   * @return the cache object.
   */
  protected JSONObject generateCache(JSONObject response) {
    JSONObject cache = new JSONObject();
    // cache.put("beforeRequest", new JSONObject());
    // cache.put("afterRequest", new JSONObject());
    // content.put("comment", "");
    // TODO(azlatin): Add cache info.
    return cache;
  }

  /**
   * Parses a list of cookies from Cookie and Set-Cookie headers.
   * 
   * @param cookieList A list of raw cookies.
   * @return A JSONArray of HAR formatted cookie objects.
   */
  private static JSONArray parseCookieHeader(String[] cookieList) {
    JSONArray cookies = new JSONArray();
    if (cookieList != null) {
      for (String cookieStr : cookieList) {
        // Parse cookie
        String[] fields = cookieStr.split(";\\s*");

        String[] cookieNameValue = fields[0].split("=", 2);
        String cookieName = cookieNameValue[0];
        String cookieValue = cookieNameValue.length == 1 ? "" : cookieNameValue[1];
        String expires = null;
        String path = null;
        String domain = null;
        boolean secure = false;

        // Parse each field
        for (int j = 1; j < fields.length; j++) {
          if ("secure".equalsIgnoreCase(fields[j])) {
            secure = true;
          } else if (fields[j].indexOf("=") > 0) {
            String[] f = fields[j].split("=", 2);
            if ("expires".equalsIgnoreCase(f[0])) {
              expires = f[1];
            } else if ("domain".equalsIgnoreCase(f[0])) {
              domain = f[1];
            } else if ("path".equalsIgnoreCase(f[0])) {
              path = f[1];
            }
          }
        }

        JSONObject cookieObj = new JSONObject();
        cookieObj.put("name", cookieName);
        cookieObj.put("value", cookieValue);
        cookieObj.put("secure", secure);
        if (expires != null) {
          cookieObj.put("expires", expires);
        }
        if (path != null) {
          cookieObj.put("path", path);
        }
        if (domain != null) {
          cookieObj.put("domain", domain);
        }
        // cookie_obj.put("comment", );
        cookies.add(cookieObj);
      }
    }
    return cookies;
  }

  /**
   * Parses name to value maps into a list of HAR formatted name value objects.
   * 
   * @param obj A name->value Map/JSONObject.
   * @return A JSONArray of HAR formatted name value objects.
   */
  private static JSONArray parseNVPairs(JSONObject obj) {
    JSONArray pairs = new JSONArray();
    if (obj == null) {
      return pairs;
    }
    for (Object key : obj.keySet()) {
      String name = (String) key;
      String value = (String) obj.get(name);
      JSONObject header = new JSONObject();
      header.put("name", name);
      header.put("value", value);
      // header.put("comment", "");
      pairs.add(header);
    }
    return pairs;
  }
  
  /**
   * Gets the difference between Doubles a and b.
   * 
   * @param a Double value Start time
   * @param b Double value End time
   * @param mult Multiplier for the final value
   * @return -1 if a or b are -1, abs(a-b)*mult otherwise
   */
  private static int interval(Object a, Object b, int mult) {
    Double valA = Double.parseDouble(a.toString());
    Double valB = Double.parseDouble(b.toString());
    if (valA > valB) {
      System.err.println("Start time is greater than end time in interval().");
    }
    if (valA == -1 || valB == -1) {
      return -1;
    }
    return (int) Math.round((valB - valA) * mult);
  }

  private static int interval(Object a, Object b) {
    return interval(a,  b,  1);
  }

  /**
   * Convert a data object to a ISO 8601 formatted string.
   * 
   * @param date The date to convert.
   * @return `date` as an ISO 8601 formatted string.
   */
  private static String toISO8601(Date date) {
    SimpleDateFormat df = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SZ");
    String result = df.format(date);
    int length = result.length();
    result = result.substring(0, length - 2) + ":" + result.substring(length - 2);
    return result;
  }
  
  /**
   * Represents a single resource;
   */
  private class Resource {
    public final Object id;
    private ArrayList<JSONObject> requestChain;
    private JSONObject response = null;
    private ArrayList<JSONObject> dataChunks;
    private JSONObject dataComplete = null;
    private JSONObject content = null;
    private boolean cached = false;
    
    public Resource(Object aId) {
      id = aId;
      requestChain = new ArrayList<JSONObject>();
      dataChunks = new ArrayList<JSONObject>();
    }

    protected void printError(Boolean condition, String error) {
      if (condition) {
        System.err.println(String.format("%s (%s - %s)",
          error, id, DataUtils.getByPath(requestChain.get(0), "request.url")));
        //System.err.println("Requests: " + requestChain);
        //System.err.println("Response: " + response);
      }
    }
    
    public void setResponse(JSONObject obj) {
      printError(response != null, "Duplicate response.");
      response = obj;
    }
    public void addRequest(JSONObject obj) {
      requestChain.add(obj);
    }
    public void setDataComplete(JSONObject obj) {
      printError(dataComplete != null, "Duplicate data complete.");
      dataComplete = obj;
    }
    public void addDataChunk(JSONObject obj) {
      dataChunks.add(obj);
    }
    public void setContent(JSONObject obj) {
      printError(content != null, "Duplicate content.");
      content = obj;
    }
    public void setCached() {
      cached = true;
    }

    /**
     * Returns the list of requests that led up to receiving the content.
     * This includes length - 1 redirects. The last (and possibly first) request
     * is the request for the final content (not a redirect).
     * 
     * @return a list of requests made for this resource.
     */
    public List<JSONObject> getRequests() {
      return requestChain;
    }

    public boolean isLastRequest(JSONObject obj) {
      JSONObject last = requestChain.get(requestChain.size() - 1);
      // We look for both the notification params or the ResourceRequest.
      return last == obj || last.get("request") == obj;
    }

    public JSONObject getNextRequest(JSONObject obj) {
      // The next request will contain the response to a (non-last) redirect.
      for (int x = 0; x < requestChain.size(); x++) {
        JSONObject req = requestChain.get(x);
        if (req == obj || req.get("request") == obj) {
          return requestChain.get(x + 1);
        }
      }
      return null;
    }
    
    public JSONObject getFinalResponse() {
      // printError(response == null, "No final response.");
      return response;
    }
    public Long getResponseContentLength(JSONObject aResponse) {
      String contentLength = (String) DataUtils
          .getByPath(aResponse, "headers.Content-Length");
      if (contentLength != null) {
        return Long.parseLong(contentLength);
      }
      return 0L;
    }
    
    public Long getDataSize(String type) {
      // Type = dataLength, encodedDataLength
      Long total = 0L;
      for (JSONObject chunk : dataChunks) {
        Long size = (Long) DataUtils.getByPath(chunk, type);
        // encodedDataLength is sometimes 0, so default to unencoded.
        // My guess is that is the case if it isn't encoded.
        if (size == 0 && type.equals("encodedDataLength")) {
          size = (Long) DataUtils.getByPath(chunk, "dataLength");
        }
        total += size;
      }
      if (total == 0) {
        total = getResponseContentLength((JSONObject) response.get("response"));
      }
      return total;
    }

    public Object getDataCompleteTime() {
      // We will also ignore these.
      // printError(dataComplete == null, "Data didn't finish loading.");
      if (dataComplete == null) {
        return DataUtils.getByPath(getFinalResponse(), "timestamp");
      }
      return DataUtils.getByPath(dataComplete, "timestamp");
    }
    
    public JSONObject getContent() {
      // We can't get plugin resource content.
      // printError(content == null, "No content.");
      return content;
    }
    
    public boolean isCached() {
      return cached;
    }
    
  }

}
