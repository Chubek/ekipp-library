# LibEKIPP: Header-Only C Library for Preprocessing

LibEKIPP is a header-only C library that offers preprocessing to the host. You include `ekipp.hpp` in your project, and now you have a configurable generic macro preprocessor to use in your project.

Let's demonstrate with an example:

```cpp
// LibEKIPP: Header-Only C++ Library for Preprocessing

// LibEKIPP is a header-only C++ library that offers preprocessing to the host.
// You include `ekipp.hpp` in your project, and now you have a configurable
// generic macro preprocessor to use in your project.

#include <ekipp.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdexcept>

namespace fs = std::filesystem;

class IncludeFooDirective : public ekipp::Directive<ekipp::IODirective>
{
private:
    ekipp::Configuration cnf;
    ekipp::Directive dir;

public:
    IncludeFooDirective() {
        cnf = ekipp::Config::create();
        dir = ekipp::Directive::create(cnf);
        
        dir << ekipp::Directive::Name("Include")
            << ekipp::Directive::Prefix('@')
            << ekipp::Directive::LeftBracket('(')
            << ekipp::Directive::RightBracket(')')
            << ekipp::Directive::NumParams(1)
            << ekipp::Directive::IO("r")
            << ekipp::Directive::Semantics([this](ekipp::Arguments& args) {
                std::vector<fs::path> incl_dirs = cnf.params.include_dirs;
                fs::path incl_path = args.get<fs::path>(0);
                
                // Search for the file in include directories
                fs::path resolved_path;
                bool found = false;
                
                // Try relative path first
                if (fs::exists(incl_path)) {
                    resolved_path = incl_path;
                    found = true;
                } else {
                    // Search in include directories
                    for (const auto& dir : incl_dirs) {
                        fs::path candidate = dir / incl_path;
                        if (fs::exists(candidate)) {
                            resolved_path = candidate;
                            found = true;
                            break;
                        }
                    }
                }
                
                if (!found) {
                    throw std::runtime_error("Include file not found: " + incl_path.string());
                }
                
                // Read and return file contents
                std::ifstream file(resolved_path);
                if (!file.is_open()) {
                    throw std::runtime_error("Cannot open file: " + resolved_path.string());
                }
                
                std::stringstream buffer;
                buffer << file.rdbuf();
                return buffer.str();
            });
    }
    
    ekipp::Directive& getDirective() {
        return dir;
    }
    
    void addIncludeDir(const fs::path& path) {
        cnf.params.include_dirs.push_back(path);
    }
};

// Usage example
int main() {
    IncludeFooDirective includeDir;
    
    // Add include directories
    includeDir.addIncludeDir("/usr/local/include");
    includeDir.addIncludeDir("./includes");
    
    // Process a file with @Include(filename) directives
    ekipp::Preprocessor preprocessor;
    preprocessor.registerDirective(includeDir.getDirective());
    
    std::string input = R"(
        Some text here
        @Include(header.txt)
        More text
        @Include(footer.txt)
    )";
    
    std::string output = preprocessor.process(input);
    std::cout << output << std::endl;
    
    return 0;
}

```
