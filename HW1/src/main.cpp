#include <iostream>
#include <string>
#include <filesystem>
#include <utility>
#include <cstdlib>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <nlohmann/json.hpp>

#include "state.hpp"
#include "view.hpp"
#include "cycle_context.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;
using program_t = std::vector<std::string>;



std::pair<std::string, std::string> parse_paths(int argc, char* argv[]) 
{
    if (argc != 3) 
    {
        std::cerr << "Usage: hw1 <input_path> <output_path>\n";
        std::exit(-1);
    }

    std::string input_path = argv[1];
    std::string output_path = argv[2];

    if (!fs::exists(input_path)) 
    {
        std::cerr << "Error: Input path does not exist: " << input_path << "\n";
        std::exit(-1);
    }

    if (!fs::is_regular_file(input_path)) 
    {
        std::cerr << "Error: Input path is not a regular file: " << input_path << "\n";
        std::exit(-1);
    }

    return 
    {
        fs::absolute(input_path).string(),
        fs::absolute(output_path).string()
    };
}





program_t parse_instructions(const std::string& path) {
    json j = json::parse(std::ifstream(path));

    program_t program;
    program.reserve(j.size());

    for (const auto& x : j) {
        program.push_back(x.get<std::string>());
    }

    return program;
}


void latch(State& curr, const State& next)
{
    curr = next;
}

void save_log(const json& log, const std::string& output_path)
{
    std::ofstream out(output_path);

    if (!out)
    {
        throw std::runtime_error("Failed to open output file: " + output_path);
    }

    out << log.dump(2);
}


int main(int argc, char* argv[]) 
{

    auto [input_path, output_path] = parse_paths(argc, argv);

    State curr;
    State next;
    View view;
    CycleContext cycle_context;
    json log = json::array();

    
    // 0. parse JSON to get the program
    auto program = parse_instructions(input_path);

    // 1. dump the state of the reset system
    log.push_back(curr.dump());

    // 2. the loop for cycle-by-cycle iterations.
    while(not (no_instruction() and active_list_is_empty()))
    {
        next = curr;
        view.load_from_state(curr);
        cycle_context.clear();


       // do propagation
       // Propagation uses four layers of state:
       // - curr: the official state at the start of the cycle
       // - view: working copies of same-cycle-visible queues (Active List, IQ, Free List)
       // - next: the final next-state container to be latched
       // - cycle_context: transient forwarding, exception, and control signals
       

       // Propagation order is constrained by the homework text:
       //
       // - Execution is before Commit because Commit marks instructions done/exception
       //   on "receiving results from forwarding paths".
       //
       // - Execution is before Issue because "The Integer Queue observes the results of
       //   all functional units through forwarding paths", and "The Issue unit can issue"
       //   instructions whose operands are "provided by a forwarding path".
       //
       // - Commit is before Rename/Dispatch because queue updates are visible in the
       //   same cycle: if "the Commit stage just releases four physical registers",
       //   you can "immediately allocate these released registers for the incoming
       //   decode instructions".
       //
       // - Rename/Dispatch is before Fetch/Decode because if the Rename and Dispatch
       //   stage "applies backpressure, no instructions are processed".
       //
       // - Commit is before Fetch/Decode because when Commit detects an exception,
       //   Fetch/Decode should "set the PC to 0x10000" and clear the DIR "on the same cycle".
       //
       // - Commit is placed before Issue here so that exception entry and queue reset
       //   take priority over normal downstream behavior: in Exception Mode, the Commit
       //   stage should "Reset the Integer Queue and the Execution stage" and should
       //   "Notify the Fetch and Decode stage that no instruction should be decoded
       //   and supplied during the Exception Mode".
       //
       // - The relative order between Issue and Rename/Dispatch is left as a modeling
       //   choice here; the homework text does not give a single mandatory ordering
       //   between them. 
       //   We choose Rename/Dispatch -> Issue because queue updates are visible to later same-cycle readers.
       //
       // - The relative order between Issue and Fetch/Decode is also not directly fixed
       //   by the homework text; Fetch/Decode is mainly constrained by Rename/Dispatch
       //   backpressure and Commit exception control.
       //
       // Important:
       // - "The updates to all queues ... in a cycle can be used by the incoming
       //   instructions in the same cycle."
       // - exception entry has priority over normal downstream behavior
       // - after propagation, finalize next-state from the working views and then latch
        execution_unit.propagate(curr, view, next, cycle_context);
        commit_unit.propagate(curr, view, next, cycle_context);
        rename_and_dispatch_unit.propagate(curr, view, next, cycle_context);
        issue_unit.propagate(curr, view, next, cycle_context);
        fetch_and_decode_unit.propagate(curr, view, next, cycle_context);

        // advance clock, start next cycle
        view.write_back_to_state(next);
        latch(curr, next);
        log.push_back(curr.dump());
    }
    
    // 3. save the output JSON log
    save_log(log, output_path);
}


















 

    
