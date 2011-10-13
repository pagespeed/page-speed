package com.googlecode.page_speed.autotester;

import com.googlecode.page_speed.autotester.output.OutputGenerator;
import com.googlecode.page_speed.autotester.util.Flags;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;

import java.net.InetSocketAddress;

// Copyright 2011 Google Inc. All Rights Reserved.


/**
 * Runs tests and then converts the data to a format suitable for analysis.
 *
 * @author azlatin@google.com (Alexander Zlatin)
 */
public final class PageSpeedChromeRunner {

  private PageSpeedChromeRunner() {
    throw new AssertionError();  // uninstantiable class
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

    // Usage
    Flags.init(args);
    if (!Flags.isValid()) {
      System.out.println("Flags:");
      Flags.printHelp();
      return;
    }

    // Tests
    TestRequestGenerator tests =
        new TestRequestGenerator(Flags.getStr("urls"),
                                 Flags.getStr("variations"),
                                 Flags.getBool("repeat"),
                                 Flags.getInt("runs"));

    // Page Speed
    PageSpeedRunner psr = new PageSpeedRunner(Flags.getStr("ps_bin"),
                                              Flags.getStr("strategy"));

    // Test Runner
    ServerUrlGenerator servers =
        new ServerUrlGenerator(Flags.getStr("servers"));
    URLTester tester = new URLTester(tests.getTestIterable());
    for (InetSocketAddress server : servers.getServers()) {
      JSONArray tabs = servers.getTabList(server);
      if (tabs.size() == 1) {
        tester.addTabConnection((JSONObject) tabs.get(0),
          10000, (long) Flags.getInt("test_timeout") * 1000);
      } else if (tabs.size() > 1) {
        System.err.println(
          String.format("Not using %s because it has more than one tab.",
                        server));
      }
    }

    // Result Handler
    OutputGenerator results = new OutputGenerator(psr,
      Flags.getStr("results"), tests.getTotalTests());
    results.addOutputsByString(Flags.getStr("output"));
    results.setSavedContentByString(Flags.getStr("save"));

    // Start the tests
    tester.addObserver(results);
    tester.start();
  }

}
