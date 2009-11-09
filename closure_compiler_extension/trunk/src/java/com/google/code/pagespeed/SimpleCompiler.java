/**
 * Copyright 2009 Google Inc.
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
package com.google.code.pagespeed;

import com.google.javascript.jscomp.Compiler;
import com.google.javascript.jscomp.CompilerOptions;
import com.google.javascript.jscomp.CompilationLevel;
import com.google.javascript.jscomp.JSSourceFile;

import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Wrapper that calls the Compiler with a set of simple options that will
 * perform transformations that will not alter the behavior of well-behaved JS.
 *
 * @author Michael Bolin
 */
public class SimpleCompiler {

  static {
    // Disable all logging from the Compiler so it does not clutter the user's
    // console.
    Logger.getLogger("com.google.javascript").setLevel(Level.OFF);
  }

  /**
   * @param uncompiledSource Uncompiled JS code
   * @return the compiled equivalent of the input JS
   */
  public String doCompile(String uncompiledSource) {
    CompilerOptions options = new CompilerOptions();
    CompilationLevel.SIMPLE_OPTIMIZATIONS.
        setOptionsForCompilationLevel(options);

    Compiler compiler = new Compiler();
    JSSourceFile[] code = new JSSourceFile[] {
        JSSourceFile.fromCode("uncompiledSource.js", uncompiledSource)
      };
    JSSourceFile[] externs = new JSSourceFile[] {};
    compiler.compile(externs, code, options);
    return compiler.toSource();
  }
}
