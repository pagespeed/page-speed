// Copyright 2010 Google Inc. All Rights Reserved.
// Author: aoates@google.com (Andrew Oates)

#ifndef PAGESPEED_L10N_L10N_H_
#define PAGESPEED_L10N_L10N_H_

#include "pagespeed/l10n/localizable_string.h"

#if defined(_) || defined(not_localized)
#error localization macros are already defined.
#endif

// Macro for marking strings that need to be localized.  Any string
// that should be localized should be surrounded by _(...).
#define _(X) (::pagespeed::LocalizableString(X))

// Macro for marking user-facing strings that should *not* be localized.
#define not_localized(X) (::pagespeed::LocalizableString(X))

#endif  // PAGESPEED_L10N_L10N_H_
