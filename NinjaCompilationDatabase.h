//
//  NinjaCompilationDatabase.h
//
//  Created by Philippe Hausler on 9/6/14.
//  Copyright (c) 2014 Philippe Hausler. All rights reserved.
//

#ifndef NINJA_COMPILATION_DATABASE_H_
#define NINJA_COMPILATION_DATABASE_H_

#include <unistd.h>
#include <libgen.h>
#include <sys/errno.h>
#include <stdio.h>
#include <ninja/build.h>
#include <ninja/manifest_parser.h>
#include <ninja/state.h>
#include <ninja/util.h>
#include <clang/Tooling/CompilationDatabase.h>

class NinjaCompilationDatabase : public clang::tooling::CompilationDatabase {
private:
    class FileReader : public ninja::ManifestParser::FileReader {
        virtual bool ReadFile(const string &path, string *contents, string *err) {
            FILE* f = fopen(path.c_str(), "r");
            if (!f) {
                err->assign(strerror(errno));
                return false;
            }
            
            char buf[64 << 10];
            size_t len;
            while ((len = fread(buf, 1, sizeof(buf), f)) > 0) {
                contents->append(buf, len);
            }
            if (ferror(f)) {
                err->assign(strerror(errno));
                contents->clear();
                fclose(f);
                return false;
            }
            fclose(f);
            return true;
        }
    };
    
    ninja::BuildConfig config;
    ninja::State state;
    ninja::ManifestParser parser;
    FileReader reader;
    std::string base;
    std::vector<std::string> defaultArgs;
    
    std::string nodePath(std::string path) const;
    std::string nodePath(ninja::Node *node) const;
    
public:
    NinjaCompilationDatabase(std::string input, std::vector<std::string> defaultArguments) :
        parser(&state, &reader),
        defaultArgs(defaultArguments) {
        config.dry_run = true;
        parser.Load(input, nullptr);
        base = ::dirname((char *)input.c_str());
    }
    
    std::vector<clang::tooling::CompileCommand> getCompileCommands(llvm::StringRef FilePath) const;
    
    std::vector<std::string> getAllFiles() const;
    
    std::vector<clang::tooling::CompileCommand> getAllCompileCommands() const;
};

#endif /* NINJA_COMPILATION_DATABASE_H_ */
