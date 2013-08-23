/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Product name: redemption, a FLOSS RDP proxy
   Copyright (C) Wallix 2010
   Author(s): Christophe Grosjean, Javier Caverni, Meng Tan

*/

#ifndef _REDEMPTION_MOD_MOD_API_HPP_
#define _REDEMPTION_MOD_MOD_API_HPP_

#include "RDP/RDPGraphicDevice.hpp"
#include "front_api.hpp"
#include "wait_obj.hpp"
#include "callback.hpp"
#include "config.hpp"
#include "font.hpp"
#include "draw_api.hpp"

enum {
    BUTTON_STATE_UP   = 0,
    BUTTON_STATE_DOWN = 1,
};
class Timeout {

    time_t timeout;

public:
    typedef enum {
        TIMEOUT_REACHED,
        TIMEOUT_NOT_REACHED,
        TIMEOUT_INACTIVE
    } timeout_result_t;

    Timeout(time_t now, time_t length = 0)
        : timeout(length ?(now + length):0)
    {
    }

    ~Timeout() {}

    timeout_result_t check(time_t now)
    {
        if (this->timeout) {
            if (now > this->timeout) {
                return TIMEOUT_REACHED;
            }
            else {
                return TIMEOUT_NOT_REACHED;
            }
        }
        return TIMEOUT_INACTIVE;
    }

    void cancel_timeout() {
        this->timeout = 0;
    }

    void restart_timeout(time_t now, time_t length) {
        this->timeout = now + length;
    }

};

struct mod_api : public Callback, public DrawApi {
    wait_obj event;
    RDPPen   pen;
    bool     pointer_displayed;

    uint16_t front_width;
    uint16_t front_height;

    time_t now;

    mod_api(const uint16_t front_width, const uint16_t front_height)
        : event(-1)
        , front_width(front_width)
        , front_height(front_height)
        , now(0)
    {
        this->pointer_displayed = false;
        this->event.set(0);
    }

    virtual ~mod_api() {}

    virtual void send_to_front_channel(const char * const mod_channel_name, uint8_t* data, size_t length, size_t chunk_size, int flags) = 0;

    // draw_event is run when mod socket received some data (drawing order)
    // or auto-generated by modules, say to comply to some refresh.
    // draw event decodes incoming traffic from backend and eventually calls front to draw things
    // may raise an exception (say if connection to server is closed), but returns nothings
    virtual void draw_event(void) = 0;

    // used when context changed to avoid creating a new module
    // it usually perform some task identical to what constructor does
    // henceforth it should often be called by constructors
    virtual void refresh_context(Inifile & ini){
    }

    bool get_pointer_displayed() {
        return this->pointer_displayed;
    }

    void set_pointer_display() {
        this->pointer_displayed = true;
    }
    void update_time(time_t now) {
        this->now = now;
    }
};

#endif
