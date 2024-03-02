#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include "Token.h"

using namespace std;

// All function prototypes required
void __parseerror(int errcode, Token token);
int readInteger(Token token);
string readSymbol(Token token);
string readMARIE(Token token);
vector<Symbol> firstPass(string fileName);
vector<Symbol> secondPass(string fileName, vector<Symbol> symbolTable);

#endif // PARSER_H
