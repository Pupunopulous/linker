#include <iostream>
#include "Token.h"
#include "Parser.h"

using namespace std;

extern vector<Module> module_base;

int main(int argc, char** argv) {
    // Check if the number of command-line arguments is not = 2 (program name + one input file).
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <input-file>\n";
        return 1;
    }

    // Filename is at index 1 (since index 0 is the program name).
    string fileName = argv[1];

    // Perform the first pass of the linker, generating an initial symbol table.
    vector<Symbol> symbolTable = firstPass(fileName); 

    cout << "Symbol Table" << endl;
    // Iterate through the symbol table to print each symbol and its address.
    for (auto& symbol : symbolTable) {
        cout << symbol.value << "=" << symbol.Addr;
        // If the symbol is defined multiple times, the first definition is used.
        if (symbol.alreadyDefined) {
            cout << " Error: This variable is multiple times defined; first value used";
        } cout << endl;
    }

    cout << "\nMemory Map" << endl;
    // Perform the second pass of the linker, updating the symbol table with final addresses.
    symbolTable = secondPass(fileName, symbolTable);
    cout << endl; // Print an empty line for formatting.

    // Iterate through the final symbol table to check for unused symbols.
    for (auto symbol : symbolTable) {
        // If a symbol was defined but never used, print a warning message.
        if (!symbol.used) {
            cout << "Warning: Module " << symbol.moduleNumber - 1 << ": " << symbol.value << " was defined but never used" << endl;
        }
    }

    return 0;
    // end of program
}
