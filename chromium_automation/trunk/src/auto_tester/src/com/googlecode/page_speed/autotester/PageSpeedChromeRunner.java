package com.googlecode.page_speed.autotester;

import com.googlecode.page_speed.autotester.chrome.ChromeInstance;
import com.googlecode.page_speed.autotester.chrome.ChromeTab;
import com.googlecode.page_speed.autotester.chrome.TabConnection;
import com.googlecode.page_speed.autotester.output.OutputGenerator;
import com.googlecode.page_speed.autotester.util.FileUtils;
import com.googlecode.page_speed.autotester.util.Flags;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

// Copyright 2011 Google Inc. All Rights Reserved.


/**
 * Runs tests and then converts the data to a format suitable for analysis.
 *
 * @author azlatin@google.com (Alexander Zlatin)
 */
public final class PageSpeedChromeRunner {

  private static final int WEBSOCKET_CONNECTION_TIMEOUT_MILLIS = 10000;
  private static final int MAX_TEST_BATCH_RETRIES = 4;

  private PageSpeedChromeRunner() {
    throw new AssertionError();  // uninstantiable class
  }

  /**
   * Get lines of text from a config file.
   *
   * @throws ConfigException if the file can't be read.
   */
  private static List<FileUtils.Line> getConfigLines(String path)
      throws ConfigException {
    try {
      return FileUtils.getTrimmedLines(path);
    } catch (IOException e) {
      System.err.println("Error reading from " + path + ": " + e.getMessage());
      throw new ConfigException();
    }
  }

  /**
   * Parse the specified URLs file and return a list of URLs.
   *
   * @throws ConfigException if the file can't be read or is malformed.
   */
  private static List<URL> parseUrlFile(String filepath)
      throws ConfigException {
    boolean error = false;
    List<URL> urls = new ArrayList<URL>();

    for (FileUtils.Line line : getConfigLines(filepath)) {
      try {
        urls.add(new URL(line.text));
      } catch (MalformedURLException e) {
        line.printError("Invalid URL: " + line.text);
        error = true;
      }
    }

    if (error) {
      throw new ConfigException();
    }

    return urls;
  }

  /**
   * Parse the specified variations file and return a mapping from variation
   * names to variation query strings.
   *
   * @throws ConfigException if the file can't be read or is malformed.
   */
  private static Map<String,String> parseVariationsFile(String filepath)
      throws ConfigException {
    Map<String,String> variations = new HashMap<String,String>();

    if (filepath != null && !filepath.isEmpty()) {
      boolean error = false;

      for (FileUtils.Line line : getConfigLines(filepath)) {
        String[] pair = line.text.split(":", 2);
        if (pair.length == 2) {
          String name = pair[0].trim();
          if (variations.containsKey(name)) {
            line.printError("Repeated variation name: " + name);
            error = true;
          } else {
            variations.put(name, pair[1].trim());
          }
        } else {
          line.printError("Malformed variation entry: " + line.text);
          error = true;
        }
      }

      if (error) {
        throw new ConfigException();
      }
    }

    // If we weren't given a variations file (or if the file didn't specify any
    // variations at all), then use a single, default variation.
    if (variations.isEmpty()) {
      variations.put("Default", "");
    }

    return variations;
  }

  /**
   * Parse the specified servers file and return a list of Chrome instances.
   *
   * @throws ConfigException if the file can't be read or is malformed.
   */
  private static List<ChromeInstance> parseServersFile(String filepath)
      throws ConfigException {
    boolean error = false;
    List<ChromeInstance> instances = new ArrayList<ChromeInstance>();

    for (FileUtils.Line line : getConfigLines(filepath)) {
      ChromeInstance instance;
      try {
        instance = new ChromeInstance(line.text);
      } catch (IllegalArgumentException e) {
        line.printError(e.getMessage());
        error = true;
        continue;
      }
      instances.add(instance);
    }

    if (error) {
      throw new ConfigException();
    }

    return instances;
  }

  /**
   * Given a list of Chrome instances, get the list of Chrome tabs that we will
   * use.
   *
   * @throws ConfigException if any of the connections to the Chrome instances
   *   fail.
   */
  private static List<TabConnection> getTabConnections(List<ChromeInstance> servers)
      throws ConfigException {
    boolean error = false;

    List<ChromeTab> tabs = new ArrayList<ChromeTab>();
    for (ChromeInstance instance : servers) {
      List<ChromeTab> serverTabs;
      try {
        serverTabs = instance.getTabList();
      } catch (IOException e) {
        System.err.println("FATAL: could not connect to Chrome instance at " +
                           instance);
        error = true;
        continue;
      }

      if (serverTabs.size() == 1) {
        tabs.add(serverTabs.get(0));
      } else {
        System.err.println("Warning: Not using Chrome instance at " +
                           instance + " because it has more than one tab.");
      }
    }

    List<TabConnection> connections = new ArrayList<TabConnection>();
    for (ChromeTab tab : tabs) {
      try {
        connections.add(tab.connect(WEBSOCKET_CONNECTION_TIMEOUT_MILLIS));
      } catch (IOException e) {
        System.err.println("FATAL: Could not connect to Chrome tab at " + tab +
                           ": " + e.getMessage());
        error = true;
      }
    }

    if (error) {
      throw new ConfigException();
    }

    return connections;
  }

  private static PageSpeedRunner newPageSpeedRunner(String pathToPageSpeedBin, String strategyName,
                                                    String pathToLogFile) throws ConfigException {
    final PageSpeedRunner.Strategy strategy;
    if (strategyName.equalsIgnoreCase("desktop")) {
      strategy = PageSpeedRunner.Strategy.DESKTOP;
    } else if (strategyName.equalsIgnoreCase("mobile")) {
      strategy = PageSpeedRunner.Strategy.MOBILE;
    } else {
      System.err.println("Invalid strategy name: " + strategyName);
      throw new ConfigException();
    }

    try {
      return new PageSpeedRunner(pathToPageSpeedBin, strategy, pathToLogFile);
    } catch (IOException e) {
      System.err.println(e.getMessage());
      throw new ConfigException();
    } catch (InterruptedException e) {
      System.err.println("Interrupted.");
      throw new ConfigException();
    }
  }

  /**
   * @param args command line arguments.
   */
  public static void main(String[] args) {
    Runtime.getRuntime().traceMethodCalls(true);
    Flags.define("servers", "Chrome server list file", null, true);
    Flags.define("urls", "URL list file", null, true);
    Flags.define("variations", "Variation list file", null, false);
    Flags.define("results", "Directory to store test results", null, false);
    Flags.define("repeat", "Test repeat view", null, false);
    Flags.define("runs", "Runs per URL", "1", false);
    Flags.define("test_timeout", "Seconds to wait until aborting test", "120", false);
    Flags.define("output", "Output format [analysis,csv]", "csv", false);
    Flags.define("save", "What files to save [har,dom,tl]", "", false);
    Flags.define("strategy", "Scoring strategy [desktop,mobile]", "desktop", false);
    Flags.define("ps_bin", "Path to pagespeed_bin",
        "./pagespeed_bin" + (System.getProperty("os.name").startsWith("Windows") ? ".exe" : ""),
        false);
    Flags.define("ps_log", "Path to logfile to write pagespeed_bin error output", null, false);

    // Usage
    Flags.init(args);
    if (!Flags.isValid()) {
      System.out.println("Flags:");
      Flags.printHelp();
      System.exit(1);
    }

    final long testTimeoutMillis = Flags.getInt("test_timeout") * 1000L;

    final List<List<TestRequest>> testBatches;
    final List<TabConnection> connections;
    final PageSpeedRunner psr;
    try {
      // Parse the configuration data.  These methods will print error messages
      // and exit if anything is wrong with the user's input.
      List<URL> urls = parseUrlFile(Flags.getStr("urls"));
      Map<String,String> variations = parseVariationsFile(Flags.getStr("variations"));
      List<ChromeInstance> servers = parseServersFile(Flags.getStr("servers"));

      testBatches = TestRequest.generateRequests(urls, variations, Flags.getBool("repeat"),
                                                 Flags.getInt("runs"));

      // Connect to the Chrome tabs we will be using.
      connections = getTabConnections(servers);

      psr = newPageSpeedRunner(Flags.getStr("ps_bin"), Flags.getStr("strategy"),
                               Flags.getStr("ps_log"));
    } catch (ConfigException e) {
      // An error message has already been printed to the user.
      System.exit(1);
      throw new AssertionError("unreachable");
    }

    try {
      // Result Handler
      int totalNumberOfTests = 0;
      for (List<TestRequest> testBatch : testBatches) {
        totalNumberOfTests += testBatches.size();
      }
      OutputGenerator output = new OutputGenerator(
          psr, Flags.getStr("results"), totalNumberOfTests);
      output.addOutputsByString(Flags.getStr("output"));
      output.setSavedContentByString(Flags.getStr("save"));

      // Run the tests
      TestController testController = TestController.newInstance();
      testController.runTests(connections, testBatches, MAX_TEST_BATCH_RETRIES,
                              testTimeoutMillis, output);
    } catch (InterruptedException e) {
      System.err.println("Interrupted.");
      System.exit(1);
    }
  }

}
