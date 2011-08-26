// Copyright 2011 Google Inc. All Rights Reserved.

package com.googlecode.page_speed.autotester.output;

import com.googlecode.page_speed.autotester.TestRequest;
import com.googlecode.page_speed.autotester.iface.OutputBuilder;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;

import java.text.DateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.Iterator;
import java.util.List;

/**
 * Generates a CSV of test metrics and Page Speed results.
 * 
 * @author azlatin@google.com (Alexander Zlatin)
 * 
 */
public class CSVOutput implements OutputBuilder {

  private StringBuilder output;

  public CSVOutput() {
    output = new StringBuilder();
  }

  @Override
  public void buildPart(TestRequest run, JSONObject metrics, JSONObject pagespeed) {
    List<String> line = new ArrayList<String>();
    if (output.length() == 0) {
      line = new ArrayList<String>();
      line.add("Test");
      line.add("Date Completed");
      line.add("Variation");
      line.add("Full URL");
      line.add("View");
      line.add("Run");
      for (Object name : metrics.keySet()) {
        line.add((String) name);
      }
      line.add("PS_Score");
      JSONArray psRules = (JSONArray) pagespeed.get("rule_results");
      for (Object ruleObj : psRules) {
        JSONObject rule = (JSONObject) ruleObj;
        String name = (String) rule.get("rule_name");
        line.add("PSI_" + name);
        line.add("PSS_" + name);
      }

      output.append("\"" + join(line, "\",\"") + "\"\n");
      line.clear();
    }

    line.add(run.url);
    line.add(DateFormat.getInstance().format(new Date()));
    line.add(run.varName);
    line.add(run.getFullURL());
    line.add(String.valueOf(run.first));
    line.add(String.valueOf(run.run));

    for (Object name : metrics.keySet()) {
      line.add(String.valueOf(metrics.get(name)));
    }
    line.add(String.valueOf(pagespeed.get("score")));
    JSONArray psRules = (JSONArray) pagespeed.get("rule_results");
    for (Object ruleObj : psRules) {
      JSONObject rule = (JSONObject) ruleObj;
      line.add(String.valueOf(rule.get("rule_impact")));
      line.add(String.valueOf(rule.get("rule_score")));
    }

    output.append("\"" + join(line, "\",\"") + "\"\n");
  }

  @Override
  public String getResultExtension() {
    return "csv";
  }

  @Override
  public String getResult() {
    return output.toString();
  }

  /**
   * Joins a list into a string. Opposite of split(). Also escapes " with \ in
   * all values.
   * 
   * @param s List of elements.
   * @param delimiter The delimiter to join on.
   * @return A delimiter separated string of s.
   */
  public static String join(List<String> s, String delimiter) {
    if (s == null || s.isEmpty()) {
      return "";
    }
    Iterator<String> iter = s.iterator();
    StringBuilder builder = new StringBuilder(iter.next());
    while (iter.hasNext()) {
      builder.append(delimiter).append(
          iter.next().replace("\\", "\\\\").replace("\"", "\\\""));
    }
    return builder.toString();
  }

}
