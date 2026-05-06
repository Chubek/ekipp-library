// directive_bank.hpp - Standard Directive Library for EKIPP
// A collection of commonly-used preprocessor directives
// Version: 1.0.0
// License: MIT

#ifndef EKIPP_DIRECTIVE_BANK_HPP
#define EKIPP_DIRECTIVE_BANK_HPP

#include "ekipp.hpp"
#include <array>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <memory>
#include <regex>
#include <sstream>

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

namespace ekipp
{
namespace directive_bank
{

// ============================================================================
// Utility functions
// ============================================================================

namespace detail
{

inline std::string
exec_command (const std::string &cmd)
{
  std::array<char, 128> buffer;
  std::string result;

#ifdef _WIN32
  std::unique_ptr<FILE, decltype (&_pclose)> pipe (_popen (cmd.c_str (), "r"),
                                                   _pclose);
#else
  std::unique_ptr<FILE, decltype (&pclose)> pipe (popen (cmd.c_str (), "r"),
                                                  pclose);
#endif

  if (!pipe)
    {
      throw DirectiveError ("failed to execute command: " + cmd);
    }

  while (fgets (buffer.data (), buffer.size (), pipe.get ()) != nullptr)
    {
      result += buffer.data ();
    }

  return result;
}

inline std::string
read_file (const std::filesystem::path &path)
{
  std::ifstream file (path, std::ios::binary);
  if (!file)
    {
      throw DirectiveError ("cannot open file: " + path.string ());
    }

  std::ostringstream ss;
  ss << file.rdbuf ();
  return ss.str ();
}

inline std::string
format_time (const std::string &format_str)
{
  auto now = std::chrono::system_clock::now ();
  auto time_t_now = std::chrono::system_clock::to_time_t (now);
  std::tm tm_now;

#ifdef _WIN32
  localtime_s (&tm_now, &time_t_now);
#else
  localtime_r (&time_t_now, &tm_now);
#endif

  std::array<char, 256> buffer;
  std::strftime (buffer.data (), buffer.size (), format_str.c_str (), &tm_now);
  return std::string (buffer.data ());
}

inline std::string
trim (const std::string &s)
{
  auto start = s.begin ();
  while (start != s.end ()
         && std::isspace (static_cast<unsigned char> (*start)))
    {
      ++start;
    }

  auto end = s.end ();
  do
    {
      --end;
    }
  while (std::distance (start, end) > 0
         && std::isspace (static_cast<unsigned char> (*end)));

  return std::string (start, end + 1);
}

} // namespace detail

// ============================================================================
// include - Include and process external file
// ============================================================================

inline Directive
make_include ()
{
    return Directive::fluent()
        << Directive::Name("include")
        << Directive::Description("Include and process an external file")
        << Directive::Category("file")
        << Directive::NumParams(1)
        << Directive::IO("r")
        << directives::arg("path", "string", false)
        << Directive::Semantics([](Arguments& args, Context& ctx) -> std::string {
    auto path_str = args.raw (0);
    std::filesystem::path path (path_str);

    // Try relative to current source
    if (!path.is_absolute () && !ctx.location.source_name.empty ())
      {
        auto parent
            = std::filesystem::path (ctx.location.source_name).parent_path ();
        if (!parent.empty ())
          {
            auto candidate = parent / path;
            if (std::filesystem::exists (candidate))
              {
                path = candidate;
              }
          }
      }

    // Try include directories
    if (!std::filesystem::exists (path) && ctx.configuration)
      {
        for (const auto &inc_dir : ctx.configuration->params.include_dirs)
          {
            auto candidate = inc_dir / path_str;
            if (std::filesystem::exists (candidate))
              {
                path = candidate;
                break;
              }
          }
      }

    throw DirectiveError ("include file not found: " + path_str);
            }

            auto content = detail::read_file(path);

            SourceLocation nested_loc = ctx.location;
            nested_loc.source_name = path.string();
            nested_loc.line = 1;
            nested_loc.column = 1;

            return ctx.preprocessor
                ? ctx.preprocessor->process(content, nested_loc)
                : content;
});
}

// ============================================================================
// define - Define a macro in the symbol table
// Forms:
//   define(NAME, VALUE)
//   define(NAME(params...), BODY)
// ============================================================================

inline Directive
make_define ()
{
  return Directive::fluent ()
         << Directive::Name ("define")
         << Directive::Description (
                "Define an object-like or function-like macro")
         << Directive::Category ("macro") << Directive::MinParams (2)
         << Directive::IO ("w")
         << Directive::Semantics (
                [] (Arguments &args, Context &ctx) -> std::string
                  {
                    if (!ctx.symbols)
                      {
                        throw DirectiveError (
                            "define requires an active symbol table");
                      }

                    std::string signature = detail::trim (args.raw (0));
                    std::string body = args.raw (1);

                    auto open = signature.find ('(');
                    auto close = signature.rfind (')');

                    if (open == std::string::npos || close == std::string::npos
                        || close < open)
                      {
                        SymbolTable::Macro macro;
                        macro.name = signature;
                        macro.kind = SymbolTable::MacroKind::ObjectLike;
                        macro.body = body;
                        ctx.symbols->define (macro);
                        return std::string ();
                      }

                    SymbolTable::Macro macro;
                    macro.kind = SymbolTable::MacroKind::FunctionLike;
                    macro.name = detail::trim (signature.substr (0, open));
                    macro.body = body;

                    std::string params_str
                        = signature.substr (open + 1, close - open - 1);
                    std::stringstream ss (params_str);
                    std::string item;

                    while (std::getline (ss, item, ','))
                      {
                        item = detail::trim (item);
                        if (item.empty ())
                          {
                            continue;
                          }
                        if (item == "...")
                          {
                            macro.variadic = true;
                            macro.parameters.push_back ("__VA_ARGS__");
                          }
                        else
                          {
                            if (item.size () >= 3
                                && item.substr (item.size () - 3) == "...")
                              {
                                macro.variadic = true;
                                macro.parameters.push_back (detail::trim (
                                    item.substr (0, item.size () - 3)));
                              }
                            else
                              {
                                macro.parameters.push_back (item);
                              }
                          }
                      }

                    if (macro.name.empty ())
                      {
                        throw DirectiveError ("invalid macro signature");
                      }

                    ctx.symbols->define (macro);
                    return std::string ();
                  });
}

// ============================================================================
// date - Emit current date/time
// date()
// date(format)
// ============================================================================

inline Directive
make_date ()
{
  return Directive::fluent ()
         << Directive::Name ("date")
         << Directive::Description ("Insert the current date/time, optionally "
                                    "using strftime format")
         << Directive::Category ("utility") << Directive::MinParams (0)
         << Directive::MaxParams (1) << Directive::IO ("r")
         << Directive::Semantics (
                [] (Arguments &args, Context &) -> std::string
                  {
                    if (args.size () == 0)
                      {
                        return detail::format_time ("%Y-%m-%d %H:%M:%S");
                      }
                    return detail::format_time (args.raw (0));
                  });
}

// ============================================================================
// exec - Execute a command and process its stdout as EKIPP input
// ============================================================================

inline Directive
make_exec ()
{
  return Directive::fluent ()
         << Directive::Name ("exec")
         << Directive::Description ("Execute a command and process its output "
                                    "as preprocessor input")
         << Directive::Category ("system") << Directive::NumParams (1)
         << Directive::IO ("r")
         << Directive::Semantics (
                [] (Arguments &args, Context &ctx) -> std::string
                  {
                    std::string output = detail::exec_command (args.raw (0));

                    SourceLocation nested_loc = ctx.location;
                    nested_loc.source_name = "<exec>";
                    nested_loc.line = 1;
                    nested_loc.column = 1;

                    return ctx.preprocessor
                               ? ctx.preprocessor->process (output, nested_loc)
                               : output;
                  });
}

// ============================================================================
// changequote - Change quoting delimiters in configuration
// changequote(open, close)
// ============================================================================

inline Directive
make_changequote ()
{
  return Directive::fluent ()
         << Directive::Name ("changequote")
         << Directive::Description ("Change the current quote delimiters")
         << Directive::Category ("parser") << Directive::NumParams (2)
         << Directive::IO ("w")
         << Directive::Semantics (
                [] (Arguments &args, Context &ctx) -> std::string
                  {
                    if (!ctx.configuration)
                      {
                        throw DirectiveError (
                            "changequote requires mutable configuration");
                      }

                    ctx.configuration->style.quote_open = args.raw (0);
                    ctx.configuration->style.quote_close = args.raw (1);
                    return std::string ();
                  });
}

// ============================================================================
// system - Execute a command and return raw stdout without EKIPP processing
// ============================================================================

inline Directive
make_system ()
{
  return Directive::fluent ()
         << Directive::Name ("system")
         << Directive::Description ("Execute a command and insert raw stdout")
         << Directive::Category ("system") << Directive::NumParams (1)
         << Directive::IO ("r")
         << Directive::Semantics (
                [] (Arguments &args, Context &) -> std::string
                  { return detail::exec_command (args.raw (0)); });
}

// ============================================================================
// foreach - Iterate over a delimited list and expand a template
// foreach(var, list, template)
// Example list: a,b,c
// ============================================================================

inline Directive
make_foreach ()
{
  return Directive::fluent ()
         << Directive::Name ("foreach")
         << Directive::Description (
                "Iterate a variable over a comma-separated list and expand a "
                "template")
         << Directive::Category ("flow") << Directive::NumParams (3)
         << Directive::IO ("r")
         << Directive::Semantics (
                [] (Arguments &args, Context &ctx) -> std::string
                  {
                    if (!ctx.symbols)
                      {
                        throw DirectiveError (
                            "foreach requires an active symbol table");
                      }

                    std::string var = detail::trim (args.raw (0));
                    std::string list = args.raw (1);
                    std::string templ = args.raw (2);

                    std::stringstream ss (list);
                    std::string item;
                    std::string out;

                    bool had_previous = ctx.symbols->is_defined (var);
                    SymbolTable::Macro previous;
                    if (had_previous)
                      {
                        previous = ctx.symbols->get (var);
                      }

                    while (std::getline (ss, item, ','))
                      {
                        item = detail::trim (item);

                        SymbolTable::Macro macro;
                        macro.name = var;
                        macro.kind = SymbolTable::MacroKind::ObjectLike;
                        macro.body = item;
                        ctx.symbols->define (macro);

                        std::string expanded = ctx.symbols->expand (templ);
                        if (ctx.preprocessor)
                          {
                            SourceLocation nested_loc = ctx.location;
                            nested_loc.source_name = "<foreach>";
                            nested_loc.line = 1;
                            nested_loc.column = 1;
                            out += ctx.preprocessor->process (expanded,
                                                              nested_loc);
                          }
                        else
                          {
                            out += expanded;
                          }
                      }

                    if (had_previous)
                      {
                        ctx.symbols->define (previous);
                      }
                    else
                      {
                        ctx.symbols->undefine (var);
                      }

                    return out;
                  });
}

// ============================================================================
// match - Regex match/extract
// match(pattern, text)
// Returns the first full match or empty string
// ============================================================================

inline Directive
make_match ()
{
  return Directive::fluent ()
         << Directive::Name ("match")
         << Directive::Description ("Match a regular expression against text "
                                    "and return the first match")
         << Directive::Category ("text") << Directive::NumParams (2)
         << Directive::IO ("r")
         << Directive::Semantics (
                [] (Arguments &args, Context &) -> std::string
                  {
                    std::regex re (args.raw (0));
                    std::smatch m;
                    std::string text = args.raw (1);

                    if (std::regex_search (text, m, re) && !m.empty ())
                      {
                        return m.str (0);
                      }
                    return std::string ();
                  });
}

// ============================================================================
// translit - Transliterate characters from one set to another
// translit(text, from, to)
// Characters in "from" are mapped positionally to "to"
// If "to" is shorter, extra source chars are deleted
// ============================================================================

inline Directive
make_translit ()
{
  return Directive::fluent ()
         << Directive::Name ("translit")
         << Directive::Description (
                "Transliterate characters from one set to another")
         << Directive::Category ("text") << Directive::NumParams (3)
         << Directive::IO ("r")
         << Directive::Semantics (
                [] (Arguments &args, Context &) -> std::string
                  {
                    const std::string text = args.raw (0);
                    const std::string from = args.raw (1);
                    const std::string to = args.raw (2);

                    std::string out;
                    out.reserve (text.size ());

                    for (char ch : text)
                      {
                        auto pos = from.find (ch);
                        if (pos == std::string::npos)
                          {
                            out.push_back (ch);
                          }
                        else if (pos < to.size ())
                          {
                            out.push_back (to[pos]);
                          }
                        else
                          {
                            // delete character
                          }
                      }

                    return out;
                  });
}

// ============================================================================
// exit - Abort preprocessing
// exit()
// exit(message)
// ============================================================================

inline Directive
make_exit ()
{
  return Directive::fluent ()
         << Directive::Name ("exit")
         << Directive::Description ("Abort preprocessing immediately")
         << Directive::Category ("flow") << Directive::MinParams (0)
         << Directive::MaxParams (1) << Directive::IO ("w")
         << Directive::Semantics (
                [] (Arguments &args, Context &) -> std::string
                  {
                    if (args.size () == 0)
                      {
                        throw DirectiveError (
                            "preprocessing terminated by exit()");
                      }
                    throw DirectiveError ("preprocessing terminated: "
                                          + args.raw (0));
                  });
}

// ============================================================================
// Registration helpers
// ============================================================================

inline void
register_all (DirectiveRegistry &registry)
{
  registry.add (make_include ());
  registry.add (make_define ());
  registry.add (make_date ());
  registry.add (make_exec ());
  registry.add (make_changequote ());
  registry.add (make_system ());
  registry.add (make_foreach ());
  registry.add (make_match ());
  registry.add (make_translit ());
  registry.add (make_exit ());
}

inline std::vector<Directive>
all ()
{
  return { make_include (),     make_define (), make_date (),    make_exec (),
           make_changequote (), make_system (), make_foreach (), make_match (),
           make_translit (),    make_exit () };
}

} // namespace directive_bank
} // namespace ekipp

#endif // EKIPP_DIRECTIVE_BANK_HPP
