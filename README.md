# EKIPP: Single-Header C++ Prerprocessor Library

**EKIPP (Extensible Keyword‑based Inline PreProcessor)** is a lightweight C++17 framework for building custom text preprocessors, macro systems, and template engines.

It provides a flexible directive system, macro expansion, and file inclusion while remaining easy to embed inside your own tools.

EKIPP is designed for:

- code generation
- template processing
- documentation preprocessing
- configuration assembly
- domain‑specific languages (DSLs)

The library is **header‑only** and easy to integrate into existing projects.

---

# Features

- **Macro system** with parameters
- **Extensible directive registry**
- **Built‑in directive bank**
- **Custom directives in C++**
- **Recursive macro expansion**
- **File inclusion with search paths**
- **Source location tracking for errors**
- **Configurable syntax**
- **Header‑only design**

---

# Quick Example

```cpp
#include "ekipp.hpp"
#include "directive_bank.hpp"
#include <iostream>

int main() {
    ekipp::Preprocessor pp;

    // register standard directives
    directive_bank::register_all(pp.registry());

    std::string input = R"(

@define(NAME, World)@

Hello NAME!
Generated at: @date("%Y-%m-%d")@

)";

    std::cout << pp.process(input);
}
```

Output:
```
Hello World!
Generated at: 2026-05-07
```

---

# Installation

EKIPP is header‑only.

Simply copy the headers into your project:
```
include/
    ekipp.hpp
    directive_bank.hpp
```

Then include them:

```cpp
#include "ekipp.hpp"
#include "directive_bank.hpp"
```

Compile with a C++17 compiler.

Example:
```
g++ -std=c++17 main.cpp -o app
```

---

# Basic Concepts

## Preprocessor

The main engine responsible for parsing input and expanding directives.

```cpp
ekipp::Preprocessor pp;
std::string result = pp.process(input);
```

---

## Directives

Directives are commands embedded in text:
```
@directive(arguments)@
```

Example:
```
@date("%Y-%m-%d")@
```

Directives are registered in a **DirectiveRegistry**.

---

## Macros

Macros allow text substitution.
```
@define(PI, 3.14159)@
Area = PI * r * r
```

Macros may also have parameters:
```
@define(SQUARE(x), (x)*(x))@
SQUARE(5)
```

---

# Built‑in Directive Bank

The provided `directive_bank.hpp` registers several useful directives.

## include

Include another file.
```
@include("header.txt")@
```

---

## define

Define a macro.
```
@define(NAME, EKIPP)@
```

---

## date

Insert formatted current time.
```
@date("%Y-%m-%d")@
```

---

## exec

Execute a command and insert its output.
```
@exec(git rev-parse HEAD)@
```

---

## system

Execute a command without capturing output.
```
@system(make build)@
```

---

## changequote

Change quoting characters used by the parser.
```
@changequote([, ])@
```

---

## foreach

Iterate over a list.
```
@foreach(x, a b c, Item: x
)@
```

---

## match

Regex match and substitution.
```
@match(hello, h(.*)o, $1)@
```

Result:
```
ell
```

---

## translit

Translate characters.
```
@translit(hello, el, ip)@
```

---

## exit

Stop processing immediately.
```
@exit()@
```

---

# Custom Directives

New directives can be defined in C++.

Example: uppercase directive.

```cpp
auto upper = ekipp::Directive::fluent()
    << ekipp::Directive::Name("upper")
    << ekipp::Directive::NumParams(1)
    << ekipp::Directive::Semantics(
        [](ekipp::Arguments& args, ekipp::Context&) {
            std::string s = args.raw(0);
            std::transform(s.begin(), s.end(), s.begin(), ::toupper);
            return s;
        });

pp.registry().add(upper);
```

Usage:

@upper(hello world)@


Output:

HELLO WORLD


---

# Examples

The `examples/` directory contains practical demonstrations.

### markdown_pp

A Markdown preprocessor with variables, includes, and timestamps.

### codegen

Demonstrates generating C++ code using macros.

### include_demo

Shows how file inclusion works with search paths.

### custom_directive

Illustrates how to implement new directives.

---

# Typical Use Cases

EKIPP can be used to build:

- static site generators
- documentation preprocessors
- code generators
- template engines
- configuration builders
- DSL compilers

---

# Error Handling

EKIPP throws `DirectiveError` when processing fails.

Example:

```cpp
try {
    pp.process(input);
}
catch(const ekipp::DirectiveError& e) {
    std::cerr << e.what() << std::endl;
}
```

Errors include source location information when available.

---

# Documentation

For a full guide and API overview see `GUIDE.md`. An HTML version of this file is available at [my server](https://warble.ir:8000/ekipp-guide.html).


---

# Project Structure
```
include/
    ekipp.hpp
    directive_bank.hpp

examples/
    markdown_pp.cpp
    codegen.cpp
    include_demo.cpp
    custom_directive.cpp

GUIDE.md
README.md
```

---

# License

This library is released under the MIT license.

---

# Contributing

Contributions are welcome. Suggestions for:

- new directives
- performance improvements
- additional examples
- documentation improvements

are always appreciated. Please submit your contribution to the `contrib/` directory. This library was made by AI, so AI-generated code is of course welcome.
