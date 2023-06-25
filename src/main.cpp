#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <queue>
#include <unistd.h>
#include <sys/wait.h>
#include <stdexcept>
constexpr auto CHILD_CREATION_FAILED = "Failed to create child process";
constexpr auto EXCECUTION_FAILED = "Failed to execute command";
constexpr auto COMMAND_SYMBOL = "> ";
constexpr auto SPACE = " ";
constexpr auto BACKGROUND_PROCESS = "&";
constexpr auto EXIT_COMMAND = "exit";
constexpr auto CMD_NOT_FOUND_OR_NO_PERMISSION = "Command not found or no permission";
constexpr auto COMMAND_NOT_FOUND = "Command not found";
constexpr auto HISTORY_FILE = "history.txt";


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

// ============================= FUNCTIONS =============================
/**
 * @brief Executes a command with the given arguments.
 * @param args The command and its arguments.
 * @param runInBackground Whether the command should be run in the background.
 */
std::vector<std::string> splitString(const std::string& str, const std::string& delimiters) {
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


/**
 * @brief Executes a command with the given arguments.
 * @param args The command and its arguments.
 * @param runInBackground Whether the command should be run in the background.
 */
void executeCommand(const std::vector<std::string>& args, bool runInBackground) {
    // Convert argument vector to array of C-style strings
    std::vector<char*> argv;
    for (const auto& arg : args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr);

    // Check if the command is "echo" and handle variable expansion
    if (args.size() > 1 && args[0] == "echo") {
        for (size_t i = 1; i < args.size(); ++i) {
            std::string expandedArg = expandVariables(args[i]);
            std::cout << expandedArg << " ";
        }
        std::cout << std::endl;
    } else {
        // Create child process
        pid_t pid = fork();
        if (pid == -1) {
            throw std::runtime_error(CHILD_CREATION_FAILED);
        } else if (pid == 0) {
            // Child process
         if (execvp(argv[0], argv.data()) == -1) {
        // Print the value of the variable if it exists
        char* varValue = getenv(args[0].c_str());
        if (varValue != nullptr) {
            std::cout << varValue << std::endl;
        } else {
            std::string command = args[0];
            if (command[0] == '$') {
                command = command.substr(1);
            }
            std::vector<std::string> newArgs = { "/bin/sh", "-c", command };
            executeCommand(newArgs, runInBackground);
        }
    }
        } else {
            // Parent process
            if (!runInBackground) {
                int status;
                waitpid(pid, &status, 0);
            }
        }
    }

    // Clean up argument array
    for (auto& arg : argv) {
        arg = nullptr;
    }
}

/*

void displayCommandHistory() {
  std::ifstream historyFile(HISTORY_FILE);
  std::string line;
  int count = 1;
  while (std::getline(historyFile, line)) {
    std::cout << count << ". " << line << std::endl;
    count++;
  }
}

// ============================= MAIN =============================
int main() {
  // Open command history file in append mode
  std::ofstream historyFile(HISTORY_FILE, std::ios::app);

  while (true) {
    try {
      std::string input;
      std::cout << COMMAND_SYMBOL;
      std::getline(std::cin, input);

      // Write command to history file
      historyFile << input << std::endl;

      // Tokenize user input
      std::vector<std::string> tokens = splitString(input, SPACE);

      // Check if command should be run in background
      bool runInBackground = false;
      if (!tokens.empty() && tokens.back() == BACKGROUND_PROCESS) {
        runInBackground = true;
        tokens.pop_back();
      }

      if (tokens[0] == "myhistory") {
        displayCommandHistory();
        continue;
      }

      // Check if command should expand environment variables
      if (!tokens.empty() && tokens[0].front() == '$') {
        tokens[0] = expandVariables(tokens[0]);
      }

      // Check if command is a file path
      if (!tokens.empty() && tokens[0].find('/') != std::string::npos) {
        std::string filePath = tokens[0];
        if (access(filePath.c_str(), X_OK) == -1) {
          throw std::runtime_error(CMD_NOT_FOUND_OR_NO_PERMISSION);
        }
        executeCommand(tokens, runInBackground);
      }
      else {
        // Search for command in PATH
        std::string pathEnv = std::getenv("PATH");
        std::vector<std::string> pathDirs = splitString(pathEnv, ":");
        bool commandFound = false;
        for (const auto& dir : pathDirs) {
          std::string commandPath = dir + "/" + tokens[0];
          if (access(commandPath.c_str(), X_OK) != -1) {
            tokens[0] = commandPath;
            executeCommand(tokens, runInBackground);
            commandFound = true;
            break;
          }
        }
        if (!commandFound) {
            std::string commandPath;
            if (tokens[0] == "$PATH") {
                commandPath = std::getenv("PATH");
                //std::cout << commandPath << std::endl;
            } else {
                throw std::runtime_error(COMMAND_NOT_FOUND);
            }
            tokens[0] = commandPath;
            executeCommand(tokens, runInBackground);
        }
      }
    }
    catch (const std::exception& e) {
      std::cerr << "Error: " << e.what() << std::endl;
    }
  }
  return EXIT_SUCCESS;
}

 */

int main() {
    // Open command history file in append mode
    std::ofstream historyFile(HISTORY_FILE, std::ios::app);

    // Create CommandFactory object
    CommandFactory commandFactory;

    while (true) {
        try {
            std::string input;

            // Read user input from standard input
            std::cout << "myshell> ";
            std::getline(std::cin, input);

            // Create Command object using CommandFactory
            std::unique_ptr<Command> command = commandFactory.createCommand(input);

            // Execute Command object directly
            command->execute();

            // Write user input to command history file
            historyFile << input << std::endl;

        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    return 0;
}
