/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   Product name: redemption, a FLOSS RDP proxy
 *   Copyright (C) Wallix 2010-2012
 *   Author(s): Christophe Grosjean, Dominique Lafages, Jonathan Poelen
 */

#include "to_wrm.hpp"
#include "wrm_recorder_option.hpp"
#include "wrm_recorder.hpp"
#include "timer_compute.hpp"
#include "capture.hpp"

void to_wrm(WRMRecorder& recorder, const char* outfile,
            std::size_t start, std::size_t stop, std::size_t interval,
            uint frame_limit,
            bool screenshot_start, bool screenshot_wrm,
            const char* metaname
)
{
    Capture capture(recorder.meta().width,
                    recorder.meta().height,
                    outfile, metaname,
                    0, 0, true);
    recorder.consumer(&capture);
    TimerCompute timercompute(recorder);
    uint64_t msecond = timercompute.start();

    //std::cout << "start: " << msecond << '\n';
    uint64_t mtime = timercompute.advance_second(start);

    //std::cout << "mtime: " << mtime << '\n';
    if (start && !mtime)
        return /*0*/;
    if (msecond){
        capture.timestamp(msecond);
        msecond += mtime;
        capture.timer().sec()  = msecond / TimerCompute::coeff_sec_to_usec;
        capture.timer().usec() = msecond % TimerCompute::coeff_sec_to_usec;
    }

    if (screenshot_wrm && screenshot_start)
        capture.dump_png();

    //uint64_t chunk_time = 0;
    timercompute.usec() = mtime - start;
    uint frame = 0;
    mtime = TimerCompute::coeff_sec_to_usec * interval;
    msecond = TimerCompute::coeff_sec_to_usec * (stop - start);

    while (recorder.selected_next_order())
    {
        if (timercompute.interpret_is_time_chunk()) {
            uint64_t usec = timercompute.usec();
            if (timercompute.chunk_time_value) {
                //chunk_time += timercompute.chunk_time_value;
                //std::cout << "chunk_time: " << chunk_time << '\n';
                capture.timestamp(timercompute.chunk_time_value);
                capture.timer() += timercompute.chunk_time_value;
            }

            if (usec >= mtime) {
                /*if (chunk_time) {
                    *                   std::cout << "timestamp + breakpoint chunk_time: " << chunk_time  << '\n';
                    *                   capture.timestamp(chunk_time);
                    *                   chunk_time = 0;
                }*/
                capture.breakpoint();
                if (screenshot_wrm)
                    capture.dump_png();
                timercompute.reset();
                if (++frame == frame_limit)
                    break;
            }

            if (msecond <= usec){
                msecond = 0;
                break;
            } else {
                msecond -= usec;
            }
        }
        else {
            /*if (chunk_time) {
                *               std::cout << "timestamp chunk_time: " << chunk_time  << '\n';
                *               capture.timestamp(chunk_time);
                *               chunk_time = 0;
            }*/

            recorder.interpret_order();
        }
    }

    /*if (chunk_time) {
        *       capture.timestamp(chunk_time);
    }*/

    //return frame;
}
