#pragma once

#include <memory>
#include "Command.h"


class CommandFactory {
public:

    // ============================= FUNCTIONS =============================
    /**
     * @brief Executes a command with the given arguments.
     * @param args The command and its arguments.
     * @param runInBackground Whether the command should be run in the background.
     */
    static std::vector<std::string> splitString(const std::string& str, const std::string& delimiters) {
        std::vector<std::string> tokens;
        std::size_t start = 0, end = 0;
        while ((end = str.find_first_of(delimiters, start)) != std::string::npos) {
            if (end != start) {
                tokens.push_back(str.substr(start, end - start));
            }
            start = end + 1;
        }
        if (start != str.length()) {
            tokens.push_back(str.substr(start));
        }
        return tokens;
    }

    std::unique_ptr<Command> createCommand(const std::string& input) {
        // Tokenize user input
        std::vector<std::string> tokens = splitString(input, SPACE);

        // Check if command should be run in background
        bool runInBackground = false;
        if (!tokens.empty() && tokens.back() == BACKGROUND_PROCESS) {
            runInBackground = true;
            tokens.pop_back();
        }

        if (tokens[0] == EXIT_COMMAND) {
            return std::make_unique<ExitCommand>();
        }
        else if (tokens[0] == "myhistory") {
            return std::make_unique<HistoryCommand>();
        }
        else if (tokens[0] == "echo") {
            return std::make_unique<EchoCommand>(tokens);
        }
        else {
            return std::make_unique<ExternalCommand>(tokens, runInBackground);
        }
    }
};