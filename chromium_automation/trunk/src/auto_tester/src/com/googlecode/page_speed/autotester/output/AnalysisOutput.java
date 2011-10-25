// Copyright 2011 Google Inc. All Rights Reserved.

package com.googlecode.page_speed.autotester.output;

import com.googlecode.page_speed.autotester.TestData;
import com.googlecode.page_speed.autotester.TestRequest;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;

import java.util.Map.Entry;

/**
 * Generates a json file of test metrics and Page Speed results.
 *
 * @author azlatin@google.com (Alexander Zlatin)
 *
 */
public class AnalysisOutput implements OutputBuilder {

  private JSONObject output;

  public AnalysisOutput() {
    output = new JSONObject();
  }

  /**
   * Adds a single test run to the output json data structure.
   *
   * @param run The test run to fetch data from.
   * @param metrics The non-pagespeed metrics map name->value
   * @param pagespeed The pagespeed unformatted json output.
   */
  @SuppressWarnings("unchecked")
  @Override
  public void addTestResult(TestData testData, JSONObject pagespeed) {
    final TestRequest run = testData.getTestRequest();
    final JSONObject metrics = testData.getMetrics();

    JSONObject rootObj = output;

    if (!rootObj.containsKey(run.url)) {
      rootObj.put(run.url, new JSONObject());
    }
    JSONObject varObj = (JSONObject) rootObj.get(run.url);

    if (!varObj.containsKey(run.varName)) {
      varObj.put(run.varName, new JSONArray());
    }
    JSONArray viewArr = (JSONArray) varObj.get(run.varName);

    if (viewArr.size() < 2) {
      viewArr.add(new JSONObject());
      viewArr.add(new JSONObject());
    }
    JSONObject metricsObj = (JSONObject) viewArr.get(run.first);

    for (Object metricObj : metrics.entrySet()) {
      Entry<Object, Object> metric = (Entry<Object, Object>) metricObj;
      addRunValue(metricsObj, metric.getKey(), metric.getValue());
    }

    addRunValue(metricsObj, "PS_Score", pagespeed.get("score"));

    JSONArray psRules = (JSONArray) pagespeed.get("rule_results");

    for (Object ruleObj : psRules) {
      JSONObject rule = (JSONObject) ruleObj;
      String name = (String) rule.get("rule_name");
      addRunValue(metricsObj, "PSI_" + name, rule.get("rule_impact"));
      addRunValue(metricsObj, "PSS_" + name, rule.get("rule_score"));
    }
  }

  /**
   * Adds a single run value to the array of values for a metric.
   *
   * @param parentObj Parent object that holds metrics.
   * @param key The metric name
   * @param value The metric value.
   */
  @SuppressWarnings("unchecked")
  private void addRunValue(JSONObject parentObj, Object key, Object value) {
    if (!parentObj.containsKey(key)) {
      parentObj.put(key, new JSONArray());
    }
    JSONArray runValueArr = null;
    runValueArr = (JSONArray) parentObj.get(key);
    runValueArr.add(value);
  }

  @Override
  public String getResultExtension() {
    return "json";
  }

  @Override
  public String getResult() {
    return output.toJSONString();
  }

}
