/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Product name: redemption, a FLOSS RDP proxy
   Copyright (C) Wallix 2010-2013
   Author(s): Christophe Grosjean, Clément Moroldo
*/



#pragma once

#include "utils/log.hpp"



#include "../client_input_output_api.hpp"
#include "keymaps/qt_scancode_keymap.hpp"
#include "qt_graphics_components/qt_progress_bar_window.hpp"
#include "qt_graphics_components/qt_options_window.hpp"
#include "qt_graphics_components/qt_screen_window.hpp"
#include "qt_graphics_components/qt_form_window.hpp"



#include <QtGui/QBitmap>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QRgb>
#include <QtGui/QWheelEvent>
// #include <QtGui/QWindowsCEStyle>

#if REDEMPTION_QT_VERSION == 4
#   include <QtCore/QUrl>
#   define REDEMPTION_QT_INCLUDE_WIDGET(name) <QtGui/name>
#else
#   define REDEMPTION_QT_INCLUDE_WIDGET(name) <QtWidgets/name>
#endif

#include REDEMPTION_QT_INCLUDE_WIDGET(QApplication)
#include REDEMPTION_QT_INCLUDE_WIDGET(QDesktopWidget)

#undef REDEMPTION_QT_INCLUDE_WIDGET




class QtIOGraphicMouseKeyboard : public ClientOutputGraphicAPI, public ClientInputMouseKeyboardAPI
{

public:
    int                  mod_bpp;
    Form_Qt            * form;
    Screen_Qt          * screen;
    QPixmap              cache;
    ProgressBarWindow  * bar;
    QPainter             painter;
    QImage cursor_image;
    std::map<uint32_t, Screen_Qt *> remote_app_screen_map;
    //     QPixmap            * trans_cache;
    Qt_ScanCode_KeyMap   qtRDPKeymap;



    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    //------------------------
    //      CONSTRUCTOR
    //------------------------

    QtIOGraphicMouseKeyboard()
      : ClientOutputGraphicAPI(QApplication::desktop()->width(), QApplication::desktop()->height())
      , ClientInputMouseKeyboardAPI()
      , mod_bpp(24)
      , form(nullptr)
      , screen(nullptr)
//       , trans_cache(nullptr)
      , qtRDPKeymap()

    {
        this->form = new Form_Qt(this);
    }



    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    //-----------------------------
    // WINDOW MANAGEMENT FUNCTIONS
    //-----------------------------

    virtual void set_drawn_client(ClientRedemptionIOAPI * client) override {
        this->drawn_client = client;
        this->qtRDPKeymap.setKeyboardLayout(this->drawn_client->info.keylayout);
    }

    virtual void show_screen() override {
         this->form->hide();
        if (this->screen) {
            this->screen->show();
        }
    }

    virtual void set_screen_size(int x, int y) override {
        if (this->screen) {
            this->screen->setFixedSize(x, y);
        }
    }

    virtual void reset_cache(int w,  int h) override {

        if (this->painter.isActive()) {
            this->painter.end();
        }
        this->cache = QPixmap(w, h);
        this->painter.begin(&(this->cache));
        this->painter.fillRect(0, 0, w, h, Qt::black);
    }

    virtual void create_screen() override {
        QPixmap * map = &(this->cache);
        this->screen = new Screen_Qt(this->drawn_client, this, map);
    }

    virtual void create_screen(std::string const & movie_dir, std::string const & movie_path) override {
        QPixmap * map = &(this->cache);
        this->screen = new Screen_Qt(this->drawn_client, this, movie_dir, movie_path, map);
    }

    QWidget * get_static_qwidget() {
        return this->form;
    }

    virtual void open_options() override {
        new DialogOptions_Qt(this->drawn_client, this->form);
    }

    void dropScreen() override {
        if (this->screen != nullptr) {
            this->screen->disconnection();
            this->screen = nullptr;
        }
    }

    virtual void update_keylayout() override {
        this->qtRDPKeymap.setKeyboardLayout(this->client->info.keylayout);
    }

    void readError(std::string const & movie_path) {
        const std::string errorMsg("Cannot read movie \""+movie_path+ "\".");
        LOG(LOG_INFO, "%s", errorMsg.c_str());
        std::string labelErrorMsg("<font color='Red'>"+errorMsg+"</font>");

        this->form->set_ErrorMsg(labelErrorMsg);
    }

    virtual void set_ErrorMsg(std::string const & error_msg) override {
        this->form->set_ErrorMsg(error_msg);
    }

    virtual void init_form() override {
        this->form->init_form();
        this->form->set_IPField(this->client->target_IP);
        this->form->set_portField(this->client->port);
        this->form->set_PWDField(this->client->user_password);
        this->form->set_userNameField(this->client->user_name);
        this->form->show();
    }



    /////////////////////////////////////////////////////////////////////////////////////////////////
    //  REMOTE APP FUNCTIONS
    //////////////////////////

    virtual void create_remote_app_screen(uint32_t id, int w, int h, int x, int y) override {
        this->remote_app_screen_map.insert(std::pair<uint32_t, Screen_Qt *>(id, nullptr));
        this->remote_app_screen_map[id] = new Screen_Qt(this->drawn_client, this, w, h, x, y, &(this->cache));
    }

    virtual void show_screen(uint32_t id) override {
        this->form->hide();
        if (this->remote_app_screen_map[id]) {
            this->remote_app_screen_map[id]->show();
        }
    }

    virtual void move_screen(uint32_t id, int x, int y) override {
        if (this->remote_app_screen_map[id]) {
            this->remote_app_screen_map[id]->move(x, y);
        }
    }

    virtual void set_screen_size(uint32_t id, int x, int y) override {
        if (this->remote_app_screen_map[id]) {
            this->remote_app_screen_map[id]->setFixedSize(x, y);
        }
    }

    virtual void set_pixmap_shift(uint32_t id, int x, int y) override {
        if (this->remote_app_screen_map[id]) {
            this->remote_app_screen_map[id]->x_pixmap_shift = x;
            this->remote_app_screen_map[id]->y_pixmap_shift = y;
        }
    }

    virtual int get_visible_width(uint32_t id) override {
        return this->remote_app_screen_map[id]->width();
    }

    virtual int get_visible_height(uint32_t id) override {
        return this->remote_app_screen_map[id]->height();
    }

    virtual int get_mem_width(uint32_t id) override {
        return this->remote_app_screen_map[id]->_width;
    }

    virtual int get_mem_height(uint32_t id) override {
        return this->remote_app_screen_map[id]->_height;
    }

    virtual void set_mem_size(uint32_t id, int w, int h) override {
        this->remote_app_screen_map[id]->_width = w;
        this->remote_app_screen_map[id]->_height = h;
    }

    void dropScreen(uint32_t id) override {
        if (this->remote_app_screen_map[id] != nullptr) {
            this->remote_app_screen_map[id]->disconnection();
            this->remote_app_screen_map[id] = nullptr;
        }

        std::map<uint32_t, Screen_Qt *>::iterator it = remote_app_screen_map.find(id);
        remote_app_screen_map.erase (it);
    }

    virtual void clear_remote_app_screen() override {
        for (std::map<uint32_t, Screen_Qt *>::iterator it=this->remote_app_screen_map.begin(); it!=this->remote_app_screen_map.end(); ++it) {
            if (it->second) {
                it->second->disconnection();
                it->second = nullptr;
            }
        }
        this->remote_app_screen_map.clear();

        this->form->show();
    }



//      void setScreenDimension() {
//         if (!this->is_spanning) {
//             this->_screen_dimensions[0].cx = this->client->info.width;
//             this->_screen_dimensions[0].cy = this->client->info.height;
//
//         } else {
//
//             QDesktopWidget* desktop = QApplication::desktop();
//             int screen_count(desktop->screenCount());
//             if (this->_monitorCount > screen_count) {
//                 this->_monitorCount = screen_count;
//             }
//             this->client->info.width  = 0;
//             this->client->info.height = 0;
//             this->client->info.cs_monitor.monitorCount = this->_monitorCount;
//
//             for (int i = 0; i < this->_monitorCount; i++) {
//                 const QRect rect = desktop->screenGeometry(i);
//                 this->_screen_dimensions[i].x   = this->client->info.width;
//                 this->client->info.cs_monitor.monitorDefArray[i].left   = this->client->info.width;
//                 this->client->info.width  += rect.width();
//
//                 if (this->client->info.height < rect.height()) {
//                     this->client->info.height = rect.height();
//                 }
//                 this->client->info.cs_monitor.monitorDefArray[i].top    = rect.top();
//                 this->client->info.cs_monitor.monitorDefArray[i].right  = this->client->info.width + rect.width() - 1;
//                 this->client->info.cs_monitor.monitorDefArray[i].bottom = rect.height() - 1 - 3*Screen_Qt::BUTTON_HEIGHT;
//
//                 this->client->info.cs_monitor.monitorDefArray[i].flags  = 0;
//
//                 this->_screen_dimensions[i].y   = 0;
//                 this->_screen_dimensions[i].cx  = rect.width();
//                 this->_screen_dimensions[i].cy  = rect.height() - 3*Screen_Qt::BUTTON_HEIGHT;
//             }
//             this->client->info.cs_monitor.monitorDefArray[0].flags  = GCC::UserData::CSMonitor::TS_MONITOR_PRIMARY;
//             this->client->info.height -= 3*Screen_Qt::BUTTON_HEIGHT;
//         }
//     }

    void begin_update() override {


    }

    void end_update() override {
        if (this->drawn_client->mod_state == ClientRedemptionIOAPI::MOD_RDP_REMOTE_APP) {
            for (std::map<uint32_t, Screen_Qt *>::iterator it=this->remote_app_screen_map.begin(); it!=this->remote_app_screen_map.end(); ++it) {
                if (it->second) {
                    it->second->update_view();
                }
            }
        } else {
            if (this->screen != nullptr) {
                this->screen->update_view();
            }
        }

    }

    FrontAPI::ResizeResult server_resize(int width, int height, int bpp) override {

        if (this->client->mod_state == ClientRedemptionIOAPI::MOD_RDP_REMOTE_APP) {
            return FrontAPI::ResizeResult::remoteapp;
        }

        if (width == 0 || height == 0) {
            return FrontAPI::ResizeResult::fail;
        }

        if ((this->client->connected || this->client->is_replaying) && this->screen != nullptr) {
            this->client->info.bpp = bpp;

            if (this->client->info.width != width || this->client->info.height != height) {
                this->client->info.width = width;
                this->client->info.height = height;
                if (this->screen) {
                    this->dropScreen();
                }

                this->reset_cache(this->client->info.width, this->client->info.height);

                if (this->client->is_replaying) {

                    this->screen = new Screen_Qt(this->drawn_client, this, this->client->_movie_dir, this->client->_movie_name, &(this->cache));

                } else {

                    this->screen = new Screen_Qt(this->drawn_client, this, &(this->cache));
                }

                if (this->client->mod_state != ClientRedemptionIOAPI::MOD_RDP_REMOTE_APP) {
                    this->screen->show();
                }
            }
        }

        return FrontAPI::ResizeResult::instant_done;
    }

    virtual void set_pointer(Pointer const & cursor) override {

        QImage image_data(cursor.data, cursor.width, cursor.height, this->bpp_to_QFormat(24, false));
        QImage image_mask(cursor.mask, cursor.width, cursor.height, QImage::Format_Mono);

        if (cursor.mask[0x48] == 0xFF &&
            cursor.mask[0x49] == 0xFF &&
            cursor.mask[0x4A] == 0xFF &&
            cursor.mask[0x4B] == 0xFF) {

            image_mask = image_data.convertToFormat(QImage::Format_ARGB32_Premultiplied);
            image_data.invertPixels();

        } else {
            image_mask.invertPixels();
        }

        image_data = image_data.mirrored(false, true).convertToFormat(QImage::Format_ARGB32_Premultiplied);
        image_mask = image_mask.mirrored(false, true).convertToFormat(QImage::Format_ARGB32_Premultiplied);

        const uchar * data_data = image_data.bits();
        const uchar * mask_data = image_mask.bits();

        uint8_t data[Pointer::DATA_SIZE*4];

        for (int i = 0; i < Pointer::DATA_SIZE; i += 4) {
            data[i  ] = data_data[i+2];
            data[i+1] = data_data[i+1];
            data[i+2] = data_data[i  ];
            data[i+3] = mask_data[i+0];
        }

        if (this->client->is_replaying) {
            this->cursor_image = QImage(static_cast<uchar *>(data), cursor.x, cursor.y, QImage::Format_ARGB32_Premultiplied);

        } else {
            if (this->drawn_client->mod_state == ClientRedemptionIOAPI::MOD_RDP_REMOTE_APP) {
                 for (std::map<uint32_t, Screen_Qt *>::iterator it=this->remote_app_screen_map.begin(); it!=this->remote_app_screen_map.end(); ++it) {
                    if (it->second) {
                        it->second->set_mem_cursor(static_cast<uchar *>(data), cursor.x, cursor.y);
                    }
                }
            } else if (this->screen) {
                this->screen->set_mem_cursor(static_cast<uchar *>(data), cursor.x, cursor.y);
            }
        }
    }

    void pre_load_movie() override {

        long int movie_length = Screen_Qt::get_movie_time_length(this->client->replay_mod->get_mwrm_path().c_str());
        this->form->hide();
        this->bar = new ProgressBarWindow(this->screen->movie_time);
        long int endin_frame = 0;
        int i = 0;

        while (endin_frame < movie_length) {
            //timeval end_fram_time = {long int(endin_frame), 0};
            this->client->replay_mod.get()->instant_play_client(std::chrono::microseconds(endin_frame*1000000));

            if (this->screen) {
                this->screen->balises.push_back(nullptr);
                this->screen->balises[i] = new QPixmap(this->cache);
                endin_frame += Screen_Qt::BALISED_FRAME;
                i++;
                if (this->bar) {
                    this->bar->setValue(endin_frame);
                }
            }
        }

        this->screen->stopRelease();
    }

//     void answer_question(int color) {
//         QImage image = this->cache.toImage();
//
//         QRgb asked(color);
//
//         QRgb top_left  = image.pixel(0, 0);
//         QRgb top_right = image.pixel(this->client->info.width-1, 0);
//         QRgb bot_left  = image.pixel(0, this->client->info.height-1);
//         QRgb bot_right = image.pixel(this->client->info.width-1, this->client->info.height-1);
//
//         //LOG(LOG_INFO, "         top_left = 0x%04x top_right = 0x%04x bot_left = 0x%04x bot_right = 0x%04x asked_color = 0x%04x, top_left, top_right, bot_left, bot_right, asked);
//
//         if        (top_left == asked) {
//             this->mod->rdp_input_mouse(MOUSE_FLAG_BUTTON1 | MOUSE_FLAG_DOWN, 0, 0, &(this->keymap));
//             this->mod->rdp_input_mouse(MOUSE_FLAG_BUTTON1, 0, 0, &(this->keymap));
//             this->wab_diag_question = false;
// //             LOG(LOG_INFO, "CLIENT >> answer_question top_left");
//
//         } else if (top_right == asked) {
//             this->mod->rdp_input_mouse(MOUSE_FLAG_BUTTON1 | MOUSE_FLAG_DOWN, this->client->info.width-1, 0, &(this->keymap));
//             this->mod->rdp_input_mouse(MOUSE_FLAG_BUTTON1, this->client->info.width-1, 0, &(this->keymap));
//             this->wab_diag_question = false;
// //             LOG(LOG_INFO, "CLIENT >> answer_question top_right");
//
//         } else if (bot_left == asked) {
//             this->mod->rdp_input_mouse(MOUSE_FLAG_BUTTON1 | MOUSE_FLAG_DOWN, 0, this->client->info.height-1, &(this->keymap));
//             this->mod->rdp_input_mouse(MOUSE_FLAG_BUTTON1, 0, this->client->info.height-1, &(this->keymap));
//             this->wab_diag_question = false;
// //             LOG(LOG_INFO, "CLIENT >> answer_question bot_left");
//
//         } else if (bot_right == asked) {
//             this->mod->rdp_input_mouse(MOUSE_FLAG_BUTTON1 | MOUSE_FLAG_DOWN, this->client->info.width-1, this->client->info.height-1, &(this->keymap));
//             this->mod->rdp_input_mouse(MOUSE_FLAG_BUTTON1, this->client->info.width-1, this->client->info.height-1, &(this->keymap));
//             this->wab_diag_question = false;
// //             LOG(LOG_INFO, "CLIENT >> answer_question bot_right");
//
//         }
//     }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //-----------------------
    //   GRAPHIC FUNCTIONS
    //-----------------------

    struct Op_0x11 {
        uchar op(const uchar src, const uchar dst) const { // +------+-------------------------------+
             return ~(src | dst);                          // | 0x11 | ROP: 0x001100A6 (NOTSRCERASE) |
        }                                                  // |      | RPN: DSon                     |
    };                                                     // +------+-------------------------------+

    struct Op_0x22 {
        uchar op(const uchar src, const uchar dst) const { // +------+-------------------------------+
             return (~src & dst);                          // | 0x22 | ROP: 0x00220326               |
        }                                                  // |      | RPN: DSna                     |
    };                                                     // +------+-------------------------------+

    struct Op_0x33 {
        uchar op(const uchar src, const uchar) const {     // +------+-------------------------------+
             return (~src);                                // | 0x33 | ROP: 0x00330008 (NOTSRCCOPY)  |
        }                                                  // |      | RPN: Sn                       |
    };                                                     // +------+-------------------------------+

    struct Op_0x44 {
        uchar op(const uchar src, const uchar dst) const { // +------+-------------------------------+
            return (src & ~dst);                           // | 0x44 | ROP: 0x00440328 (SRCERASE)    |
        }                                                  // |      | RPN: SDna                     |
    };                                                     // +------+-------------------------------+

    struct Op_0x55 {
        uchar op(const uchar, const uchar dst) const {     // +------+-------------------------------+
             return (~dst);                                // | 0x55 | ROP: 0x00550009 (DSTINVERT)   |
        }                                                  // |      | RPN: Dn                       |
    };                                                     // +------+-------------------------------+

    struct Op_0x66 {
        uchar op(const uchar src, const uchar dst) const { // +------+-------------------------------+
            return (src ^ dst);                            // | 0x66 | ROP: 0x00660046 (SRCINVERT)   |
        }                                                  // |      | RPN: DSx                      |
    };                                                     // +------+-------------------------------+

    struct Op_0x77 {
        uchar op(const uchar src, const uchar dst) const { // +------+-------------------------------+
             return ~(src & dst);                          // | 0x77 | ROP: 0x007700E6               |
        }                                                  // |      | RPN: DSan                     |
    };                                                     // +------+-------------------------------+

    struct Op_0x88 {
        uchar op(const uchar src, const uchar dst) const { // +------+-------------------------------+
            return (src & dst);                            // | 0x88 | ROP: 0x008800C6 (SRCAND)      |
        }                                                  // |      | RPN: DSa                      |
    };                                                     // +------+-------------------------------+

    struct Op_0x99 {
        uchar op(const uchar src, const uchar dst) const { // +------+-------------------------------+
            return ~(src ^ dst);                           // | 0x99 | ROP: 0x00990066               |
        }                                                  // |      | RPN: DSxn                     |
    };                                                     // +------+-------------------------------+

    struct Op_0xBB {
        uchar op(const uchar src, const uchar dst) const { // +------+-------------------------------+
            return (~src | dst);                           // | 0xBB | ROP: 0x00BB0226 (MERGEPAINT)  |
        }                                                  // |      | RPN: DSno                     |
    };                                                     // +------+-------------------------------+

    struct Op_0xDD {
        uchar op(const uchar src, const uchar dst) const { // +------+-------------------------------+
            return (src | ~dst);                           // | 0xDD | ROP: 0x00DD0228               |
        }                                                  // |      | RPN: SDno                     |
    };                                                     // +------+-------------------------------+

    struct Op_0xEE {
        uchar op(const uchar src, const uchar dst) const { // +------+-------------------------------+
            return (src | dst);                            // | 0xEE | ROP: 0x00EE0086 (SRCPAINT)    |
        }                                                  // |      | RPN: DSo                      |
    };                                                     // +------+-------------------------------+


    QColor u32_to_qcolor(RDPColor color, gdi::ColorCtx color_ctx) {

        if (uint8_t(this->client->info.bpp) != color_ctx.depth().to_bpp()) {
            BGRColor d = color_decode(color, color_ctx);
            color      = color_encode(d, uint8_t(this->client->info.bpp));
        }

        BGRColor bgr = color_decode(color, this->client->info.bpp, this->client->mod_palette);

        return {bgr.red(), bgr.green(), bgr.blue()};
    }

    template<class Op>
    void draw_memblt_op(const Rect & drect, const Bitmap & bitmap) {
        const uint16_t mincx = std::min<int16_t>(bitmap.cx(), std::min<int16_t>(this->client->info.width - drect.x, drect.cx));
        const uint16_t mincy = std::min<int16_t>(bitmap.cy(), std::min<int16_t>(this->client->info.height - drect.y, drect.cy));

        if (mincx <= 0 || mincy <= 0) {
            return;
        }
        if (this->screen) {
            QImage::Format format(this->bpp_to_QFormat(bitmap.bpp(), false)); //bpp
            QImage srcBitmap(bitmap.data(), mincx, mincy, bitmap.line_size(), format);
            QImage dstBitmap(this->cache.toImage().copy(drect.x, drect.y, mincx, mincy));

            if (bitmap.bpp() == 24) {
                srcBitmap = srcBitmap.rgbSwapped();
            }

            if (bitmap.bpp() != this->client->info.bpp) {
                srcBitmap = srcBitmap.convertToFormat(this->bpp_to_QFormat(this->client->info.bpp, false));
            }
            dstBitmap = dstBitmap.convertToFormat(srcBitmap.format());

            QImage srcBitmapMirrored = srcBitmap.mirrored(false, true);
            //uchar data[1600*900*3];

            uchar * data = new uchar[srcBitmapMirrored.byteCount()];

            const uchar * srcData = srcBitmapMirrored.constBits();
            const uchar * dstData = dstBitmap.constBits();

            int data_len = bitmap.line_size() * mincy;
            Op op;
            for (int i = 0; i < data_len; i++) {
                data[i] = op.op(srcData[i], dstData[i]);
            }

            QImage image(data, mincx, mincy, srcBitmapMirrored.format());
            QRect trect(drect.x, drect.y, mincx, mincy);

            if (this->client->connected || this->client->is_replaying) {
                this->painter.drawImage(trect, image);
            }

            delete[](data);
        }
    }

    void draw_MemBlt(const Rect & drect, const Bitmap & bitmap, bool invert, int srcx, int srcy) {
        const int16_t mincx = bitmap.cx();
        const int16_t mincy = bitmap.cy();

        if (mincx <= 0 || mincy <= 0) {
            return;
        }

        const unsigned char * row = bitmap.data();

        QImage qbitmap(row, mincx, mincy, this->bpp_to_QFormat(bitmap.bpp(), false));

        qbitmap = qbitmap.mirrored(false, true);

        qbitmap = qbitmap.copy(srcx, srcy, drect.cx, drect.cy);

        if (invert) {
            qbitmap.invertPixels();
        }

        if (bitmap.bpp() == 24) {
            qbitmap = qbitmap.rgbSwapped();
        }

        const QRect trect(drect.x, drect.y, drect.cx, drect.cy);
        if (this->client->connected || this->client->is_replaying) {
             this->painter.drawImage(trect, qbitmap);
        }
    }


    void draw_RDPScrBlt(int srcx, int srcy, const Rect & drect, bool invert) {
        QImage qbitmap(this->cache.toImage().copy(srcx, srcy, drect.cx, drect.cy));
        if (invert) {
            qbitmap.invertPixels();
        }
        const QRect trect(drect.x, drect.y, drect.cx, drect.cy);
        if (this->client->connected || this->client->is_replaying) {
            this->painter.drawImage(trect, qbitmap);
        }
    }


    QImage::Format bpp_to_QFormat(int bpp, bool alpha) {
        QImage::Format format(QImage::Format_RGB16);

        if (alpha) {

            switch (bpp) {
                case 15: format = QImage::Format_ARGB4444_Premultiplied; break;
                case 16: format = QImage::Format_ARGB4444_Premultiplied; break;
                case 24: format = QImage::Format_ARGB8565_Premultiplied; break;
                case 32: format = QImage::Format_ARGB32_Premultiplied;   break;
                default : break;
            }
        } else {

            switch (bpp) {
                case 15: format = QImage::Format_RGB555; break;
                case 16: format = QImage::Format_RGB16;  break;
                case 24: format = QImage::Format_RGB888; break;
                case 32: format = QImage::Format_RGB32;  break;
                default : break;
            }
        }

        return format;
    }

    void draw_RDPPatBlt(const Rect & rect, const QColor color, const QPainter::CompositionMode mode, const Qt::BrushStyle style) {
        QBrush brush(color, style);
        if (this->client->connected || this->client->is_replaying) {
            this->painter.setBrush(brush);
            this->painter.setCompositionMode(mode);
            this->painter.drawRect(rect.x, rect.y, rect.cx, rect.cy);
            this->painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            this->painter.setBrush(Qt::SolidPattern);
        }
    }

    void draw_RDPPatBlt(const Rect & rect, const QPainter::CompositionMode mode) {
        if (this->client->connected || this->client->is_replaying) {
            this->painter.setCompositionMode(mode);
            this->painter.drawRect(rect.x, rect.y, rect.cx, rect.cy);
            this->painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        }
    }



    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //-----------------------------
    //       DRAW FUNCTIONS
    //-----------------------------

    //using ClientRedemptionIOAPI::draw;

    void draw(const RDPPatBlt & cmd, Rect clip, gdi::ColorCtx color_ctx) override {

        const Rect rect = clip.intersect(this->client->info.width, this->client->info.height).intersect(cmd.rect);

        QColor backColor = this->u32_to_qcolor(cmd.back_color, color_ctx);
        QColor foreColor = this->u32_to_qcolor(cmd.fore_color, color_ctx);

        if (cmd.brush.style == 0x03 && (cmd.rop == 0xF0 || cmd.rop == 0x5A)) { // external

            switch (cmd.rop) {

                // +------+-------------------------------+
                // | 0x5A | ROP: 0x005A0049 (PATINVERT)   |
                // |      | RPN: DPx                      |
                // +------+-------------------------------+
                case 0x5A:
                    {
                        QBrush brush(backColor, Qt::Dense4Pattern);
                        if (this->client->connected || this->client->is_replaying) {
                            this->painter.setBrush(brush);
                            this->painter.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
                            this->painter.drawRect(rect.x, rect.y, rect.cx, rect.cy);
                            this->painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
                            this->painter.setBrush(Qt::SolidPattern);
                        }
                    }
                    break;

                // +------+-------------------------------+
                // | 0xF0 | ROP: 0x00F00021 (PATCOPY)     |
                // |      | RPN: P                        |
                // +------+-------------------------------+
                case 0xF0:
                    {
                        if (this->client->connected || this->client->is_replaying) {
                            QBrush brush(foreColor, Qt::Dense4Pattern);
                            this->painter.setPen(Qt::NoPen);
                            this->painter.fillRect(rect.x, rect.y, rect.cx, rect.cy, backColor);
                            this->painter.setBrush(brush);
                            this->painter.drawRect(rect.x, rect.y, rect.cx, rect.cy);
                            this->painter.setBrush(Qt::SolidPattern);
                        }
                    }
                    break;
                default: LOG(LOG_WARNING, "RDPPatBlt brush_style = 0x03 rop = %x", cmd.rop);
                    break;
            }

        } else {
            switch (cmd.rop) {

                case 0x00: // blackness
                    if (this->client->connected || this->client->is_replaying) {
                        this->painter.fillRect(rect.x, rect.y, rect.cx, rect.cy, Qt::black);
                    }
                    break;
                    // +------+-------------------------------+
                    // | 0x05 | ROP: 0x000500A9               |
                    // |      | RPN: DPon                     |
                    // +------+-------------------------------+

                    // +------+-------------------------------+
                    // | 0x0F | ROP: 0x000F0001               |
                    // |      | RPN: Pn                       |
                    // +------+-------------------------------+
                case 0x0F:
                    this->draw_RDPPatBlt(rect, QPainter::RasterOp_NotSource);
                    break;
                    // +------+-------------------------------+
                    // | 0x50 | ROP: 0x00500325               |
                    // |      | RPN: PDna                     |
                    // +------+-------------------------------+
                case 0x50:
                    this->draw_RDPPatBlt(rect, QPainter::RasterOp_NotSourceAndNotDestination);
                    break;
                    // +------+-------------------------------+
                    // | 0x55 | ROP: 0x00550009 (DSTINVERT)   |
                    // |      | RPN: Dn                       |
                    // +------+-------------------------------+
                /*case 0x55:
                    this->draw_RDPPatBlt(rect, QPainter::RasterOp_NotDestination);

                    break;*/
                    // +------+-------------------------------+
                    // | 0x5A | ROP: 0x005A0049 (PATINVERT)   |
                    // |      | RPN: DPx                      |
                    // +------+-------------------------------+
                case 0x5A:
                    this->draw_RDPPatBlt(rect, QPainter::RasterOp_SourceXorDestination);
                    break;
                    // +------+-------------------------------+
                    // | 0x5F | ROP: 0x005F00E9               |
                    // |      | RPN: DPan                     |
                    // +------+-------------------------------+

                    // +------+-------------------------------+
                    // | 0xA0 | ROP: 0x00A000C9               |
                    // |      | RPN: DPa                      |
                    // +------+-------------------------------+
                case 0xA0:
                    this->draw_RDPPatBlt(rect, QPainter::RasterOp_SourceAndDestination);
                    break;
                    // +------+-------------------------------+
                    // | 0xA5 | ROP: 0x00A50065               |
                    // |      | RPN: PDxn                     |
                    // +------+-------------------------------+
                /*case 0xA5:
                    // this->draw_RDPPatBlt(rect, QPainter::RasterOp_NotSourceXorNotDestination);
                    break;*/
                    // +------+-------------------------------+
                    // | 0xAA | ROP: 0x00AA0029               |
                    // |      | RPN: D                        |
                    // +------+-------------------------------+
                case 0xAA: // change nothing
                    break;
                    // +------+-------------------------------+
                    // | 0xAF | ROP: 0x00AF0229               |
                    // |      | RPN: DPno                     |
                    // +------+-------------------------------+
                /*case 0xAF:
                    //this->draw_RDPPatBlt(rect, QPainter::RasterOp_NotSourceOrDestination);
                    break;*/
                    // +------+-------------------------------+
                    // | 0xF0 | ROP: 0x00F00021 (PATCOPY)     |
                    // |      | RPN: P                        |
                    // +------+-------------------------------+
                case 0xF0:
                    if (this->client->connected || this->client->is_replaying) {
                        this->painter.setPen(Qt::NoPen);
                        this->painter.fillRect(rect.x, rect.y, rect.cx, rect.cy, backColor);
                        this->painter.drawRect(rect.x, rect.y, rect.cx, rect.cy);
                    }
                    break;
                    // +------+-------------------------------+
                    // | 0xF5 | ROP: 0x00F50225               |
                    // |      | RPN: PDno                     |
                    // +------+-------------------------------+
                //case 0xF5:
                    //this->draw_RDPPatBlt(rect, QPainter::RasterOp_SourceOrNotDestination);
                    //break;
                    // +------+-------------------------------+
                    // | 0xFA | ROP: 0x00FA0089               |
                    // |      | RPN: DPo                      |
                    // +------+-------------------------------+
                case 0xFA:
                    this->draw_RDPPatBlt(rect, QPainter::RasterOp_SourceOrDestination);
                    break;

                case 0xFF: // whiteness
                    if (this->client->connected || this->client->is_replaying) {
                        this->painter.fillRect(rect.x, rect.y, rect.cx, rect.cy, Qt::white);
                    }
                    break;
                default: LOG(LOG_WARNING, "RDPPatBlt rop = %x", cmd.rop);
                    break;
            }
        }
    }


    void draw(const RDPOpaqueRect & cmd, Rect clip, gdi::ColorCtx color_ctx) override {

        if (this->client->connected || this->client->is_replaying) {
            QColor qcolor(this->u32_to_qcolor(cmd.color, color_ctx));
            Rect rect(cmd.rect.intersect(clip));

            this->painter.fillRect(rect.x, rect.y, rect.cx, rect.cy, qcolor);
        }
    }


    void draw(const RDPBitmapData & bitmap_data, const Bitmap & bmp) override {
        if (!bmp.is_valid()){
            return;
        }

        Rect rectBmp( bitmap_data.dest_left, bitmap_data.dest_top,
                                (bitmap_data.dest_right - bitmap_data.dest_left + 1),
                                (bitmap_data.dest_bottom - bitmap_data.dest_top + 1));
        const Rect clipRect(0, 0, this->client->info.width, this->client->info.height);
        const Rect drect = rectBmp.intersect(clipRect);

        const int16_t mincx = std::min<int16_t>(bmp.cx(), std::min<int16_t>(this->client->info.width - drect.x, drect.cx));
        const int16_t mincy = std::min<int16_t>(bmp.cy(), std::min<int16_t>(this->client->info.height - drect.y, drect.cy));;

        if (mincx <= 0 || mincy <= 0) {
            return;
        }

        QImage::Format format(this->bpp_to_QFormat(bmp.bpp(), false)); //bpp
        QImage qbitmap(bmp.data(), mincx, mincy, bmp.line_size(), format);

        if (bmp.bpp() == 24) {
            qbitmap = qbitmap.rgbSwapped();
        }

        if (bmp.bpp() != this->client->info.bpp) {
            qbitmap = qbitmap.convertToFormat(this->bpp_to_QFormat(this->client->info.bpp, false));
            LOG(LOG_INFO, "RDPBitmapData convertToFormat");
        }

        qbitmap = qbitmap.mirrored(false, true);
        QRect trect(drect.x, drect.y, mincx, mincy);
        if (this->client->connected || this->client->is_replaying) {
            this->painter.drawImage(trect, qbitmap);
        }

    }


    void draw(const RDPLineTo & cmd, Rect clip, gdi::ColorCtx color_ctx) override {
        (void) clip;

        // TODO clipping
        if (this->client->connected || this->client->is_replaying) {
            this->screen->setPenColor(this->u32_to_qcolor(cmd.back_color, color_ctx));

            this->painter.drawLine(cmd.startx, cmd.starty, cmd.endx, cmd.endy);
        }

    }


    void draw(const RDPScrBlt & cmd, Rect clip) override {

        //std::cout << "RDPScrBlt" << std::endl;

        const Rect drect = clip.intersect(this->client->info.width, this->client->info.height).intersect(cmd.rect);
        if (drect.isempty()) {
            return;
        }

        int srcx(drect.x + cmd.srcx - cmd.rect.x);
        int srcy(drect.y + cmd.srcy - cmd.rect.y);

        switch (cmd.rop) {

            case 0x00:
                if (this->client->connected || this->client->is_replaying) {
                    this->painter.fillRect(drect.x, drect.y, drect.cx, drect.cy, Qt::black);
                }
                break;

            case 0x55: this->draw_RDPScrBlt(srcx, srcy, drect, true);
                break;

            case 0xAA: // nothing to change
                break;

            case 0xCC: this->draw_RDPScrBlt(srcx, srcy, drect, false);
                break;

            case 0xFF:
                if (this->client->connected || this->client->is_replaying) {
                    this->painter.fillRect(drect.x, drect.y, drect.cx, drect.cy, Qt::white);
                }
                break;
            default: LOG(LOG_WARNING, "DEFAULT: RDPScrBlt rop = %x", cmd.rop);
                break;
        }

    }


    void draw(const RDPMemBlt & cmd, Rect clip, const Bitmap & bitmap) override {
        //std::cout << "RDPMemBlt (" << std::hex << static_cast<int>(cmd.rop) << ")" <<  std::dec <<  std::endl;
        const Rect drect = clip.intersect(cmd.rect);
        if (drect.isempty()){
            return ;
        }

        switch (cmd.rop) {

            case 0x00:
                if (this->client->connected || this->client->is_replaying) {
                    this->painter.fillRect(drect.x, drect.y, drect.cx, drect.cy, Qt::black);
                }
                break;

            case 0x22: this->draw_memblt_op<Op_0x22>(drect, bitmap);
                break;

            case 0x33: this->draw_MemBlt(drect, bitmap, true, cmd.srcx + (drect.x - cmd.rect.x), cmd.srcy + (drect.y - cmd.rect.y));
                break;

            case 0x55:
                this->draw_memblt_op<Op_0x55>(drect, bitmap);
                break;

            case 0x66: this->draw_memblt_op<Op_0x66>(drect, bitmap);
                break;

            case 0x99:  this->draw_memblt_op<Op_0x99>(drect, bitmap);
                break;

            case 0xAA:  // nothing to change
                break;

            case 0xBB: this->draw_memblt_op<Op_0xBB>(drect, bitmap);
                break;

            case 0xCC: this->draw_MemBlt(drect, bitmap, false, cmd.srcx + (drect.x - cmd.rect.x), cmd.srcy + (drect.y - cmd.rect.y));
                break;

            case 0xEE: this->draw_memblt_op<Op_0xEE>(drect, bitmap);
                break;

            case 0x88: this->draw_memblt_op<Op_0x88>(drect, bitmap);
                break;

            case 0xFF:
                if (this->client->connected || this->client->is_replaying) {
                    this->painter.fillRect(drect.x, drect.y, drect.cx, drect.cy, Qt::white);
                }
                break;

            default: LOG(LOG_WARNING, "DEFAULT: RDPMemBlt rop = %x", cmd.rop);
                break;
        }

    }


    void draw(const RDPMem3Blt & cmd, Rect clip, gdi::ColorCtx color_ctx, const Bitmap & bitmap) override {

        const Rect drect = clip.intersect(cmd.rect);
        if (drect.isempty()){
            return ;
        }

        switch (cmd.rop) {
            case 0xB8:
            {
                const uint16_t mincx = std::min<int16_t>(bitmap.cx(), std::min<int16_t>(this->client->info.width  - drect.x, drect.cx));
                const uint16_t mincy = std::min<int16_t>(bitmap.cy(), std::min<int16_t>(this->client->info.height - drect.y, drect.cy));

                if (mincx <= 0 || mincy <= 0) {
                    return;
                }

                const QColor fore(this->u32_to_qcolor(cmd.fore_color, color_ctx));
                const uint8_t r(fore.red());
                const uint8_t g(fore.green());
                const uint8_t b(fore.blue());

                int rowYCoord(drect.y + drect.cy-1);
                const QImage::Format format(this->bpp_to_QFormat(bitmap.bpp(), true));

                if (this->client->connected || this->client->is_replaying) {
                    QImage dstBitmap(this->cache.toImage().copy(drect.x, drect.y, mincx, mincy));
                    QImage srcBitmap(bitmap.data(), mincx, mincy, bitmap.line_size(), format);
                    srcBitmap = srcBitmap.convertToFormat(QImage::Format_RGB888);
                    dstBitmap = dstBitmap.convertToFormat(QImage::Format_RGB888);

                    const size_t rowsize(srcBitmap.bytesPerLine());
                    std::unique_ptr<uchar[]> data = std::make_unique<uchar[]>(rowsize);

                    for (size_t k = 1 ; k < drect.cy; k++) {

                        const uchar * srcData = srcBitmap.constScanLine(k);
                        const uchar * dstData = dstBitmap.constScanLine(mincy - k);

                        for (size_t x = 0; x < rowsize-2; x += 3) {
                            data[x  ] = ((dstData[x  ] ^ r) & srcData[x  ]) ^ r;
                            data[x+1] = ((dstData[x+1] ^ g) & srcData[x+1]) ^ g;
                            data[x+2] = ((dstData[x+2] ^ b) & srcData[x+2]) ^ b;
                        }

                        QImage image(data.get(), mincx, 1, srcBitmap.format());
                        if (image.depth() != this->client->info.bpp) {
                            image = image.convertToFormat(this->bpp_to_QFormat(this->client->info.bpp, false));
                        }
                        QRect trect(drect.x, rowYCoord, mincx, 1);

                        this->painter.drawImage(trect, image);

                        rowYCoord--;
                    }
                }
            }
            break;

            default: LOG(LOG_WARNING, "DEFAULT: RDPMem3Blt rop = %x", cmd.rop);
            break;
        }

    }


    void draw(const RDPDestBlt & cmd, Rect clip) override {

        const Rect drect = clip.intersect(this->client->info.width, this->client->info.height).intersect(cmd.rect);

        switch (cmd.rop) {
            case 0x00: // blackness
                if (this->client->connected || this->client->is_replaying) {
                    this->painter.fillRect(drect.x, drect.y, drect.cx, drect.cy, Qt::black);
                }
                break;
            case 0x55:                                         // inversion
                this->draw_RDPScrBlt(drect.x, drect.y, drect, true);
                break;
            case 0xAA: // change nothing
                break;
            case 0xFF: // whiteness
                if (this->client->connected || this->client->is_replaying) {
                    this->painter.fillRect(drect.x, drect.y, drect.cx, drect.cy, Qt::white);

                }
                break;
            default: LOG(LOG_WARNING, "DEFAULT: RDPDestBlt rop = %x", cmd.rop);
                break;
        }

    }

    void draw(const RDPMultiDstBlt & cmd, Rect clip) override {
        (void) cmd;
        (void) clip;

        LOG(LOG_WARNING, "DEFAULT: RDPMultiDstBlt");
    }

    void draw(const RDPMultiOpaqueRect & cmd, Rect clip, gdi::ColorCtx color_ctx) override {
        (void) cmd;
        (void) clip;
        (void) color_ctx;

        LOG(LOG_WARNING, "DEFAULT: RDPMultiOpaqueRect");
    }

    void draw(const RDP::RDPMultiPatBlt & cmd, Rect clip, gdi::ColorCtx color_ctx) override {
        (void) color_ctx;
        (void) cmd;
        (void) clip;
        LOG(LOG_WARNING, "DEFAULT: RDPMultiPatBlt");
    }

    void draw(const RDP::RDPMultiScrBlt & cmd, Rect clip) override {
        (void) cmd;
        (void) clip;

        LOG(LOG_WARNING, "DEFAULT: RDPMultiScrBlt");
    }

    void draw(const RDPGlyphIndex & cmd, Rect clip, gdi::ColorCtx color_ctx, const GlyphCache & gly_cache) override {

        Rect screen_rect = clip.intersect(this->client->info.width, this->client->info.height);
        if (screen_rect.isempty()){
            return ;
        }

        Rect const clipped_glyph_fragment_rect = cmd.bk.intersect(screen_rect);
        if (clipped_glyph_fragment_rect.isempty()) {
            return;
        }
        //std::cout << "RDPGlyphIndex " << std::endl;

        // set a background color
        {
            Rect ajusted = cmd.f_op_redundant ? cmd.bk : cmd.op;
            if ((ajusted.cx > 1) && (ajusted.cy > 1)) {
                ajusted.cy--;
                ajusted = ajusted.intersect(screen_rect);
                 if (this->client->connected || this->client->is_replaying) {
                    this->painter.fillRect(ajusted.x, ajusted.y, ajusted.cx, ajusted.cy, this->u32_to_qcolor(cmd.fore_color, color_ctx));
                 }
            }
        }

        bool has_delta_bytes = (!cmd.ui_charinc && !(cmd.fl_accel & 0x20));

        const QColor color = this->u32_to_qcolor(cmd.back_color, color_ctx);
        const int16_t offset_y = /*cmd.bk.cy - (*/cmd.glyph_y - cmd.bk.y/* + 1)*/;
        const int16_t offset_x = cmd.glyph_x - cmd.bk.x;

        uint16_t draw_pos = 0;

        InStream variable_bytes(cmd.data, cmd.data_len);

        //uint8_t const * fragment_begin_position = variable_bytes.get_current();

        while (variable_bytes.in_remain()) {
            uint8_t data = variable_bytes.in_uint8();

            if (data <= 0xFD) {
                FontChar const & fc = gly_cache.glyphs[cmd.cache_id][data].font_item;
                if (!fc)
                {
                    LOG( LOG_INFO
                        , "RDPDrawable::draw_VariableBytes: Unknown glyph, cacheId=%u cacheIndex=%u"
                        , cmd.cache_id, data);
                    assert(fc);
                }

                if (has_delta_bytes)
                {
                    data = variable_bytes.in_uint8();
                    if (data == 0x80)
                    {
                        draw_pos += variable_bytes.in_uint16_le();
                    }
                    else
                    {
                        draw_pos += data;
                    }
                }

                if (fc)
                {
                    const int16_t x = draw_pos + cmd.bk.x + offset_x;
                    const int16_t y = offset_y + cmd.bk.y;
                    if (Rect(0,0,0,0) != clip.intersect(Rect(x, y, fc.incby, fc.height))){

                        const uint8_t * fc_data            = fc.data.get();
                        for (int yy = 0 ; yy < fc.height; yy++)
                        {
                            uint8_t   fc_bit_mask        = 128;
                            for (int xx = 0 ; xx < fc.width; xx++)
                            {
                                if (!fc_bit_mask)
                                {
                                    fc_data++;
                                    fc_bit_mask = 128;
                                }
                                if (clip.contains_pt(x + fc.offset + xx, y + fc.baseline + yy)
                                && (fc_bit_mask & *fc_data))
                                {
                                    if (this->client->connected || this->client->is_replaying) {
                                        this->painter.fillRect(x + fc.offset + xx, y + fc.baseline + yy, 1, 1, color);
                                    }
                                }
                                fc_bit_mask >>= 1;
                            }
                            fc_data++;
                        }
                    }
                } else {
                    LOG(LOG_WARNING, "DEFAULT: RDPGlyphIndex glyph_cache unknow FontChar");
                }

                if (cmd.ui_charinc) {
                    draw_pos += cmd.ui_charinc;
                }

            } else {
                LOG(LOG_WARNING, "DEFAULT: RDPGlyphIndex glyph_cache 0xFD");
            }


        }
        //this->draw_VariableBytes(cmd.data, cmd.data_len, has_delta_bytes,
            //draw_pos, offset_y, color, cmd.bk.x + offset_x, cmd.bk.y,
            //clipped_glyph_fragment_rect, cmd.cache_id, gly_cache);
    }

    void draw(const RDPPolygonSC & cmd, Rect clip, gdi::ColorCtx color_ctx) override {
        (void) cmd;
        (void) clip;
        (void) color_ctx;
        LOG(LOG_WARNING, "DEFAULT: RDPPolygonSC");

        /*RDPPolygonSC new_cmd24 = cmd;
        new_cmd24.BrushColor  = color_decode_opaquerect(cmd.BrushColor,  this->mod_bpp, this->mod_palette);*/
        //this->gd.draw(new_cmd24, clip);
    }

    void draw(const RDPPolygonCB & cmd, Rect clip, gdi::ColorCtx color_ctx) override {

        (void) cmd;
        (void) clip;
        (void) color_ctx;
        LOG(LOG_WARNING, "DEFAULT: RDPPolygonCB");

        /*RDPPolygonCB new_cmd24 = cmd;
        new_cmd24.foreColor  = color_decode_opaquerect(cmd.foreColor,  this->mod_bpp, this->mod_palette);
        new_cmd24.backColor  = color_decode_opaquerect(cmd.backColor,  this->mod_bpp, this->mod_palette);*/
        //this->gd.draw(new_cmd24, clip);
    }

    void draw(const RDPPolyline & cmd, Rect clip, gdi::ColorCtx color_ctx) override {
        (void) clip;
        (void) cmd;
        (void) color_ctx;
        LOG(LOG_WARNING, "DEFAULT: RDPPolyline");
        /*RDPPolyline new_cmd24 = cmd;
        new_cmd24.PenColor  = color_decode_opaquerect(cmd.PenColor,  this->mod_bpp, this->mod_palette);*/
        //this->gd.draw(new_cmd24, clip);
    }

    void draw(const RDPEllipseSC & cmd, Rect clip, gdi::ColorCtx color_ctx) override {

        (void) cmd;
        (void) clip;
        (void) color_ctx;
        LOG(LOG_WARNING, "DEFAULT: RDPEllipseSC");

        /*RDPEllipseSC new_cmd24 = cmd;
        new_cmd24.color = color_decode_opaquerect(cmd.color, this->mod_bpp, this->mod_palette);*/
        //this->gd.draw(new_cmd24, clip);
    }

    void draw(const RDPEllipseCB & cmd, Rect clip, gdi::ColorCtx color_ctx) override {

        (void) cmd;
        (void) clip;
        (void) color_ctx;
        LOG(LOG_WARNING, "DEFAULT: RDPEllipseCB");
    /*
        RDPEllipseCB new_cmd24 = cmd;
        new_cmd24.fore_color = color_decode_opaquerect(cmd.fore_color, this->mod_bpp, this->mod_palette);
        new_cmd24.back_color = color_decode_opaquerect(cmd.back_color, this->mod_bpp, this->mod_palette);*/
        //this->gd.draw(new_cmd24, clip);
    }

    void draw(const RDP::FrameMarker & order) override {
        (void) order;
        LOG(LOG_INFO, "DEFAULT: FrameMarker");
    }

    virtual void draw(RDPNineGrid const & /*cmd*/, Rect /*clip*/, gdi::ColorCtx /*color_ctx*/, Bitmap const & /*bmp*/) override {
        LOG(LOG_INFO, "DEFAULT: RDPNineGrid");
    }



    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //------------------------
    //      CONTROLLERS
    //------------------------

    void keyPressEvent(const uint16_t key, const uint16_t text) override {
//         if (this->client->mod_state ==  ClientRedemptionIOAPI::MOD_VNC) {
//             this->client->send_rdp_unicode(text, 0);
//         } else {
            this->qtRDPKeymap.keyEvent(0, key, text);
            if (this->qtRDPKeymap.scanCode != 0) {
                this->client->send_rdp_scanCode(this->qtRDPKeymap.scanCode, this->qtRDPKeymap.flag);
            }
//         }
    }

    void keyReleaseEvent(const uint16_t key, const uint16_t text) override {
//          if (this->client->mod_state ==  ClientRedemptionIOAPI::MOD_VNC) {
//             this->client->send_rdp_unicode(text, KBD_FLAG_UP);
//         } else {
            this->qtRDPKeymap.keyEvent(KBD_FLAG_UP, key, text);
            if (this->qtRDPKeymap.scanCode != 0) {
                this->client->send_rdp_scanCode(this->qtRDPKeymap.scanCode, this->qtRDPKeymap.flag);
            }
//         }
    }

    void connexionReleased() override {
        this->form->setCursor(Qt::WaitCursor);
        this->client->user_name     = this->form->get_userNameField();
        this->client->target_IP     = this->form->get_IPField();
        this->client->user_password = this->form->get_PWDField();
        this->client->port          = this->form->get_portField();

        if (!this->client->target_IP.empty()){
            this->client->connect();
        }
        this->form->setCursor(Qt::ArrowCursor);
    }

    void closeFromScreen() override {

        this->disconnexionReleased();

        if (this->form != nullptr && this->client->connected) {
            this->form->close();
        }
    }

};
