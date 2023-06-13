#pragma once
#include <string>
#include <vector>
#include <iostream>
#include "constants.h"
#include <fstream>
#include <queue>
#include <unistd.h>
#include <sys/wait.h>
#include <stdexcept>



class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    /***
    * Expands environment variables in a string to their values.
    * @param str The string to expand.
    * @return The expanded string.
    */
    std::string expandVariables(const std::string& str) {
        std::string result;
        std::size_t start = 0, end = 0;
        while ((end = str.find_first_of("$}", start)) != std::string::npos) {
            if (str[start] == '$' && end > start + 1) {
                std::string varName = str.substr(start + 1, end - start - 1);
                char* varValue = getenv(varName.c_str());
                if (varValue != nullptr) {
                    result += varValue;
                }
                start = end + 1;
            }
            else if (str[start] == '{' && end > start + 1 && str[end - 1] == '$') {
                std::string varName = str.substr(start + 2, end - start - 2);
                char* varValue = getenv(varName.c_str());
                if (varValue != nullptr) {
                    result += varValue;
                }
                start = end + 1;
            }
            else {
                result += str.substr(start, end - start + 1);
                start = end + 1;
            }
        }
        if (start != str.length()) {
            result += str.substr(start);
        }
        return result;
    }

};

class EchoCommand : public Command {
private:
    std::vector<std::string> args;
public:
    EchoCommand(const std::vector<std::string>& args) : args(args) {}
    void execute() override {
        for (size_t i = 1; i < args.size(); ++i) {
            std::string expandedArg = expandVariables(args[i]);
            std::cout << expandedArg << " ";
        }
        std::cout << std::endl;
    }
};

class ExternalCommand : public Command {
private:
    std::vector<std::string> args;
    bool runInBackground;
public:
    ExternalCommand(const std::vector<std::string>& args, bool runInBackground) : args(args), runInBackground(runInBackground) {}
    void execute() override {
        // Convert argument vector to array of C-style strings
        std::vector<char*> argv;
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);

        // Create child process
        pid_t pid = fork();
        if (pid == -1) {
            throw std::runtime_error(CHILD_CREATION_FAILED);
        }
        else if (pid == 0) {
            // Child process
            if (execvp(argv[0], argv.data()) == -1) {
                // Print the value of the variable if it exists
                char* varValue = getenv(args[0].c_str());
                if (varValue != nullptr) {
                    std::cout << varValue << std::endl;
                }
                else {
                    std::string command = args[0];
                    if (command[0] == '$') {
                        command = command.substr(1);
                    }
                    std::vector<std::string> newArgs = { "/bin/sh", "-c", command };
                    ExternalCommand(newArgs, runInBackground).execute();
                }
            }
        }
        else {
            // Parent process
            if (!runInBackground) {
                int status;
                waitpid(pid, &status, 0);
            }
        }

        // Clean up argument array
        for (auto& arg : argv) {
            arg = nullptr;
        }
    }
};

class ExitCommand : public Command {
public:
    void execute() override {
        exit(0);
    }
};

class HistoryCommand : public Command {
public:
    void execute() override {
        std::ifstream historyFile(HISTORY_FILE);
        std::string line;
        int count = 1;
        while (std::getline(historyFile, line)) {
            std::cout << count << ". " << line << std::endl;
            count++;
        }
    }
};

class Invoker {
public:
    void executeCommand(std::unique_ptr<Command> command) {
        // Add command to queue
        commandQueue.push(std::move(command));

        // Execute commands in queue
        while (!commandQueue.empty()) {
            std::unique_ptr<Command>& frontCommand = commandQueue.front();
            frontCommand->execute();
            commandQueue.pop();
        }
    }

private:
    std::queue<std::unique_ptr<Command>> commandQueue;
};
