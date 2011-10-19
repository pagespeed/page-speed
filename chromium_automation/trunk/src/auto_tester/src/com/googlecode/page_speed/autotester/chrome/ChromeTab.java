// Copyright 2011 Google Inc. All Rights Reserved.

package com.googlecode.page_speed.autotester.chrome;

import org.chromium.sdk.internal.websocket.WsConnection;
import org.json.simple.JSONObject;
import org.json.simple.JSONValue;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.URI;
import java.util.HashMap;
import java.util.Map;

/**
 * Represents a debuggable Chrome tab that we will connect to to perform tests.
 *
 * @author mdsteele@google.com (Matthew D. Steele)
 */
public class ChromeTab {

  private final URI uri;

  public ChromeTab(URI uri) {
    if (uri == null) {
      throw new IllegalArgumentException("uri must be non-null");
    }
    this.uri = uri;
  }

  /**
   * @return The URI used to connect to this Chrome tab.
   */
  public URI getURI() {
    return this.uri;
  }

  /**
   * Open a debugging connection to the Chrome tab.
   * @param timeoutMillis the maximum time to block, in milliseconds, before giving up
   * @return an open debugging connection to the Chrome tab.
   * @throws IOException if the connection cannot be opened within the timeout.
   */
  public TabConnection connect(int timeoutMillis) throws IOException {
    InetSocketAddress addr = new InetSocketAddress(this.uri.getHost(), this.uri.getPort());
    WsConnection conn = WsConnection.connect(addr, timeoutMillis, this.uri.getPath(),
                                             "none", null);
    return new ConnImpl(conn);
  }

  @Override
  public String toString() {
    return this.uri.toString();
  }

  private static final class ConnImpl extends TabConnection implements WsConnection.Listener {

    private final WsConnection webSocket;
    private boolean isClosed = false;
    private NotificationListener listener = null;
    private Map<Long,AsyncCallback> callbacks = new HashMap<Long,AsyncCallback>();
    private long nextMethodId = 0L;

    private ConnImpl(WsConnection webSocket) {
      this.webSocket = webSocket;
      this.webSocket.startListening(this);
    }

    @Override
    public synchronized void setNotificationListener(NotificationListener listener) {
      this.listener = listener;
    }

    @Override
    public synchronized void asyncCall(String methodName, JSONObject params,
                                       AsyncCallback callback) {
      if (this.isClosed) {
        callback.onError("Connection is already closed.");
        return;
      }

      final Long id = Long.valueOf(this.newMethodId());
      if (callback != null) {
        this.callbacks.put(id, callback);
      }
      JSONObject obj = new JSONObject();
      obj.put("id", id);
      obj.put("method", methodName);
      obj.put("params", params);
      try {
        this.webSocket.sendTextualMessage(obj.toJSONString());
      } catch (IOException e) {
        this.callbacks.remove(id);
        callback.onError(e.getMessage());
      }
    }

    @Override
    public synchronized boolean hasOutstandingCalls() {
      return !this.callbacks.isEmpty();
    }

    @Override
    public synchronized void close() {
      if (!this.isClosed) {
        this.isClosed = true;
        this.listener = null;
        this.webSocket.close();

        for (AsyncCallback callback : this.callbacks.values()) {
          callback.onError("Connection closed during call.");
        }
        this.callbacks.clear();
      }
    }

    private synchronized long newMethodId() {
      return this.nextMethodId++;
    }

    @Override
    public synchronized void textMessageRecieved(String text) {
      if (this.isClosed) {
        return;
      }

      // TODO(mdsteele): Deal with all these casts in a safer way, just in case Chrome sends us
      //   back malformed data for some reason (e.g. because we're using an incompatible version).

      JSONObject obj = (JSONObject)JSONValue.parse(text);

      if (obj.containsKey("id")) {
        Long id = (Long)obj.get("id");
        AsyncCallback callback = this.callbacks.remove(id);

        String error = (String)obj.get("error");
        if (error == null) {
          callback.onSuccess((JSONObject)obj.get("result"));
        } else {
          callback.onError(error);
        }
      } else if (this.listener != null) {
        String method = (String)obj.get("method");
        JSONObject params = (JSONObject)obj.get("params");
        this.listener.handleNotification(method, params);
      }
    }

    @Override
    public synchronized void errorMessage(Exception ex) {
      // TODO(mdsteele): Decide a better way to respond to this.
      System.err.println("WebSocket protocol error");
    }

    @Override
    public synchronized void eofMessage() {
      // TODO(mdsteele): Decide a better way to respond to this.
      System.err.println("WebSocket EOF");
    }

  }

}
