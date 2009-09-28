/*
 * This file contains a C++ port of jsmin.c. The copyright notice
 * below is the copyright notice from jsmin.c.
 */

/* jsmin.c
   2008-08-03

Copyright (c) 2002 Douglas Crockford  (www.crockford.com)

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "third_party/jsmin/cpp/jsmin.h"

#include <stdlib.h>
#include <stdio.h>

#include "base/logging.h"

/* isAlphanum -- return true if the character is a letter, digit, underscore,
        dollar sign, or non-ASCII character.
*/

namespace {

int
isAlphanum(int c)
{
    return ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
        (c >= 'A' && c <= 'Z') || c == '_' || c == '$' || c == '\\' ||
        c > 126);
}

}  // namespace

namespace jsmin {

Minifier::Minifier(const char *input)
  : theA(-1),
    theB(-1),
    theLookahead(EOF),
    input_(input),
    input_index_(0),
    output_buffer_(),
    error_(false),
    done_(false) {
}

Minifier::~Minifier() {}

bool
Minifier::GetMinifiedOutput(std::string *out)
{
    if (!done_) {
        jsmin();
    }
    if (!error_) {
        *out = output_buffer_;
    }
    return !error_;
}


/* get -- return the next character from the input. Watch out for lookahead. If
        the character is a control character, translate it to a space or
        linefeed.
*/

int
Minifier::get()
{
    int c = theLookahead;
    theLookahead = EOF;
    if (c == EOF) {
        c = (0xff & input_[input_index_++]);
        if (c == '\0') {
            c = EOF;
        }
    }
    if (c >= ' ' || c == '\n' || c == EOF) {
        return c;
    }
    if (c == '\r') {
        return '\n';
    }
    return ' ';
}


/* peek -- get the next character without getting it.
*/

int
Minifier::peek()
{
    theLookahead = get();
    return theLookahead;
}


/* next -- get the next character, excluding comments. peek() is used to see
        if a '/' is followed by a '/' or '*'.
*/

int
Minifier::next()
{
    int c = get();
    if  (c == '/') {
        switch (peek()) {
        case '/':
            for (;;) {
                c = get();
                if (c <= '\n') {
                    return c;
                }
            }
        case '*':
            get();
            for (;;) {
                switch (get()) {
                case '*':
                    if (peek() == '/') {
                        get();
                        return ' ';
                    }
                    break;
                case EOF:
                    LOG(WARNING) << "Error: JSMIN Unterminated comment.";
                    error_ = true;
                    return EOF;
                }
            }
        default:
            return c;
        }
    }
    return c;
}


/* action -- do something! What you do is determined by the argument:
        1   Output A. Copy B to A. Get the next B.
        2   Copy B to A. Get the next B. (Delete A).
        3   Get the next B. (Delete B).
   action treats a string as a single character. Wow!
   action recognizes a regular expression if it is preceded by ( or , or =.
*/

void
Minifier::action(int d)
{
    switch (d) {
    case 1:
        output_buffer_.push_back(theA);
    case 2:
        theA = theB;
        if (theA == '\'' || theA == '"') {
            for (;;) {
                output_buffer_.push_back(theA);
                theA = get();
                if (theA == theB) {
                    break;
                }
                if (theA == '\\') {
                    output_buffer_.push_back(theA);
                    theA = get();
                }
                if (theA == EOF) {
                    LOG(WARNING) << "Error: JSMIN unterminated string literal.";
                    error_ = true;
                    return;
                }
            }
        }
    case 3:
        theB = next();
        if (error_) {
            return;
        }
        if (theB == '/' && (theA == '(' || theA == ',' || theA == '=' ||
                            theA == ':' || theA == '[' || theA == '!' ||
                            theA == '&' || theA == '|' || theA == '?' ||
                            theA == '{' || theA == '}' || theA == ';' ||
                            theA == '\n')) {
            output_buffer_.push_back(theA);
            output_buffer_.push_back(theB);
            for (;;) {
                theA = get();
                if (theA == '/') {
                    break;
                }
                if (theA =='\\') {
                    output_buffer_.push_back(theA);
                    theA = get();
                }
                if (theA == EOF) {
                    LOG(WARNING) << "Error: JSMIN unterminated "
                                 << "Regular Expression literal.\n";
                    error_ = true;
                    return;
                }
                output_buffer_.push_back(theA);
            }
            theB = next();
        }
    }
}


/* jsmin -- Copy the input to the output, deleting the characters which are
        insignificant to JavaScript. Comments will be removed. Tabs will be
        replaced with spaces. Carriage returns will be replaced with linefeeds.
        Most spaces and linefeeds will be removed.
*/

void
Minifier::jsmin()
{
    theA = '\n';
    action(3);
    if (error_) {
        return;
    }
    while (theA != EOF) {
        switch (theA) {
        case ' ':
            if (isAlphanum(theB)) {
                action(1);
            } else {
                action(2);
            }
            break;
        case '\n':
            switch (theB) {
            case '{':
            case '[':
            case '(':
            case '+':
            case '-':
                action(1);
                break;
            case ' ':
                action(3);
                break;
            default:
                if (isAlphanum(theB)) {
                    action(1);
                } else {
                    action(2);
                }
            }
            break;
        default:
            switch (theB) {
            case ' ':
                if (isAlphanum(theA)) {
                    action(1);
                    break;
                }
                action(3);
                break;
            case '\n':
                switch (theA) {
                case '}':
                case ']':
                case ')':
                case '+':
                case '-':
                case '"':
                case '\'':
                    action(1);
                    break;
                default:
                    if (isAlphanum(theA)) {
                        action(1);
                    } else {
                        action(3);
                    }
                }
                break;
            default:
                action(1);
                break;
            }
        }
    }
}

}  // namespace jsmin
