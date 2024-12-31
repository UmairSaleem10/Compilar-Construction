#include <iostream>
#include <vector>
#include <string>
#include <cctype>
#include <map>
#include <unordered_map>
#include <memory>
#include <algorithm> // For std::all_of

using namespace std;
struct ASTNode;                         // Forward declaration of ASTNode
using ASTNodePtr = shared_ptr<ASTNode>; // Define ASTNodePtr before ASTNode

enum TokenType
{
    T_INT,
    T_ID,
    T_NUM,
    T_IF,
    T_ELSE,
    T_RETURN,
    T_ASSIGN,
    T_PLUS,
    T_MINUS,
    T_MUL,
    T_DIV,
    T_LPAREN,
    T_RPAREN,
    T_LBRACE,
    T_RBRACE,
    T_SEMICOLON,
    T_GT,
    T_ST,
    T_GTE,
    T_STE,
    T_EQUALITY,
    T_EOF,
    T_FOR,
    T_WHILE,
    T_PRINT,
    T_INPUT,
    T_DEF,
    T_COMMA,
    T_CALL
};

struct Token
{
    TokenType type;
    string value;
    size_t lineNo;
};

struct SymbolInfo
{
    string dataType;
    string value;
    size_t lineNo;
    string scope;

    SymbolInfo() : dataType("int"), value(""), lineNo(0), scope("main") {} // Default constructor

    SymbolInfo(const string type, const string value, const size_t lineNo, const string scope = "main")
        : dataType(type), value(value), lineNo(lineNo), scope(scope) {}
};

struct TAC
{
    std::string result;
    std::string op;
    std::string arg1;
    std::string arg2;
    vector<string> extras;

    TAC(std::string res, std::string operation = "", std::string a1 = "", std::string a2 = "", vector<string> extras = {})
        : result(res), op(operation), arg1(a1), arg2(a2), extras(extras) {}

    void print() const
    {
        if (result == "if")
        {
            cout << result + " " + op + " " + arg1 + " " + arg2 << endl;
        }
        else if (result == "print")
        {
            cout << result + " " + arg1 + " " + op + " " + arg2 << endl;
        }
        else if (result == "input")
        {
            cout << result + " " + arg1 + " " + op + " " + arg2 << endl;
        }
        else if (result == "goto")
        {
            cout << result + " " + arg1 << endl;
        }
        else if (result == "label")
        {
            cout << arg1 << endl; // arg1 is label
        }
        else if (result == "function")
        {
            cout << result + " " + arg1 << endl;
        }
        else if (result == "call")
        {
            cout << result + " " + arg1 << endl;
        }
        else if (result == "param")
        {
            cout << result + " " + arg1 << endl;
        }
        else if (result == "return")
        {
            cout << result << endl;
        }
        else if (arg2.empty())
        {
            std::cout << result << " = " << " " << arg1 << "\n";
        }
        else
        {
            std::cout << result << " = " << arg1 << " " << op << " " << arg2 << "\n";
        }
    }
};

class SymbolTable
{
private:
public:
    unordered_map<string, SymbolInfo> table;
    void addSymbol(const string variableName, const string type, size_t lineNo, const string value = "", const string scope = "main")
    {
        auto it = table.find(variableName);
        if (it == table.end() || it->second.scope != scope)
        {
            table[variableName + scope] = SymbolInfo{type, value, lineNo, scope};
        }
        else
        {
            cout << "Semantic Error: Symbol \'" << variableName << "\' already declared." << endl;
            exit(1);
        }
    }

    void updateVariableValue(const string &variableName, const string &value)
    {
        auto it = table.find(variableName);
        if (it != table.end())
        {
            it->second.value = value;
        }
        else
        {
            cout << "Error: Variable '" << variableName << "' not found in the symbol table." << endl;
        }
    }

    int symbolExists(const string variableName, const string scope = "main")
    {
        auto it = table.find(variableName + scope);
        if (it != table.end() && it->second.scope == scope)
        {
            return it->second.lineNo;
        }
        return -1;
    }

    // void displaySymbols() const
    // {
    //     cout << "\nSymbol Table:\n";
    //     for (const auto &entry : table)
    //     {
    //         cout << "Name: " << entry.first << ", Type: " << entry.second.type
    //              << ", Declared at line: " << entry.second.lineNo << endl;
    //     }
    // }
};
struct RegisterInfo
{
    bool isFree;
    string variable; // the variable whose value is inside the register
    RegisterInfo(string var = "", bool isFree = true)
        : variable(var), isFree(isFree) {}
};
struct DataSegment
{
    string var;
    string type;
    string val;
    string scope;
    DataSegment(string var, string type, string val, string scope = "main") : var(var), type(type), val(val), scope(scope) {}
};

class Assembly
{
public:
        string getAssembly()
    {
        string assembly = "Include Irvine32.inc\n.stack 4086\n.data\n.code\nmain proc\n";
        // generate main data segment
        for (auto s : getDataSegmentForScope("main"))
        {
            assembly += s;
        }
        // generate main assembly
        for (auto s : mainAssembly)
        {
            assembly += s;
        }
        assembly += "invoke ExitProcess,0\nmain endp\n";
        // generate functions
        for (auto s : functions)
        {
            assembly += s;
        }
        assembly += "end main\n";
        return assembly;
    }

    Assembly(SymbolTable &symbolTable) : symbolTable(symbolTable)
    {
        regMap["eax"];
        regMap["ebx"];
        regMap["ecx"];
        regMap["edx"];

        funcRegMap["eax"];
        funcRegMap["ebx"];
        funcRegMap["ecx"];
        funcRegMap["edx"];

        insMap["+"] = "Add";
        insMap["-"] = "Sub";
        insMap["*"] = "Imul";
        insMap["/"] = "IDiv";
        insMap[">"] = "Cmp";
        insMap["<"] = "Cmp";
        insMap["=="] = "Cmp";
        insMap[">="] = "Cmp";
        insMap["<="] = "Cmp";
        insMap[">i"] = "JNC";
        insMap["<i"] = "JC";
        insMap["<=i"] = "JLE";
        insMap[">=i"] = "JGE";
        insMap["==i"] = "JZ";
        insMap["goto"] = "JMP";
    }
    // string getDataSegment(string scope = "main")
    // {
    //     string dataSegment = ".data\n";
    //     dataSegment += "newLine db 0Ah, 0\n";
    //     for (auto ds : dataSegmentVariables)
    //     {
    //         if (ds.scope == scope)
    //         {
    //             string var = ds.var + " " + ds.type + " " + ds.val;
    //             dataSegment += var + "\n";
    //         }
    //     }
    //     return dataSegment;
    // }
    vector<string> getDataSegmentForScope(string scope)
    {
        vector<string> dataSegment;
        for (auto ds : dataSegmentVariables)
        {
            if (ds.scope == scope)
            {
                string var = string("LOCAL ") + ds.var + ":" + ds.type + "\n";
                dataSegment.push_back(var);
            }
        }
        return dataSegment;
    }

    void generateMainAssembly(TAC tac)
    {
        if (tac.result == "if")
        {
            if (tac.op == ">")
            {
                mainAssembly.push_back(insMap[">i"] + " " + tac.arg2 + "\n"); // arg2 is label
            }
            else if (tac.op == "<")
            {
                mainAssembly.push_back(insMap["<i"] + " " + tac.arg2 + "\n"); // arg2 is label
            }
            else if (tac.op == "==")
            {
                mainAssembly.push_back(insMap["==i"] + " " + tac.arg2 + "\n"); // arg2 is label
            }
            else if (tac.op == "<=")
            {
                mainAssembly.push_back(insMap["<=i"] + " " + tac.arg2 + "\n"); // arg2 is label
            }
            else if (tac.op == ">=")
            {
                mainAssembly.push_back(insMap[">=i"] + " " + tac.arg2 + "\n"); // arg2 is label
            }
        }
        else if (tac.result == "goto")
        {
            mainAssembly.push_back(insMap["goto"] + " " + tac.arg1 + "\n"); // arg1 is label
        }
        else if (tac.result == "label")
        {
            mainAssembly.push_back(tac.arg1 + "\n"); // arg1 is label
        }
        else if (tac.result == "print")
        {
            // string retVal = "push eax\npush edx \n";
            // retVal += string("Mov EAX,") + " [" + tac.op + "]" + "\n";
            // retVal += "mov edx, offset newLine \n";
            // retVal += "Call writeDec \n";
            // retVal += "call WriteString \n";
            // retVal += "pop edx\npop eax \n";

            string retVal = "push eax\n";
            retVal += string("Mov EAX,") + " [" + tac.op + "]" + "\n";
            retVal += "Call writeDec \n";
            retVal += "call CRLF \n";
            retVal += "pop eax \n";

            mainAssembly.push_back(retVal);
        }

        else if (tac.result == "input")
        {
            string retVal = "push eax\n";
            retVal += "Call ReadInt \n";
            retVal += string("Mov ") + "[" + tac.op + "]" + ", " + "EAX" + "\n";
            retVal += "pop eax \n";
            mainAssembly.push_back(retVal);
        }
        else if (tac.result == "call")
        {
            string retVal = "PUSHAD\n";

            int numberOfParams = stoi(tac.arg2);
            if (numberOfParams > 0)
            {
                vector<string> parms = tac.extras;
                for (int i = 0; i < parms.size(); i++)
                {
                    try
                    {
                        int num = stoi(parms[i]);
                        retVal += "PUSH " + to_string(num) + "\n";
                    }
                    catch (const std::exception &e)
                    {
                        string var = parms[i];
                        retVal += "PUSH [" + var + "]\n";
                    }
                }
            }
            retVal += "CALL " + tac.arg1 + "\n";
            retVal += "ADD ESP, " + to_string(numberOfParams * 4) + "\n";
            retVal += "POPAD\n";

            mainAssembly.push_back(retVal);
        }
        else if (tac.arg2.empty())
        {
            if (isLiteral(tac.arg1))
            {
                mainAssembly.push_back(string("Mov ") + string("[") + tac.result + string("]") + " " + "," + tac.arg1 + "\n");
            }
            else
            {
                bool isFound = false;
                string retVal;
                for (auto it : regMap)
                {
                    if (it.second.variable == tac.arg1)
                    {
                        isFound = true;
                        regMap[it.first].isFree = true;
                        mainAssembly.push_back(retVal = string("Mov ") + string("[") + tac.result + string("]") + string(" ") + "," + it.first + "\n");
                    }
                }
                if (!isFound)
                {
                    cout << tac.arg1 + " not found in regMap";
                    exit(1);
                }
            }
        }
        else
        {
            // case one when both args are digits
            if (isLiteral(tac.arg1) && isLiteral(tac.arg2))
            {
                // load one operand in reg
                // use that reg as temp and add second
                string reg = getFreeRegister();
                if (reg == "null")
                {
                    cout << "out of registers";
                    exit(1);
                }
                regMap[reg].isFree = false;
                regMap[reg].variable = tac.result;
                string retVal = string("Mov ") + reg + string(" ") + "," + tac.arg1 + "\n";
                string ins = insMap[tac.op];
                retVal += ins + string(" ") + reg + string(" ") + "," + tac.arg2 + "\n";
                mainAssembly.push_back(retVal);
            }
            // case one when one is digit and other is temp
            else if ((isLiteral(tac.arg1) && startsWithTAndNumber(tac.arg2)) || (isLiteral(tac.arg2) && startsWithTAndNumber(tac.arg1)))
            {
                // finding literal and temp
                string literal;
                string temp;
                if (isLiteral(tac.arg1))
                {
                    literal = tac.arg1;
                    temp = tac.arg2;
                }
                else
                {
                    literal = tac.arg2;
                    temp = tac.arg1;
                }
                string tempReg = getTempRegister(temp);
                if (tempReg == "null")
                {
                    cout << temp + " is not assigned any reg";
                    exit(1);
                }
                regMap[tempReg].variable = tac.result;
                string ins = insMap[tac.op];
                mainAssembly.push_back(ins + " " + tempReg + " " + "," + literal + "\n");
            }
            // case one when one is digit and other is var
            else if ((isLiteral(tac.arg1) && isAlphanumeric(tac.arg2)) || (isLiteral(tac.arg2) && isAlphanumeric(tac.arg1)))
            {
                // finding literal and temp
                string literal;
                string var;
                if (isLiteral(tac.arg1))
                {
                    literal = tac.arg1;
                    var = tac.arg2;
                }
                else
                {
                    literal = tac.arg2;
                    var = tac.arg1;
                }
                string tempReg = getFreeRegister();
                if (tempReg == "null")
                {
                    cout << "no register avaiable for " + var;
                    exit(1);
                }
                regMap[tempReg].variable = tac.result;
                regMap[tempReg].isFree = false;

                string ins = insMap[tac.op];
                string retVal = string("Mov") + " " + tempReg + "," + "[" + var + "]" + "\n";
                retVal += ins + " " + tempReg + " " + "," + literal + "\n";
                mainAssembly.push_back(retVal);
            }
            // case one when one is temp and other is var
            else if ((startsWithTAndNumber(tac.arg1) && isAlphanumeric(tac.arg2)) || (startsWithTAndNumber(tac.arg2) && isAlphanumeric(tac.arg1)))
            {
                // finding literal and temp
                string temp;
                string var;
                if (startsWithTAndNumber(tac.arg1))
                {
                    temp = tac.arg1;
                    var = tac.arg2;
                }
                else
                {
                    temp = tac.arg2;
                    var = tac.arg1;
                }
                string tempReg = getTempRegister(temp);
                if (tempReg == "null")
                {
                    cout << temp + " is not assigned any reg";
                    exit(1);
                }

                regMap[tempReg].variable = tac.result;

                string ins = insMap[tac.op];
                mainAssembly.push_back(ins + " " + tempReg + " " + "," + "[" + var + "]" + "\n");
            }
            // case one when both are var
            else if ((isAlphanumeric(tac.arg1) && isAlphanumeric(tac.arg2)))
            {
                string var1 = tac.arg1;
                string var2 = tac.arg2;

                string tempReg = getFreeRegister();
                if (tempReg == "null")
                {
                    cout << "no register is available";
                    exit(1);
                }

                regMap[tempReg].variable = tac.result;
                regMap[tempReg].isFree = false;

                string ins = insMap[tac.op];
                string retVal = string("Mov") + " " + tempReg + " " + "," + var1 + "\n";
                retVal += ins + " " + tempReg + " " + "," + " " + var2 + "\n";
                mainAssembly.push_back(retVal);
            }
        }
    }

    void generateFunctionAssembly(TAC tac)
    {
        if (tac.result == "if")
        {
            if (tac.op == ">")
            {
                functions.push_back(insMap[">i"] + " " + tac.arg2 + "\n"); // arg2 is label
            }
            else if (tac.op == "<")
            {
                functions.push_back(insMap["<i"] + " " + tac.arg2 + "\n"); // arg2 is label
            }
            else if (tac.op == "==")
            {
                functions.push_back(insMap["==i"] + " " + tac.arg2 + "\n"); // arg2 is label
            }
            else if (tac.op == "<=")
            {
                functions.push_back(insMap["<=i"] + " " + tac.arg2 + "\n"); // arg2 is label
            }
            else if (tac.op == ">=")
            {
                functions.push_back(insMap[">=i"] + " " + tac.arg2 + "\n"); // arg2 is label
            }
        }
        else if (tac.result == "goto")
        {
            functions.push_back(insMap["goto"] + " " + tac.arg1 + "\n"); // arg1 is label
        }
        else if (tac.result == "label")
        {
            functions.push_back(tac.arg1 + "\n"); // arg1 is label
        }
        else if (tac.result == "print")
        {
            // string retVal = "push eax\npush edx \n";
            // retVal += string("Mov EAX,") + " [" + tac.op + "]" + "\n";
            // retVal += "mov edx, offset newLine \n";
            // retVal += "Call writeDec \n";
            // retVal += "call WriteString \n";
            // retVal += "pop edx\npop eax \n";

            string retVal = "push eax\n";
            retVal += string("Mov EAX,") + " [" + tac.op + "]" + "\n";
            retVal += "Call writeDec \n";
            retVal += "call CRLF \n";
            retVal += "pop eax \n";

            functions.push_back(retVal);
        }

        else if (tac.result == "input")
        {
            string retVal = "push eax\n";
            retVal += "Call ReadInt \n";
            retVal += string("Mov ") + "[" + tac.op + "]" + ", " + "EAX" + "\n";
            retVal += "pop eax \n";
            functions.push_back(retVal);
        }
        else if (tac.result == "function")
        {
            string func = tac.arg1 + " " + "PROC" + "\n";
            functions.push_back(func);
            // get function local variables
            int numberOfLocalVar = 0;
            for (auto ds : getDataSegmentForScope(tac.arg1))
            {
                functions.push_back(ds);
                numberOfLocalVar++;
            }
            int numberOfParams = stoi(tac.arg2);
            if (numberOfParams > 0)
            {
                int index = 8 + 4 * (numberOfLocalVar); // points to last param
                vector<string> parms = tac.extras;
                string paramStr = "";
                for (int i = numberOfParams - 1; i >= 0; i--)
                {
                    string param = "MOV EAX, [esp+" + to_string(index) + "] \n";
                    param += "MOV [" + parms[i] + "], EAX \n";
                    index += 4;
                    paramStr += param;
                }
                functions.push_back(paramStr);
            }
        }
        else if (tac.result == "call")
        {
            string retVal = "PUSHAD\n";

            int numberOfParams = stoi(tac.arg2);
            if (numberOfParams > 0)
            {
                vector<string> parms = tac.extras;
                for (int i = 0; i < parms.size(); i++)
                {
                    try
                    {
                        int num = stoi(parms[i]);
                        retVal += "PUSH " + to_string(num) + "\n";
                    }
                    catch (const std::exception &e)
                    {
                        string var = parms[i];
                        retVal += "PUSH [" + var + "]\n";
                    }
                }
            }
            retVal += "CALL " + tac.arg1 + "\n";
            retVal += "ADD ESP, " + to_string(numberOfParams * 4) + "\n";
            retVal += "POPAD\n";

            functions.push_back(retVal);
        }
        else if (tac.result == "return")
        {
            string func = string("ret") + "\n";
            functions.push_back(func);
            func = tac.arg1 + " endp\n";
            functions.push_back(func);
            funcRegMap["eax"].isFree = true;
            funcRegMap["ebx"].isFree = true;
            funcRegMap["ecx"].isFree = true;
            funcRegMap["edx"].isFree = true;
        }

        else if (tac.arg2.empty())
        {
            if (isLiteral(tac.arg1))
            {
                functions.push_back(string("Mov ") + string("[") + tac.result + string("]") + " " + "," + tac.arg1 + "\n");
            }
            else
            {
                bool isFound = false;
                string retVal;
                for (auto it : funcRegMap)
                {
                    if (it.second.variable == tac.arg1)
                    {
                        isFound = true;
                        funcRegMap[it.first].isFree = true;
                        functions.push_back(retVal = string("Mov ") + string("[") + tac.result + string("]") + string(" ") + "," + it.first + "\n");
                    }
                }
                if (!isFound)
                {
                    cout << tac.arg1 + " not found in funcRegMap";
                    exit(1);
                }
            }
        }
        else
        {
            // case one when both args are digits
            if (isLiteral(tac.arg1) && isLiteral(tac.arg2))
            {
                // load one operand in reg
                // use that reg as temp and add second
                string reg = getFreeRegisterForFunction();
                if (reg == "null")
                {
                    cout << "out of registers";
                    exit(1);
                }
                funcRegMap[reg].isFree = false;
                funcRegMap[reg].variable = tac.result;
                string retVal = string("Mov ") + reg + string(" ") + "," + tac.arg1 + "\n";
                string ins = insMap[tac.op];
                retVal += ins + string(" ") + reg + string(" ") + "," + tac.arg2 + "\n";
                functions.push_back(retVal);
            }
            // case one when one is digit and other is temp
            else if ((isLiteral(tac.arg1) && startsWithTAndNumber(tac.arg2)) || (isLiteral(tac.arg2) && startsWithTAndNumber(tac.arg1)))
            {
                // finding literal and temp
                string literal;
                string temp;
                if (isLiteral(tac.arg1))
                {
                    literal = tac.arg1;
                    temp = tac.arg2;
                }
                else
                {
                    literal = tac.arg2;
                    temp = tac.arg1;
                }
                string tempReg = getTempRegisterForFunction(temp);
                if (tempReg == "null")
                {
                    cout << temp + " is not assigned any reg";
                    exit(1);
                }
                funcRegMap[tempReg].variable = tac.result;
                string ins = insMap[tac.op];
                functions.push_back(ins + " " + tempReg + " " + "," + literal + "\n");
            }
            // case one when one is digit and other is var
            else if ((isLiteral(tac.arg1) && isAlphanumeric(tac.arg2)) || (isLiteral(tac.arg2) && isAlphanumeric(tac.arg1)))
            {
                // finding literal and temp
                string literal;
                string var;
                if (isLiteral(tac.arg1))
                {
                    literal = tac.arg1;
                    var = tac.arg2;
                }
                else
                {
                    literal = tac.arg2;
                    var = tac.arg1;
                }
                string tempReg = getFreeRegisterForFunction();
                if (tempReg == "null")
                {
                    cout << "no register avaiable for " + var;
                    exit(1);
                }
                funcRegMap[tempReg].variable = tac.result;
                funcRegMap[tempReg].isFree = false;

                string ins = insMap[tac.op];
                string retVal = string("Mov") + " " + tempReg + "," + "[" + var + "]" + "\n";
                retVal += ins + " " + tempReg + " " + "," + literal + "\n";
                functions.push_back(retVal);
            }
            // case one when one is temp and other is var
            else if ((startsWithTAndNumber(tac.arg1) && isAlphanumeric(tac.arg2)) || (startsWithTAndNumber(tac.arg2) && isAlphanumeric(tac.arg1)))
            {
                // finding literal and temp
                string temp;
                string var;
                if (startsWithTAndNumber(tac.arg1))
                {
                    temp = tac.arg1;
                    var = tac.arg2;
                }
                else
                {
                    temp = tac.arg2;
                    var = tac.arg1;
                }
                string tempReg = getTempRegisterForFunction(temp);
                if (tempReg == "null")
                {
                    cout << temp + " is not assigned any reg";
                    exit(1);
                }

                funcRegMap[tempReg].variable = tac.result;

                string ins = insMap[tac.op];
                functions.push_back(ins + " " + tempReg + " " + "," + "[" + var + "]" + "\n");
            }
            // case one when both are var
            else if ((isAlphanumeric(tac.arg1) && isAlphanumeric(tac.arg2)))
            {
                string var1 = tac.arg1;
                string var2 = tac.arg2;

                string tempReg = getFreeRegisterForFunction();
                if (tempReg == "null")
                {
                    cout << "no register is available";
                    exit(1);
                }

                funcRegMap[tempReg].variable = tac.result;
                funcRegMap[tempReg].isFree = false;

                string ins = insMap[tac.op];
                string retVal = string("Mov") + " " + tempReg + " " + "," + var1 + "\n";
                retVal += ins + " " + tempReg + " " + "," + " " + var2 + "\n";
                functions.push_back(retVal);
            }
        }
    }
    // string getAssembly(TAC tac)
    // {
    //     if (tac.result == "if")
    //     {
    //         if (tac.op == ">")
    //         {
    //             return insMap[">i"] + " " + tac.arg2 + "\n"; // arg2 is label
    //         }
    //         else if (tac.op == "<")
    //         {
    //             return insMap["<i"] + " " + tac.arg2 + "\n"; // arg2 is label
    //         }
    //         else if (tac.op == "==")
    //         {
    //             return insMap["==i"] + " " + tac.arg2 + "\n"; // arg2 is label
    //         }
    //         else if (tac.op == "<=")
    //         {
    //             return insMap["<=i"] + " " + tac.arg2 + "\n"; // arg2 is label
    //         }
    //         else if (tac.op == ">=")
    //         {
    //             return insMap[">=i"] + " " + tac.arg2 + "\n"; // arg2 is label
    //         }
    //     }
    //     else if (tac.result == "goto")
    //     {
    //         return insMap["goto"] + " " + tac.arg1 + "\n"; // arg1 is label
    //     }
    //     else if (tac.result == "label")
    //     {
    //         return tac.arg1 + "\n"; // arg1 is label
    //     }
    //     else if (tac.result == "print")
    //     {
    //         // string retVal = "push eax\npush edx \n";
    //         // retVal += string("Mov EAX,") + " [" + tac.op + "]" + "\n";
    //         // retVal += "mov edx, offset newLine \n";
    //         // retVal += "Call writeDec \n";
    //         // retVal += "call WriteString \n";
    //         // retVal += "pop edx\npop eax \n";

    //         string retVal = "push eax\n";
    //         retVal += string("Mov EAX,") + " [" + tac.op + "]" + "\n";
    //         retVal += "Call writeDec \n";
    //         retVal += "call CRLF \n";
    //         retVal += "pop eax \n";

    //         return retVal;
    //     }

    //     else if (tac.result == "input")
    //     {
    //         string retVal = "push eax\n";
    //         retVal += "Call ReadInt \n";
    //         retVal += string("Mov ") + "[" + tac.op + "]" + ", " + "EAX" + "\n";
    //         retVal += "pop eax \n";
    //         return retVal;
    //     }
    //     else if (tac.result == "call")
    //     {
    //         string retVal = "PUSHAD\n";

    //         int numberOfParams = stoi(tac.arg2);
    //         if (numberOfParams > 0)
    //         {
    //             vector<string> parms = tac.extras;
    //             for (int i = 0; i < parms.size(); i++)
    //             {
    //                 try
    //                 {
    //                     int num = stoi(parms[i]);
    //                     retVal += "PUSH " + to_string(num) + "\n";
    //                 }
    //                 catch (const std::exception &e)
    //                 {
    //                     string var = parms[i];
    //                     retVal += "PUSH [" + var + "]\n";
    //                 }
    //             }
    //         }
    //         retVal += "CALL " + tac.arg1 + "\n";
    //         retVal += "ADD ESP, " + to_string(numberOfParams * 4) + "\n";
    //         retVal += "POPAD\n";

    //         return retVal;
    //     }
    //     else if (tac.arg2.empty())
    //     {
    //         if (isLiteral(tac.arg1))
    //         {
    //             return string("Mov ") + string("[") + tac.result + string("]") + " " + "," + tac.arg1 + "\n";
    //         }
    //         else
    //         {
    //             bool isFound = false;
    //             string retVal;
    //             for (auto it : regMap)
    //             {
    //                 if (it.second.variable == tac.arg1)
    //                 {
    //                     isFound = true;
    //                     regMap[it.first].isFree = true;
    //                     return retVal = string("Mov ") + string("[") + tac.result + string("]") + string(" ") + "," + it.first + "\n";
    //                 }
    //             }
    //             if (!isFound)
    //             {
    //                 cout << tac.arg1 + " not found in regMap";
    //                 exit(1);
    //             }
    //         }
    //     }
    //     else
    //     {
    //         // case one when both args are digits
    //         if (isLiteral(tac.arg1) && isLiteral(tac.arg2))
    //         {
    //             // load one operand in reg
    //             // use that reg as temp and add second
    //             string reg = getFreeRegister();
    //             if (reg == "null")
    //             {
    //                 cout << "out of registers";
    //                 exit(1);
    //             }
    //             regMap[reg].isFree = false;
    //             regMap[reg].variable = tac.result;
    //             string retVal = string("Mov ") + reg + string(" ") + "," + tac.arg1 + "\n";
    //             string ins = insMap[tac.op];
    //             retVal += ins + string(" ") + reg + string(" ") + "," + tac.arg2 + "\n";
    //             return retVal;
    //         }
    //         // case one when one is digit and other is temp
    //         else if ((isLiteral(tac.arg1) && startsWithTAndNumber(tac.arg2)) || (isLiteral(tac.arg2) && startsWithTAndNumber(tac.arg1)))
    //         {
    //             // finding literal and temp
    //             string literal;
    //             string temp;
    //             if (isLiteral(tac.arg1))
    //             {
    //                 literal = tac.arg1;
    //                 temp = tac.arg2;
    //             }
    //             else
    //             {
    //                 literal = tac.arg2;
    //                 temp = tac.arg1;
    //             }
    //             string tempReg = getTempRegister(temp);
    //             if (tempReg == "null")
    //             {
    //                 cout << temp + " is not assigned any reg";
    //                 exit(1);
    //             }
    //             regMap[tempReg].variable = tac.result;
    //             string ins = insMap[tac.op];
    //             return ins + " " + tempReg + " " + "," + literal + "\n";
    //         }
    //         // case one when one is digit and other is var
    //         else if ((isLiteral(tac.arg1) && isAlphanumeric(tac.arg2)) || (isLiteral(tac.arg2) && isAlphanumeric(tac.arg1)))
    //         {
    //             // finding literal and temp
    //             string literal;
    //             string var;
    //             if (isLiteral(tac.arg1))
    //             {
    //                 literal = tac.arg1;
    //                 var = tac.arg2;
    //             }
    //             else
    //             {
    //                 literal = tac.arg2;
    //                 var = tac.arg1;
    //             }
    //             string tempReg = getFreeRegister();
    //             if (tempReg == "null")
    //             {
    //                 cout << "no register avaiable for " + var;
    //                 exit(1);
    //             }
    //             regMap[tempReg].variable = tac.result;
    //             regMap[tempReg].isFree = false;

    //             string ins = insMap[tac.op];
    //             string retVal = string("Mov") + " " + tempReg + "," + "[" + var + "]" + "\n";
    //             retVal += ins + " " + tempReg + " " + "," + literal + "\n";
    //             return retVal;
    //         }
    //         // case one when one is temp and other is var
    //         else if ((startsWithTAndNumber(tac.arg1) && isAlphanumeric(tac.arg2)) || (startsWithTAndNumber(tac.arg2) && isAlphanumeric(tac.arg1)))
    //         {
    //             // finding literal and temp
    //             string temp;
    //             string var;
    //             if (startsWithTAndNumber(tac.arg1))
    //             {
    //                 temp = tac.arg1;
    //                 var = tac.arg2;
    //             }
    //             else
    //             {
    //                 temp = tac.arg2;
    //                 var = tac.arg1;
    //             }
    //             string tempReg = getTempRegister(temp);
    //             if (tempReg == "null")
    //             {
    //                 cout << temp + " is not assigned any reg";
    //                 exit(1);
    //             }

    //             regMap[tempReg].variable = tac.result;

    //             string ins = insMap[tac.op];
    //             return ins + " " + tempReg + " " + "," + "[" + var + "]" + "\n";
    //         }
    //         // case one when both are var
    //         else if ((isAlphanumeric(tac.arg1) && isAlphanumeric(tac.arg2)))
    //         {
    //             string var1 = tac.arg1;
    //             string var2 = tac.arg2;

    //             string tempReg = getFreeRegister();
    //             if (tempReg == "null")
    //             {
    //                 cout << "no register is available";
    //                 exit(1);
    //             }

    //             regMap[tempReg].variable = tac.result;
    //             regMap[tempReg].isFree = false;

    //             string ins = insMap[tac.op];
    //             string retVal = string("Mov") + " " + tempReg + " " + "," + var1 + "\n";
    //             retVal += ins + " " + tempReg + " " + "," + " " + var2 + "\n";
    //             return retVal;
    //         }
    //     }
    // }

    string getTempRegister(const std::string &temp)
    {
        for (auto it : regMap)
        {
            if (it.second.variable == temp)
                return it.first;
        }
        return "null";
    }
    string getTempRegisterForFunction(const std::string &temp)
    {
        for (auto it : funcRegMap)
        {
            if (it.second.variable == temp)
                return it.first;
        }
        return "null";
    }
    string getFreeRegister()
    {
        for (auto it : regMap)
        {
            if (it.second.isFree)
                return it.first;
        }
        return "null";
    }
    string getFreeRegisterForFunction()
    {
        for (auto it : funcRegMap)
        {
            if (it.second.isFree)
                return it.first;
        }
        return "null";
    }
    bool startsWithTAndNumber(const std::string &str)
    {
        // Check if the string is long enough and follows the required format
        return str.length() >= 2 && str[0] == 't' && std::isdigit(str[1]);
    }
    bool isAlphanumeric(const std::string &str)
    {
        if (startsWithTAndNumber(str))
            return false;
        return all_of(str.begin(), str.end(), ::isalnum);
    }
    bool isLiteral(const std::string &value)
    {
        // Check for integer literal
        if (!value.empty() && std::isdigit(value[0]))
        {
            for (char c : value)
            {
                if (!std::isdigit(c))
                {
                    return false;
                }
            }
            return true;
        }

        // Check for string literal (enclosed in quotes)
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
        {
            return true;
        }

        return false;
    }
    void declareVariablesInDataSegment()
    {
        for (auto sym : symbolTable.table)
        {
            if (sym.second.dataType == "int")
            {
                dataSegmentVariables.push_back(DataSegment(sliceString(sym.first, sym.second.scope), "dword", "", sym.second.scope));
            }
        }
    }
    string sliceString(const std::string &source, const std::string &lengthReference)
    {
        // Calculate how many characters to remove
        size_t lengthToRemove = lengthReference.length();

        // Ensure we do not remove more characters than the source has
        if (lengthToRemove >= source.length())
        {
            return ""; // Return an empty string if we remove everything or more
        }

        // Return the sliced string
        return source.substr(0, source.length() - lengthToRemove);
    }

private:
    unordered_map<string, RegisterInfo> regMap;
    unordered_map<string, RegisterInfo> funcRegMap;
    unordered_map<string, string> insMap;
    vector<DataSegment> dataSegmentVariables;
    SymbolTable &symbolTable;
    vector<string> functions;
    vector<string> mainAssembly;
};

struct ASTNode
{
    string value;
    vector<std::shared_ptr<ASTNode>> children;

    ASTNode(const std::string &val) : value(val) {}
};
using ASTNodePtr = shared_ptr<ASTNode>;

class Lexer
{

private:
    string src;
    size_t pos;
    size_t lineNo;
    /*
    It hold positive values.
    In C++, size_t is an unsigned integer data type used to represent the
    size of objects in bytes or indices, especially when working with memory-related
    functions, arrays, and containers like vector or string. You can also use the int data type but size_t is recommended one
    */

public:
    Lexer(const string &src)
    {
        this->src = src;
        this->pos = 0;
        this->lineNo = 0;
    }

    vector<Token> tokenize()
    {
        vector<Token> tokens;
        while (pos < src.size())
        {
            char current = src[pos];

            if (current == '/')
            {
                if (pos + 1 < src.size())
                {
                    if (src[pos + 1] == '/')
                    {
                        while (current != '\n')
                        {
                            pos++;
                            current = src[pos];
                        }
                        this->lineNo++;
                        pos++;
                        continue;
                    }
                }
            }
            if (current == '\n')
            {
                this->lineNo++;
                pos++;
                continue;
            }
            if (isspace(current))
            {
                pos++;
                continue;
            }
            if (isdigit(current))
            {
                tokens.push_back(Token{T_NUM, consumeNumber(), this->lineNo});
                continue;
            }
            if (isalpha(current))
            {
                string word = consumeWord();
                if (word == "int")
                    tokens.push_back(Token{T_INT, word, this->lineNo});
                else if (word == "if")
                    tokens.push_back(Token{T_IF, word, this->lineNo});
                else if (word == "else")
                    tokens.push_back(Token{T_ELSE, word, this->lineNo});
                else if (word == "return")
                    tokens.push_back(Token{T_RETURN, word, this->lineNo});
                else if (word == "for")
                    tokens.push_back(Token{T_FOR, word, this->lineNo});
                else if (word == "while")
                    tokens.push_back(Token{T_WHILE, word, this->lineNo});
                else if (word == "print")
                    tokens.push_back(Token{T_PRINT, word, this->lineNo});
                else if (word == "input")
                    tokens.push_back(Token{T_INPUT, word, this->lineNo});
                else if (word == "def")
                    tokens.push_back(Token{T_DEF, word, this->lineNo});
                else if (word == "call")
                    tokens.push_back(Token{T_CALL, word, this->lineNo});
                else
                    tokens.push_back(Token{T_ID, word, this->lineNo});
                continue;
            }
            // cheking operators with two char
            string tempCurr = "";
            tempCurr += current;
            tempCurr += src[pos + 1];

            if (tempCurr == ">=")
            {
                tokens.push_back(Token{T_GTE, ">=", this->lineNo});
                pos += 2;
                continue;
            }
            else if (tempCurr == "<=")
            {
                tokens.push_back(Token{T_STE, "<=", this->lineNo});
                pos += 2;
                continue;
            }
            else if (tempCurr == "==")
            {
                tokens.push_back(Token{T_EQUALITY, "==", this->lineNo});
                pos += 2;
                continue;
            }

            switch (current)
            {
            case '=':
                tokens.push_back(Token{T_ASSIGN, "=", this->lineNo});
                break;
            case '+':
                tokens.push_back(Token{T_PLUS, "+", this->lineNo});
                break;
            case '-':
                tokens.push_back(Token{T_MINUS, "-", this->lineNo});
                break;
            case '*':
                tokens.push_back(Token{T_MUL, "*", this->lineNo});
                break;
            case '/':
                tokens.push_back(Token{T_DIV, "/", this->lineNo});
                break;
            case '(':
                tokens.push_back(Token{T_LPAREN, "(", this->lineNo});
                break;
            case ')':
                tokens.push_back(Token{T_RPAREN, ")", this->lineNo});
                break;
            case '{':
                tokens.push_back(Token{T_LBRACE, "{", this->lineNo});
                break;
            case '}':
                tokens.push_back(Token{T_RBRACE, "}", this->lineNo});
                break;
            case ';':
                tokens.push_back(Token{T_SEMICOLON, ";", this->lineNo});
                break;
            case '>':
                tokens.push_back(Token{T_GT, ">", this->lineNo});
                break;
            case '<':
                tokens.push_back(Token{T_ST, "<", this->lineNo});
                break;
            case ',':
                tokens.push_back(Token{T_COMMA, "<", this->lineNo});
                break;

            default:
                cout << "Unexpected character: " << current << endl;
                exit(1);
            }
            pos++;
        }
        tokens.push_back(Token{T_EOF, "", this->lineNo});
        return tokens;
    }

    string consumeNumber()
    {
        size_t start = pos;
        while (pos < src.size() && isdigit(src[pos]))
            pos++;
        return src.substr(start, pos - start);
    }

    string consumeWord()
    {
        size_t start = pos;
        while (pos < src.size() && isalnum(src[pos]))
        {
            char x = src[pos];
            pos++;
        }
        return src.substr(start, pos - start);
    }
};

class Parser
{

public:
    Parser(const vector<Token> &tokens, SymbolTable &symbolTable) : symbolTable(symbolTable)
    {
        this->tokens = tokens;
        this->pos = 0;
        // this->symbolTable = symbolTable;
        tokenMap[T_INT] = "int";
        tokenMap[T_ID] = "identifier";
        tokenMap[T_NUM] = "number";
        tokenMap[T_IF] = "if";
        tokenMap[T_ELSE] = "else";
        tokenMap[T_RETURN] = "return";
        tokenMap[T_ASSIGN] = "=";    // Assignment operator
        tokenMap[T_PLUS] = "+";      // Addition operator
        tokenMap[T_MINUS] = "-";     // Subtraction operator
        tokenMap[T_MUL] = "*";       // Multiplication operator
        tokenMap[T_DIV] = "/";       // Division operator
        tokenMap[T_LPAREN] = "(";    // Left parenthesis
        tokenMap[T_RPAREN] = ")";    // Right parenthesis
        tokenMap[T_LBRACE] = "{";    // Left brace
        tokenMap[T_RBRACE] = "}";    // Right brace
        tokenMap[T_SEMICOLON] = ";"; // Semicolon
        tokenMap[T_GT] = ">";        // Greater-than operator
        tokenMap[T_ST] = "<";
        tokenMap[T_GTE] = ">=";
        tokenMap[T_EQUALITY] = "==";
        tokenMap[T_STE] = "<=";
        tokenMap[T_EOF] = "end of file";
        tokenMap[T_FOR] = "for";
        tokenMap[T_WHILE] = "while";
        tokenMap[T_PRINT] = "print";
        tokenMap[T_INPUT] = "input";
        tokenMap[T_DEF] = "def";
        tokenMap[T_CALL] = "call";
    }

    shared_ptr<ASTNode> parseProgram()
    {
        shared_ptr<ASTNode> node;
        // while (tokens[pos].type != T_EOF)
        // {
        //     node = parseStatement();
        // }
        node = parseBlock();
        cout << "Parsing completed successfully! No Syntax Error" << endl;
        return node;
    }

    std::vector<TAC> tacList;
    int tempCounter = 0;
    int labelCounter = 0;

    std::string generateTemp()
    {
        return "t" + std::to_string(tempCounter++);
    }
    std::string generateLabel()
    {
        return "L" + std::to_string(labelCounter++);
    }
    TAC generateConditionTAC(const std::shared_ptr<ASTNode> &node)
    {
        // Ensure the node has at least 3 children: left operand, operator, and right operand
        if (node->children.size() < 2)
        {
            throw std::runtime_error("Invalid condition node structure");
        }

        // Generate TAC for the condition
        string temp = generateTemp();

        std::string lhs = node->children[0]->value; // Left-hand side operand
        std::string op = node->value;               // Operator stored in the node itself
        std::string rhs = node->children[1]->value; // Right-hand side operand

        return TAC(temp, op, lhs, rhs);
    }
    std::string generateTAC(const std::shared_ptr<ASTNode> &node)
    {
        if (!node)
            return "";

        if (node->value == "print")
        {
            std::string exprResult = generateTAC(node->children[0]);
            tacList.emplace_back("print", exprResult, "(", ")"); // op is expRes
            return "";
        }
        else if (node->value == "input")
        {
            std::string exprResult = generateTAC(node->children[0]);
            tacList.emplace_back("input", exprResult, "(", ")"); // op is expRes
            return "";
        }
        else if (node->value == "function")
        {

            auto funcName = node->children[0];                  // first child is name
            int numberOfParam = stoi(node->children[2]->value); // third child is no of param
            vector<string> params;
            for (int i = 0; i < numberOfParam; i++)
            {
                params.push_back(node->children[3 + i]->value);
            }
            tacList.emplace_back("function", "", funcName->value, to_string(numberOfParam), params);

            generateTAC(node->children[1]); // second child is code
            tacList.emplace_back("return", "", funcName->value);

            return "";
        }
        else if (node->value == "call")
        {

            auto funcName = node->children[0];                  // first child is name
            int numberOfParam = stoi(node->children[1]->value); // second child is no of param
            vector<string> params;
            for (int i = 0; i < numberOfParam; i++)
            {
                params.push_back(node->children[2 + i]->value);
            }
            tacList.emplace_back("call", "", funcName->value, to_string(numberOfParam), params);
            return "";
        }
        else if (node->value == "for")
        {
            std::string labelLoop = generateLabel();
            std::string labelBody = generateLabel();
            std::string labelEnd = generateLabel();

            // first child initializaion
            auto initialization = generateTAC(node->children[0]);
            // tacList.emplace_back(initialization);

            // Condition is the second child
            tacList.emplace_back("label", "", labelLoop + ":");

            auto condition = generateConditionTAC(node->children[1]);
            tacList.emplace_back(condition);

            // inc dec is the third child

            tacList.emplace_back("if", condition.op, " goto ", labelBody);

            tacList.emplace_back("goto", " ", labelEnd); // Jump over the 'if' block

            tacList.emplace_back("label", "", labelBody + ":");
            generateTAC(node->children[3]);
            generateTAC(node->children[2]);
            tacList.emplace_back("goto", " ", labelLoop); // Jump over the 'if' block

            // Generate TAC for the 'if' block (second child)
            tacList.emplace_back("label", "", labelEnd + ":");

            // End label
            // tacList.emplace_back("label", "", labelEnd + ":");
            return "";
        }
        else if (node->value == "while")
        {
            std::string labelLoop = generateLabel();
            std::string labelBody = generateLabel();
            std::string labelEnd = generateLabel();

            // Condition is the first child
            tacList.emplace_back("label", "", labelLoop + ":");

            auto condition = generateConditionTAC(node->children[0]);
            tacList.emplace_back(condition);

            tacList.emplace_back("if", condition.op, " goto ", labelBody);

            tacList.emplace_back("goto", " ", labelEnd); // Jump over the 'if' block

            tacList.emplace_back("label", "", labelBody + ":");
            generateTAC(node->children[1]);
            tacList.emplace_back("goto", " ", labelLoop); // Jump over the 'if' block

            // Generate TAC for the 'if' block (second child)
            tacList.emplace_back("label", "", labelEnd + ":");
            return "";
        }
        else if (node->value == "if")
        {
            // Condition is the first child
            auto condition = generateConditionTAC(node->children[0]);

            std::string labelIfTrue = generateLabel();
            std::string labelEnd = generateLabel();

            // Generate TAC for the condition
            tacList.emplace_back(condition);
            tacList.emplace_back("if", condition.op, " goto ", labelIfTrue);

            // Check for the 'else' block (third child if present)
            if (node->children.size() > 2)
            {
                auto elseBlock = node->children[2];

                // Generate TAC for the 'else' block
                generateTAC(elseBlock);

                tacList.emplace_back("goto", " ", labelEnd); // Jump over the 'if' block

                // Label for the 'if' block
                tacList.emplace_back("label", "", labelIfTrue + ":");
                generateTAC(node->children[1]);
            }
            else
            {
                // Generate TAC for the 'if' block (second child)
                tacList.emplace_back("label", "", labelIfTrue + ":");

                generateTAC(node->children[1]);
            }

            // End label
            tacList.emplace_back("label", "", labelEnd + ":");
            return "";
        }

        else if (node->value == "block")
        {
            for (const auto &child : node->children)
            {
                generateTAC(child);
            }
            return "";
        }
        else if (node->value == "declaration")
        {
            std::string varName = node->children[1]->value;
            if (node->children.size() > 2)
            {
                std::string exprResult = generateTAC(node->children[2]);
                tacList.emplace_back(varName, "=", exprResult);
            }
            return "";
        }
        else if (node->value == "assignment")
        {
            std::string varName = node->children[0]->value;
            std::string exprResult = generateTAC(node->children[1]);
            tacList.emplace_back(varName, "=", exprResult);
            return "";
        }
        else if (node->value == "+" || node->value == "-" || node->value == "*" || node->value == "/")
        {
            std::string left = generateTAC(node->children[0]);
            std::string right = generateTAC(node->children[1]);
            std::string temp = generateTemp();
            tacList.emplace_back(temp, node->value, left, right);
            return temp;
        }
        else if (isLiteral(node->value) || isIdentifier(node->value))
        {
            return node->value;
        }
        else
        {
            throw std::runtime_error("Unknown AST node type: " + node->value);
        }
    }
    bool isLiteral(const std::string &value)
    {
        // Check for integer literal
        if (!value.empty() && std::isdigit(value[0]))
        {
            for (char c : value)
            {
                if (!std::isdigit(c))
                {
                    return false;
                }
            }
            return true;
        }

        // Check for string literal (enclosed in quotes)
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
        {
            return true;
        }

        return false;
    }

    // Check if a string is a valid identifier (e.g., variable name)
    bool isIdentifier(const std::string &value)
    {
        if (value.empty() || !std::isalpha(value[0]) && value[0] != '_')
        {
            return false; // Identifiers must start with a letter or underscore
        }

        for (char c : value)
        {
            if (!std::isalnum(c) && c != '_')
            {
                return false; // Identifiers can only contain letters, digits, or underscores
            }
        }

        return true;
    }
    // Helper function to print TAC instructions
    void printTAC(const vector<string> &instructions)
    {
        for (const string &instr : instructions)
        {
            cout << instr << endl;
        }
    }

private:
    vector<Token> tokens;
    size_t pos;

    SymbolTable &symbolTable;

    unordered_map<int, string> tokenMap;
    // Map each enum value to its corresponding string representation

    shared_ptr<ASTNode> parseStatement(string scope = "main")
    {
        if (tokens[pos].type == T_INT)
        {
            return parseDeclaration(scope);
        }
        else if (tokens[pos].type == T_ID)
        {
            return parseAssignment(scope);
        }
        else if (tokens[pos].type == T_IF)
        {
            return parseIfStatement(scope);
        }
        else if (tokens[pos].type == T_PRINT)
        {
            return parsePrintStatement(scope);
        }
        else if (tokens[pos].type == T_INPUT)
        {
            return parseInputStatement(scope);
        }
        else if (tokens[pos].type == T_FOR)
        {
            return parseForLoop(scope);
        }
        else if (tokens[pos].type == T_WHILE)
        {
            return parseWhileLoop(scope);
        }
        else if (tokens[pos].type == T_DEF)
        {
            return parseFunction();
        }
        else if (tokens[pos].type == T_CALL)
        {
            return parseFunctionCall(scope);
        }
        // else if (tokens[pos].type == T_RETURN)
        // {
        //     parseReturnStatement();
        // }
        else if (tokens[pos].type == T_LBRACE)
        {
            parseBlock();
        }
        else
        {
            cout << "Syntax error: unexpected token " << tokens[pos].value << endl;
            exit(1);
        }
        return nullptr;
    }

    shared_ptr<ASTNode> parseBlock(string scope = "main")
    {
        expect(T_LBRACE);
        auto blockNode = make_shared<ASTNode>("block");
        while (tokens[pos].type != T_RBRACE && tokens[pos].type != T_EOF)
        {
            auto statementNode = parseStatement(scope);
            blockNode->children.push_back(statementNode);
        }
        expect(T_RBRACE);
        return blockNode;
    }

    shared_ptr<ASTNode> parsePrintStatement(string scope = "main")
    {
        auto printStat = tokens[pos].value;
        expect(T_PRINT);
        expect(T_LPAREN);

        string varName = tokens[pos].value;
        expect(T_ID); // Expect the variable identifier

        size_t line = symbolTable.symbolExists(varName, scope);
        if (line == -1)
        {
            cout << "Error: Variable " << varName << " not declared!" << endl;
            exit(1);
        }

        expect(T_RPAREN);
        expect(T_SEMICOLON);

        auto printNode = std::make_shared<ASTNode>("print");
        printNode->children.push_back(make_shared<ASTNode>(varName));

        return printNode;
    }
    shared_ptr<ASTNode> parseInputStatement(string scope = "main")
    {
        expect(T_INPUT);
        expect(T_LPAREN);

        string varName = tokens[pos].value;
        expect(T_ID); // Expect the variable identifier

        size_t line = symbolTable.symbolExists(varName, scope);
        if (line == -1)
        {
            cout << "Error: Variable " << varName << " not declared!" << endl;
            exit(1);
        }

        expect(T_RPAREN);
        expect(T_SEMICOLON);

        auto inputNode = std::make_shared<ASTNode>("input");
        inputNode->children.push_back(make_shared<ASTNode>(varName));

        return inputNode;
    }
    shared_ptr<ASTNode> parseFunction()
    {
        auto def = tokens[pos].value;

        expect(T_DEF);

        string funcName = tokens[pos].value;
        expect(T_ID); // Expect the function identifier

        size_t line = symbolTable.symbolExists(funcName);
        if (line != -1)
        {
            cout << "Error: function " << funcName << " already declared! on Line " << line << endl;
            exit(1);
        }
        symbolTable.addSymbol(funcName, def, tokens[pos].lineNo);

        expect(T_LPAREN);
        vector<shared_ptr<ASTNode>> parameters;
        if (tokens[pos].type != T_RPAREN) // Check if parameters exist
        {
            do
            {
                string paramName = tokens[pos].value;
                expect(T_ID); // Expect parameter name

                // Check for duplicate parameter names
                for (const auto &param : parameters)
                {
                    if (param->value == paramName)
                    {
                        cout << "Error: duplicate parameter name '" << paramName << "' in function " << funcName << "!" << endl;
                        exit(1);
                    }
                }
                symbolTable.addSymbol(paramName, "int", tokens[pos].lineNo, "", funcName);
                // Add parameter as an ASTNode
                parameters.push_back(make_shared<ASTNode>(paramName));
                if (tokens[pos].type == T_COMMA)
                {
                    expect(T_COMMA);
                    if (tokens[pos].type == T_RPAREN)
                    {
                        cout << "Error: Expect param name on Line " << tokens[pos].lineNo << endl;
                        exit(1);
                    }
                }

            } while (tokens[pos].type != T_RPAREN); // Consume ',' if there are more parameters
        }
        expect(T_RPAREN);
        auto funcBlock = parseBlock(funcName);

        auto funcNode = std::make_shared<ASTNode>("function");
        funcNode->children.push_back(make_shared<ASTNode>(funcName));
        funcNode->children.push_back(funcBlock);
        funcNode->children.push_back(make_shared<ASTNode>(to_string(parameters.size()))); // number of params
        for (auto param : parameters)
        {
            funcNode->children.push_back(param);
        }

        return funcNode;
    }
    shared_ptr<ASTNode> parseFunctionCall(string scope = "main")
    {
        expect(T_CALL);
        string funcName = tokens[pos].value;
        expect(T_ID); // Expect the function identifier

        size_t line = symbolTable.symbolExists(funcName);
        if (line == -1)
        {
            cout << "Error: function " << funcName << " is not declared " << line << endl;
            exit(1);
        }
        expect(T_LPAREN);
        vector<shared_ptr<ASTNode>> parameters;
        if (tokens[pos].type != T_RPAREN)
        {
            do
            {
                string paramName = tokens[pos].value;
                int type = expectTwoToken(T_ID, T_NUM);
                if (type == 1)
                {
                    size_t line = symbolTable.symbolExists(paramName, scope);
                    if (line == -1)
                    {
                        cout << "Error: variable " << paramName << " is not declared in this scope " << scope << line << endl;
                        exit(1);
                    }
                    parameters.push_back(make_shared<ASTNode>(paramName));
                }
                else
                {
                    parameters.push_back(make_shared<ASTNode>(paramName));
                }
                // Add parameter as an ASTNode
                if (tokens[pos].type == T_COMMA)
                {
                    expect(T_COMMA);
                    if (tokens[pos].type == T_RPAREN)
                    {
                        cout << "Error: Expect param name on Line " << tokens[pos].lineNo << endl;
                        exit(1);
                    }
                }

            } while (tokens[pos].type != T_RPAREN); // Consume ',' if there are more parameters
        }
        expect(T_RPAREN);
        expect(T_SEMICOLON);
        auto funcNode = std::make_shared<ASTNode>("call");
        funcNode->children.push_back(make_shared<ASTNode>(funcName));
        funcNode->children.push_back(make_shared<ASTNode>(to_string(parameters.size()))); // number of params
        for (auto param : parameters)
        {
            funcNode->children.push_back(param);
        }

        return funcNode;
    }
    shared_ptr<ASTNode> parseForLoop(string scope = "main")
    {
        auto forNode = std::make_shared<ASTNode>("for");

        expect(T_FOR);
        expect(T_LPAREN);

        auto initialization = parseAssignment(scope);

        // expect(T_SEMICOLON);

        auto condition = parseExpression(scope);

        expect(T_SEMICOLON);

        auto incDec = parseAssignment(scope);

        expect(T_RPAREN);

        forNode->children.push_back(initialization);
        forNode->children.push_back(condition);
        forNode->children.push_back(incDec);

        auto forBlock = parseBlock(scope);
        forNode->children.push_back(forBlock);

        return forNode;
    }
    shared_ptr<ASTNode> parseWhileLoop(string scope = "main")
    {
        auto whileNode = std::make_shared<ASTNode>("while");

        expect(T_WHILE);
        expect(T_LPAREN);

        auto condition = parseExpression(scope);

        expect(T_RPAREN);

        auto whileBlock = parseBlock(scope);
        whileNode->children.push_back(condition);
        whileNode->children.push_back(whileBlock);

        return whileNode;
    }
    shared_ptr<ASTNode> parseDeclaration(string scope = "main")
    {
        auto type = tokens[pos].value;
        expect(T_INT); // Expect 'int'
        string varName = tokens[pos].value;
        expect(T_ID); // Expect the variable identifier

        size_t line = symbolTable.symbolExists(varName, scope);
        if (line != -1)
        {
            cout << "Error: Variable " << varName << " already declared! on Line " << line << endl;
            exit(1);
        }
        symbolTable.addSymbol(varName, type, tokens[pos].lineNo, "", scope);
        expect(T_SEMICOLON); // Expect the semicolon at the end of the declaration

        auto declNode = std::make_shared<ASTNode>("declaration");
        declNode->children.push_back(make_shared<ASTNode>(type));
        declNode->children.push_back(make_shared<ASTNode>(varName));

        return declNode;
    }
    shared_ptr<ASTNode> parseAssignment(string scope = "main")
    {
        string id = tokens[pos].value;
        expect(T_ID);

        size_t line = symbolTable.symbolExists(id, scope);
        if (line == -1)
        {
            cout << "Error: Variable " << id << " not declared!" << endl;
            exit(1);
        }
        expect(T_ASSIGN);
        auto expNode = parseExpression(scope);
        // symbolTable.updateVariableValue(id, tokens[pos - 1].value); may be problem
        expect(T_SEMICOLON);

        auto assignNode = std::make_shared<ASTNode>("assignment");
        assignNode->children.push_back(make_shared<ASTNode>(id));
        assignNode->children.push_back(expNode);

        return assignNode;
    }

    shared_ptr<ASTNode> parseIfStatement(string scope = "main")
    {
        expect(T_IF);
        auto ifNode = std::make_shared<ASTNode>("if");
        expect(T_LPAREN);

        auto condition = parseExpression(scope);
        ifNode->children.push_back(condition);

        expect(T_RPAREN);

        auto ifBlock = parseBlock(scope);
        ifNode->children.push_back(ifBlock);

        if (tokens[pos].type == T_ELSE)
        {
            expect(T_ELSE);

            auto elseBlock = parseBlock(scope);
            ifNode->children.push_back(elseBlock);
        }
        return ifNode;
    }

    // void parseReturnStatement()
    // {
    //     expect(T_RETURN);
    //     parseExpression();
    //     expect(T_SEMICOLON);
    // }

    shared_ptr<ASTNode> parseExpression(string scope = "main")
    {
        auto node = parseTerm(scope);
        while (tokens[pos].type == T_PLUS || tokens[pos].type == T_MINUS || tokens[pos].type == T_GT || tokens[pos].type == T_ST || tokens[pos].type == T_GTE || tokens[pos].type == T_STE || tokens[pos].type == T_EQUALITY)
        {
            auto op = tokens[pos].value;
            pos++;
            auto right = parseTerm(scope);
            auto opNode = make_shared<ASTNode>(op);
            opNode->children.push_back(node);
            opNode->children.push_back(right);
            node = opNode; // Update the root of the expression tree
        }
        return node;
        // if (tokens[pos].type == T_GT)
        // {
        //     pos++;
        //     parseExpression(); // After relational operator, parse the next expression
        // }
    }

    shared_ptr<ASTNode> parseTerm(string scope = "main")
    {
        auto node = parseFactor(scope);
        while (tokens[pos].type == T_MUL || tokens[pos].type == T_DIV)
        {
            auto op = tokens[pos].value;
            pos++;
            auto right = parseFactor(scope);
            auto opNode = make_shared<ASTNode>(op);
            opNode->children.push_back(node);
            opNode->children.push_back(right);
            node = opNode; // Update the root of the term tree
        }
        return node;
    }

    shared_ptr<ASTNode> parseFactor(string scope = "main")
    {
        if (tokens[pos].type == T_NUM || tokens[pos].type == T_ID)
        {
            auto factor = tokens[pos].value;
            auto node = make_shared<ASTNode>(factor);
            pos++;
            return node;
        }
        else if (tokens[pos].type == T_LPAREN)
        {
            expect(T_LPAREN);
            auto node = parseExpression(scope);
            expect(T_RPAREN);
            return node;
        }
        else
        {
            cout << "Syntax error: unexpected token " << tokens[pos].value << endl;
            exit(1);
        }
        return nullptr;
    }

    void expect(TokenType type)
    {
        if (tokens[pos].type == type)
        {
            pos++;
        }
        else
        {
            cout << "Syntax error: expected " << tokenMap[type] << " but found " << tokens[pos].value << " on line no: " << tokens[pos].lineNo << endl;
            exit(1);
        }
    }

    // just for function call
    int expectTwoToken(TokenType type1, TokenType type2)
    {
        if (tokens[pos].type == type1)
        {
            pos++;
            return 1;
        }
        else if (tokens[pos].type == type2)
        {
            pos++;
            return 2;
        }
        else
        {
            cout << "Syntax error: expected " << tokenMap[type1] << " or " << tokenMap[type2] << " but found " << tokens[pos].value << " on line no: " << tokens[pos].lineNo << endl;
            exit(1);
        }
    }
};

int main()
{
    string input = R"(
       {
            def findFact(a)
            {
                int f;
                f = 1;
                int i;
                for(i = 1 ; i<=a ; i = i+1;)
                {
                    f = f*i;
                }
                print(f);
            }

            int num;
            input(num);
            call findFact(num);
        }
    )";

    Lexer lexer(input);
    vector<Token> tokens = lexer.tokenize();

    SymbolTable symbolTable;

    Parser parser(tokens, symbolTable);
    auto node = parser.parseProgram();
    // parser.printAST(node);
    int t = 1;
    auto tac = parser.generateTAC(node);

    for (auto s : parser.tacList)
    {
        s.print();
    }
    cout << endl
         << endl;

    Assembly asembly(symbolTable);
    asembly.declareVariablesInDataSegment();

    // for (auto ds : asembly.getDataSegmentForScope("main"))
    // {
    //     cout << ds;
    // }

    bool isFunction = false;
    for (auto s : parser.tacList)
    {
        if (s.result == "return")
        {
            isFunction = false;
            asembly.generateFunctionAssembly(s);
        }
        else if (s.result == "function" || isFunction)
        {
            isFunction = true;
            asembly.generateFunctionAssembly(s);
        }
        else if (!isFunction)
        {
            asembly.generateMainAssembly(s);
        }
    }

    cout << asembly.getAssembly();

    // cout << endl
    //      << asembly.getDataSegment() << endl;
    // parsser.printTAC(tac.instructions);
    return 0;
}