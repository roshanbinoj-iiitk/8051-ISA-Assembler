#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

class Assembler
{
public:
    bool assembleFile(const string &inputPath, vector<uint8_t> &outputBytes);
    bool assembleString(const string &source, vector<uint8_t> &outputBytes);
    bool writeBinaryFile(const string &outputPath, const vector<uint8_t> &bytes) const;

    const string &getLastError() const;

private:
    struct SourceLine
    {
        size_t lineNumber = 0;
        string text;
    };

    struct ParsedInstruction
    {
        size_t lineNumber = 0;
        uint16_t address = 0;
        string mnemonic;
        vector<string> operands;
    };

    string lastError_;

    static string trim(const string &value);
    static string toUpper(string value);
    static string stripComment(const string &value);
    static vector<string> splitOperands(const string &value);
    static bool parseNumber(const string &token, int &value);
    static bool parseRegister(const string &token, uint8_t &index);

    size_t instructionSize(const ParsedInstruction &instr) const;
    bool firstPass(
        const vector<SourceLine> &lines,
        vector<ParsedInstruction> &instructions,
        unordered_map<string, uint16_t> &labels);
    bool secondPass(
        const vector<ParsedInstruction> &instructions,
        const unordered_map<string, uint16_t> &labels,
        vector<uint8_t> &outputBytes);

    void setError(const string &message);
};
