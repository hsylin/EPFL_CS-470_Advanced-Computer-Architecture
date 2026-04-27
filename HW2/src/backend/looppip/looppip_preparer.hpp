#pragma once

#include "looppip_scheduler.hpp"

#include "../../common/schedule.hpp"

schedule_t prepare_looppip_schedule(
    const schedule_t& allocated_schedule,
    const looppip_schedule_result_t& schedule_result
);