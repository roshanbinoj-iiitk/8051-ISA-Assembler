#include "assembler.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <limits>
#include <sstream>

namespace
{
    bool isLabelName(const std::string &value)
    {
        if (value.empty())
        {
            return false;
        }

        const unsigned char first = static_cast<unsigned char>(value[0]);
        if (!(std::isalpha(first) || value[0] == '_'))
        {
            return false;
        }

        for (char c : value)
        {
            const unsigned char uc = static_cast<unsigned char>(c);
            if (!(std::isalnum(uc) || c == '_'))
            {
                return false;
            }
        }

        return true;
    }
} // namespace

bool Assembler::assembleFile(const std::string &inputPath, std::vector<uint8_t> &outputBytes)
{
    std::ifstream input(inputPath);
    if (!input)
    {
        setError("Failed to open input file: " + inputPath);
        return false;
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return assembleString(buffer.str(), outputBytes);
}

bool Assembler::assembleString(const std::string &source, std::vector<uint8_t> &outputBytes)
{
    std::vector<SourceLine> lines;
    lines.reserve(128);

    std::istringstream stream(source);
    std::string line;
    std::size_t lineNumber = 1;
    while (std::getline(stream, line))
    {
        lines.push_back(SourceLine{lineNumber, line});
        ++lineNumber;
    }

    std::vector<ParsedInstruction> instructions;
    std::unordered_map<std::string, uint16_t> labels;

    if (!firstPass(lines, instructions, labels))
    {
        return false;
    }

    return secondPass(instructions, labels, outputBytes);
}

bool Assembler::writeBinaryFile(const std::string &outputPath, const std::vector<uint8_t> &bytes) const
{
    std::ofstream output(outputPath, std::ios::binary);
    if (!output)
    {
        return false;
    }

    if (!bytes.empty())
    {
        output.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }

    return output.good();
}

const std::string &Assembler::getLastError() const
{
    return lastError_;
}

std::string Assembler::trim(const std::string &value)
{
    std::size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])))
    {
        ++start;
    }

    std::size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])))
    {
        --end;
    }

    return value.substr(start, end - start);
}

std::string Assembler::toUpper(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c)
                   { return static_cast<char>(std::toupper(c)); });
    return value;
}

std::string Assembler::stripComment(const std::string &value)
{
    const std::size_t commentPos = value.find(';');
    if (commentPos == std::string::npos)
    {
        return value;
    }
    return value.substr(0, commentPos);
}

std::vector<std::string> Assembler::splitOperands(const std::string &value)
{
    std::vector<std::string> result;
    std::string current;

    for (char c : value)
    {
        if (c == ',')
        {
            result.push_back(trim(current));
            current.clear();
        }
        else
        {
            current.push_back(c);
        }
    }

    if (!current.empty() || !result.empty())
    {
        result.push_back(trim(current));
    }

    return result;
}

bool Assembler::parseNumber(const std::string &token, int &value)
{
    try
    {
        std::size_t pos = 0;
        const long parsed = std::stol(token, &pos, 0);
        if (pos != token.size())
        {
            return false;
        }

        if (parsed < std::numeric_limits<int>::min() || parsed > std::numeric_limits<int>::max())
        {
            return false;
        }

        value = static_cast<int>(parsed);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool Assembler::parseRegister(const std::string &token, uint8_t &index)
{
    const std::string upper = toUpper(trim(token));
    if (upper.size() != 2 || upper[0] != 'R')
    {
        return false;
    }

    if (upper[1] < '0' || upper[1] > '7')
    {
        return false;
    }

    index = static_cast<uint8_t>(upper[1] - '0');
    return true;
}

std::size_t Assembler::instructionSize(const ParsedInstruction &instr) const
{
    if (instr.mnemonic == "NOP" || instr.mnemonic == "HLT")
    {
        return 1;
    }

    if (instr.mnemonic == "MOV")
    {
        if (instr.operands.size() != 2)
        {
            return 0;
        }
        const std::string lhs = toUpper(trim(instr.operands[0]));
        const std::string rhs = trim(instr.operands[1]);
        if (lhs != "A" || rhs.empty() || rhs[0] != '#')
        {
            return 0;
        }
        return 2;
    }

    if (instr.mnemonic == "ADD")
    {
        if (instr.operands.size() != 2)
        {
            return 0;
        }
        const std::string lhs = toUpper(trim(instr.operands[0]));
        uint8_t reg = 0;
        if (lhs != "A" || !parseRegister(instr.operands[1], reg))
        {
            return 0;
        }
        return 1;
    }

    if (instr.mnemonic == "SJMP")
    {
        if (instr.operands.size() != 1)
        {
            return 0;
        }
        return 2;
    }

    return 0;
}

bool Assembler::firstPass(
    const std::vector<SourceLine> &lines,
    std::vector<ParsedInstruction> &instructions,
    std::unordered_map<std::string, uint16_t> &labels)
{
    instructions.clear();
    labels.clear();
    lastError_.clear();

    uint16_t address = 0;
    for (const SourceLine &sourceLine : lines)
    {
        std::string text = trim(stripComment(sourceLine.text));
        if (text.empty())
        {
            continue;
        }

        while (true)
        {
            const std::size_t colon = text.find(':');
            if (colon == std::string::npos)
            {
                break;
            }

            const std::string rawLabel = trim(text.substr(0, colon));
            if (!isLabelName(rawLabel))
            {
                setError("Line " + std::to_string(sourceLine.lineNumber) + ": invalid label name '" + rawLabel + "'.");
                return false;
            }

            const std::string label = toUpper(rawLabel);
            if (labels.find(label) != labels.end())
            {
                setError("Line " + std::to_string(sourceLine.lineNumber) + ": duplicate label '" + rawLabel + "'.");
                return false;
            }

            labels[label] = address;
            text = trim(text.substr(colon + 1));
            if (text.empty())
            {
                break;
            }
        }

        if (text.empty())
        {
            continue;
        }

        ParsedInstruction instr;
        instr.lineNumber = sourceLine.lineNumber;
        instr.address = address;

        const std::size_t splitPos = text.find_first_of(" \t");
        if (splitPos == std::string::npos)
        {
            instr.mnemonic = toUpper(trim(text));
        }
        else
        {
            instr.mnemonic = toUpper(trim(text.substr(0, splitPos)));
            instr.operands = splitOperands(trim(text.substr(splitPos + 1)));
        }

        const std::size_t size = instructionSize(instr);
        if (size == 0)
        {
            setError("Line " + std::to_string(instr.lineNumber) + ": unsupported or malformed instruction.");
            return false;
        }

        if (static_cast<std::size_t>(address) + size > 65536)
        {
            setError("Line " + std::to_string(instr.lineNumber) + ": program exceeds memory size.");
            return false;
        }

        instructions.push_back(instr);
        address = static_cast<uint16_t>(address + size);
    }

    return true;
}

bool Assembler::secondPass(
    const std::vector<ParsedInstruction> &instructions,
    const std::unordered_map<std::string, uint16_t> &labels,
    std::vector<uint8_t> &outputBytes)
{
    outputBytes.clear();
    outputBytes.reserve(instructions.size() * 2);

    for (const ParsedInstruction &instr : instructions)
    {
        if (instr.mnemonic == "NOP")
        {
            outputBytes.push_back(0x00);
            continue;
        }

        if (instr.mnemonic == "HLT")
        {
            outputBytes.push_back(0xFF);
            continue;
        }

        if (instr.mnemonic == "MOV")
        {
            const std::string immToken = trim(instr.operands[1]).substr(1);
            int immediate = 0;
            if (!parseNumber(immToken, immediate) || immediate < 0 || immediate > 255)
            {
                setError("Line " + std::to_string(instr.lineNumber) + ": MOV immediate must be in [0,255].");
                return false;
            }

            outputBytes.push_back(0x74);
            outputBytes.push_back(static_cast<uint8_t>(immediate));
            continue;
        }

        if (instr.mnemonic == "ADD")
        {
            uint8_t reg = 0;
            if (!parseRegister(instr.operands[1], reg))
            {
                setError("Line " + std::to_string(instr.lineNumber) + ": ADD requires register R0..R7.");
                return false;
            }

            outputBytes.push_back(static_cast<uint8_t>(0x28 + reg));
            continue;
        }

        if (instr.mnemonic == "SJMP")
        {
            int relative = 0;
            const std::string operand = trim(instr.operands[0]);
            if (!parseNumber(operand, relative))
            {
                const std::string label = toUpper(operand);
                const auto it = labels.find(label);
                if (it == labels.end())
                {
                    setError("Line " + std::to_string(instr.lineNumber) + ": unknown label '" + operand + "'.");
                    return false;
                }

                const int nextAddress = static_cast<int>(instr.address) + 2;
                relative = static_cast<int>(it->second) - nextAddress;
            }

            if (relative < -128 || relative > 127)
            {
                setError("Line " + std::to_string(instr.lineNumber) + ": SJMP offset out of range [-128,127].");
                return false;
            }

            outputBytes.push_back(0x80);
            outputBytes.push_back(static_cast<uint8_t>(static_cast<int8_t>(relative)));
            continue;
        }
    }

    lastError_.clear();
    return true;
}

void Assembler::setError(const std::string &message)
{
    lastError_ = message;
}
