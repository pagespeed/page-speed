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
// UncalledFunctionTreeViewDelegate implements
// BasicTreeViewDelegateInterface and provides a list of functions
// that were never called.

#ifndef UNCALLED_FUNCTION_TREE_VIEW_DELEGATE_H_
#define UNCALLED_FUNCTION_TREE_VIEW_DELEGATE_H_

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "basic_tree_view_delegate_interface.h"

namespace activity {

class CallGraphProfile;
class FindFirstInvocationsVisitor;

class UncalledFunctionTreeViewDelegate : public BasicTreeViewDelegateInterface {
 public:
  explicit UncalledFunctionTreeViewDelegate(const CallGraphProfile &profile);
  virtual ~UncalledFunctionTreeViewDelegate();

  /**
   * Initialize this UncalledFunctionTreeViewDelegate based on the
   * contents of the FindFirstInvocationsVisitor.
   */
  void Initialize(const FindFirstInvocationsVisitor &visitor);

  // Implement BasicTreeViewDelegateInterface.
  virtual int32_t GetRowCount();
  virtual bool GetCellText(int32_t row, int32_t column, std::string *retval);

  // The columns in the tree view. Must be declared in the same order
  // as the XUL tree widget.
  enum ColumnId {
    INSTANTIATION_TIME = 0,
    FUNCTION_NAME,
    FUNCTION_SOURCE,
    FILE_NAME,
    LAST_COLUMN_ID = FILE_NAME
  };

 private:
  // Populate the given vector with the function tags for all uncalled
  // functions.
  void PopulateUncalledVector(const FindFirstInvocationsVisitor &visitor);

  std::vector<int32_t> uncalled_function_tags_;
  const CallGraphProfile &profile_;

  DISALLOW_COPY_AND_ASSIGN(UncalledFunctionTreeViewDelegate);
};

}  // namespace activity

#endif  // UNCALLED_FUNCTION_TREE_VIEW_DELEGATE_H_
