#include "Parser.h" 
#include <iostream> 
#include <iomanip>  
#include <fstream>  
#include <vector>   
#include <cctype>   
#include <string>   
#include <cstring>  
#include <algorithm>

using namespace std;

vector<Module> module_base; // Vector to store the module_base table


// Function to handle parse errors
void __parseerror(int errcode, Token token) {
    static const char* errors[] = {
        "NUM_EXPECTED",            
        "SYM_EXPECTED",            
        "MARIE_EXPECTED",          
        "SYM_TOO_LONG",            
        "TOO_MANY_DEF_IN_MODULE",  
        "TOO_MANY_USE_IN_MODULE",  
        "TOO_MANY_INSTR",          
    };
    cout << "Parse Error line " << token.lineNumber << " offset " << token.lineOffset << ": " << errors[errcode] << endl;
    exit(1);
}


// Function to read an integer from a token, with error handling for invalid or out-of-range inputs
int readInteger(Token token) {
    try {
        return stoi(token.tokenContents);
    } catch(const invalid_argument& e) {
        __parseerror(0, token);
    } catch(const out_of_range& e) {
        __parseerror(0, token);
    }
    return 0;
}


// Function to read and validate a MARIE symbol from a token
string readMARIE(Token token) {
    string currentSymbol = token.tokenContents;
    if (currentSymbol.length() > 1 || (
        currentSymbol[0] != 'M' && 
        currentSymbol[0] != 'A' && 
        currentSymbol[0] != 'R' && 
        currentSymbol[0] != 'I' && 
        currentSymbol[0] != 'E')) {
        __parseerror(2, token); // Error if not a valid MARIE symbol
    }
    return currentSymbol;
}


// Function to read and validate a symbol from a token
string readSymbol(Token token) {
    string currentSymbol = token.tokenContents;
    if (currentSymbol.empty()) {
        __parseerror(1, token);
    }

    // Check first character is alphabetic and remaining characters are alphanumeric
    for (int i = 0; currentSymbol[i]; i++) {
        if ((i == 0 && !isalpha(currentSymbol[i])) || !isalnum(currentSymbol[i])) 
            __parseerror(1, token);
    }

    // Error if symbol length exceeds 16 characters
    if (currentSymbol.size() > 16) {
        __parseerror(3, token);
    }
    return currentSymbol;
}


// Function representing the first pass of the two-pass linker, processes definitions and uses
vector<Symbol> firstPass(string fileName) {
    // Create a tokenizer object
    Tokenizer tokenizer;
    // Exit if file cannot be opened
    if (!tokenizer.openFile(fileName)) { // Attempt to open the specified file
        cout << "Unable to open file " << fileName << endl;
        exit(0);
    }

    string line;
    int totalInstructions = 0;

    vector<Symbol> symbols;     // Vector to store distinct symbols found in the first pass 
    vector<Symbol> defList;     // Vector to store all symbol definitions

    int baseAddress = 0;        // Starting address for the current module
    int moduleNumber = 0;       // Counter for the current module number
    
    Token currentToken = tokenizer.getNextToken();  // Get the first token
    while (!currentToken.tokenContents.empty()) {   // Continue until there are no more tokens
        moduleNumber++;
        // Number of definitions in the current module
        int definitionCount;

        // Read the definition count and move to the next token
        try {     
            definitionCount = readInteger(currentToken);
        } catch(exception E) {
            Token lastToken = tokenizer.getLastToken();
            __parseerror(0, lastToken);
        }

        // Error if the number of definitions exceeds 16
        if (definitionCount > 16) {
            __parseerror(4, currentToken);
        }
        currentToken = tokenizer.getNextToken();

        // Process each definition
        for (int i = 0; i < definitionCount; i++) {
            Symbol currentSymbol;
            // Read the symbol and move to the next token
            try {
                currentSymbol.value = readSymbol(currentToken);
                currentToken = tokenizer.getNextToken();
            }
            catch(exception E) {
                Token lastToken = tokenizer.getLastToken();
                __parseerror(1, lastToken);
            }

            // Read the symbol address, add the base address, and move to the next token
            try {
                currentSymbol.relativeAddr = readInteger(currentToken);
                currentSymbol.Addr = currentSymbol.relativeAddr + baseAddress;
                currentSymbol.moduleNumber = moduleNumber;
                currentToken = tokenizer.getNextToken();
            }
            catch(exception E) {
                Token lastToken = tokenizer.getLastToken();
                __parseerror(0, lastToken);   
            }

            // Check if the symbol is already defined
            bool flag = false;
            for (int j = 0; j < symbols.size(); j++) {
                if (symbols[j].value.compare(currentSymbol.value) == 0) {
                    symbols[j].alreadyDefined = true;
                    flag = true;
                }
            }
            // If the symbol is not already defined, add it to the symbols vector
            if (!flag) symbols.push_back(currentSymbol);

            // Update the defList similarly
            flag = false;
            for (int j = 0; j < defList.size(); j++) {
                if (defList[j].value.compare(currentSymbol.value) == 0) {
                    flag = true;
                    break;
                }
            }
            defList.push_back(currentSymbol);
            if (flag) defList.back().alreadyDefined = true;
        }

        // Read the use count and move to the next token
        int useCount;
        try {
            useCount = readInteger(currentToken);
        } catch(exception E) {
            Token lastToken = tokenizer.getLastToken();
            __parseerror(0, lastToken);
        }

        // Error if the number of uses exceeds 16
        if (useCount > 16) {
            __parseerror(5, currentToken);
        }
        currentToken = tokenizer.getNextToken();
        
        // Skip past the uses, as they're not processed in the first pass
        for (int i = 0; i < useCount; i++) {
            try {
                readSymbol(currentToken);
                currentToken = tokenizer.getNextToken();
            } catch(exception E) {
                Token lastToken = tokenizer.getLastToken();
                __parseerror(1, lastToken);
            }
        }

        // Read the instruction count for the current module and update the total instruction count
        int instructionCount;
        Token instructionToken; // Token to hold the current instruction
        try {
            // Update the total instructions with the count for this module
            instructionToken = currentToken;
            instructionCount = readInteger(instructionToken);
            totalInstructions += instructionCount;
            // Move to the next token
            currentToken = tokenizer.getNextToken();
        } catch(const exception& e) {
            Token lastToken = tokenizer.getLastToken();
            __parseerror(0, lastToken);
        }

        // Error if the total number of instructions exceeds 512
        if (totalInstructions > 512) {
            __parseerror(6, instructionToken);
        }

        // Skip past the instructions, as they're not processed in the first pass
        for (int i = 0; i < instructionCount; i++) {
            try {
                // Read and discard each instruction type
                readMARIE(currentToken);
                currentToken = tokenizer.getNextToken();
            } catch(const exception& e) {
                Token lastToken = tokenizer.getLastToken();
                __parseerror(2, lastToken);
            }
            
            try {
                // Read and discard each instruction operand
                readInteger(currentToken);
                currentToken = tokenizer.getNextToken();
            } catch(const exception& e) {
                Token lastToken = tokenizer.getLastToken();
                __parseerror(0, lastToken);
            }
        }

        Module currentModule;
        currentModule.moduleBaseAddr = baseAddress;
        currentModule.moduleSize = instructionCount;
        module_base.push_back(currentModule);

        // Update the base address for the next module
        baseAddress += instructionCount;
    }

    // Check if the symbol is already defined, set the flag if so
    for (int i = 0; i < defList.size(); i++) {
        int index = -1;
        for (int j = 0; j < symbols.size(); j++) {
            if (defList[i].value.compare(symbols[j].value) == 0) {
                index = j;
                break;
            }
        }

        // Check if the symbol's relative address exceeds the size of its module.
        if (defList[i].relativeAddr > module_base[defList[i].moduleNumber - 1].moduleSize - 1 && !defList[i].alreadyDefined) { 
            if (symbols[index].moduleNumber > 1) {
                symbols[index].Addr -= module_base[symbols[index].moduleNumber - 1].moduleBaseAddr; 
                defList[i].Addr -= module_base[defList[i].moduleNumber - 1].moduleBaseAddr;
            }
            // Print a warning for symbols with invalid relative addresses, assuming a zero relative address.
            cout << "Warning: Module " << defList[i].moduleNumber - 1 << ": " 
                 << defList[i].value << "=" << defList[i].Addr 
                 << " valid=[0.." << module_base[defList[i].moduleNumber - 1].moduleSize  - 1 << "] assume zero relative\n";

            // Reset the symbol's address to the base address of its module.
            symbols[index].Addr = module_base[symbols[index].moduleNumber - 1].moduleBaseAddr;
            defList[i].Addr = module_base[defList[i].moduleNumber - 1].moduleBaseAddr;
        }

        // Print a warning if the symbol is redefined.
        if (defList[i].alreadyDefined) cout << "Warning: Module " << defList[i].moduleNumber - 1 << ": " << defList[i].value << " redefinition ignored\n";
    }

    // Return the vector of symbols found in the first pass
    return symbols;
}


// Function representing the second pass of the two-pass linker, generates the memory map
vector<Symbol> secondPass(string fileName, vector<Symbol> symbolTable) {
    // Create a tokenizer object
    Tokenizer tokenizer;
    // Exit if file cannot be opened
    if (!tokenizer.openFile(fileName)) {
        cout << "Unable to open file " << fileName << endl;
        exit(0); 
    }

    string line;
    // Vector to store symbols found in the second pass
    vector<Symbol> symbols;

    int baseAddress = 0;    // Starting address for the current module
    int moduleNumber = 0;   // Counter for the current module number
    int memoryMapIndex = 0;      // Counter for the memory map entries

    Token currentToken = tokenizer.getNextToken();  // Get the first token
    // Continue until there are no more tokens   
    while (!currentToken.tokenContents.empty()) {
        moduleNumber++;
        // Store the number of definitions in the current module 
        int definitionCount;

        try {
            // Read the definition count
            definitionCount = readInteger(currentToken);
            currentToken = tokenizer.getNextToken();
        } catch(exception E) {
             Token lastToken = tokenizer.getLastToken();
            __parseerror(0, lastToken);
        }

        // Skip past the definitions, as they're not processed in the second pass
        for (int i = 0; i < 2 * definitionCount; i++) currentToken = tokenizer.getNextToken();

        
        // Read the use count and move to the next token, handling any exceptions
        int useCount;
        try {
            // Read the use count
            useCount = readInteger(currentToken);
            currentToken = tokenizer.getNextToken();
        } catch(const exception& e) {
            Token lastToken = tokenizer.getLastToken();
            __parseerror(0, lastToken);
        }

        // Process each use symbol and mark corresponding symbols in the symbol table as used
        vector<string> externalSymbols;     // Vector to store external symbols used in the module
        vector<int> externalReferences;     // Vector to store external symbol references
        for(int i = 0; i < useCount; i++) {
            try {
                // Read the external symbol and add it to the vector
                string currentSymbol = readSymbol(currentToken);
                externalSymbols.push_back(currentSymbol);
                currentToken = tokenizer.getNextToken();
            }
            catch(exception E) {
                Token lastToken = tokenizer.getLastToken();
                __parseerror(1, lastToken);
            }
        }

        // Read the instruction count for the current module and update the instruction indexing
        int instructionCount;
        Token instructionToken; // Token to hold the current instruction
        try {
            // Read the instruction count
            instructionToken = currentToken;
            instructionCount = readInteger(instructionToken);
            currentToken = tokenizer.getNextToken();
        } catch(const exception& e) {
            Token lastToken = tokenizer.getLastToken();
            __parseerror(0, lastToken);
        }

        // Error if the instruction count for the module exceeds 512
        if (instructionCount > 512) {
            __parseerror(6, instructionToken);
        }

        // Initialize variables to handle errors and error messages
        bool errorExists = false;
        string errorString = "";

        // Process each instruction in the current module
        for(int i = 0; i < instructionCount; i++) {   
            string addressMode;     // Variable to store the address mode of the instruction
            int address;            // Variable to store the address part of the instruction

            // Read the address mode and move to the next token
            try {
                addressMode = readMARIE(currentToken); 
                currentToken = tokenizer.getNextToken();
            }
            catch(const exception& e) {
                Token lastToken = tokenizer.getLastToken();
                __parseerror(2, lastToken);
            }
            
            // Read the address part of the instruction and move to the next token, handling any exceptions
            try {
                address = readInteger(currentToken);
                currentToken = tokenizer.getNextToken();
            } catch(const exception& e) {
                Token lastToken = tokenizer.getLastToken();
                __parseerror(0, lastToken);
            }
            // Flag to indicate if there's an error with the opcode
            bool opcodeErrorExists = false;
            
            // Extract the opcode and operand from the address
            int opcode = address / 1000;
            int operand = address % 1000;
            int finalAddress = 0;   // Variable to store the final computed address

            // Check for illegal opcode error
            if (address > 9999) {
                // Set the opcode error flag and message
                opcodeErrorExists = true;
                // Set the final address to 9999 (error condition)
                finalAddress = 9999;
                errorString = "Error: Illegal opcode; treated as 9999";
                operand = 999;
                opcode = 9;

            } else if (addressMode[0] == 'M') {
                // Handle 'M' address mode (module)
                // Extract the requested module number from the operand
                int requestedModule = operand % 1000;
                // Check if the requested module is out of range
                if (requestedModule > module_base.size() - 1) {
                    errorString = "Error: Illegal module operand ; treated as module=0";
                    // Set the final address (with module set to 0)
                    finalAddress = opcode * 1000;
                    errorExists = true;
                } else {
                    // Compute the final address using the opcode and the base address of the requested module
                    finalAddress = opcode * 1000 + module_base[requestedModule].moduleBaseAddr;
                }

            } else if (addressMode[0] == 'A') {
                // Handle 'A' address mode (absolute)
                // Check if the operand is within the machine size limit
                if (operand < 512) {
                    // Use the address as-is
                    finalAddress = address;
                } else {
                    errorString = "Error: Absolute address exceeds machine size; zero used";
                    // Set the final address (with operand set to 0)
                    finalAddress = opcode * 1000;
                    errorExists = true;
                }
                
            } else if (addressMode[0] == 'R') {
                // Handle 'R' address mode (relative)
                // Check if the operand is within the current module's instruction count
                if (operand < instructionCount) {
                    // Compute the final address using the base address, opcode, and operand
                    finalAddress = baseAddress + opcode * 1000 + operand;
                } else {
                    // Set the final address (with operand set to 0)
                    finalAddress = baseAddress + opcode * 1000;
                    errorString = "Error: Relative address exceeds module size; relative zero used";
                    errorExists = true;
                }

            } else if (addressMode[0] == 'I') {
                // Handle 'I' address mode (immediate)
                // Check for illegal immediate operand
                if (address % 1000 >= 900) {
                    // Adjust the address to the error condition
                    address = address / 1000 * 1000 + 999;
                    errorString = "Error: Illegal immediate operand; treated as 999";
                    errorExists = true;
                }
                // Use the address as-is
                finalAddress = address;

            } else if (addressMode[0] == 'E') {
                // Handle 'E' address mode (external)
                string externalSymbol;  // Variable to store the external symbol being referenced
                try {
                    // Attempt to resolve the external symbol reference
                    // Get the external symbol from the operand index
                    externalSymbol = externalSymbols.at(operand);
                    // Add the operand index to the external references vector
                    externalReferences.push_back(operand);

                    // Flag to indicate if the symbol is not found in the symbol table
                    bool notfound = true;
                    // Attempt to find the symbol in the symbol table and compute the final address
                    for (int j = 0; j < symbolTable.size(); j++) {
                        // Get the current symbol from the symbol table
                        Symbol currentSymbol = symbolTable[j];
                        if (currentSymbol.value.compare(externalSymbol) == 0) { 
                            // Compute the final address using the symbol's address 
                            finalAddress = (opcode * 1000) + currentSymbol.Addr;
                            notfound = false;
                            // Mark the symbol as used
                            symbolTable[j].used = true;
                            break;
                        }
                    }
                    if (notfound) {
                        // If the symbol is not found in the symbol table
                        errorString = "Error: " + externalSymbol + " is not defined; zero used";
                        // Set the error flag and final address with operand set to 0
                        errorExists = true;
                        finalAddress = opcode * 1000 + 0;
                    }

                } catch(const exception& e) {
                    // Handle exceptions for external operand exceeding the length of the uselist
                    errorString = "Error: External operand exceeds length of uselist; treated as relative=0";
                    errorExists = true;
                    // Set the final address (with relative address = 0)
                    finalAddress = opcode * 1000 + baseAddress;
                }

            } else;
            // End of address mode handling
            
            // Print the memory map entry with the memory map index and the final address
            cout << setfill('0') << setw(3) << memoryMapIndex << ":" << " " << setfill('0') << setw(4) << finalAddress;

            // If there's any error, print the corresponding error message
            if (opcodeErrorExists || errorExists) {
                cout << " " << errorString;
            } cout << endl;

            // Reset error variables for the next iteration
            errorString = "";
            errorExists = false;            
            // Increment the memory map index
            memoryMapIndex++;
        }

        // Update the base address for the next module
        baseAddress = baseAddress + instructionCount;

        // Warn about unused external symbols in the module's uselist
        for (int i = 0; i < externalSymbols.size(); i++) {
            // Flag to indicate if the symbol was used
            bool found = false;
            for (int j = 0; j < externalReferences.size(); j++) {
                // Set the found flag to true if the symbol was used
                if (i == externalReferences[j]) found = true;
            }
            // Print a warning message
            if(!found) cout << "Warning: Module " << moduleNumber - 1 << ": uselist[" << i << "]=" << externalSymbols[i] << " was not used\n";   
        }
    }

    // Return the updated symbol table
    return symbolTable;
}
