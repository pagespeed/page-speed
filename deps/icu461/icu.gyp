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
          'source/common',
          'source/i18n',
          'genfiles/arch/<(OS)/common/include',
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
             'genfiles/arch/linux/common/icudata/icudt46l_dat.S',
             'genfiles/arch/mac/common/icudata/icudt46l_dat.s',
          ],
          'conditions': [
            [ 'OS == "win"', {
              'link_settings': {
                'libraries': [
                  '<(DEPTH)/third_party/icu/genfiles/arch/win/ia32/icudata/icudt46.lib',
                ],
              },
              'sources': [
                # In order to pass our icudt46.lib dependency on to dependent
                # targets, we need to have at least one source file.
                'genfiles/arch/win/ia32/icudata/empty.c',
              ],
            }],
            [ 'OS == "win" or OS == "mac"', {
              'sources!': ['genfiles/arch/linux/common/icudata/icudt46l_dat.S'],
            }],
            [ 'OS != "mac"', {
              'sources!': ['genfiles/arch/mac/common/icudata/icudt46l_dat.s'],
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
            'source/common/bmpset.cpp',
            'source/common/brkeng.cpp',
            'source/common/brkiter.cpp',
            'source/common/bytestream.cpp',
            'source/common/caniter.cpp',
            'source/common/chariter.cpp',
            'source/common/charstr.cpp',
            'source/common/cmemory.c',
            'source/common/cstring.c',
            'source/common/cwchar.c',
            'source/common/dictbe.cpp',
            'source/common/dtintrv.cpp',
            'source/common/errorcode.cpp',
            'source/common/filterednormalizer2.cpp',
            'source/common/icudataver.c',
            'source/common/icuplug.c',
            'source/common/locavailable.cpp',
            'source/common/locbased.cpp',
            'source/common/locdispnames.cpp',
            'source/common/locid.cpp',
            'source/common/loclikely.cpp',
            'source/common/locmap.c',
            'source/common/locresdata.cpp',
            'source/common/locutil.cpp',
            'source/common/mutex.cpp',
            'source/common/normalizer2.cpp',
            'source/common/normalizer2impl.cpp',
            'source/common/normlzr.cpp',
            'source/common/parsepos.cpp',
            'source/common/propname.cpp',
            'source/common/propsvec.c',
            'source/common/punycode.c',
            'source/common/putil.c',
            'source/common/rbbi.cpp',
            'source/common/rbbidata.cpp',
            'source/common/rbbinode.cpp',
            'source/common/rbbirb.cpp',
            'source/common/rbbiscan.cpp',
            'source/common/rbbisetb.cpp',
            'source/common/rbbistbl.cpp',
            'source/common/rbbitblb.cpp',
            'source/common/resbund.cpp',
            'source/common/resbund_cnv.cpp',
            'source/common/ruleiter.cpp',
            'source/common/schriter.cpp',
            'source/common/serv.cpp',
            'source/common/servlk.cpp',
            'source/common/servlkf.cpp',
            'source/common/servls.cpp',
            'source/common/servnotf.cpp',
            'source/common/servrbf.cpp',
            'source/common/servslkf.cpp',
            'source/common/stringpiece.cpp',
            'source/common/triedict.cpp',
            'source/common/uarrsort.c',
            'source/common/ubidi.c',
            'source/common/ubidi_props.c',
            'source/common/ubidiln.c',
            'source/common/ubidiwrt.c',
            'source/common/ubrk.cpp',
            'source/common/ucase.c',
            'source/common/ucasemap.c',
            'source/common/ucat.c',
            'source/common/uchar.c',
            'source/common/uchriter.cpp',
            'source/common/ucln_cmn.c',
            'source/common/ucmndata.c',
            'source/common/ucnv.c',
            'source/common/ucnv2022.c',
            'source/common/ucnv_bld.c',
            'source/common/ucnv_cb.c',
            'source/common/ucnv_cnv.c',
            'source/common/ucnv_err.c',
            'source/common/ucnv_ext.c',
            'source/common/ucnv_io.c',
            'source/common/ucnv_lmb.c',
            'source/common/ucnv_set.c',
            'source/common/ucnv_u16.c',
            'source/common/ucnv_u32.c',
            'source/common/ucnv_u7.c',
            'source/common/ucnv_u8.c',
            'source/common/ucnvbocu.c',
            'source/common/ucnvdisp.c',
            'source/common/ucnvhz.c',
            'source/common/ucnvisci.c',
            'source/common/ucnvlat1.c',
            'source/common/ucnvmbcs.c',
            'source/common/ucnvscsu.c',
            'source/common/ucnvsel.cpp',
            'source/common/ucol_swp.cpp',
            'source/common/udata.cpp',
            'source/common/udatamem.c',
            'source/common/udataswp.c',
            'source/common/uenum.c',
            'source/common/uhash.c',
            'source/common/uhash_us.cpp',
            'source/common/uidna.cpp',
            'source/common/uinit.c',
            'source/common/uinvchar.c',
            'source/common/uiter.cpp',
            'source/common/ulist.c',
            'source/common/uloc.c',
            'source/common/uloc_tag.c',
            'source/common/umapfile.c',
            'source/common/umath.c',
            'source/common/umutex.c',
            'source/common/unames.c',
            'source/common/unifilt.cpp',
            'source/common/unifunct.cpp',
            'source/common/uniset.cpp',
            'source/common/uniset_props.cpp',
            'source/common/unisetspan.cpp',
            'source/common/unistr.cpp',
            'source/common/unistr_case.cpp',
            'source/common/unistr_cnv.cpp',
            'source/common/unistr_props.cpp',
            'source/common/unorm.cpp',
            'source/common/unorm_it.c',
            'source/common/unormcmp.cpp',
            'source/common/uobject.cpp',
            'source/common/uprops.cpp',
            'source/common/ures_cnv.c',
            'source/common/uresbund.c',
            'source/common/uresdata.c',
            'source/common/usc_impl.c',
            'source/common/uscript.c',
            'source/common/uset.cpp',
            'source/common/uset_props.cpp',
            'source/common/usetiter.cpp',
            'source/common/ushape.c',
            'source/common/usprep.cpp',
            'source/common/ustack.cpp',
            'source/common/ustr_cnv.c',
            'source/common/ustr_wcs.c',
            'source/common/ustrcase.c',
            'source/common/ustrenum.cpp',
            'source/common/ustrfmt.c',
            'source/common/ustring.c',
            'source/common/ustrtrns.c',
            'source/common/utext.cpp',
            'source/common/utf_impl.c',
            'source/common/util.cpp',
            'source/common/util_props.cpp',
            'source/common/utrace.c',
            'source/common/utrie.c',
            'source/common/utrie2.cpp',
            'source/common/utrie2_builder.c',
            'source/common/uts46.cpp',
            'source/common/utypes.c',
            'source/common/uvector.cpp',
            'source/common/uvectr32.cpp',
            'source/common/uvectr64.cpp',
            'source/common/wintz.c',
          ],
          'defines': [
            'U_COMMON_IMPLEMENTATION',
          ],
          'dependencies': [
            'icudata',
          ],
          'direct_dependent_settings': {
            'include_dirs': [
              'source/common',
              'genfiles/arch/<(OS)/common/include',
            ],
            'conditions': [
              [ 'component=="static_library"', {
                'defines': [
                  'U_STATIC_IMPLEMENTATION',
                ],
              }],
              # This is needed due to an icu header that contains
              # inline code with an unused variable.
              [ 'os_posix == 1 and OS != "mac"', {
                'conditions': [
                  ['<(gcc_version) < 46', {
                    'cflags': ['-Wno-unused-variable']
                  }, {
                    'cflags': ['-Wno-unused-but-set-variable']
                  }], 
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
              'conditions': [
                ['clang==1', {
                  'cflags': [
                    '-Wno-dynamic-class-memaccess',
                    '-Wno-switch',
                    '-Wno-tautological-compare',
                  ],
                }],
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
          'type': 'none',
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
