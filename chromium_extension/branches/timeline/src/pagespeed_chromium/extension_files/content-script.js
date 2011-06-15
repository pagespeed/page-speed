// Copyright 2010 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

"use strict";

function collectElement(element, outList) {
  var obj = {tag: element.tagName};
  // If the tag has any attributes, add an attribute map to the output object.
  var attributes = element.attributes;
  if (attributes && attributes.length > 0) {
    obj.attrs = {};
    for (var i = 0, len = attributes.length; i < len; ++i) {
      var attribute = attributes[i];
      obj.attrs[attribute.name] = attribute.value;
    }
  }
  // If the tag has any attributes, add children list to the output object.
  var children = element.children;
  if (children && children.length > 0) {
    for (var j = 0, len = children.length; j < len; ++j) {
      collectElement(children[j], outList);
    }
  }
  // If the tag has a content document, add that to the output object.
  if (element.contentDocument) {
    obj.contentDocument = collectDocument(element.contentDocument);
  }
  // If this is an IMG tag, record the size to which the image is scaled.
  if (element.tagName === 'IMG' && element.complete) {
    obj.width = element.width;
    obj.height = element.height;
  }
  outList.push(obj);
}

function collectDocument(document) {
  var elements = [];
  collectElement(document.documentElement, elements);
  return {
    documentUrl: document.URL,
    baseUrl: document.baseURI,
    elements: elements
  };
}

// Collect the page DOM and send it back to the background page.
try {
  chrome.extension.sendRequest({kind: 'dom', dom: collectDocument(document)});
} catch (e) {
  var message = 'Error in Page Speed content script:\n ' + e.stack;
  alert(message + '\n\nPlease file a bug at\n' +
        'http://code.google.com/p/page-speed/issues/');
  chrome.extension.sendRequest({kind: 'error', message: message});
}
