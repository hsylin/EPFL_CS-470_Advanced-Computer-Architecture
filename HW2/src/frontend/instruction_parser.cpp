#include "instruction_parser.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>

// -----------------------------------------------------------------------------
// Basic string utilities
// -----------------------------------------------------------------------------

std::string trim(const std::string& text)
{
    std::size_t start = 0;

    while (start < text.size() &&
           std::isspace(static_cast<unsigned char>(text[start]))) {
        start++;
    }

    std::size_t end = text.size();

    while (end > start &&
           std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        end--;
    }

    return text.substr(start, end - start);
}

std::string to_lower_copy(std::string text)
{
    std::transform(
        text.begin(),
        text.end(),
        text.begin(),
        [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        }
    );

    return text;
}

static std::vector<std::string> split_whitespace(const std::string& text)
{
    std::vector<std::string> result;
    std::stringstream ss(text);
    std::string token;

    while (ss >> token) {
        result.push_back(token);
    }

    return result;
}

std::vector<std::string> split_operands(const std::string& text)
{
    std::string s = trim(text);

    if (s.empty()) {
        return {};
    }

    std::vector<std::string> result;
    std::string current;
    int parenthesis_depth = 0;
    bool saw_comma = false;

    for (char c : s) {
        if (c == '(') {
            parenthesis_depth++;
            current.push_back(c);
        } else if (c == ')') {
            parenthesis_depth--;
            current.push_back(c);
        } else if (c == ',' && parenthesis_depth == 0) {
            saw_comma = true;

            std::string operand = trim(current);
            if (!operand.empty()) {
                result.push_back(operand);
            }

            current.clear();
        } else {
            current.push_back(c);
        }
    }

    std::string operand = trim(current);
    if (!operand.empty()) {
        result.push_back(operand);
    }

    // Supports output/reference style such as:
    //   st x42 0(x33)
    // where the comma is omitted.
    if (!saw_comma && result.size() == 1) {
        return split_whitespace(result[0]);
    }

    return result;
}

// -----------------------------------------------------------------------------
// Low-level parsing helpers
// -----------------------------------------------------------------------------

static bool is_decimal_number(const std::string& text)
{
    if (text.empty()) {
        return false;
    }

    std::size_t i = 0;

    if (text[i] == '+' || text[i] == '-') {
        i++;
    }

    if (i >= text.size()) {
        return false;
    }

    for (; i < text.size(); i++) {
        if (!std::isdigit(static_cast<unsigned char>(text[i]))) {
            return false;
        }
    }

    return true;
}

register_ref_t parse_register(const std::string& text)
{
    std::string s = trim(text);
    std::string lower = to_lower_copy(s);

    if (lower == "lc") {
        return {register_kind_t::LC, -1};
    }

    if (lower == "ec") {
        return {register_kind_t::EC, -1};
    }

    if (lower == "rrb") {
        return {register_kind_t::RRB, -1};
    }

    if (lower.size() < 2) {
        throw std::runtime_error("Invalid register: " + text);
    }

    char prefix = lower[0];
    std::string number_part = lower.substr(1);

    if ((prefix != 'x' && prefix != 'p') ||
        !is_decimal_number(number_part)) {
        throw std::runtime_error("Invalid register: " + text);
    }

    int index = std::stoi(number_part);

    if (index < 0 || index > 95) {
        throw std::runtime_error(
            "Register index out of range [0, 95]: " + text
        );
    }

    if (prefix == 'x') {
        return {register_kind_t::X, index};
    }

    return {register_kind_t::P, index};
}

int64_t parse_immediate_value(const std::string& text)
{
    std::string s = trim(text);

    if (s.empty()) {
        throw std::runtime_error("Empty immediate.");
    }

    std::size_t parsed_chars = 0;
    int64_t value = 0;

    try {
        // base 0 supports decimal and hexadecimal, e.g. 42 and 0x1000.
        value = std::stoll(s, &parsed_chars, 0);
    } catch (...) {
        throw std::runtime_error("Invalid immediate: " + text);
    }

    if (parsed_chars != s.size()) {
        throw std::runtime_error("Invalid immediate: " + text);
    }

    return value;
}

memory_operand_t parse_memory_operand(const std::string& text)
{
    std::string s = trim(text);

    std::size_t left = s.find('(');
    std::size_t right = s.find(')', left == std::string::npos ? 0 : left);

    if (left == std::string::npos ||
        right == std::string::npos ||
        right <= left + 1) {
        throw std::runtime_error("Invalid memory operand: " + text);
    }

    std::string immediate_part = trim(s.substr(0, left));
    std::string base_part = trim(s.substr(left + 1, right - left - 1));

    int64_t immediate = 0;

    if (!immediate_part.empty()) {
        immediate = parse_immediate_value(immediate_part);
    }

    register_ref_t base_register = parse_register(base_part);

    if (!is_x_register(base_register)) {
        throw std::runtime_error(
            "Memory base register must be an x register: " + text
        );
    }

    return {immediate, base_register};
}

// -----------------------------------------------------------------------------
// Operand construction
// -----------------------------------------------------------------------------

static operand_t make_register_operand(
    const register_ref_t& reg,
    const std::string& original_text
)
{
    operand_t operand;
    operand.kind = operand_kind_t::Register;
    operand.reg = reg;
    operand.original_text = original_text;
    return operand;
}

static operand_t make_immediate_operand(
    int64_t immediate,
    const std::string& original_text
)
{
    operand_t operand;
    operand.kind = operand_kind_t::Immediate;
    operand.immediate = immediate;
    operand.original_text = original_text;
    return operand;
}

static operand_t make_memory_operand(
    const memory_operand_t& memory,
    const std::string& original_text
)
{
    operand_t operand;
    operand.kind = operand_kind_t::Memory;
    operand.memory = memory;
    operand.original_text = original_text;
    return operand;
}

static operand_t make_boolean_operand(
    bool value,
    const std::string& original_text
)
{
    operand_t operand;
    operand.kind = operand_kind_t::Boolean;
    operand.boolean_value = value;
    operand.original_text = original_text;
    return operand;
}

operand_t parse_operand(const std::string& text)
{
    std::string s = trim(text);
    std::string lower = to_lower_copy(s);

    if (lower == "true") {
        return make_boolean_operand(true, s);
    }

    if (lower == "false") {
        return make_boolean_operand(false, s);
    }

    if (s.find('(') != std::string::npos &&
        s.find(')') != std::string::npos) {
        return make_memory_operand(parse_memory_operand(s), s);
    }

    try {
        register_ref_t reg = parse_register(s);
        return make_register_operand(reg, s);
    } catch (...) {
        // Not a register. Try immediate below.
    }

    return make_immediate_operand(parse_immediate_value(s), s);
}

// -----------------------------------------------------------------------------
// Instruction-level helpers
// -----------------------------------------------------------------------------

instruction_opcode_t parse_opcode(const std::string& text)
{
    std::string opcode = to_lower_copy(trim(text));

    if (opcode == "add") {
        return instruction_opcode_t::Add;
    }

    if (opcode == "addi") {
        return instruction_opcode_t::Addi;
    }

    if (opcode == "sub") {
        return instruction_opcode_t::Sub;
    }

    if (opcode == "mulu") {
        return instruction_opcode_t::Mulu;
    }

    if (opcode == "ld") {
        return instruction_opcode_t::Ld;
    }

    if (opcode == "st") {
        return instruction_opcode_t::St;
    }

    if (opcode == "loop") {
        return instruction_opcode_t::Loop;
    }

    if (opcode == "loop.pip") {
        return instruction_opcode_t::LoopPip;
    }

    if (opcode == "nop") {
        return instruction_opcode_t::Nop;
    }

    if (opcode == "mov") {
        return instruction_opcode_t::Mov;
    }

    throw std::runtime_error("Unknown opcode: " + text);
}

execution_unit_t execution_unit_for_opcode(instruction_opcode_t opcode)
{
    switch (opcode) {
        case instruction_opcode_t::Add:
        case instruction_opcode_t::Addi:
        case instruction_opcode_t::Sub:
        case instruction_opcode_t::Mov:
            return execution_unit_t::ALU;

        case instruction_opcode_t::Mulu:
            return execution_unit_t::Mult;

        case instruction_opcode_t::Ld:
        case instruction_opcode_t::St:
            return execution_unit_t::Mem;

        case instruction_opcode_t::Loop:
        case instruction_opcode_t::LoopPip:
            return execution_unit_t::Branch;

        case instruction_opcode_t::Nop:
        case instruction_opcode_t::Unknown:
            return execution_unit_t::None;
    }

    return execution_unit_t::None;
}

int latency_for_opcode(instruction_opcode_t opcode)
{
    if (opcode == instruction_opcode_t::Mulu) {
        return 3;
    }

    return 1;
}

static void require_operand_count(
    const std::vector<std::string>& operands,
    std::size_t expected_count,
    const std::string& raw
)
{
    if (operands.size() != expected_count) {
        throw std::runtime_error(
            "Wrong operand count in instruction: " + raw +
            ", expected " + std::to_string(expected_count) +
            ", got " + std::to_string(operands.size())
        );
    }
}

static register_ref_t require_x_operand(
    const std::string& text,
    const std::string& raw
)
{
    register_ref_t reg = parse_register(text);

    if (!is_x_register(reg)) {
        throw std::runtime_error(
            "Expected x register in instruction: " + raw
        );
    }

    return reg;
}

static void add_source_register(
    std::vector<register_ref_t>& src_regs,
    const register_ref_t& reg
)
{
    src_regs.push_back(reg);
}

// -----------------------------------------------------------------------------
// Full instruction parser
// -----------------------------------------------------------------------------

instruction_t parse_instruction(const std::string& raw, int instruction_address)
{
    std::string s = trim(raw);

    if (s.empty()) {
        throw std::runtime_error(
            "Empty instruction at address " +
            std::to_string(instruction_address)
        );
    }

    instruction_t instruction;
    instruction.raw = raw;
    instruction.instruction_address = instruction_address;
    instruction.original_pc = instruction_address;
    instruction.block = basic_block_t::Unknown;

    // Optional predicate prefix:
    //   (p32) add x1, x2, x3
    if (!s.empty() && s[0] == '(') {
        std::size_t close = s.find(')');

        if (close == std::string::npos) {
            throw std::runtime_error("Invalid predicate prefix: " + raw);
        }

        std::string predicate_text = s.substr(1, close - 1);
        register_ref_t predicate = parse_register(predicate_text);

        if (!is_p_register(predicate)) {
            throw std::runtime_error(
                "Predicate must be a p register: " + raw
            );
        }

        instruction.predicate = predicate;
        s = trim(s.substr(close + 1));
    }

    std::size_t first_space = s.find_first_of(" \t");

    std::string opcode_text;
    std::string operand_text;

    if (first_space == std::string::npos) {
        opcode_text = s;
        operand_text = "";
    } else {
        opcode_text = s.substr(0, first_space);
        operand_text = s.substr(first_space + 1);
    }

    instruction.opcode = parse_opcode(opcode_text);
    instruction.unit = execution_unit_for_opcode(instruction.opcode);
    instruction.latency = latency_for_opcode(instruction.opcode);

    std::vector<std::string> operands = split_operands(operand_text);

    switch (instruction.opcode) {
        case instruction_opcode_t::Add: {
            require_operand_count(operands, 3, raw);

            register_ref_t dest = require_x_operand(operands[0], raw);
            register_ref_t src1 = require_x_operand(operands[1], raw);
            register_ref_t src2 = require_x_operand(operands[2], raw);

            instruction.dest = dest;

            instruction.operands.push_back(make_register_operand(dest, operands[0]));
            instruction.operands.push_back(make_register_operand(src1, operands[1]));
            instruction.operands.push_back(make_register_operand(src2, operands[2]));

            add_source_register(instruction.src_regs, src1);
            add_source_register(instruction.src_regs, src2);
            break;
        }

        case instruction_opcode_t::Addi: {
            require_operand_count(operands, 3, raw);

            register_ref_t dest = require_x_operand(operands[0], raw);
            register_ref_t src = require_x_operand(operands[1], raw);
            int64_t immediate = parse_immediate_value(operands[2]);

            instruction.dest = dest;

            instruction.operands.push_back(make_register_operand(dest, operands[0]));
            instruction.operands.push_back(make_register_operand(src, operands[1]));
            instruction.operands.push_back(make_immediate_operand(immediate, operands[2]));

            add_source_register(instruction.src_regs, src);
            break;
        }

        case instruction_opcode_t::Sub: {
            require_operand_count(operands, 3, raw);

            register_ref_t dest = require_x_operand(operands[0], raw);
            register_ref_t src1 = require_x_operand(operands[1], raw);
            register_ref_t src2 = require_x_operand(operands[2], raw);

            instruction.dest = dest;

            instruction.operands.push_back(make_register_operand(dest, operands[0]));
            instruction.operands.push_back(make_register_operand(src1, operands[1]));
            instruction.operands.push_back(make_register_operand(src2, operands[2]));

            add_source_register(instruction.src_regs, src1);
            add_source_register(instruction.src_regs, src2);
            break;
        }

        case instruction_opcode_t::Mulu: {
            require_operand_count(operands, 3, raw);

            register_ref_t dest = require_x_operand(operands[0], raw);
            register_ref_t src1 = require_x_operand(operands[1], raw);
            register_ref_t src2 = require_x_operand(operands[2], raw);

            instruction.dest = dest;

            instruction.operands.push_back(make_register_operand(dest, operands[0]));
            instruction.operands.push_back(make_register_operand(src1, operands[1]));
            instruction.operands.push_back(make_register_operand(src2, operands[2]));

            add_source_register(instruction.src_regs, src1);
            add_source_register(instruction.src_regs, src2);
            break;
        }

        case instruction_opcode_t::Ld: {
            require_operand_count(operands, 2, raw);

            register_ref_t dest = require_x_operand(operands[0], raw);
            memory_operand_t memory = parse_memory_operand(operands[1]);

            instruction.dest = dest;

            instruction.operands.push_back(make_register_operand(dest, operands[0]));
            instruction.operands.push_back(make_memory_operand(memory, operands[1]));

            add_source_register(instruction.src_regs, memory.base_register);
            break;
        }

        case instruction_opcode_t::St: {
            require_operand_count(operands, 2, raw);

            register_ref_t source = require_x_operand(operands[0], raw);
            memory_operand_t memory = parse_memory_operand(operands[1]);

            instruction.dest = std::nullopt;

            instruction.operands.push_back(make_register_operand(source, operands[0]));
            instruction.operands.push_back(make_memory_operand(memory, operands[1]));

            add_source_register(instruction.src_regs, source);
            add_source_register(instruction.src_regs, memory.base_register);
            break;
        }

        case instruction_opcode_t::Loop: {
            require_operand_count(operands, 1, raw);

            int64_t target = parse_immediate_value(operands[0]);

            instruction.dest = std::nullopt;
            instruction.operands.push_back(make_immediate_operand(target, operands[0]));
            break;
        }

        case instruction_opcode_t::LoopPip: {
            require_operand_count(operands, 1, raw);

            int64_t target = parse_immediate_value(operands[0]);

            instruction.dest = std::nullopt;
            instruction.operands.push_back(make_immediate_operand(target, operands[0]));
            break;
        }

        case instruction_opcode_t::Nop: {
            if (!operands.empty()) {
                throw std::runtime_error(
                    "nop should not have operands: " + raw
                );
            }

            instruction.dest = std::nullopt;
            break;
        }

        case instruction_opcode_t::Mov: {
            require_operand_count(operands, 2, raw);

            register_ref_t dest = parse_register(operands[0]);
            operand_t source = parse_operand(operands[1]);

            if (dest.kind == register_kind_t::None) {
                throw std::runtime_error(
                    "Invalid mov destination: " + raw
                );
            }

            if (source.kind == operand_kind_t::Memory) {
                throw std::runtime_error(
                    "mov source cannot be a memory operand: " + raw
                );
            }

            if (dest.kind == register_kind_t::P &&
                source.kind != operand_kind_t::Boolean) {
                throw std::runtime_error(
                    "mov to predicate register must use true/false: " + raw
                );
            }

            if ((dest.kind == register_kind_t::LC ||
                 dest.kind == register_kind_t::EC ||
                 dest.kind == register_kind_t::RRB) &&
                source.kind != operand_kind_t::Immediate) {
                throw std::runtime_error(
                    "mov to LC/EC/RRB must use an immediate: " + raw
                );
            }

            instruction.dest = dest;

            instruction.operands.push_back(make_register_operand(dest, operands[0]));
            instruction.operands.push_back(source);

            if (source.kind == operand_kind_t::Register) {
                add_source_register(instruction.src_regs, source.reg);
            }

            break;
        }

        case instruction_opcode_t::Unknown: {
            throw std::runtime_error(
                "Cannot parse unknown instruction: " + raw
            );
        }
    }

    return instruction;
}




std::vector<instruction_t> parse_program(
    const std::vector<std::string>& raw_program
)
{
    std::vector<instruction_t> parsed_program;
    parsed_program.reserve(raw_program.size());

    for (std::size_t i = 0; i < raw_program.size(); i++) {
        parsed_program.push_back(
            parse_instruction(raw_program[i], static_cast<int>(i))
        );
    }

    // Map old instruction addresses to compacted addresses after removing nops.
    std::vector<int> old_to_new(raw_program.size() + 1, -1);

    int new_address = 0;
    for (std::size_t old_address = 0; old_address < parsed_program.size(); old_address++) {
        if (parsed_program[old_address].opcode != instruction_opcode_t::Nop) {
            old_to_new[old_address] = new_address;
            new_address++;
        }
    }

    // If a loop target points to a nop, redirect it to the next non-nop instruction.
    int next_valid_address = new_address;
    old_to_new[raw_program.size()] = new_address;

    for (int old_address = static_cast<int>(raw_program.size()) - 1;
         old_address >= 0;
         old_address--) {
        if (old_to_new[old_address] == -1) {
            old_to_new[old_address] = next_valid_address;
        } else {
            next_valid_address = old_to_new[old_address];
        }
    }

    std::vector<instruction_t> program;
    program.reserve(new_address);

    for (std::size_t old_address = 0; old_address < parsed_program.size(); old_address++) {
        instruction_t instruction = parsed_program[old_address];

        // Discard input nops.
        if (instruction.opcode == instruction_opcode_t::Nop) {
            continue;
        }

        const int compact_address = static_cast<int>(program.size());

        instruction.instruction_address = compact_address;
        instruction.original_pc = compact_address;

        // Remap loop target from old input address to compacted address.
        if (instruction.opcode == instruction_opcode_t::Loop ||
            instruction.opcode == instruction_opcode_t::LoopPip) {

            int old_target = static_cast<int>(instruction.operands[0].immediate);

            if (old_target < 0 ||
                old_target >= static_cast<int>(old_to_new.size()) ||
                old_to_new[old_target] >= new_address) {
                throw std::runtime_error(
                    "Invalid loop target after removing input nop instructions."
                );
            }

            instruction.operands[0].immediate = old_to_new[old_target];
        }

        program.push_back(instruction);
    }

    return program;
}