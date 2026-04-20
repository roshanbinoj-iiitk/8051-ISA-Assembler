#include "assembler.hpp"
#include "cpu.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace
{
    void printUsage(const std::string &programName)
    {
        std::cout << "Usage:\n"
                  << "  " << programName << " assemble <input.asm> <output.bin>\n"
                  << "  " << programName << " run <program.bin> [--trace] [--r0 <0..255>]\n"
                  << "  " << programName << " asmrun <input.asm> [--trace] [--r0 <0..255>]\n";
    }

    bool parseUint8(const std::string &token, uint8_t &value)
    {
        try
        {
            std::size_t consumed = 0;
            const int parsed = std::stoi(token, &consumed, 0);
            if (consumed != token.size() || parsed < 0 || parsed > 255)
            {
                return false;
            }
            value = static_cast<uint8_t>(parsed);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool parseRunOptions(int startIndex, int argc, char *argv[], bool &trace, uint8_t &r0, std::string &error)
    {
        trace = false;
        r0 = 0;

        for (int i = startIndex; i < argc; ++i)
        {
            const std::string arg = argv[i];
            if (arg == "--trace")
            {
                trace = true;
                continue;
            }

            if (arg == "--r0")
            {
                if (i + 1 >= argc)
                {
                    error = "Missing value for --r0.";
                    return false;
                }

                if (!parseUint8(argv[i + 1], r0))
                {
                    error = "Invalid --r0 value. Expected 0..255.";
                    return false;
                }
                ++i;
                continue;
            }

            error = "Unknown option: " + arg;
            return false;
        }

        return true;
    }

    bool readBinaryFile(const std::string &path, std::vector<uint8_t> &bytes)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
        {
            return false;
        }

        bytes.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        return !bytes.empty() || file.good();
    }

    int executeProgram(const std::vector<uint8_t> &program, bool trace, uint8_t r0)
    {
        CPU cpu;
        if (!cpu.loadProgram(program, 0))
        {
            std::cerr << "Failed to load program: " << cpu.getLastError() << '\n';
            return 1;
        }

        cpu.setRegister(0, r0);
        cpu.setTraceEnabled(trace);

        if (!cpu.run())
        {
            std::cerr << "Execution failed: " << cpu.getLastError() << '\n';
            return 1;
        }

        std::cout << "Execution finished.\n";
        std::cout << "Final A = " << static_cast<int>(cpu.getA()) << '\n';
        std::cout << "Final PC = " << cpu.getPC() << '\n';
        std::cout << "Cycles = " << cpu.getCycles() << '\n';
        return 0;
    }
} // namespace

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printUsage(argv[0]);
        return 1;
    }

    const std::string command = argv[1];

    if (command == "assemble")
    {
        if (argc != 4)
        {
            printUsage(argv[0]);
            return 1;
        }

        Assembler assembler;
        std::vector<uint8_t> bytes;
        if (!assembler.assembleFile(argv[2], bytes))
        {
            std::cerr << "Assembly failed: " << assembler.getLastError() << '\n';
            return 1;
        }

        if (!assembler.writeBinaryFile(argv[3], bytes))
        {
            std::cerr << "Failed to write output file: " << argv[3] << '\n';
            return 1;
        }

        std::cout << "Assembled " << bytes.size() << " bytes to " << argv[3] << '\n';
        return 0;
    }

    if (command == "run")
    {
        if (argc < 3)
        {
            printUsage(argv[0]);
            return 1;
        }

        bool trace = false;
        uint8_t r0 = 0;
        std::string optionError;
        if (!parseRunOptions(3, argc, argv, trace, r0, optionError))
        {
            std::cerr << optionError << '\n';
            return 1;
        }

        std::vector<uint8_t> bytes;
        if (!readBinaryFile(argv[2], bytes))
        {
            std::cerr << "Failed to read binary file: " << argv[2] << '\n';
            return 1;
        }

        return executeProgram(bytes, trace, r0);
    }

    if (command == "asmrun")
    {
        if (argc < 3)
        {
            printUsage(argv[0]);
            return 1;
        }

        bool trace = false;
        uint8_t r0 = 0;
        std::string optionError;
        if (!parseRunOptions(3, argc, argv, trace, r0, optionError))
        {
            std::cerr << optionError << '\n';
            return 1;
        }

        Assembler assembler;
        std::vector<uint8_t> bytes;
        if (!assembler.assembleFile(argv[2], bytes))
        {
            std::cerr << "Assembly failed: " << assembler.getLastError() << '\n';
            return 1;
        }

        return executeProgram(bytes, trace, r0);
    }

    printUsage(argv[0]);
    return 1;
}
