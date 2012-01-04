/* ***** BEGIN LICENSE BLOCK *****
* Version: MPL 1.1/GPL 2.0/LGPL 2.1
* This code was based on the npsimple.c sample code in Gecko-sdk.
*
* The contents of this file are subject to the Mozilla Public License Version
* 1.1 (the "License"); you may not use this file except in compliance with
* the License. You may obtain a copy of the License at
* http://www.mozilla.org/MPL/
*
* Software distributed under the License is distributed on an "AS IS" basis,
* WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
* for the specific language governing rights and limitations under the
* License.
*
* Contributor(s):
*
* Alternatively, the contents of this file may be used under the terms of
* either the GNU General Public License Version 2 or later (the "GPL"), or
* the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
* in which case the provisions of the GPL or the LGPL are applicable instead
* of those above. If you wish to allow use of your version of this file only
* under the terms of either the GPL or the LGPL, and not to allow others to
* use your version of this file under the terms of the NPL, indicate your
* decision by deleting the provisions above and replace them with the notice
* and other provisions required by the GPL or the LGPL. If you do not delete
* the provisions above, a recipient may use your version of this file under
* the terms of any one of the NPL, the GPL or the LGPL.
* ***** END LICENSE BLOCK ***** */

#include <stdio.h>

#ifdef _WINDOWS
#include <windows.h>
#endif

#include "third_party/npapi/npapi.h"
#include "third_party/npapi/npfunctions.h"
#include "third_party/npapi/npruntime.h"

#include "pagespeed_chromium/pagespeed_chromium.h"

namespace {

const char* kPluginName = "Page Speed Plugin";
const char* kPluginDescription = "Native component for Page Speed extension.";

struct PSPlugin {
  NPP npp;
  NPObject* npobject;
};

}  // namespace

NPError NPP_GetValue(NPP npp, NPPVariable variable, void* value) {
  switch(variable) {
    case NPPVpluginNameString:
      *static_cast<char**>(value) = const_cast<char*>(kPluginName);
      break;

    case NPPVpluginDescriptionString:
      *static_cast<char**>(value) = const_cast<char*>(kPluginDescription);
      break;

    case NPPVpluginScriptableNPObject:
      if (npp == NULL) {
        return NPERR_INVALID_INSTANCE_ERROR;
      } else {
        PSPlugin* plugin = static_cast<PSPlugin*>(npp->pdata);
        if (plugin == NULL) {
          return NPERR_GENERIC_ERROR;
        }
        if (NULL == plugin->npobject) {
          plugin->npobject = npnfuncs->createobject(npp, GetNPSimpleClass());
        }
        if (NULL != plugin->npobject) {
          npnfuncs->retainobject(plugin->npobject);
        }
        *static_cast<NPObject**>(value) = plugin->npobject;
      }
      break;

    case NPPVpluginNeedsXEmbed:
      *static_cast<char*>(value) = 1;
      break;

    default:
      return NPERR_GENERIC_ERROR;
  }
  return NPERR_NO_ERROR;
}

NPError NPP_New(NPMIMEType pluginType, NPP npp,
                uint16_t mode, int16_t argc, char* argn[],
                char* argv[], NPSavedData* saved) {
  if (npp == NULL) {
    return NPERR_INVALID_INSTANCE_ERROR;
  }

#ifdef _WINDOWS
  int bWindowed = 1;
#else
  int bWindowed = 0;
#endif
  npnfuncs->setvalue(npp, NPPVpluginWindowBool,
                     reinterpret_cast<void*>(bWindowed));

  PSPlugin* plugin = new PSPlugin;
  plugin->npp = npp;
  plugin->npobject = NULL;

  npp->pdata = plugin;

  return NPERR_NO_ERROR;
}

NPError NPP_Destroy(NPP npp, NPSavedData** save) {
  if (npp == NULL) {
    return NPERR_INVALID_INSTANCE_ERROR;
  }

  // free plugin
  if (NULL != npp->pdata) {
    delete static_cast<PSPlugin*>(npp->pdata);
    npp->pdata = NULL;
  }

  return NPERR_NO_ERROR;
}

NPError NPP_SetWindow(NPP npp, NPWindow* window) {
  return NPERR_NO_ERROR;
}

NPError NPP_NewStream(NPP npp, NPMIMEType type, NPStream* stream,
                      NPBool seekable, uint16_t* stype) {
  return NPERR_GENERIC_ERROR;
}

NPError NPP_DestroyStream(NPP npp, NPStream* stream, NPReason reason) {
  return NPERR_GENERIC_ERROR;
}

int16_t NPP_HandleEvent(NPP npp, void* event) {
  return 0;
}
