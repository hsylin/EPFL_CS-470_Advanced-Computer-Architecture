#include <iostream>
#include <string>
#include <filesystem>
#include <utility>
#include <cstdlib>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <cstdint>
#include <nlohmann/json.hpp>

#include "state.hpp"
#include "view.hpp"
#include "cycle_context.hpp"
#include "program_loader.hpp"

#include "instruction_memory.hpp"
#include "execution_stage.hpp"
#include "commit_stage.hpp"
#include "rename_and_dispatch_stage.hpp"
#include "issue_stage.hpp"
#include "fetch_and_decode_stage.hpp"

using json = nlohmann::json;
constexpr std::uint64_t EXCEPTION_VECTOR = 0x10000;

void latch(State& curr, const State& next, View& view, CycleContext& cycle_context)
{
    curr = next;
    view.load_from_state(curr);
    cycle_context.clear();
}

bool no_inflight_work(const State& curr, const execution_stage& execution)
{
    return curr.decoded_instruction_register.size() == 0
        && curr.integer_queue.empty()
        && curr.active_list.empty()
        && execution.empty();
}


bool simulation_done(const State& curr, std::size_t program_size, const execution_stage& execution)
{
    if (curr.exception_flag.is_exception_mode())
    {
        return false;
    }

    return curr.program_counter.get() >= program_size &&
           no_inflight_work(curr, execution);
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

    execution_stage execution;
    commit_stage commit;
    rename_and_dispatch_stage rename_dispatch;
    issue_stage issue;
    fetch_and_decode_stage fetch_decode;

    
    // 0. parse JSON to get the program
    auto program = parse_instructions(input_path);
    const std::size_t program_size = program.size();
    instruction_memory imem(std::move(program));

    // 1. dump the state of the reset system
    log.push_back(curr.dump());



    // 2. cycle-by-cycle simulation
    while (!simulation_done(curr, program_size, execution))
    {
        // prepare next-state working copies for this cycle
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
       // - Issue is before Rename/Dispatch to prevent instructions put to the IQ from being
       //   issued in the same cycle. 
       //
       // Important:
       // - "The updates to all queues ... in a cycle can be used by the incoming
       //   instructions in the same cycle."
       // - exception entry has priority over normal downstream behavior
       // - after propagation, finalize next-state from the working views and then latch
       execution.propagate(curr, view, next, cycle_context);
       commit.propagate(curr, view, next, cycle_context);
       if (cycle_context.is_execution_reset_requested())
        {
            execution.reset_stage();
        }
       issue.propagate(curr, view, next, cycle_context, execution);
       rename_dispatch.propagate(curr, view, next, cycle_context);
       fetch_decode.propagate(curr, view, next, cycle_context, imem);
        // advance clock, start next cycle
        view.write_back_to_state(next);
        latch(curr, next, view, cycle_context);
        log.push_back(curr.dump());
    }
    
    // 4. save the output JSON log
    save_log(log, output_path);
    return 0;
}




















 

    





















