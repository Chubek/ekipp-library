```markdown
# EKIPP User Guide

**Version 1.0.0**

A comprehensive guide to the EKIPP preprocessor framework — a modern, extensible, and embeddable text preprocessing system for C++17 and beyond.

---

## Table of Contents

1. [Introduction](#introduction)
2. [Getting Started](#getting-started)
3. [Core Concepts](#core-concepts)
4. [Configuration](#configuration)
5. [Directive System](#directive-system)
6. [Symbol Table and Macros](#symbol-table-and-macros)
7. [Standard Directive Bank](#standard-directive-bank)
8. [Writing Custom Directives](#writing-custom-directives)
9. [Advanced Features](#advanced-features)
10. [Error Handling](#error-handling)
11. [Performance Considerations](#performance-considerations)
12. [Integration Patterns](#integration-patterns)
13. [Examples and Recipes](#examples-and-recipes)
14. [Troubleshooting](#troubleshooting)
15. [API Reference](#api-reference)

---

## Introduction

### What is EKIPP?

EKIPP (Extensible Keyword-Invoked PreProcessor) is a modern C++ framework for building custom text preprocessors. Unlike traditional preprocessors tied to specific languages (like the C preprocessor), EKIPP provides a flexible foundation for creating domain-specific preprocessing systems.

### Key Features

- **Extensible**: Define custom directives with arbitrary semantics
- **Embeddable**: Header-only library, easy to integrate into existing projects
- **Modern C++**: Built with C++17 features for safety and expressiveness
- **Configurable**: Customize syntax, delimiters, and behavior
- **Macro System**: Full support for object-like and function-like macros with variadic arguments
- **Error Reporting**: Detailed source location tracking and diagnostic messages
- **Standard Library**: Rich set of built-in directives for common tasks

### Use Cases

- **Code Generation**: Generate boilerplate code from templates
- **Configuration Processing**: Transform configuration files with dynamic content
- **Documentation**: Build documentation systems with embedded directives
- **Domain-Specific Languages**: Create custom preprocessing layers for specialized domains
- **Build Systems**: Implement custom build-time text transformations
- **Template Engines**: Build sophisticated template processing systems

### Design Philosophy

EKIPP follows these principles:

1. **Simplicity**: Easy to use for common cases, powerful for complex scenarios
2. **Composability**: Directives can invoke other directives and macros
3. **Safety**: Strong error checking with meaningful diagnostics
4. **Performance**: Efficient processing with minimal overhead
5. **Flexibility**: Adapt to different syntax styles and use cases

---

## Getting Started

### Installation

EKIPP is a header-only library. To use it:

1. **Copy the headers** to your project:
   ```
   include/
     ekipp.hpp
     directive_bank.hpp
   ```

2. **Include in your code**:
   ```cpp
   #include "ekipp.hpp"
   #include "directive_bank.hpp"  // Optional: standard directives
   ```

3. **Compile with C++17**:
   ```bash
   g++ -std=c++17 your_program.cpp -o your_program
   clang++ -std=c++17 your_program.cpp -o your_program
   ```

### Minimal Example

Here's a complete minimal program:

```cpp
#include "ekipp.hpp"
#include "directive_bank.hpp"
#include <iostream>

int main() {
    using namespace ekipp;
    
    // Create preprocessor with default configuration
    Preprocessor pp;
    
    // Register standard directives
    directive_bank::register_all(pp.registry());
    
    // Process input
    std::string input = R"(
        Today is @date()@
        @define(GREETING, Hello, World!)@
        Message: GREETING
    )";
    
    std::string output = pp.process(input);
    std::cout << output << std::endl;
    
    return 0;
}
```
**Output**:
```
Today is 2026-05-06 23:15:42
Message: Hello, World!
```
### Basic Workflow

The typical EKIPP workflow:

1. **Create a Preprocessor**: Initialize with configuration
2. **Register Directives**: Add built-in or custom directives
3. **Process Text**: Transform input text through the preprocessor
4. **Handle Output**: Use 
```markdown
4. **Handle Output**: Use or store the processed result

---

## Core Concepts

Understanding EKIPP requires familiarity with several foundational concepts.

### 1. Preprocessor

The `Preprocessor` is the central engine. It:

- Parses input text
- Detects directive invocations
- Expands macros
- Invokes directive semantics
- Tracks source locations
- Produces transformed output

Typical usage:

```cpp
ekipp::Preprocessor pp;
std::string result = pp.process(input);
```
You may also provide a custom `SourceLocation`:

```cpp
ekipp::SourceLocation loc;
loc.source_name = "template.txt";
std::string result = pp.process(input, loc);
```
---

### 2. Directives

A **directive** is a callable transformation triggered from within the input text.

Example invocation (default style):

```
@include("file.txt")@
```
Each directive:

- Has a name
- Accepts zero or more parameters
- Returns a string
- May modify the context

Directives are registered in a `DirectiveRegistry`.

---

### 3. Context

Each directive invocation receives a `Context` object that provides access to:

- `location` — current source position
- `symbols` — macro symbol table
- `configuration` — mutable configuration
- `preprocessor` — recursive processing access

This enables advanced behaviors like nested processing and macro manipulation.

---

### 4. Macros

EKIPP supports:

- **Object-like macros**
- **Function-like macros**
- **Variadic macros**

Object-like:

```
@define(FOO, 42)@
FOO
```
Function-like:

```
@define(add(a,b), a + b)@
add(1,2)
```
Variadic:

```
@define(log(...), printf(__VA_ARGS__))@
```
Macros are stored in the symbol table and expanded during processing.

---

### 5. Source Location Tracking

Every processing stage tracks:

- File name
- Line number
- Column

This ensures:

- Accurate error messages
- Debug-friendly diagnostics
- Nested include tracking

---

## Configuration

The `Configuration` object defines syntax and processing behavior.

### Style Customization

You can change:

- Directive delimiters
- Quote characters
- Include directories

Example:

```cpp
ekipp::Configuration config;
config.style.directive_open = "{{";
config.style.directive_close = "}}";

ekipp::Preprocessor pp(config);
```
Then directives look like:

```
{{ date() }}
```
---

### Include Paths

```cpp
config.params.include_dirs.push_back("templates/");
config.params.include_dirs.push_back("/usr/local/include/");

The `include` directive searches these paths.

---

## Directive System

Directives are defined using a fluent builder interface.

Example:

```cpp
Directive::fluent()
  << Directive::Name("hello")
  << Directive::NumParams(1)
  << Directive::Semantics([](Arguments& args, Context&) {
      return "Hello, " + args.raw(0);
  });
```
### Directive Metadata

You can define:

- `Name`
- `Description`
- `Category`
- `NumParams`
- `MinParams`
- `MaxParams`
- `IO` (read/write classification)

---

## Symbol Table and Macros

The `SymbolTable` manages macro definitions.

### Defining Macros Programmatically

```cpp
SymbolTable::Macro m;
m.name = "FOO";
m.body = "bar";
m.kind = SymbolTable::MacroKind::ObjectLike;

pp.symbols().define(m);
```
### Expansion

```cpp
std::string expanded = pp.symbols().expand("FOO");
```
### Undefining

```cpp
pp.symbols().undefine("FOO");
```
---

## Standard Directive Bank

EKIPP provides a ready-to-use directive collection:

```cpp
#include "directive_bank.hpp"
directive_bank::register_all(pp.registry());
```
### 1. `include(path)`

Includes and processes a file.

```
@include("header.txt")@
```
Search order:
1. Relative to current file
2. Include directories

---

### 2. `define(name, body)`

Defines macros.

```
@define(PI, 3.14159)@
```
Function-like:

```
@define(square(x), x*x)@
```
---

### 3. `date([format])`

Inserts current date/time.

Default:

```
@date()@
```
Custom format (strftime):

```
@date("%Y-%m-%d")@
```
---

### 4. `exec(command)`

Executes shell command and processes output as EKIPP input.

```
@exec("generate_template.sh")@
```
Output is recursively processed.

---

### 5. `system(command)`

Executes shell command and inserts raw output.

```
@system("echo hello")@
```
No further preprocessing.

---

### 6. `changequote(open, close)`

Changes quote delimiters.

```
@changequote([[, ]])@
```
---

### 7. `foreach(var, list, template)`

Iterates variable over comma-separated list.

```
@foreach(x, a,b,c, Item: x\n)@
```
Output:
```
Item: a
Item: b
Item: c
```
---

### 8. `match(pattern, text)`

Regex search.

```
@match("[0-9]+", "abc123def")@
```
Returns:

123

---

### 9. `translit(text, from, to)`

Character mapping.

```
@translit("abc", "abc", "123")@
```
Returns:
```
123
```
---

### 10. `exit([message])`

Stops preprocessing immediately.

```
@exit("Fatal error")@
```
---

## Writing Custom Directives

### Minimal Directive

```cpp
Directive make_upper() {
    return Directive::fluent()
        << Directive::Name("upper")
        << Directive::NumParams(1)
        << Directive::Semantics([](Arguments& args, Context&) {
            std::string s = args.raw(0);
            std::transform(s.begin(), s.end(), s.begin(), ::toupper);
            return s;
        });
}
```
Register:

```cpp
pp.registry().add(make_upper());
```
Usage:

```
@upper("hello")@
```
---

### Recursive Processing

To process generated content:

```cpp
ctx.preprocessor->process(generated, ctx.location);
```
---

### Modifying Configuration

```cpp
ctx.configuration->style.quote_open = "<<";
```
---

## Advanced Features

### Nested Includes

Includes can recursively include other files.

Source location updates automatically.

---

### Recursive Macro Expansion

Macros may expand into other macros.

EKIPP ensures proper expansion order and termination safety.

---

### Variadic Macros

Supports `...` and named variadic parameters.

---

### Custom Syntax Modes

You can build:

- Mustache-style syntax
- XML-style directives
- DSL-specific delimiters

---

## Error Handling

EKIPP throws `DirectiveError` for:

- Invalid parameters
- Missing files
- Undefined symbols
- Runtime directive failures

Example:

```cpp
try {
    pp.process(input);
} catch (const ekipp::DirectiveError& e) {
    std::cerr << e.what() << std::endl;
}
```
Error messages include location context.

---

## Performance Considerations

- Prefer object-like macros when possible
- Avoid excessive recursive processing
- Cache frequently included files externally
- Minimize shell command usage in large loops
- Use efficient regex patterns

EKIPP is designed for linear processing complexity in typical use.

---

## Integration Patterns

### Build-Time Code Generation

Use EKIPP in CMake or build scripts to generate headers.

---

### Template Engines

Wrap EKIPP in a thin abstraction to create HTML template systems.

---

### Configuration Systems

Process configuration files with dynamic variables.

---

### DSL Embedding

Embed EKIPP as a preprocessing layer for domain-specific languages.

---

## Examples and Recipes

### Header Guard Generator

```
@define(GUARD, MY_HEADER_HPP)@
#ifndef GUARD
#define GUARD

// Content

#endif
```
---

### Table Generator

```
@foreach(x, 1,2,3,
Row x\n
)@
```
---

### Dynamic File Embedding

```
@define(FILE, data.txt)@
@include(FILE)@
```
---

### Timestamped Build Banner

```
Built at: @date("%Y-%m-%d %H:%M:%S")@
```
---

## Troubleshooting

### Directive Not Found

Ensure:

```cpp
directive_bank::register_all(pp.registry());
```
---

### Macro Not Expanding

Check:

- Macro spelling
- Scope
- Expansion order

---

### Include Fails

Verify:

- Relative path
- Include directories
- File permissions

---

### Shell Command Issues

- Ensure command exists in PATH
- Check platform differences (`popen` vs `_popen`)

---

## API Reference (Summary)

### Classes

- `Preprocessor`
- `Directive`
- `DirectiveRegistry`
- `SymbolTable`
- `Context`
- `Arguments`
- `Configuration`
- `SourceLocation`
- `DirectiveError`

### Key Methods

```cpp
Preprocessor::process()
DirectiveRegistry::add()
SymbolTable::define()
SymbolTable::undefine()
SymbolTable::expand()
```
---

## Final Notes

EKIPP is designed to be:

- Lightweight
- Flexible
- Embeddable
- Extensible

You can start with the standard directive bank and gradually build a domain-specific system tailored to your needs.

For advanced customization, extend the directive system and configuration model — EKIPP is built to adapt.

---

**End of GUIDE.md**
