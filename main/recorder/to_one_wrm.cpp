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

#include "to_one_wrm.hpp"
#include "wrm_recorder.hpp"
#include "timer_compute.hpp"
#include "nativecapture.hpp"
#include "urt.hpp"

void to_one_wrm(WRMRecorder& recorder, const char* outfile,
                std::size_t start, std::size_t stop, const char* metaname,
                CipherMode::enum_t mode,
                const unsigned char * key, const unsigned char * iv
)
{
    NativeCapture capture(recorder.meta().width,
                          recorder.meta().height,
                          outfile, metaname,
                          mode, key, iv
                         );
    recorder.consumer(&capture);
    TimerCompute timercompute(recorder);

    timeval mstart = timercompute.start();
    uint64_t mtime = timercompute.advance_second(start);
    GraphicsToFile& caprecorder = capture.recorder;

    if (start && !mtime)
        return /*0*/;

    if (mstart.tv_sec != 0)
    {
        if (mtime)
        {
            URT urt(mtime);
            urt += mstart;
            mstart = urt.tv;
        }
        capture.send_time_start_order(mstart);
        fprintf(capture.meta_file, "%s, %ld %ld\n", capture.filename, mstart.tv_sec, mstart.tv_usec);
    }
    else {
        fprintf(capture.meta_file, "%s, %ld %ld\n",
                capture.filename, mstart.tv_sec, mstart.tv_usec);
    }

    if (mtime){
        caprecorder.timestamp(mtime);
        caprecorder.timer += mtime;
    }

    timercompute.usec() = mtime - start;
    mtime = TimerCompute::coeff_sec_to_usec * (stop - start);
    BStream& stream = recorder.reader.stream;

    while (recorder.selected_next_order())
    {
        if (timercompute.interpret_is_time_chunk()) {
            if (timercompute.chunk_time_value) {
                caprecorder.timestamp(timercompute.chunk_time_value);
                caprecorder.timer += timercompute.chunk_time_value;
            }

            if (mtime <= timercompute.usec()){
                break;
            }
        }
        else {
            switch (recorder.chunk_type()) {
                case WRMChunk::NEXT_FILE_ID:
                    recorder.interpret_order();
                    break;
                case WRMChunk::META_FILE:
                case WRMChunk::TIME_START:
                    recorder.ignore_chunks();
                    break;
                case WRMChunk::BREAKPOINT:
                {
                    recorder.ignore_chunks();
                    recorder.selected_next_order();

                    while (1)
                    {
                        recorder.reader.stream.init(14);
                        recorder.reader.trans->recv(&recorder.reader.stream.end, 14);
                        if (recorder.reader.stream.in_uint16_le() == 8192 * 3 + 1){
                            break;
                        }
                        recorder.reader.stream.p += 8;
                        uint32_t buffer_size = recorder.reader.stream.in_uint32_le();
                        recorder.reader.stream.init(buffer_size);
                        recorder.reader.trans->recv(&recorder.reader.stream.end, buffer_size);
                    }

                    recorder.ignore_chunks();
                }
                    break;
                default:
                    caprecorder.trans->send(stream.data,
                                            stream.size());
                    recorder.ignore_chunks();
                    break;
            }
        }
    }

    //return frame;
}
