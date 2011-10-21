// Copyright 2011 Google Inc. All Rights Reserved.

package com.googlecode.page_speed.autotester.chrome;

import com.googlecode.page_speed.autotester.util.Json;
import com.googlecode.page_speed.autotester.util.JsonException;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.json.simple.JSONValue;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;

/**
 * Represents a Chrome browser instance that we will connect to to perform
 * tests.
 *
 * @author mdsteele@google.com (Matthew D. Steele)
 */
public class ChromeInstance {

  private final InetSocketAddress address;

  /**
   * Create a reference to a Chrome instance at the specified address.
   * @param address the host/port of the Chrome instance
   * @throws IllegalArgumentException if the address is null or unresolved.
   */
  public ChromeInstance(InetSocketAddress address) {
    if (address == null) {
      throw new IllegalArgumentException("address must be non-null");
    } else if (address.isUnresolved()) {
      throw new IllegalArgumentException("could not resolve address: " +
                                         address);
    }
    this.address = address;
  }

  /**
   * Create a reference to a Chrome instance at the specified address.
   * @param address the host/port of the Chrome instance
   * @throws IllegalArgumentException if the address is malformed or cannot be
   *   resolved.
   */
  public ChromeInstance(String address) {
    this(parseAddress(address));
  }

  /**
   * Helper method for the ChromeInstance(String) constructor, to get around
   * the restriction that the call to this() must be the first statement.
   */
  private static InetSocketAddress parseAddress(String address) {
    String[] parts = address.split(":");
    if (parts.length < 2) {
      throw new IllegalArgumentException("address is missing port number: " +
                                         address);
    } else if (parts.length > 2) {
      throw new IllegalArgumentException("malformed address: " + address);
    }
    return new InetSocketAddress(parts[0], Integer.parseInt(parts[1]));
  }

  /**
   * Return a list of all debuggable tabs available in this Chrome instance.
   *
   * @return A list of ChromeTab objects representing all debuggable tabs.
   * @throws IOException if we are unabled to connect to the Chrome browser.
   */
  public List<ChromeTab> getTabList() throws IOException {
    URL url = new URL("http", address.getHostName(), address.getPort(),
                      "/json");

    try {
      BufferedReader in = new BufferedReader(new InputStreamReader(url.openStream()));
      JSONArray list = Json.parseArray(in);
      in.close();

      List<ChromeTab> tabs = new ArrayList<ChromeTab>();
      for (JSONObject tabInfo : Json.iterateObjects(list)) {
        if (!tabInfo.containsKey("webSocketDebuggerUrl")) {
          // This tab is not debuggable, possibly because another client is
          // already debugging it.
          continue;
        }

        String tabUrl = Json.getString(tabInfo, "webSocketDebuggerUrl");
        try {
          tabs.add(new ChromeTab(new URI(tabUrl)));
        } catch (URISyntaxException e) {
          throw new IOException("Chrome returned an invalid tab URL: " + tabUrl);
        }
      }
      return tabs;
    } catch (JsonException e) {
      throw new IOException("Bad JSON from Chrome: " + e.getMessage());
    }
  }

  @Override
  public String toString() {
    return this.address.toString();
  }

}
