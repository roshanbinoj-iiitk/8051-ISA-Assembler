#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class Assembler
{
public:
    bool assembleFile(const std::string &inputPath, std::vector<uint8_t> &outputBytes);
    bool assembleString(const std::string &source, std::vector<uint8_t> &outputBytes);
    bool writeBinaryFile(const std::string &outputPath, const std::vector<uint8_t> &bytes) const;

    const std::string &getLastError() const;

private:
    struct SourceLine
    {
        std::size_t lineNumber = 0;
        std::string text;
    };

    struct ParsedInstruction
    {
        std::size_t lineNumber = 0;
        uint16_t address = 0;
        std::string mnemonic;
        std::vector<std::string> operands;
    };

    std::string lastError_;

    static std::string trim(const std::string &value);
    static std::string toUpper(std::string value);
    static std::string stripComment(const std::string &value);
    static std::vector<std::string> splitOperands(const std::string &value);
    static bool parseNumber(const std::string &token, int &value);
    static bool parseRegister(const std::string &token, uint8_t &index);

    std::size_t instructionSize(const ParsedInstruction &instr) const;
    bool firstPass(
        const std::vector<SourceLine> &lines,
        std::vector<ParsedInstruction> &instructions,
        std::unordered_map<std::string, uint16_t> &labels);
    bool secondPass(
        const std::vector<ParsedInstruction> &instructions,
        const std::unordered_map<std::string, uint16_t> &labels,
        std::vector<uint8_t> &outputBytes);

    void setError(const std::string &message);
};
