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

#include "basic_tree_view.h"

#include <string>

#include "basic_tree_view_delegate_interface.h"

#include "nsITreeColumns.h"
#include "nsStringAPI.h"

NS_IMPL_ISUPPORTS1(activity::BasicTreeView, nsITreeView)

namespace activity {

BasicTreeView::BasicTreeView(
    BasicTreeViewDelegateInterface *delegate, nsISupports *optional_ref)
    : delegate_(delegate),
      optional_ref_(optional_ref) {
}

BasicTreeView::~BasicTreeView() {
}

NS_IMETHODIMP BasicTreeView::GetRowCount(PRInt32 *row_count) {
  *row_count = delegate_->GetRowCount();
  return NS_OK;
}

NS_IMETHODIMP BasicTreeView::GetCellText(
    PRInt32 row, nsITreeColumn *col, nsAString &retval) {
  PRInt32 column_index = -1;
  nsresult rv = col->GetIndex(&column_index);
  if (NS_FAILED(rv)) {
    return rv;
  }

  std::string cell_text;
  if (!delegate_->GetCellText(row, column_index, &cell_text)) {
    return NS_ERROR_FAILURE;
  }

  NS_ConvertASCIItoUTF16 cell_text_utf16(cell_text.c_str());
  retval = cell_text_utf16;
  return NS_OK;
}

NS_IMETHODIMP BasicTreeView::GetRowProperties(
    PRInt32 index, nsISupportsArray *properties) {
  return NS_OK;
}

NS_IMETHODIMP BasicTreeView::GetCellProperties(
    PRInt32 row, nsITreeColumn *col, nsISupportsArray *properties) {
  return NS_OK;
}

NS_IMETHODIMP BasicTreeView::GetColumnProperties(
    nsITreeColumn *col, nsISupportsArray *properties) {
  return NS_OK;
}

NS_IMETHODIMP BasicTreeView::IsContainer(
    PRInt32 index, PRBool *retval) {
  *retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP BasicTreeView::IsSeparator(
    PRInt32 index, PRBool *retval) {
  *retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP BasicTreeView::IsSorted(
    PRBool *retval) {
  *retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP BasicTreeView::GetLevel(
    PRInt32 index, PRInt32 *retval) {
  // BasicTreeView isn't really a "tree" view at all; it's a flat
  // list. Thus, all rows are at level 0.
  *retval = 0;
  return NS_OK;
}

NS_IMETHODIMP BasicTreeView::GetImageSrc(
    PRInt32 row, nsITreeColumn *col, nsAString &retval) {
  retval = static_cast<const PRUnichar*>(NULL);
  return NS_OK;
}

NS_IMETHODIMP BasicTreeView::SetTree(
    nsITreeBoxObject *tree) {
  tree_box_object_ = tree;
  return NS_OK;
}

NS_IMETHODIMP BasicTreeView::GetSelection(
    nsITreeSelection **selection) {
  *selection = NULL;
  return NS_OK;
}

NS_IMETHODIMP BasicTreeView::SetSelection(
    nsITreeSelection *selection) {
  return NS_OK;
}

NS_IMETHODIMP BasicTreeView::IsContainerOpen(
    PRInt32 index, PRBool *retval) {
  *retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP BasicTreeView::IsContainerEmpty(
    PRInt32 index, PRBool *retval) {
  *retval = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP BasicTreeView::CanDrop(
    PRInt32 index,
    PRInt32 orientation,
    nsIDOMDataTransfer *dataTransfer,
    PRBool *retval) {
  *retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP BasicTreeView::Drop(
    PRInt32 row, PRInt32 orientation, nsIDOMDataTransfer *dataTransfer) {
  return NS_OK;
}

NS_IMETHODIMP BasicTreeView::GetParentIndex(
    PRInt32 row_index, PRInt32 *retval) {
  // BasicTreeView isn't really a "tree" view at all; it's a flat
  // list. Thus, none of the rows have parents. Per the comment in
  // nsITreeView.idl, when there is no parent index, GetParentIndex()
  // returns -1.
  *retval = -1;
  return NS_OK;
}

NS_IMETHODIMP BasicTreeView::HasNextSibling(
    PRInt32 row_index, PRInt32 after_index, PRBool *retval) {
  const PRInt32 last_row_index = delegate_->GetRowCount() - 1;
  if (last_row_index < 0) {
    // No elements.
    *retval = PR_FALSE;
    return NS_OK;
  }

  if (row_index >= last_row_index || after_index >= last_row_index) {
    // If the parameters are at or past the last index, then there is
    // not a next sibling.
    *retval = PR_FALSE;
    return NS_OK;
  }

  // BasicTreeView isn't really a "tree" view at all; it's a flat
  // list. Thus, as long as after_index is before last_row_index,
  // there is a next sibling.
  *retval = (after_index < last_row_index) ? PR_TRUE : PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP BasicTreeView::GetProgressMode(
    PRInt32 row, nsITreeColumn *col, PRInt32 *retval) {
  // We don't support columns of type 'progressmeter', so we do not
  // implement this method.
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP BasicTreeView::GetCellValue(
    PRInt32 row, nsITreeColumn *col, nsAString &retval) {
  // We don't support columns other than type 'text', so we do not
  // implement this method.
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP BasicTreeView::ToggleOpenState(PRInt32 index) {
  // We don't support rows for which IsContainer returns true, so we
  // do not implement this method.
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP BasicTreeView::CycleHeader(nsITreeColumn *col) {
  // This is where we'd implement sortable columns (called whenever a
  // row heading is clicked).
  return NS_OK;
}

NS_IMETHODIMP BasicTreeView::SelectionChanged() {
  return NS_OK;
}

NS_IMETHODIMP BasicTreeView::CycleCell(PRInt32 row, nsITreeColumn *col) {
  return NS_OK;
}

NS_IMETHODIMP BasicTreeView::IsEditable(
    PRInt32 row, nsITreeColumn *col, PRBool *retval) {
  *retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP BasicTreeView::IsSelectable(
    PRInt32 row, nsITreeColumn *col, PRBool *retval) {
  *retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP BasicTreeView::SetCellValue(
    PRInt32 row, nsITreeColumn *col, const nsAString &value) {
  // We don't support columns other than type 'text', so we do not
  // implement this method.
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP BasicTreeView::SetCellText(
    PRInt32 row, nsITreeColumn *col, const nsAString &value) {
  // We don't support editable cells, so we do not implement this
  // method.
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP BasicTreeView::PerformAction(const PRUnichar *action) {
  return NS_OK;
}

NS_IMETHODIMP BasicTreeView::PerformActionOnRow(
    const PRUnichar *action, PRInt32 row) {
  return NS_OK;
}

NS_IMETHODIMP BasicTreeView::PerformActionOnCell(
    const PRUnichar *action, PRInt32 row, nsITreeColumn *col) {
  return NS_OK;
}

}  // namespace activity
