//
//  NinjaCompilationDatabase.cpp
//
//  Created by Philippe Hausler on 9/6/14.
//  Copyright (c) 2014 Philippe Hausler. All rights reserved.
//

#include "NinjaCompilationDatabase.h"

std::string NinjaCompilationDatabase::nodePath(std::string path) const {
    if (path[0] == '/') {
        return path;
    } else {
        std::string absolute = base + "/" + path;
        char rpath[PATH_MAX];
        realpath(absolute.c_str(), rpath);
        return std::string(rpath);
    }
}

std::string NinjaCompilationDatabase::nodePath(ninja::Node *node) const {
    return nodePath(node->path());
}

static std::vector<std::string> splitArguments(std::string command) {
    std::vector<std::string> args;
    std::string arg = "";
    char quoteType = '\0';
    bool escaped = false;
    
    for (char c : command) {
        if (escaped) {
            escaped = false;
            arg += c;
        } else if (c == '\\') {
            escaped = true;
        } else if ((quoteType == '\0' && c == '\'') ||
                   (quoteType == '\0' && c == '"')) {
            quoteType = c;
        } else if ((quoteType == '\'' && c == '\'') ||
                   (quoteType == '"' && c == '"')) {
            quoteType = '\0';
        } else if (!isspace(c) || quoteType != '\0') {
            arg += c;
        } else {
            args.push_back(arg);
            arg = "";
        }
    }
    
    args.push_back(arg);
    
    return args;
}

std::vector<clang::tooling::CompileCommand> NinjaCompilationDatabase::getCompileCommands(llvm::StringRef FilePath) const {
    std::vector<clang::tooling::CompileCommand> commands;
    
    for (auto *edge : state.edges_) {
        for (auto *input : edge->inputs_) {
            if (nodePath(input) == FilePath.str()) {
                clang::tooling::CompileCommand command;
                command.Directory = base;
                
                const ninja::EvalString *commandStr = edge->rule_->GetBinding("command");
                if (commandStr != nullptr) {
                    std::string evaluatedCommand = edge->EvaluateCommand();
                    command.CommandLine = splitArguments(evaluatedCommand);
                    for (auto arg : defaultArgs) {
                        command.CommandLine.push_back(arg);
                    }
                }
                
                if (command.CommandLine.size() > 0) {
                    commands.push_back(command);
                }
                break;
            }
        }
        if (commands.size() > 0) {
            break;
        }
    }
    
    return commands;
}

std::vector<std::string> NinjaCompilationDatabase::getAllFiles() const {
    std::vector<std::string> files;
    std::set<std::string> paths;
    
    // add all sources
    for (auto *edge : state.edges_) {
        for (auto *input : edge->inputs_) {
            paths.insert(input->path());
        }
    }
    
    // remove all derived sources
    for (auto *edge : state.edges_) {
        for (auto *output : edge->outputs_) {
            paths.erase(output->path());
        }
    }
    
    // filter for compiled, non-derived clang compatible sources
    for (auto path : paths) {
        std::string ext = path.substr(path.find_last_of(".") + 1);
        if (!(ext == "c" ||
              ext == "C" ||
              ext == "m" ||
              ext == "mm" ||
              ext == "cpp" ||
              ext == "CPP" ||
              ext == "cc" ||
              ext == "CC")) {
            continue;
        }
        files.push_back(nodePath(path));
    }
    
    return files;
}

std::vector<clang::tooling::CompileCommand> NinjaCompilationDatabase::getAllCompileCommands() const {
    std::vector<clang::tooling::CompileCommand> commands;
    for (std::string file : getAllFiles()) {
        for (clang::tooling::CompileCommand command : getCompileCommands(file)) {
            commands.push_back(command);
        }
    }
    return commands;
}
