package com.googlecode.page_speed.autotester.util;

import org.json.simple.JSONObject;

import java.util.HashMap;

// Copyright 2011 Google Inc. All Rights Reserved.


/**
 * Various utility methods for dealing with JSON data.
 * 
 * @author azlatin@google.com (Alexander Zlatin)
 * 
 */
public class DataUtils {
  public static final String ID_KEY = "identifier";  //"requestId";

  private static HashMap<String, String> identifierPaths = null;
  
  /**
   * Gets a sub-object of a JSON object by traversing down the object by key.
   * All objects that are traversed must be JSONObjects. The purpose of this
   * method is to avoid the ugly code caused by having to cast to JSONObject
   * after each call to get().
   * 
   * @param obj The root object.
   * @param path A period delimited list of keys.
   * @return The object at obj.get(key[0]).get(key[1]).get(key[...])
   */
  public static Object getByPath(JSONObject obj, String path) {
    String[] parts = path.split("\\.");
    JSONObject curObj = obj;
    Object finalObj = null;
    try {
      for (int x = 0; x < parts.length; x++) {
        if (x < parts.length - 1) {
          curObj = (JSONObject) curObj.get(parts[x]);
        } else {
          finalObj = curObj.get(parts[x]);
        }
      }
    } catch (NullPointerException e) {
      return null;
    }
    return finalObj;
  }

  /**
   * Gets the id from an object. 
   * @param obj The json object response to get the id from
   * @return The id object.
   */
  public static Object getId(JSONObject obj) {
    if (identifierPaths == null) {
      // Map of method -> identifier path.
      identifierPaths = new HashMap<String, String>();
      identifierPaths.put("Timeline.eventRecorded", "params.record.data." + ID_KEY);
      identifierPaths.put("Network.dataReceived", "params." + ID_KEY);
      identifierPaths.put("Network.requestWillBeSent", "params." + ID_KEY);
      identifierPaths.put("Network.responseReceived", "params." + ID_KEY);
      identifierPaths.put("Network.loadingFinished", "params." + ID_KEY);
      identifierPaths.put("Network.loadingFailed", "params." + ID_KEY);
      identifierPaths.put("Network.resourceMarkedAsCached", "params." + ID_KEY);
      identifierPaths.put("Network.resourceLoadedFromMemoryCache", "params." + ID_KEY);
      identifierPaths.put("__Internal.resourceContent", ID_KEY);
    }
    if (obj != null) {
      if (obj.containsKey("method") && identifierPaths.containsKey(obj.get("method"))) {
        return DataUtils.getByPath(obj, identifierPaths.get(obj.get("method")));
      }
      if (obj.containsKey(ID_KEY)) {
        return obj.get(ID_KEY);
      }
      if (obj.containsKey("id")) {
        return obj.get("id");
      }
    }
    return ""; // This will never match anything and is better than returning null.
  }
  
  /**
   * Checks whether a json object response is of a certain method type.
   * @param obj The JSON object response
   * @param method The method to check for.
   * @return True if the object is of the specified method, false otherwise.
   */
  public static boolean isMethod(JSONObject obj, String method) {
    return obj != null && obj.containsKey("method") && obj.get("method").equals(method);
  }
  
}
