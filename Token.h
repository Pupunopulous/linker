#ifndef TOKEN_H
#define TOKEN_H

#include <string>
#include <vector>
#include <fstream>
#include <cstring>

using namespace std;


// Class representing one token
class Token {
public:
    int lineNumber;
    int lineOffset;
    string tokenContents;

    void setToken(int lineNumber, int lineOffset, string tokenContents) {
        this -> lineNumber = lineNumber;
        this -> lineOffset = lineOffset;
        this -> tokenContents = tokenContents;
    }
};


// Class representing one symbol
class Symbol {
public:
    string value;
    int Addr;
    int relativeAddr;
    int moduleNumber;
    bool alreadyDefined = false;
    bool used = false;
};


// Class representing one module (module_base)
class Module {
public:
    int moduleBaseAddr;
    int moduleSize;
};


// Class implementing the logic for the tokenizer
class Tokenizer {
private:
    ifstream file;
    string currentLine;
    size_t lineOffset;
    size_t lineNumber;
    char* nextToken;
    vector<char> lineBuffer;

    size_t previousTokenLength = 0;
    size_t previousTokenLine = 0;
    bool flagEOF = false;

    // Function to prepare the next token from the current or next line
    void prepareNextToken() {
        // If nextToken is null or points to the end of a string, read the next line
        while (nextToken == nullptr || *nextToken == '\0') {
            if (!getline(file, currentLine)) {
                nextToken = nullptr; // End of file reached, no more tokens
                return;
            }
            lineNumber++;
            lineOffset = 1;
            lineBuffer.assign(currentLine.begin(), currentLine.end());
            // Ensure null-termination for string operations
            lineBuffer.push_back('\0');
            nextToken = strtok(lineBuffer.data(), " \t");
        }
    }

public:
    Tokenizer() : lineOffset(1), lineNumber(1), nextToken(nullptr) {}

    bool openFile(const string& filePath) {
        file.open(filePath);
        return file.is_open();
    }

    Token getNextToken() {
        // Prepare the next token if necessary
        prepareNextToken();
        
        Token token;
        if (nextToken == nullptr) {
            flagEOF = true;
            if (lineNumber - 1 > previousTokenLine) {
                token.setToken(lineNumber - 1, lineOffset, "");
            } else {
                token.setToken(lineNumber - 1, lineOffset + previousTokenLength, "");
            }
            // All tokens exhausted
            return token;
        }

        int currentTokenLength = strlen(nextToken);
        // Store the current token
        string tokenValue(nextToken);
        previousTokenLength = tokenValue.length();
        previousTokenLine = lineNumber - 1;
        token.setToken(lineNumber - 1, lineOffset, tokenValue);

        // Update lineOffset for the next token
        char* remainingLine = strtok(nullptr, "");
        nextToken = strtok(remainingLine, " \t");
        if (nextToken != nullptr) {
            char* nextTokenPosition = strstr(remainingLine, nextToken);
            lineOffset += currentTokenLength + strlen(remainingLine) - strlen(nextTokenPosition) + 1;
        }

        return token;
    }

    Token getLastToken() {
        Token token, lastToken;
        lastToken.setToken(1, 1, "");
        // Get the first token
        token = getNextToken();

        // Continue until no more tokens are available
        while (!token.tokenContents.empty() && !flagEOF) {
            lastToken = token;
            // Get the next token
            token = getNextToken();
        }

        // Return the last token encountered
        return lastToken;
    }

    ~Tokenizer() {
        if (file.is_open()) {
            file.close();
        }
    }
};

#endif // TOKEN_H
