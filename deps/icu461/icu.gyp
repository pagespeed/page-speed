# Copyright 2010 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Based on Chromium's icu.gyp.

{
  'variables': {
    'use_system_icu%': 0,
  },
  'target_defaults': {
    'direct_dependent_settings': {
      'defines': [
        # Tell ICU to not insert |using namespace icu;| into its headers,
        # so that chrome's source explicitly has to use |icu::|.
        'U_USING_ICU_NAMESPACE=0',
      ],
    },
  },
  'conditions': [
    ['use_system_icu==0', {
      'target_defaults': {
        'defines': [
          'U_USING_ICU_NAMESPACE=0',
        ],
        'conditions': [
          ['component=="static_library"', {
            'defines': [
              'U_STATIC_IMPLEMENTATION',
            ],
          }],
        ],
        'include_dirs': [
          'src/common',
          'src/i18n',
        ],
        'msvs_disabled_warnings': [4005, 4355, 4996],
      },
      'targets': [
        {
          'target_name': 'icudata',
          'type': '<(library)',
          'sources': [
             # These are hand-generated, but will do for now.  The linux
             # version is an identical copy of the (mac) icudt46l_dat.S file,
             # modulo removal of the .private_extern and .const directives and
             # with no leading underscore on the icudt46_dat symbol.
             'gen/arch/linux/common/icudata/icudt46l_dat.S',
             'gen/arch/mac/common/icudata/icudt46l_dat.s',
          ],
          'conditions': [
            [ 'OS == "win"', {
              'link_settings': {
                'libraries': [
                  '<(DEPTH)/third_party/icu/gen/arch/win/ia32/icudata/icudt46.lib',
                ],
              },
              'sources': [
                # In order to pass our icudt46.lib dependency on to dependent
                # targets, we need to have at least one source file.
                'gen/arch/win/ia32/icudata/empty.c',
              ],
            }],
            [ 'OS == "win" or OS == "mac"', {
              'sources!': ['gen/arch/linux/common/icudata/icudt46l_dat.S'],
            }],
            [ 'OS != "mac"', {
              'sources!': ['gen/arch/mac/common/icudata/icudt46l_dat.s'],
            }],
            [ 'library != "shared_library"', {
              'defines': [
                'U_HIDE_DATA_SYMBOL',
              ],
            }],
          ],
        },
        {
          'target_name': 'icuuc',
          'type': '<(component)',
          'sources': [
            'src/common/bmpset.cpp',
            'src/common/brkeng.cpp',
            'src/common/brkiter.cpp',
            'src/common/bytestream.cpp',
            'src/common/caniter.cpp',
            'src/common/chariter.cpp',
            'src/common/charstr.cpp',
            'src/common/cmemory.c',
            'src/common/cstring.c',
            'src/common/cwchar.c',
            'src/common/dictbe.cpp',
            'src/common/dtintrv.cpp',
            'src/common/errorcode.cpp',
            'src/common/filterednormalizer2.cpp',
            'src/common/icudataver.c',
            'src/common/icuplug.c',
            'src/common/locavailable.cpp',
            'src/common/locbased.cpp',
            'src/common/locdispnames.cpp',
            'src/common/locid.cpp',
            'src/common/loclikely.cpp',
            'src/common/locmap.c',
            'src/common/locresdata.cpp',
            'src/common/locutil.cpp',
            'src/common/mutex.cpp',
            'src/common/normalizer2.cpp',
            'src/common/normalizer2impl.cpp',
            'src/common/normlzr.cpp',
            'src/common/parsepos.cpp',
            'src/common/propname.cpp',
            'src/common/propsvec.c',
            'src/common/punycode.c',
            'src/common/putil.c',
            'src/common/rbbi.cpp',
            'src/common/rbbidata.cpp',
            'src/common/rbbinode.cpp',
            'src/common/rbbirb.cpp',
            'src/common/rbbiscan.cpp',
            'src/common/rbbisetb.cpp',
            'src/common/rbbistbl.cpp',
            'src/common/rbbitblb.cpp',
            'src/common/resbund.cpp',
            'src/common/resbund_cnv.cpp',
            'src/common/ruleiter.cpp',
            'src/common/schriter.cpp',
            'src/common/serv.cpp',
            'src/common/servlk.cpp',
            'src/common/servlkf.cpp',
            'src/common/servls.cpp',
            'src/common/servnotf.cpp',
            'src/common/servrbf.cpp',
            'src/common/servslkf.cpp',
            'src/common/stringpiece.cpp',
            'src/common/triedict.cpp',
            'src/common/uarrsort.c',
            'src/common/ubidi.c',
            'src/common/ubidi_props.c',
            'src/common/ubidiln.c',
            'src/common/ubidiwrt.c',
            'src/common/ubrk.cpp',
            'src/common/ucase.c',
            'src/common/ucasemap.c',
            'src/common/ucat.c',
            'src/common/uchar.c',
            'src/common/uchriter.cpp',
            'src/common/ucln_cmn.c',
            'src/common/ucmndata.c',
            'src/common/ucnv.c',
            'src/common/ucnv2022.c',
            'src/common/ucnv_bld.c',
            'src/common/ucnv_cb.c',
            'src/common/ucnv_cnv.c',
            'src/common/ucnv_err.c',
            'src/common/ucnv_ext.c',
            'src/common/ucnv_io.c',
            'src/common/ucnv_lmb.c',
            'src/common/ucnv_set.c',
            'src/common/ucnv_u16.c',
            'src/common/ucnv_u32.c',
            'src/common/ucnv_u7.c',
            'src/common/ucnv_u8.c',
            'src/common/ucnvbocu.c',
            'src/common/ucnvdisp.c',
            'src/common/ucnvhz.c',
            'src/common/ucnvisci.c',
            'src/common/ucnvlat1.c',
            'src/common/ucnvmbcs.c',
            'src/common/ucnvscsu.c',
            'src/common/ucnvsel.cpp',
            'src/common/ucol_swp.cpp',
            'src/common/udata.cpp',
            'src/common/udatamem.c',
            'src/common/udataswp.c',
            'src/common/uenum.c',
            'src/common/uhash.c',
            'src/common/uhash_us.cpp',
            'src/common/uidna.cpp',
            'src/common/uinit.c',
            'src/common/uinvchar.c',
            'src/common/uiter.cpp',
            'src/common/ulist.c',
            'src/common/uloc.c',
            'src/common/uloc_tag.c',
            'src/common/umapfile.c',
            'src/common/umath.c',
            'src/common/umutex.c',
            'src/common/unames.c',
            'src/common/unifilt.cpp',
            'src/common/unifunct.cpp',
            'src/common/uniset.cpp',
            'src/common/uniset_props.cpp',
            'src/common/unisetspan.cpp',
            'src/common/unistr.cpp',
            'src/common/unistr_case.cpp',
            'src/common/unistr_cnv.cpp',
            'src/common/unistr_props.cpp',
            'src/common/unorm.cpp',
            'src/common/unorm_it.c',
            'src/common/unormcmp.cpp',
            'src/common/uobject.cpp',
            'src/common/uprops.cpp',
            'src/common/ures_cnv.c',
            'src/common/uresbund.c',
            'src/common/uresdata.c',
            'src/common/usc_impl.c',
            'src/common/uscript.c',
            'src/common/uset.cpp',
            'src/common/uset_props.cpp',
            'src/common/usetiter.cpp',
            'src/common/ushape.c',
            'src/common/usprep.cpp',
            'src/common/ustack.cpp',
            'src/common/ustr_cnv.c',
            'src/common/ustr_wcs.c',
            'src/common/ustrcase.c',
            'src/common/ustrenum.cpp',
            'src/common/ustrfmt.c',
            'src/common/ustring.c',
            'src/common/ustrtrns.c',
            'src/common/utext.cpp',
            'src/common/utf_impl.c',
            'src/common/util.cpp',
            'src/common/util_props.cpp',
            'src/common/utrace.c',
            'src/common/utrie.c',
            'src/common/utrie2.cpp',
            'src/common/utrie2_builder.c',
            'src/common/uts46.cpp',
            'src/common/utypes.c',
            'src/common/uvector.cpp',
            'src/common/uvectr32.cpp',
            'src/common/uvectr64.cpp',
            'src/common/wintz.c',
          ],
          'defines': [
            'U_COMMON_IMPLEMENTATION',
          ],
          'dependencies': [
            'icudata',
          ],
          'direct_dependent_settings': {
            'include_dirs': [
              'src/common',
            ],
            'conditions': [
              [ 'component=="static_library"', {
                'defines': [
                  'U_STATIC_IMPLEMENTATION',
                ],
              }],
            ],
          },
          'conditions': [
            [ 'os_posix == 1 and OS != "mac"', {
              'cflags': [
                # Since ICU wants to internally use its own deprecated APIs,
                # don't complain about it.
                '-Wno-deprecated-declarations',
                '-Wno-unused-function',
              ],
              'cflags_cc': [
                '-frtti',
              ],
            }],
            ['OS == "mac"', {
              'xcode_settings': {
                'GCC_ENABLE_CPP_RTTI': 'YES',       # -frtti
              }
            }],
            ['OS == "win"', {
              'msvs_settings': {
                'VCCLCompilerTool': {
                  'RuntimeTypeInfo': 'true',
                },
              }
            }],
          ],
        },
      ],
    }, { # use_system_icu != 0
      'targets': [
        {
          'target_name': 'system_icu',
          'type': 'settings',
          'direct_dependent_settings': {
            'defines': [
              'USE_SYSTEM_ICU',
            ],
            'cflags+': [
              '<!@(icu-config --cppflags-searchpath)'
            ],
          },
          'link_settings': {
            'ldflags': [
              '<!@(icu-config --ldflags)',
            ],
            'libraries': [
              '<!@(icu-config --ldflags-libsonly)',
            ],
          },
        },
        {
          'target_name': 'icudata',
          'type': 'none',
          'dependencies': ['system_icu'],
          'export_dependent_settings': ['system_icu'],
        },
        {
          'target_name': 'icuuc',
          'type': 'none',
          'dependencies': ['system_icu'],
          'export_dependent_settings': ['system_icu'],
        },
      ],
    }],
  ],
}
