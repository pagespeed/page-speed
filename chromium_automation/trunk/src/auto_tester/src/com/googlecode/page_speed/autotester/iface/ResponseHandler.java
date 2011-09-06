package com.googlecode.page_speed.autotester.iface;
// Copyright 2011 Google Inc. All Rights Reserved.


/**
 * Interface for a response handler.
 *
 * @author azlatin@google.com (Alexander Zlatin)
 *
 * @param <DT> The datatype of the object passed to the handler.
 */
public interface ResponseHandler<DT> {
  public void onResponse(DT obj);
}
