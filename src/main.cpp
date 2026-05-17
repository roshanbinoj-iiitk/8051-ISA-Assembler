#include "assembler.hpp"
#include "cpu.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

namespace
{
    void printUsage(const string &programName)
    {
        cout << "Usage:\n"
             << "  " << programName << " assemble <input.asm> <output.bin>\n"
             << "  " << programName << " run <program.bin> [--trace] [--r0 <0..255>]\n"
             << "  " << programName << " asmrun <input.asm> [--trace] [--r0 <0..255>]\n";
    }

    bool parseUint8(const string &token, uint8_t &value)
    {
        try
        {
            size_t consumed = 0;
            const int parsed = stoi(token, &consumed, 0);
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

    bool parseRunOptions(int startIndex, int argc, char *argv[], bool &trace, uint8_t &r0, string &error)
    {
        trace = false;
        r0 = 0;

        for (int i = startIndex; i < argc; ++i)
        {
            const string arg = argv[i];
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

    bool readBinaryFile(const string &path, vector<uint8_t> &bytes)
    {
        ifstream file(path, ios::binary);
        if (!file)
        {
            return false;
        }

        bytes.assign(istreambuf_iterator<char>(file), istreambuf_iterator<char>());
        return !bytes.empty() || file.good();
    }

    int executeProgram(const vector<uint8_t> &program, bool trace, uint8_t r0)
    {
        CPU cpu;
        if (!cpu.loadProgram(program, 0))
        {
            cerr << "Failed to load program: " << cpu.getLastError() << '\n';
            return 1;
        }

        cpu.setRegister(0, r0);
        cpu.setTraceEnabled(trace);

        if (!cpu.run())
        {
            cerr << "Execution failed: " << cpu.getLastError() << '\n';
            return 1;
        }

        cout << "Execution finished.\n";
        cout << "Final A = " << static_cast<int>(cpu.getA()) << '\n';
        cout << "Final PC = " << cpu.getPC() << '\n';
        cout << "Cycles = " << cpu.getCycles() << '\n';
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

    const string command = argv[1];

    if (command == "assemble")
    {
        if (argc != 4)
        {
            printUsage(argv[0]);
            return 1;
        }

        Assembler assembler;
        vector<uint8_t> bytes;
        if (!assembler.assembleFile(argv[2], bytes))
        {
            cerr << "Assembly failed: " << assembler.getLastError() << '\n';
            return 1;
        }

        if (!assembler.writeBinaryFile(argv[3], bytes))
        {
            cerr << "Failed to write output file: " << argv[3] << '\n';
            return 1;
        }

        cout << "Assembled " << bytes.size() << " bytes to " << argv[3] << '\n';
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
        string optionError;
        if (!parseRunOptions(3, argc, argv, trace, r0, optionError))
        {
            cerr << optionError << '\n';
            return 1;
        }

        vector<uint8_t> bytes;
        if (!readBinaryFile(argv[2], bytes))
        {
            cerr << "Failed to read binary file: " << argv[2] << '\n';
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
        string optionError;
        if (!parseRunOptions(3, argc, argv, trace, r0, optionError))
        {
            cerr << optionError << '\n';
            return 1;
        }

        Assembler assembler;
        vector<uint8_t> bytes;
        if (!assembler.assembleFile(argv[2], bytes))
        {
            cerr << "Assembly failed: " << assembler.getLastError() << '\n';
            return 1;
        }

        return executeProgram(bytes, trace, r0);
    }

    printUsage(argv[0]);
    return 1;
}
