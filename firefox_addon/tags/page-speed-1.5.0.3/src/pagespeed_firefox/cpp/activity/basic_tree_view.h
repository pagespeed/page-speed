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
// BasicTreeView implements nsITreeView and provides a default
// implementation for many of the nsITreeView methods, so basic
// nsITreeView implementations don't have to re-implement all of these
// methods. To determine the contents of a given cell, BasicTreeView
// delegates to a BasicTreeViewDelegateInterface instance.

#ifndef BASIC_TREE_VIEW_H_
#define BASIC_TREE_VIEW_H_

#include "base/basictypes.h"
#include "base/scoped_ptr.h"

#include "nsCOMPtr.h"
#include "nsISupports.h"
#include "nsITreeView.h"

namespace activity {

class BasicTreeViewDelegateInterface;

// See comment at top of file for a complete description
class BasicTreeView : public nsITreeView {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITREEVIEW

  /**
   * Construct a BasicTreeView that delegates to the specified
   * BasicTreeViewDelegateInterface, and optionally keeps a reference
   * to the given nsISupports instance. Ownership of the
   * BasicTreeViewDelegateInterface instance is transferred to this
   * BasicTreeView, which will delete it in its
   * destructor. optional_ref is useful for guaranteeing that a given
   * nsISupports-derived instance will not get deleted during the
   * lifetime of this object. optional_ref can be NULL.
   */
  BasicTreeView(
      BasicTreeViewDelegateInterface *delegate, nsISupports *optional_ref);

 private:
  ~BasicTreeView();

  scoped_ptr<BasicTreeViewDelegateInterface> delegate_;
  const nsCOMPtr<nsISupports> optional_ref_;
  nsCOMPtr<nsISupports> tree_box_object_;

  DISALLOW_COPY_AND_ASSIGN(BasicTreeView);
};

}  // namespace activity

#endif  // BASIC_TREE_VIEW_H_
