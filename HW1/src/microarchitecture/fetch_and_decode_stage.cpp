#include "fetch_and_decode_stage.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

std::string fetch_and_decode_stage::trim(const std::string& s)
{
    const std::size_t first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
    {
        return "";
    }

    const std::size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

std::string fetch_and_decode_stage::to_lower(std::string s)
{
    std::transform(
        s.begin(),
        s.end(),
        s.begin(),
        [](unsigned char c)
        {
            return static_cast<char>(std::tolower(c));
        });

    return s;
}

std::vector<std::string> fetch_and_decode_stage::split_operands(const std::string& s)
{
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string token;

    while (std::getline(ss, token, ','))
    {
        token = trim(token);
        if (!token.empty())
        {
            result.push_back(token);
        }
    }

    return result;
}

int fetch_and_decode_stage::parse_x_register(const std::string& token)
{
    const std::string t = to_lower(trim(token));

    if (t.size() < 2 || t[0] != 'x')
    {
        throw std::runtime_error("Invalid register format: " + token);
    }

    const std::string number_part = t.substr(1);

    if (number_part.empty())
    {
        throw std::runtime_error("Invalid register format: " + token);
    }

    std::size_t pos = 0;
    int reg = 0;

    try
    {
        reg = std::stoi(number_part, &pos, 10);
    }
    catch (const std::exception&)
    {
        throw std::runtime_error("Invalid register format: " + token);
    }

    if (pos != number_part.size())
    {
        throw std::runtime_error("Invalid register format: " + token);
    }

    if (reg < 0 || reg > 31)
    {
        throw std::runtime_error("Register out of range: " + token);
    }

    return reg;
}

std::int64_t fetch_and_decode_stage::parse_immediate(const std::string& token)
{
    std::string t = trim(token);

    std::size_t pos = 0;
    std::int64_t value = std::stoll(t, &pos, 0);

    if (pos != t.size())
    {
        throw std::runtime_error("Invalid immediate: " + token);
    }

    return value;
}

instruction_decode_t fetch_and_decode_stage::decode_instruction(
    const std::string& raw_instruction,
    std::uint64_t pc)
{
    const std::string line = trim(raw_instruction);

    if (line.empty())
    {
        throw std::runtime_error("Empty instruction");
    }

    const std::size_t space_pos = line.find(' ');
    const std::string mnemonic =
        to_lower(space_pos == std::string::npos ? line : line.substr(0, space_pos));

    const std::string operand_text =
        (space_pos == std::string::npos) ? "" : trim(line.substr(space_pos + 1));

    const std::vector<std::string> operands = split_operands(operand_text);

    if (mnemonic == "add")
    {
        if (operands.size() != 3)
        {
            throw std::runtime_error("add requires 3 operands: " + raw_instruction);
        }

        return instruction_decode_t
        {
            .opcode = instruction_opcode_t::add,
            .op1_logical_reg = parse_x_register(operands[1]),
            .op2_is_immediate = false,
            .op2_logical_reg = parse_x_register(operands[2]),
            .op2_immediate = 0,
            .logical_destination = parse_x_register(operands[0]),
            .pc = pc
        };
    }

    if (mnemonic == "addi")
    {
        if (operands.size() != 3)
        {
            throw std::runtime_error("addi requires 3 operands: " + raw_instruction);
        }

        return instruction_decode_t
        {
            .opcode = instruction_opcode_t::addi,
            .op1_logical_reg = parse_x_register(operands[1]),
            .op2_is_immediate = true,
            .op2_logical_reg = 0,
            .op2_immediate = parse_immediate(operands[2]),
            .logical_destination = parse_x_register(operands[0]),
            .pc = pc
        };
    }

    if (mnemonic == "sub")
    {
        if (operands.size() != 3)
        {
            throw std::runtime_error("sub requires 3 operands: " + raw_instruction);
        }

        return instruction_decode_t
        {
            .opcode = instruction_opcode_t::sub,
            .op1_logical_reg = parse_x_register(operands[1]),
            .op2_is_immediate = false,
            .op2_logical_reg = parse_x_register(operands[2]),
            .op2_immediate = 0,
            .logical_destination = parse_x_register(operands[0]),
            .pc = pc
        };
    }

    if (mnemonic == "mulu")
    {
        if (operands.size() != 3)
        {
            throw std::runtime_error("mulu requires 3 operands: " + raw_instruction);
        }

        return instruction_decode_t
        {
            .opcode = instruction_opcode_t::mulu,
            .op1_logical_reg = parse_x_register(operands[1]),
            .op2_is_immediate = false,
            .op2_logical_reg = parse_x_register(operands[2]),
            .op2_immediate = 0,
            .logical_destination = parse_x_register(operands[0]),
            .pc = pc
        };
    }

    if (mnemonic == "divu")
    {
        if (operands.size() != 3)
        {
            throw std::runtime_error("divu requires 3 operands: " + raw_instruction);
        }

        return instruction_decode_t
        {
            .opcode = instruction_opcode_t::divu,
            .op1_logical_reg = parse_x_register(operands[1]),
            .op2_is_immediate = false,
            .op2_logical_reg = parse_x_register(operands[2]),
            .op2_immediate = 0,
            .logical_destination = parse_x_register(operands[0]),
            .pc = pc
        };
    }

    if (mnemonic == "remu")
    {
        if (operands.size() != 3)
        {
            throw std::runtime_error("remu requires 3 operands: " + raw_instruction);
        }

        return instruction_decode_t
        {
            .opcode = instruction_opcode_t::remu,
            .op1_logical_reg = parse_x_register(operands[1]),
            .op2_is_immediate = false,
            .op2_logical_reg = parse_x_register(operands[2]),
            .op2_immediate = 0,
            .logical_destination = parse_x_register(operands[0]),
            .pc = pc
        };
    }

    throw std::runtime_error("Unsupported instruction: " + raw_instruction);
}

void fetch_and_decode_stage::propagate(const State& curr,
                                       View& view,
                                       State& next,
                                       CycleContext& cycle_context,
                                       const instruction_memory& imem) const
{
    (void)view;

    if (cycle_context.is_halt_fetch_decode_requested())
    {
        next.program_counter.set(0x10000);
        next.decoded_instruction_register.reset();
        return;
    }

    if (cycle_context.is_rename_backpressure())
    {
        return;
    }

    next.decoded_instruction_register.reset();

    const std::uint64_t base_pc = curr.program_counter.get();
    std::size_t fetched_count = 0;

    for (; fetched_count < 4; ++fetched_count)
    {
        const std::uint64_t pc_value = base_pc + fetched_count;

        const std::string* raw_instruction = nullptr;
        if (imem.has_instruction(pc_value))
        {
            raw_instruction = &imem.instruction_at(pc_value);
        }
        else
        {
            break;
        }

        next.decoded_instruction_register.add(
            decode_instruction(*raw_instruction, pc_value));
    }

    for (std::size_t i = 0; i < fetched_count; ++i)
    {
        next.program_counter.increment();
    }
}