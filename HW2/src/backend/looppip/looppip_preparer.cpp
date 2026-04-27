#include "looppip_preparer.hpp"

#include <stdexcept>
#include <string>


static bool is_empty_bundle(const bundle_t& bundle)
{
    for (const std::string& instruction_text : bundle)
    {
        if (instruction_text != "nop")
        {
            return false;
        }
    }

    return true;
}

static bundle_t get_bundle_or_empty(
    const schedule_t& schedule,
    int cycle
)
{
    if (cycle < 0)
    {
        throw std::runtime_error("Schedule cycle cannot be negative.");
    }

    if (cycle >= static_cast<int>(schedule.size()))
    {
        return make_empty_bundle();
    }

    return schedule[cycle];
}

static void trim_trailing_empty_bundles(schedule_t& schedule)
{
    while (!schedule.empty() && is_empty_bundle(schedule.back()))
    {
        schedule.pop_back();
    }
}

static bool starts_with(
    const std::string& text,
    const std::string& prefix
)
{
    return text.rfind(prefix, 0) == 0;
}

static bool is_loop_instruction_text(const std::string& instruction_text)
{
    return starts_with(instruction_text, "loop ") ||
           starts_with(instruction_text, "loop.pip ");
}

static std::string predicate_for_stage(int stage)
{
    if (stage < 0)
    {
        throw std::runtime_error("Stage cannot be negative.");
    }

    return "p" + std::to_string(32 + stage);
}

static std::string add_stage_predicate(
    const std::string& instruction_text,
    int stage
)
{
    if (instruction_text == "nop")
    {
        return instruction_text;
    }

    // The loop.pip instruction itself must remain unpredicated.
    // It controls LC / EC / RRB / rotating predicates and must execute
    // during both the kernel phase and the epilogue-drain phase.
    if (is_loop_instruction_text(instruction_text))
    {
        return instruction_text;
    }

    // Avoid double-predicating an instruction if it already has one.
    if (starts_with(instruction_text, "("))
    {
        return instruction_text;
    }

    return "(" + predicate_for_stage(stage) + ") " + instruction_text;
}

static std::string make_loop_pip_instruction(int loop_start_cycle)
{
    if (loop_start_cycle < 0)
    {
        throw std::runtime_error("Loop start cycle cannot be negative.");
    }

    return "loop.pip " + std::to_string(loop_start_cycle);
}



static bool try_insert_into_existing_alu_slot(
    schedule_t& schedule,
    const std::string& instruction_text
)
{
    // Try to place the preparation instruction as late as possible before
    // the compact loop body, so it does not unnecessarily increase code size.
    for (int cycle = static_cast<int>(schedule.size()) - 1;
         cycle >= 0;
         cycle--)
    {
        if (is_schedule_slot_empty(schedule, cycle, bundle_slot_t::ALU0))
        {
            put_instruction_in_schedule(
                schedule,
                cycle,
                bundle_slot_t::ALU0,
                instruction_text
            );
            return true;
        }

        if (is_schedule_slot_empty(schedule, cycle, bundle_slot_t::ALU1))
        {
            put_instruction_in_schedule(
                schedule,
                cycle,
                bundle_slot_t::ALU1,
                instruction_text
            );
            return true;
        }
    }

    return false;
}

static void append_alu_instruction(
    schedule_t& schedule,
    const std::string& instruction_text
)
{
    bundle_t bundle = make_empty_bundle();
    put_instruction_in_bundle_slot(
        bundle,
        bundle_slot_t::ALU0,
        instruction_text
    );
    schedule.push_back(bundle);
}

static void insert_loop_pip_initialization(
    schedule_t& bb0_schedule,
    int num_stages
)
{
    if (num_stages <= 0)
    {
        throw std::runtime_error("num_stages must be positive.");
    }

    const std::string ec_initialization =
        "mov EC, " + std::to_string(num_stages - 1);

    const std::string predicate_initialization =
        "mov p32, true";

    if (!try_insert_into_existing_alu_slot(bb0_schedule, ec_initialization))
    {
        append_alu_instruction(bb0_schedule, ec_initialization);
    }

    if (!try_insert_into_existing_alu_slot(bb0_schedule, predicate_initialization))
    {
        append_alu_instruction(bb0_schedule, predicate_initialization);
    }
}



static schedule_t copy_schedule_range(
    const schedule_t& schedule,
    int begin_cycle,
    int end_cycle
)
{
    if (begin_cycle < 0 || end_cycle < begin_cycle)
    {
        throw std::runtime_error("Invalid schedule range.");
    }

    schedule_t result;

    for (int cycle = begin_cycle; cycle < end_cycle; cycle++)
    {
        result.push_back(get_bundle_or_empty(schedule, cycle));
    }

    return result;
}


static void put_compacted_instruction(
    bundle_t& compact_bundle,
    bundle_slot_t slot,
    const std::string& instruction_text
)
{
    if (instruction_text == "nop")
    {
        return;
    }

    const int index = bundle_slot_to_index(slot);

    if (compact_bundle[index] != "nop")
    {
        throw std::runtime_error(
            "Conflict while compacting loop.pip stages."
        );
    }

    compact_bundle[index] = instruction_text;
}

static schedule_t compact_loop_body(
    const schedule_t& allocated_schedule,
    const looppip_schedule_result_t& schedule_result,
    int compact_loop_start_cycle
)
{
    schedule_t compact_loop;

    const int ii = schedule_result.ii;
    const int num_stages = schedule_result.num_stages;
    const int loop_body_start_cycle = schedule_result.loop_body_start_cycle;


    for (int offset = 0; offset < ii; offset++)
    {
        bundle_t compact_bundle = make_empty_bundle();

        for (int stage = 0; stage < num_stages; stage++)
        {
            const int source_cycle =
                loop_body_start_cycle + stage * ii + offset;

            const bundle_t source_bundle =
                get_bundle_or_empty(allocated_schedule, source_cycle);

            for (int slot_index = 0; slot_index < 5; slot_index++)
            {
                const std::string& original_instruction =
                    source_bundle[slot_index];

                if (original_instruction == "nop")
                {
                    continue;
                }

                const bundle_slot_t slot =
                    static_cast<bundle_slot_t>(slot_index);



                if (is_loop_instruction_text(original_instruction))
                {
                    continue;
                }
                std::string prepared_instruction =
                add_stage_predicate(original_instruction, stage);

                put_compacted_instruction(
                    compact_bundle,
                    slot,
                    prepared_instruction
                );
            }
        }

        compact_loop.push_back(compact_bundle);
    }

    const int branch_offset = ii - 1;

    if (!is_bundle_slot_empty(compact_loop[branch_offset], bundle_slot_t::Branch))
    {
        throw std::runtime_error("Branch slot is not empty for loop.pip.");
    }

    put_instruction_in_bundle_slot(
        compact_loop[branch_offset],
        bundle_slot_t::Branch,
     make_loop_pip_instruction(compact_loop_start_cycle)
    );


    return compact_loop;
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

schedule_t prepare_looppip_schedule(
    const schedule_t& allocated_schedule,
    const looppip_schedule_result_t& schedule_result
)
{

    const int loop_body_start_cycle = schedule_result.loop_body_start_cycle;

    const int loop_body_end_cycle = loop_body_start_cycle + schedule_result.num_stages * schedule_result.ii;

    // Step 1. Copy BB0 and the delay bundles before the multi-stage loop body.
    schedule_t final_schedule =
        copy_schedule_range(
            allocated_schedule,
            0,
            loop_body_start_cycle
        );

    // Step 2. Insert EC and p32 initialization before the compact loop body.
    insert_loop_pip_initialization(
        final_schedule,
        schedule_result.num_stages
    );

   
    const int compact_loop_start_cycle =
        static_cast<int>(final_schedule.size());

    // Step 3. Compact the multi-stage loop body into II bundles.
    schedule_t compact_loop =
        compact_loop_body(
            allocated_schedule,
            schedule_result,
            compact_loop_start_cycle
        );

    for (const bundle_t& bundle : compact_loop)
    {
        final_schedule.push_back(bundle);
    }

    // Step 4. Append BB2 and any delay bundles after the multi-stage loop body.
    schedule_t bb2_schedule =
        copy_schedule_range(
            allocated_schedule,
            loop_body_end_cycle,
            static_cast<int>(allocated_schedule.size())
        );

    for (const bundle_t& bundle : bb2_schedule)
    {
        final_schedule.push_back(bundle);
    }

    trim_trailing_empty_bundles(final_schedule);

    return final_schedule;
}