#include "languagemanager.h"
#include <QRegularExpression>

LanguageManager &LanguageManager::instance()
{
    static LanguageManager instance;
    return instance;
}

LanguageManager::LanguageManager()
{
    initMappings();
}

void LanguageManager::initMappings()
{
    // C/C++
    m_extensionToLanguage["c"] = "C";
    m_extensionToLanguage["h"] = "C";
    m_extensionToLanguage["cpp"] = "C++";
    m_extensionToLanguage["cxx"] = "C++";
    m_extensionToLanguage["cc"] = "C++";
    m_extensionToLanguage["hpp"] = "C++";
    m_extensionToLanguage["hxx"] = "C++";
    m_extensionToLanguage["hh"] = "C++";

    // Python
    m_extensionToLanguage["py"] = "Python";
    m_extensionToLanguage["pyw"] = "Python";
    m_extensionToLanguage["pyx"] = "Python";
    m_extensionToLanguage["pyi"] = "Python";

    // JavaScript/TypeScript
    m_extensionToLanguage["js"] = "JavaScript";
    m_extensionToLanguage["mjs"] = "JavaScript";
    m_extensionToLanguage["cjs"] = "JavaScript";
    m_extensionToLanguage["jsx"] = "JavaScript";
    m_extensionToLanguage["ts"] = "TypeScript";
    m_extensionToLanguage["tsx"] = "TypeScript";

    // Web
    m_extensionToLanguage["html"] = "HTML";
    m_extensionToLanguage["htm"] = "HTML";
    m_extensionToLanguage["xhtml"] = "HTML";
    m_extensionToLanguage["css"] = "CSS";
    m_extensionToLanguage["scss"] = "SCSS";
    m_extensionToLanguage["sass"] = "SASS";
    m_extensionToLanguage["less"] = "LESS";

    // Data formats
    m_extensionToLanguage["json"] = "JSON";
    m_extensionToLanguage["yaml"] = "YAML";
    m_extensionToLanguage["yml"] = "YAML";
    m_extensionToLanguage["xml"] = "XML";
    m_extensionToLanguage["toml"] = "TOML";
    m_extensionToLanguage["ini"] = "INI";
    m_extensionToLanguage["cfg"] = "INI";
    m_extensionToLanguage["conf"] = "INI";

    // Markdown
    m_extensionToLanguage["md"] = "Markdown";
    m_extensionToLanguage["markdown"] = "Markdown";
    m_extensionToLanguage["mdown"] = "Markdown";

    // Shell
    m_extensionToLanguage["sh"] = "Bash";
    m_extensionToLanguage["bash"] = "Bash";
    m_extensionToLanguage["zsh"] = "Bash";
    m_extensionToLanguage["fish"] = "Fish";
    m_extensionToLanguage["ps1"] = "PowerShell";
    m_extensionToLanguage["psm1"] = "PowerShell";

    // Systems languages
    m_extensionToLanguage["rs"] = "Rust";
    m_extensionToLanguage["go"] = "Go";
    m_extensionToLanguage["asm"] = "Assembly";
    m_extensionToLanguage["s"] = "Assembly";

    // JVM languages
    m_extensionToLanguage["java"] = "Java";
    m_extensionToLanguage["kt"] = "Kotlin";
    m_extensionToLanguage["kts"] = "Kotlin";
    m_extensionToLanguage["scala"] = "Scala";
    m_extensionToLanguage["groovy"] = "Groovy";
    m_extensionToLanguage["gradle"] = "Groovy";

    // .NET languages
    m_extensionToLanguage["cs"] = "C#";
    m_extensionToLanguage["fs"] = "F#";
    m_extensionToLanguage["vb"] = "Visual Basic";

    // Ruby
    m_extensionToLanguage["rb"] = "Ruby";
    m_extensionToLanguage["erb"] = "Ruby";
    m_extensionToLanguage["rake"] = "Ruby";
    m_extensionToLanguage["gemspec"] = "Ruby";

    // PHP
    m_extensionToLanguage["php"] = "PHP";
    m_extensionToLanguage["phtml"] = "PHP";

    // Perl
    m_extensionToLanguage["pl"] = "Perl";
    m_extensionToLanguage["pm"] = "Perl";

    // Lua
    m_extensionToLanguage["lua"] = "Lua";

    // SQL
    m_extensionToLanguage["sql"] = "SQL";

    // LaTeX
    m_extensionToLanguage["tex"] = "LaTeX";
    m_extensionToLanguage["latex"] = "LaTeX";
    m_extensionToLanguage["sty"] = "LaTeX";

    // Functional languages
    m_extensionToLanguage["hs"] = "Haskell";
    m_extensionToLanguage["lhs"] = "Haskell";
    m_extensionToLanguage["ml"] = "OCaml";
    m_extensionToLanguage["mli"] = "OCaml";
    m_extensionToLanguage["erl"] = "Erlang";
    m_extensionToLanguage["ex"] = "Elixir";
    m_extensionToLanguage["exs"] = "Elixir";

    // Mobile
    m_extensionToLanguage["swift"] = "Swift";
    m_extensionToLanguage["m"] = "Objective-C";
    m_extensionToLanguage["mm"] = "Objective-C++";
    m_extensionToLanguage["dart"] = "Dart";

    // Config files
    m_extensionToLanguage["dockerfile"] = "Dockerfile";
    m_extensionToLanguage["nginx"] = "Nginx";
    m_extensionToLanguage["csv"] = "CSV";

    // Systems languages (additional)
    m_extensionToLanguage["d"] = "D";
    m_extensionToLanguage["di"] = "D";
    m_extensionToLanguage["zig"] = "Zig";
    m_extensionToLanguage["nim"] = "Nim";
    m_extensionToLanguage["nims"] = "Nim";
    m_extensionToLanguage["nimble"] = "Nim";
    m_extensionToLanguage["f90"] = "Fortran";
    m_extensionToLanguage["f95"] = "Fortran";
    m_extensionToLanguage["f03"] = "Fortran";
    m_extensionToLanguage["f08"] = "Fortran";
    m_extensionToLanguage["f"] = "Fortran";
    m_extensionToLanguage["for"] = "Fortran";

    // Scientific
    m_extensionToLanguage["r"] = "R";
    m_extensionToLanguage["R"] = "R";
    m_extensionToLanguage["rmd"] = "R";
    m_extensionToLanguage["jl"] = "Julia";

    // Legacy
    m_extensionToLanguage["cob"] = "COBOL";
    m_extensionToLanguage["cbl"] = "COBOL";
    m_extensionToLanguage["cpy"] = "COBOL";
    m_extensionToLanguage["pas"] = "Pascal";
    m_extensionToLanguage["pp"] = "Pascal";
    m_extensionToLanguage["lpr"] = "Pascal";
    m_extensionToLanguage["dpr"] = "Pascal";
    m_extensionToLanguage["adb"] = "Ada";
    m_extensionToLanguage["ads"] = "Ada";

    // Hardware description
    m_extensionToLanguage["vhd"] = "VHDL";
    m_extensionToLanguage["vhdl"] = "VHDL";

    // Scripting (additional)
    m_extensionToLanguage["tcl"] = "Tcl";
    m_extensionToLanguage["tk"] = "Tcl";

    // Functional (additional)
    m_extensionToLanguage["scm"] = "Scheme";
    m_extensionToLanguage["ss"] = "Scheme";
    m_extensionToLanguage["rkt"] = "Scheme";
    m_extensionToLanguage["cl"] = "Lisp";
    m_extensionToLanguage["lisp"] = "Lisp";
    m_extensionToLanguage["lsp"] = "Lisp";
    m_extensionToLanguage["asd"] = "Lisp";
    m_extensionToLanguage["clj"] = "Clojure";
    m_extensionToLanguage["cljs"] = "Clojure";
    m_extensionToLanguage["cljc"] = "Clojure";
    m_extensionToLanguage["edn"] = "Clojure";
    m_extensionToLanguage["fsx"] = "F#";
    m_extensionToLanguage["fsi"] = "F#";

    // Other
    m_extensionToLanguage["pro"] = "Prolog";
    m_extensionToLanguage["vim"] = "Vim";
    m_extensionToLanguage["vimrc"] = "Vim";
    m_extensionToLanguage["pgsql"] = "PostgreSQL";
    m_extensionToLanguage["rst"] = "reStructuredText";
    m_extensionToLanguage["rest"] = "reStructuredText";
    m_extensionToLanguage["graphql"] = "GraphQL";
    m_extensionToLanguage["gql"] = "GraphQL";
    m_extensionToLanguage["proto"] = "Protocol Buffers";
    m_extensionToLanguage["tf"] = "Terraform";
    m_extensionToLanguage["tfvars"] = "Terraform";
    m_extensionToLanguage["hcl"] = "HCL";
    m_extensionToLanguage["sol"] = "Solidity";
    m_extensionToLanguage["wat"] = "WebAssembly";
    m_extensionToLanguage["wast"] = "WebAssembly";
    m_extensionToLanguage["vert"] = "GLSL";
    m_extensionToLanguage["frag"] = "GLSL";
    m_extensionToLanguage["geom"] = "GLSL";
    m_extensionToLanguage["comp"] = "GLSL";
    m_extensionToLanguage["glsl"] = "GLSL";
    m_extensionToLanguage["cu"] = "CUDA";
    m_extensionToLanguage["cuh"] = "CUDA";
    m_extensionToLanguage["coffee"] = "CoffeeScript";
    m_extensionToLanguage["cr"] = "Crystal";
    m_extensionToLanguage["v"] = "Verilog";
    m_extensionToLanguage["sv"] = "Verilog";

    // Plain text
    m_extensionToLanguage["txt"] = "Plain Text";
    m_extensionToLanguage["text"] = "Plain Text";
    m_extensionToLanguage["log"] = "Plain Text";

    // Build reverse mapping
    for (auto it = m_extensionToLanguage.begin(); it != m_extensionToLanguage.end(); ++it) {
        m_languageToExtensions[it.value()].append(it.key());
    }

    // Special filenames
    m_filenameToLanguage["Makefile"] = "Makefile";
    m_filenameToLanguage["makefile"] = "Makefile";
    m_filenameToLanguage["GNUmakefile"] = "Makefile";
    m_filenameToLanguage["CMakeLists.txt"] = "CMake";
    m_filenameToLanguage["Dockerfile"] = "Dockerfile";
    m_filenameToLanguage["docker-compose.yml"] = "YAML";
    m_filenameToLanguage["docker-compose.yaml"] = "YAML";
    m_filenameToLanguage[".gitignore"] = "Git";
    m_filenameToLanguage[".gitattributes"] = "Git";
    m_filenameToLanguage[".gitmodules"] = "Git";
    m_filenameToLanguage["Gemfile"] = "Ruby";
    m_filenameToLanguage["Rakefile"] = "Ruby";
    m_filenameToLanguage["package.json"] = "JSON";
    m_filenameToLanguage["tsconfig.json"] = "JSON";
    m_filenameToLanguage["Cargo.toml"] = "TOML";
    m_filenameToLanguage["go.mod"] = "Go";
    m_filenameToLanguage["go.sum"] = "Go";

    // Shebang mappings
    m_shebangToLanguage["python"] = "Python";
    m_shebangToLanguage["python3"] = "Python";
    m_shebangToLanguage["python2"] = "Python";
    m_shebangToLanguage["node"] = "JavaScript";
    m_shebangToLanguage["nodejs"] = "JavaScript";
    m_shebangToLanguage["bash"] = "Bash";
    m_shebangToLanguage["sh"] = "Bash";
    m_shebangToLanguage["zsh"] = "Bash";
    m_shebangToLanguage["ruby"] = "Ruby";
    m_shebangToLanguage["perl"] = "Perl";
    m_shebangToLanguage["php"] = "PHP";
    m_shebangToLanguage["lua"] = "Lua";
}

QString LanguageManager::languageForExtension(const QString &extension) const
{
    QString ext = extension.toLower();
    if (ext.startsWith('.')) {
        ext = ext.mid(1);
    }
    return m_extensionToLanguage.value(ext, "Plain Text");
}

QString LanguageManager::languageForFilename(const QString &filename) const
{
    // Check exact filename match first
    if (m_filenameToLanguage.contains(filename)) {
        return m_filenameToLanguage.value(filename);
    }

    // Try extension
    int dotIndex = filename.lastIndexOf('.');
    if (dotIndex > 0) {
        QString ext = filename.mid(dotIndex + 1);
        return languageForExtension(ext);
    }

    return "Plain Text";
}

QString LanguageManager::languageForShebang(const QString &shebang) const
{
    if (!shebang.startsWith("#!")) {
        return QString();
    }

    QString line = shebang.mid(2).trimmed();

    // Handle env
    if (line.startsWith("/usr/bin/env ")) {
        line = line.mid(13).trimmed();
    } else {
        // Get just the program name
        int spaceIndex = line.indexOf(' ');
        if (spaceIndex > 0) {
            line = line.left(spaceIndex);
        }
        int slashIndex = line.lastIndexOf('/');
        if (slashIndex >= 0) {
            line = line.mid(slashIndex + 1);
        }
    }

    // Get first word (program name)
    int spaceIndex = line.indexOf(' ');
    if (spaceIndex > 0) {
        line = line.left(spaceIndex);
    }

    return m_shebangToLanguage.value(line, QString());
}

QStringList LanguageManager::availableLanguages() const
{
    return m_languageToExtensions.keys();
}

QStringList LanguageManager::extensionsForLanguage(const QString &language) const
{
    return m_languageToExtensions.value(language);
}

QString LanguageManager::detectLanguage(const QString &filename, const QString &content) const
{
    // First try filename
    QString lang = languageForFilename(filename);
    if (lang != "Plain Text") {
        return lang;
    }

    // Try shebang
    if (!content.isEmpty()) {
        QString firstLine = content.section('\n', 0, 0);
        lang = languageForShebang(firstLine);
        if (!lang.isEmpty()) {
            return lang;
        }

        // Content heuristics
        if (content.contains("<!DOCTYPE html>") || content.contains("<html")) {
            return "HTML";
        }
        if (content.contains("<?php")) {
            return "PHP";
        }
        if (content.startsWith("{") && content.contains(":")) {
            return "JSON";
        }
    }

    return "Plain Text";
}

QString LanguageManager::definitionPath(const QString &language) const
{
    return ":/syntax/" + language.toLower() + ".json";
}
