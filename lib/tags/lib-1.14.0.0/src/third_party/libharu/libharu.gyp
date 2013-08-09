# Copyright 2011 Google Inc.
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

{
  'targets': [
    {
      'target_name': 'libharu',
      'type': 'static_library',
      'dependencies': [
        '../libpng/libpng.gyp:libpng',
        '../zlib/zlib.gyp:zlib',
      ],
      'sources': [
        'libharu/src/hpdf_annotation.c',
        'libharu/src/hpdf_array.c',
        'libharu/src/hpdf_binary.c',
        'libharu/src/hpdf_boolean.c',
        'libharu/src/hpdf_catalog.c',
        'libharu/src/hpdf_destination.c',
        'libharu/src/hpdf_dict.c',
        'libharu/src/hpdf_doc.c',
        'libharu/src/hpdf_doc_png.c',
        'libharu/src/hpdf_encoder.c',
        'libharu/src/hpdf_encoder_cns.c',
        'libharu/src/hpdf_encoder_cnt.c',
        'libharu/src/hpdf_encoder_jp.c',
        'libharu/src/hpdf_encoder_kr.c',
        'libharu/src/hpdf_encrypt.c',
        'libharu/src/hpdf_encryptdict.c',
        'libharu/src/hpdf_error.c',
        'libharu/src/hpdf_ext_gstate.c',
        'libharu/src/hpdf_font.c',
        'libharu/src/hpdf_font_cid.c',
        'libharu/src/hpdf_font_tt.c',
        'libharu/src/hpdf_font_type1.c',
        'libharu/src/hpdf_fontdef.c',
        'libharu/src/hpdf_fontdef_base14.c',
        'libharu/src/hpdf_fontdef_cid.c',
        'libharu/src/hpdf_fontdef_cns.c',
        'libharu/src/hpdf_fontdef_cnt.c',
        'libharu/src/hpdf_fontdef_jp.c',
        'libharu/src/hpdf_fontdef_kr.c',
        'libharu/src/hpdf_fontdef_tt.c',
        'libharu/src/hpdf_fontdef_type1.c',
        'libharu/src/hpdf_gstate.c',
        'libharu/src/hpdf_image.c',
        'libharu/src/hpdf_image_png.c',
        'libharu/src/hpdf_info.c',
        'libharu/src/hpdf_list.c',
        'libharu/src/hpdf_mmgr.c',
        'libharu/src/hpdf_name.c',
        'libharu/src/hpdf_null.c',
        'libharu/src/hpdf_number.c',
        'libharu/src/hpdf_objects.c',
        'libharu/src/hpdf_outline.c',
        'libharu/src/hpdf_page_label.c',
        'libharu/src/hpdf_page_operator.c',
        'libharu/src/hpdf_pages.c',
        'libharu/src/hpdf_real.c',
        'libharu/src/hpdf_streams.c',
        'libharu/src/hpdf_string.c',
        'libharu/src/hpdf_u3d.c',
        'libharu/src/hpdf_utils.c',
        'libharu/src/hpdf_xref.c',
      ],
      'include_dirs': [
        'libharu/include',
        'include',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          'include',
        ],
      },
      'conditions': [
        ['OS=="win"', {
          'msvs_disabled_warnings': [ 4018, 4305 ],
          'include_dirs': [
            'gen/arch/win/ia32/include',
          ],
          'direct_dependent_settings': {
            'include_dirs': [
              'gen/arch/win/ia32/include',
            ],
          },
        }, { # else
          'include_dirs': [
            'gen/posix/include',
          ],
          'direct_dependent_settings': {
            'include_dirs': [
              'gen/posix/include',
            ],
          },
        }],
        [ 'clang==1', {
          'xcode_settings':  {
            'WARNING_CFLAGS': [
              '-Wno-literal-conversion',
              '-Wno-tautological-compare',
            ],
          },
          'cflags': [
            '-Wno-literal-conversion',
            '-Wno-tautological-compare',
          ],
        }],
      ],
    },
  ],
}
