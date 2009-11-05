/**
 * Copyright 2008-2009 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Author: Bryan McQuade
//
// BasicTreeViewDelegateInterface knows how to generate the contents
// of a cell, specified by row and column.

#ifndef BASIC_TREE_VIEW_DELEGATE_INTERFACE_H_
#define BASIC_TREE_VIEW_DELEGATE_INTERFACE_H_

#include <stdint.h>

#include <string>

#include "base/basictypes.h"

namespace activity {

// See comment at top of file for a complete description
class BasicTreeViewDelegateInterface {
 public:
  BasicTreeViewDelegateInterface() {}
  virtual ~BasicTreeViewDelegateInterface() {}

  /**
   * Get the number of rows to render.
   */
  virtual int32_t GetRowCount() = 0;

  /**
   * Get the contents of the given row and given column. Rows and
   * columns are zero-indexed. Row indices start at the top; column
   * indices start at the left.
   * @return true if success, false if failure.
   */
  virtual bool GetCellText(int32_t row, int32_t column, std::string *out) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(BasicTreeViewDelegateInterface);
};

}  // namespace activity

#endif  // BASIC_TREE_VIEW_DELEGATE_INTERFACE_H_
