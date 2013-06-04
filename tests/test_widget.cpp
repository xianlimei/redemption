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
   Author(s): Christophe Grosjean, Javier Caverni
   Based on xrdp Copyright (C) Jay Sorg 2004-2010

   Unit test to widget.cpp file
   Using lib boost functions, some tests need to be added
*/

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestCreateWidget
#include <boost/test/auto_unit_test.hpp>

#define LOGPRINT
#include "log.hpp"

#include "mod_api.hpp"

#include "internal/widget/widget_screen.hpp"
#include "internal/widget/widget_window.hpp"

struct TestWidgetMod : mod_api
{
    char buffer[32768];
    char * result;
    TestWidgetMod() 
    : mod_api(0, 0)
    , result(this->buffer) {
    }
    virtual void mod_event(int event_id) {}
    virtual void begin_update() { result += sprintf(result, "begin_update()\n"); }
    virtual void end_update() {result += sprintf(result, "end_update()\n");}
    virtual void server_draw_text(int16_t x, int16_t y, const char * text, uint32_t fgcolor, uint32_t bgcolor, const Rect & clip)
    {
        result += sprintf(result, "server_draw_text()\n");
    }
    virtual void text_metrics(const char * text, int & width, int & height)
    {
        result += sprintf(result, "text_metrics()\n");
    }
    virtual void draw(const RDPOpaqueRect & cmd, const Rect & clip)
    {
        result += sprintf(result, "draw(RDPOpaqueRect(%d, %d, %d, %d, %06x), clip(%d, %d, %d, %d))\n", 
                          cmd.rect.x, cmd.rect.y, cmd.rect.cx, cmd.rect.cy,
                          cmd.color, 
                          clip.x, clip.y, clip.cx, clip.cy);
    }
    virtual void draw(const RDPScrBlt & cmd, const Rect &clip)
    {
        result += sprintf(result, "draw(RDPScrBlt(), clip(%d, %d, %d, %d))\n", clip.x, clip.y, clip.cx, clip.cy);
    }
    virtual void draw(const RDPDestBlt & cmd, const Rect &clip)
    {
        result += sprintf(result, "draw(RDPDestBlt(), clip(%d, %d, %d, %d))\n", clip.x, clip.y, clip.cx, clip.cy);
    }
    virtual void draw(const RDPPatBlt & cmd, const Rect &clip)
    {
        result += sprintf(result, "draw(RDPPatBlt(), clip(%d, %d, %d, %d))\n", clip.x, clip.y, clip.cx, clip.cy);
    }
    virtual void draw(const RDPMemBlt & cmd, const Rect & clip, const Bitmap & bmp)
    {
        result += sprintf(result, "draw(RDPMemBlt(), clip(%d, %d, %d, %d))\n", clip.x, clip.y, clip.cx, clip.cy);
    }
    virtual void draw(const RDPMem3Blt & cmd, const Rect & clip, const Bitmap & bmp)
    {
        result += sprintf(result, "draw(RDPMem3Blt(), clip(%d, %d, %d, %d))\n", clip.x, clip.y, clip.cx, clip.cy);
    }
    virtual void draw(const RDPLineTo& cmd, const Rect & clip)
    {
        result += sprintf(result, "draw(RDPLineTo(), clip(%d, %d, %d, %d))\n", clip.x, clip.y, clip.cx, clip.cy);
    }
    virtual void draw(const RDPGlyphIndex & cmd, const Rect & clip)
    {
        result += sprintf(result, "draw(RDPGlyphIndex(), clip(%d, %d, %d, %d))\n", clip.x, clip.y, clip.cx, clip.cy);
    }
    
    virtual void rdp_input_scancode(long param1, long param2, long param3, long param4, Keymap2 * keymap) {}
    virtual void rdp_input_mouse(int device_flags, int x, int y, Keymap2 * keymap) {}
    virtual void rdp_input_synchronize(uint32_t time, uint16_t device_flags, int16_t param1, int16_t param2) {}
    virtual void rdp_input_invalidate(const Rect & r) {}
    virtual void send_to_front_channel(const char*, uint8_t*, size_t, size_t, int){}
    virtual BackEvent_t draw_event(){ return BACK_EVENT_NONE; }
};


BOOST_AUTO_TEST_CASE(TestCreateScreen)
{
    TestWidgetMod mod;
    Widget *screen = new widget_screen(&mod, 20, 10);
    BOOST_CHECK_EQUAL((int)WND_TYPE_SCREEN, screen->type);
    BOOST_CHECK_EQUAL(Rect(0, 0, 20, 10), screen->rect);
    const Rect rect(1, 2, 3, 4);
    mod.begin_update();
    screen->draw(rect);
    mod.end_update();
    const char * expected =
        "begin_update()\n"
        "draw(RDPOpaqueRect(0, 0, 20, 10, 000000), clip(1, 2, 3, 4))\n"
        "end_update()\n"
        ;
        
    if (0 != strcmp(expected, mod.buffer)){
        LOG(LOG_ERR, "expected:\n%s\n", expected); 
        LOG(LOG_ERR, "got:\n%s\n", mod.buffer); 
        BOOST_CHECK(false);
    }
    delete screen;
}

BOOST_AUTO_TEST_CASE(TestCreateScreen2)
{
    TestWidgetMod mod;
    Widget *screen = new widget_screen(&mod, 800, 600);
    BOOST_CHECK_EQUAL((int)WND_TYPE_SCREEN, screen->type);
    BOOST_CHECK_EQUAL(Rect(0, 0, 800, 600), screen->rect);
    Widget *w = new window(&mod, Rect(10, 10, 400, 200), screen, RED, "window 1");

    const Rect rect(0, 0, 800, 600);
    mod.begin_update();
    screen->draw(rect);
    mod.end_update();
    const char * expected = 
    "begin_update()\n"
    // screen background around the window
    "draw(RDPOpaqueRect(0, 0, 800, 600, 000000), clip(0, 0, 800, 10))\n"
    "draw(RDPOpaqueRect(0, 0, 800, 600, 000000), clip(0, 10, 10, 200))\n"
    "draw(RDPOpaqueRect(0, 0, 800, 600, 000000), clip(410, 10, 390, 200))\n"
    "draw(RDPOpaqueRect(0, 0, 800, 600, 000000), clip(0, 210, 800, 390))\n"
    // ------------- window 1 ----------------------
    // RED Window (as defined above)
    "draw(RDPOpaqueRect(10, 10, 400, 200, 0000ff), clip(10, 10, 400, 200))\n"
    // window borders (some pixels white, grey and black)
    "draw(RDPOpaqueRect(11, 11, 398, 1, ffffff), clip(10, 10, 400, 200))\n"
    "draw(RDPOpaqueRect(11, 11, 1, 198, ffffff), clip(10, 10, 400, 200))\n"
    "draw(RDPOpaqueRect(11, 208, 398, 1, 808080), clip(10, 10, 400, 200))\n"
    "draw(RDPOpaqueRect(408, 11, 1, 200, 808080), clip(10, 10, 400, 200))\n"
    "draw(RDPOpaqueRect(10, 209, 400, 1, 000000), clip(10, 10, 400, 200))\n"
    "draw(RDPOpaqueRect(409, 10, 1, 200, 000000), clip(10, 10, 400, 200))\n"
    "draw(RDPOpaqueRect(13, 13, 395, 18, 808080), clip(10, 10, 400, 200))\n"
    "server_draw_text()\n"
    "end_update()\n"
        ;
        
    if (0 != strcmp(expected, mod.buffer)){
        LOG(LOG_ERR, "expected:\n%s\n", expected); 
        LOG(LOG_ERR, "got:\n %s\n", mod.buffer); 
        BOOST_CHECK(false);
    }
    delete w;
    delete screen;
}

