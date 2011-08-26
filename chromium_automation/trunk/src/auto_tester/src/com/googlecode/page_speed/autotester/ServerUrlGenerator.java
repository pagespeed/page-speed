// Copyright 2011 Google Inc. All Rights Reserved.

package com.googlecode.page_speed.autotester;

import com.googlecode.page_speed.autotester.util.FileUtils;

import org.json.simple.JSONArray;
import org.json.simple.JSONValue;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.InetSocketAddress;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;

/**
 * Generates a list of servers/tabs.
 * 
 * @author azlatin@google.com (Alexander Zlatin)
 *
 */
public class ServerUrlGenerator {
  private List<InetSocketAddress> servers;
  
  /**
   * Create a new configuration for the test runner.
   * @param serversFilename The filename of the server list.
   */
  public ServerUrlGenerator(String serversFilename) {
    servers = new ArrayList<InetSocketAddress>();
    for (String server : FileUtils.getLines(new File(serversFilename))) {
      String[] pair = server.split(":");
      if (pair.length == 2) {
        servers.add(new InetSocketAddress(pair[0], Integer.parseInt(pair[1])));
      } else {
        System.err.println(String.format(
          "Ignoring server %s because no port was specified.", server));
      }
    }
  }
  
  /**
   * Get the list of server addresses.
   * @return a List of server InetSocketAddress objects.
   */
  public List<InetSocketAddress> getServers() {
    return servers;
  }
  
  /**
   * Gets a list of debuggable tabs in a Chrome instance.
   * 
   * @param server The InetSocketAddress of the debuggable Chrome instance.
   * @return A JSONArray of JSONObjects describing the tabs.
   */
  public JSONArray getTabList(InetSocketAddress server) {
    URL json = null;
    try {
      json = new URL("http", server.getHostName(), server.getPort(), "/json");
    } catch (MalformedURLException e) {
      e.printStackTrace();
    }

    JSONArray list = new JSONArray();
    try {
      BufferedReader in =
          new BufferedReader(new InputStreamReader(json.openStream()));
      list = (JSONArray) JSONValue.parse(in);
      in.close();
    } catch (IOException e) {
      System.err.println(String.format("Could not connect to %s.", json));
    }
    return list;
  }
}
