package com.googlecode.page_speed.autotester;

import com.googlecode.page_speed.autotester.iface.ResponseHandler;
import com.googlecode.page_speed.autotester.util.DataUtils;
import com.googlecode.page_speed.autotester.util.FileUtils;

import org.chromium.sdk.SyncCallback;
import org.chromium.sdk.internal.websocket.WsConnection;
import org.json.simple.JSONObject;
import org.json.simple.JSONValue;

import java.io.File;
import java.io.IOException;

// Copyright 2011 Google Inc. All Rights Reserved.

/**
 * Loads a URL into a tab and requests all needed data (DOM, Timeline).
 * 
 * @author azlatin@google.com (Alexander Zlatin)
 * 
 */
@SuppressWarnings("unchecked")
public class TabController extends TestObservable implements WsConnection.Listener {

  /*
   * We need to make sure the observer isn't notified of a test completion
   * until at least after page load. This is a dummy id used to add a handler
   * that is removed after onload.
   * 
   * The value is arbitrary and can be any negative number. Any negative
   * value can be used since real IDs can go up to 9,223,372,036,854,775,807
   * before they overflow back to -9,223,372,036,854,775,808
   */
  private static final Long PAGE_LOAD = -31337L;
  
  private Long testCompleteTimeout;
  private Thread watchDog = null;
  
  // This will contain the JS injected into pages to collect the DOM.
  // TODO(azlatin): Need a better DOM solution using
  // http://code.google.com/chrome/devtools/docs/protocol/dom.html
  // DOM.getDocument DOM.getChildNodes
  private static String domCollector = null;

  // The next id to be used for a method call.
  private Long nextId = null;
  private WsConnection tab = null;
  private String nextURL = null;

  private TestResult result = null;
  private ResponseDispatcher<Long, JSONObject> responseHandlers = null;

  /**
   * Creates a new controller for a web socket connection to a tab.
   * 
   * @param aTab The WebSocket connection to a debuggable tab.
   * @param aTimeout How long to wait for test completion before aborting.
   */
  public TabController(WsConnection aTab, Long aTimeout) {
    super();
    this.nextId = new Long(0);
    this.tab = aTab;
    this.testCompleteTimeout = aTimeout;
    this.responseHandlers = new ResponseDispatcher<Long, JSONObject>();

    ClassLoader classLoader = getClass().getClassLoader();
    if (domCollector == null) {
      try {
        domCollector = FileUtils.getContents(classLoader.getResourceAsStream("domCollector.js"));
      } catch (Exception e) {
        domCollector = FileUtils.getContents(new File("res/domCollector.js"));
      }
    }
  }

  /**
   * Sets the next URL to load.
   * 
   * @param aResult The result to save responses to.
   */
  public void setTest(TestResult aResult) {
    nextURL = aResult == null ? null : aResult.request.getFullURL();
    result = aResult;
    if (nextURL != null) {
      responseHandlers.clear();
      
      JSONObject params;
      params = new JSONObject();
      params.put("newWindow", false);
      params.put("url", "about:blank");
      sendMessage("Page.open", params);
      if (aResult.request.first == 0) {
        sendMessage("Network.clearBrowserCache");
        sendMessage("Network.clearBrowserCookies");
      }
      // params = new JSONObject();
      // params.put("cacheDisabled", aResult.request.first == 0);
      // sendMessage("Network.setCacheDisabled", params);
      watchDog = new Watchdog();
      watchDog.start();
    }
  }

  /**
   * Gets the tab connection.
   * 
   * @return The websocket connection to the tab.
   */
  public WsConnection getTab() {
    return tab;
  }

  /**
   * Gets the next method call id.
   * 
   * @return The next method call id.
   */
  public synchronized Long getNextId() {
    return nextId++;
  }
  
  /**
   * Call a method over the debugging socket with no parameters.
   * @param method  The method to call.
   * @return The id of the method call.
   */
  protected Long sendMessage(String method) {
    return sendMessage(method, new JSONObject());
  }
  
  /**
   * Call a method over the debugging socket.
   * 
   * @param method The method to call.
   * @param params The parameters to the method call.
   * @return The id of the method call.
   */
  protected Long sendMessage(String method, JSONObject params) {
    Long currentId = getNextId();

    JSONObject obj = new JSONObject();
    obj.put("id", new Long(currentId));
    obj.put("method", method);
    obj.put("params", params);

    try {
      tab.sendTextualMessage(obj.toJSONString());
      // System.out.println("Out: " + obj.toJSONString());
    } catch (IOException e) {
      e.printStackTrace();
    }
    return currentId;
  }

  @Override
  public void textMessageRecieved(String text) {
    if (result == null) {
      return;
    }
    JSONObject obj = (JSONObject) JSONValue.parse(text);
    // System.out.println(obj.toJSONString());
    // System.out.println(responseHandlers.size());

    if (obj.containsKey("method")) {
      String method = (String) obj.get("method");
      
      if (nextURL != null) {
        /*
         * nextURL is set to the next URL to be loaded.
         * When it is set using setURL, about blank is loaded.
         * After about:blank is loaded, this will set nextURL to null.
         * This block keeps about:blank events from being handled.
         */
        processAboutBlank(method, obj);
        return;
      }
      
      result.addData(obj);
      processMethod(method, obj);
    } else if (obj.containsKey("id")) {
      if (nextURL != null) {
        return;
      }
      Long id = (Long) obj.get("id");
      processResponse(id, obj);
    }
  }

  @Override
  public void errorMessage(Exception ex) {
    System.err.println("WebSocket protocol error");
  }

  @Override
  public void eofMessage() {
    System.err.println("WebSocket EOF");
  }

  /**
   * Processes a method call from the server.
   * 
   * @param method The name of the method that was called.
   * @param obj The object sent by the server.
   */
  protected void processMethod(String method, JSONObject obj) {
    if (method.equals("Page.loadEventFired")) {
      JSONObject params = new JSONObject();
      params.put("expression", domCollector);
      responseHandlers.put(sendMessage("Runtime.evaluate", params), new HandleDomResponse());
      responseHandlers.handleResponse(PAGE_LOAD, null);
    } else if (method.equals("Network.loadingFinished")
        || method.equals("Network.resourceLoadedFromMemoryCache")) {
      Object identifier = DataUtils.getId(obj);
      JSONObject params = new JSONObject();
      params.put(DataUtils.ID_KEY, identifier);
      responseHandlers.put(sendMessage("Network.getResourceContent", params),
          new HandleResourceContent(identifier));
    }
  }

  /**
   * Processes a response from a method call. If there are no more outstanding
   * responses, listeners are notified.
   * 
   * @param id The id of the call/result.
   * @param obj The result of the method call.
   */
  protected void processResponse(Long id, JSONObject obj) {
    if (responseHandlers.handleResponse(id, obj)) {
      if (responseHandlers.isEmpty()) {
        endTest();
      }
    }
  }
  
  /**
   * Ends the test.
   */
  protected void endTest() {
    if (watchDog != null) {
      watchDog.interrupt();
      watchDog = null;
    }
    sendMessage("Network.disable");
    sendMessage("Timeline.stop");
    result.setEnd();
    TestResult lastResult = result;
    setTest(null);
    notifyTestCompleted(lastResult);
  }
  
  /**
   * Processes any methods from the pre-test about:blank page load.
   * 
   * @param method The name of the method that was called.
   * @param obj The object sent by the server.
   */
  protected void processAboutBlank(String method, JSONObject obj) {
    if (method.equals("Page.loadEventFired")) {
      // about:blank has finished loading.
      JSONObject params;

      try {
        Thread.sleep(500);
        // Make sure cache had a chance to be cleared.
      } catch (InterruptedException e) {
        e.printStackTrace();
      }

      params = new JSONObject();
      //params.put("maxCallStackDepth", 2);
      sendMessage("Timeline.start", params);
      sendMessage("Network.enable");

      params = new JSONObject();
      params.put("newWindow", false);
      params.put("url", nextURL);
      sendMessage("Page.open", params);
      
      responseHandlers.put(PAGE_LOAD, null);
    } else if (method.equals("Timeline.started")) {
      /*
       * My limited observations have shown this keeps destructor/frame detached
       * events from being recorded.
       */
      nextURL = null;
      result.addData(obj);
    }
  }

  /**
   * Handles a response object containing the DOM of the document.
   * 
   */
  private class HandleDomResponse implements ResponseHandler<JSONObject> {
    @Override
    public void onResponse(JSONObject obj) {
      JSONObject dom = null;
      try {
        dom = (JSONObject) JSONValue
            .parse((String) DataUtils.getByPath(obj, "result.result.value"));
      } catch (NullPointerException e) {
        try {
          // Fallback for older Chrome versions
          // In newer versions, result.result.description is an error message,
          // but that is OK since parsing it results in null.
          dom = (JSONObject) JSONValue
              .parse((String) DataUtils.getByPath(obj, "result.result.description"));
        } catch (NullPointerException e2) {
        }
      }
      if (dom == null) {
        System.err.println("DOM failed: " + obj.toJSONString());
        dom = new JSONObject();
      }
      result.addData(dom);
    }
  }

  /**
   * Handles a response object containing resource data.
   * 
   */
  private class HandleResourceContent implements ResponseHandler<JSONObject> {
    private Object identifier;

    public HandleResourceContent(Object identifier) {
      this.identifier = identifier;
    }

    @Override
    public void onResponse(JSONObject obj) {
      // Add our own internal method name so we can find the resource data later
      obj.put("method", "__Internal.resourceContent");
      obj.put(DataUtils.ID_KEY, identifier);
      result.addData(obj);
    }
  }
  
  /**
   * Ends the test after a specified amount of time.
   * This prevents a few bad tests from blocking the remaining tests.
   */
  private class Watchdog extends Thread {
    @Override
    public void run() {
      try {
        Thread.sleep(testCompleteTimeout);
      } catch (InterruptedException e) {
        // If the test completes normally, this happens.
        return;
      }
      if (result != null && !result.isCompleted()) {
        System.err.println(String.format("Test Failed After %dms", testCompleteTimeout));
        tab.runInDispatchThread(
          new Runnable() {
            @Override
            public void run() {
              result.setFailed();
              endTest();
            }
          }, new SyncCallback() {
            @Override
            public void callbackDone(RuntimeException arg0) {
            }
          }
        );
      }
    }
  }
}
