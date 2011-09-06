package com.googlecode.page_speed.autotester;
import com.googlecode.page_speed.autotester.iface.ResponseHandler;

import java.util.HashMap;

// Copyright 2011 Google Inc. All Rights Reserved.


/**
 * A class that delegates asynchronous responses to ResponseHandlers.
 *
 * @author azlatin@google.com (Alexander Zlatin)
 *
 * @param <KT> The key type.
 * @param <DT> The data type.
 */
public class ResponseDispatcher<KT, DT> {

  private HashMap<KT, ResponseHandler<DT>> handlers;

  public ResponseDispatcher() {
    handlers = new HashMap<KT, ResponseHandler<DT>>();
  }

  /**
   * Add a new response handler.
   * @param key The key on which to dispatch.
   * @param value The Response handler to dispatch to.
   * @return The previous response handler for this key if one exists or null.
   */
  public ResponseHandler<DT> put(KT key, ResponseHandler<DT> value) {
    return handlers.put(key, value);
  }

  /**
   * Checks whether any more responses are expected.
   * @return True if no more handlers exist, false otherwise.
   */
  public boolean isEmpty() {
    return handlers.isEmpty();
  }

  /**
   * Deletes all handlers.
   */
  public void clear() {
    handlers.clear();
  }

  /**
   * Handles a response with an id and data.
   * @param id The id of the response.
   * @param data The data in the response.
   * @return True if the response was handled, false otherwise.
   */
  public boolean handleResponse(KT id, DT data) {
    if (handlers.containsKey(id)) {
      ResponseHandler<DT> handler = handlers.get(id);
      if (handler != null) {
        handler.onResponse(data);
      }
      handlers.remove(id);
      return true;
    }
    return false;
  }

}
