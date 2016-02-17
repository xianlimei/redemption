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
   Author(s): Clément Moroldo
*/


//#include <algorithm>
#include <string> 
#include <unistd.h>
/*
#include <signal.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "listen.hpp"
#include "parse_ip_conntrack.hpp"
*/

#include "../src/front/front_widget_Qt.hpp"
#include "core/channel_list.hpp"
#include "core/channel_names.hpp"

#include </usr/include/x86_64-linux-gnu/qt5/QtGui/QRgb>
#include </usr/include/x86_64-linux-gnu/qt5/QtGui/QRegion>
#include </usr/include/x86_64-linux-gnu/qt5/QtGui/QBitmap>

#define USER_CONF_PATH "userConfig.config"


Front_Qt::Front_Qt(char* argv[] = {}, int argc = 0, uint32_t verbose = 0)
    : Front_Qt_API(false, false, verbose)
    , mod_bpp(24)
    , mod_palette(BGRPalette::classic_332())
    , _form(nullptr)
    , _screen(nullptr)
    , _connector(nullptr)  
    , _timer(0)    
    , _connected(false)
    , _clipboard_channel(nullptr, &(this->_to_server_sender) ,*this , [](){
        ClipboardVirtualChannel::Params params;

        params.authentifier = nullptr;
        params.exchanged_data_limit = ~decltype(params.exchanged_data_limit){};
        params.verbose = 0xfffffff;
        
        params.clipboard_down_authorized = true;
        params.clipboard_up_authorized = true;
        params.clipboard_file_authorized = true;

        params.dont_log_data_into_syslog = true;
        params.dont_log_data_into_wrm = true;

        params.acl = nullptr;
        
        return params;
    }())
    , _keymap() 
    , _ctrl_alt_delete(false)
{
    if(this->setClientInfo()) {
        this->writeClientInfo();
    } 
    
    const char * localIPtmp = "unknow_local_IP"; 
    /*union
    {
        struct sockaddr s;
        struct sockaddr_storage ss;
        struct sockaddr_in s4;
        struct sockaddr_in6 s6;
    } localAddress;
    socklen_t addressLength = sizeof(localAddress);
    int sck = 0;
    if (-1 == getsockname(sck, &localAddress.s, &addressLength)){
        std::cout << "Error, local adress not found." << std::endl;
        _exit(1);
    }
    strcpy(localIPtmp, inet_ntoa(localAddress.s4.sin_addr));*/
    this->_localIP       = localIPtmp;
    this->_nbTry         = 3;
    this->_retryDelay    = 1000;
    
    uint8_t commandIsValid(0);
    
    for (int i = 0; i <  argc - 1; i++) {

        std::string word(argv[i]);
        std::string arg(argv[i+1]);
        
        if (       word == "-n") {
            this->_userName = arg;
            commandIsValid += NAME_GOTTEN;
        } else if (word == "-pwd") {
            this->_pwd = arg; 
            commandIsValid += PWD_GOTTEN;
        } else if (word == "-ip") {
            this->_targetIP = arg;
            commandIsValid += IP_GOTTEN;
        } else if (word == "-p") {
            std::string portStr(arg);
            this->_port = std::stoi(portStr);  
            commandIsValid += PORT_GOTTEN;
        }
    }

    this->cl.push_back({channel_names::cliprdr, 0, 0});
    
    
    if (this->mod_bpp == this->_info.bpp) {
        this->mod_palette = BGRPalette::classic_332();
    }
    this->_qtRDPKeymap.setKeyboardLayout(this->_info.keylayout);
    this->_keymap.init_layout(this->_info.keylayout);


    // Winodws and socket contrainer
    this->_form      = new Form_Qt(this);
    this->_connector = new Connector_Qt(this, this->_form);

    if (commandIsValid == Front_Qt::COMMAND_VALID) {
        
        this->connect();
        
    } else {
        std::cout << "missing argument(s) (" << (int)commandIsValid << "): ";
        if (!(commandIsValid & Front_Qt::NAME_GOTTEN)) {
            std::cout << "-n [user_name] ";
        } 
        if (!(commandIsValid & Front_Qt::PWD_GOTTEN)) {
            std::cout << "-pwd [password] ";
        } 
        if (!(commandIsValid & Front_Qt::IP_GOTTEN)) {
            std::cout << "-ip [ip_serveur] ";
        } 
        if (!(commandIsValid & Front_Qt::PORT_GOTTEN)) {
            std::cout << "-p [port] ";
        }
        std::cout << std::endl;
        
        this->disconnect("");
    }
}


bool Front_Qt::setClientInfo() {
    std::ifstream ifichier(USER_CONF_PATH, std::ios::in);
    if(ifichier) {
        
        std::string ligne;
        std::string delimiter = " ";
        
        while(getline(ifichier, ligne)) {
            
            int pos(ligne.find(delimiter));
            std::string tag  = ligne.substr(0, pos);
            std::string info = ligne.substr(pos + delimiter.length(), ligne.length());

            if (strcmp(tag.c_str(), "keylayout") == 0) {
                this->_info.keylayout = std::stoi(info);
            } else 
            if (strcmp(tag.c_str(), "console_session") == 0) {
                this->_info.console_session = std::stoi(info);
            } else 
            if (strcmp(tag.c_str(), "brush_cache_code") == 0) {
                this->_info.brush_cache_code = std::stoi(info);
            } else
            if (strcmp(tag.c_str(), "bpp") == 0) {
                this->_info.bpp = std::stoi(info);
            } else
            if (strcmp(tag.c_str(), "width") == 0) {
                this->_info.width = std::stoi(info);
            } else
            if (strcmp(tag.c_str(), "height") == 0) {
                this->_info.height = std::stoi(info);
            } else
            if (strcmp(tag.c_str(), "rdp5_performanceflags") == 0) {
                this->_info.rdp5_performanceflags = std::stoi(info);
            }
            if (strcmp(tag.c_str(), "fps") == 0) {
                this->_fps = std::stoi(info);
            }
        }
        ifichier.close();
        
        return false;
        
    } else {
        this->_info.keylayout = 0x040C;// 0x40C FR, 0x409 USA
        this->_info.console_session = 0;
        this->_info.brush_cache_code = 0;
        this->_info.bpp = 24;
        this->_imageFormatRGB  = this->bpp_to_QFormat(this->_info.bpp, false);
        this->_imageFormatARGB = this->bpp_to_QFormat(this->_info.bpp, true);
        this->_info.width = 800;
        this->_info.height = 600;
        this->_info.rdp5_performanceflags = PERF_DISABLE_WALLPAPER;
        this->_fps = 30;
        
        return true;
    }
}
 
 
void Front_Qt::writeClientInfo() {
    std::ofstream ofichier(USER_CONF_PATH, std::ios::out | std::ios::trunc);
    if(ofichier) {
        
        ofichier << "User Info" << std::endl << std::endl;
        
        ofichier << "keylayout "             << this->_info.keylayout             << std::endl;
        ofichier << "console_session "       << this->_info.console_session       << std::endl;
        ofichier << "brush_cache_code "      << this->_info.brush_cache_code      << std::endl;
        ofichier << "bpp "                   << this->_info.bpp                   << std::endl;
        ofichier << "width "                 << this->_info.width                 << std::endl;
        ofichier << "height "                << this->_info.height                << std::endl;
        ofichier << "rdp5_performanceflags " << this->_info.rdp5_performanceflags << std::endl;
        ofichier << "fps "                   << this->_fps                        << std::endl;
        
        ofichier.close();
    }
}


Front_Qt::~Front_Qt() {}


    
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 

//------------------------
//      CONTROLLERS
//------------------------
    
void Front_Qt::disconnexionReleased(){
    this->dropScreen();
    this->disconnect("");
}
    
void Front_Qt::dropScreen() {
    if (this->_screen != nullptr) {
        this->_screen->errorConnexion();
        this->_screen->close();
        this->_screen = nullptr;
    }
}
    
void Front_Qt::closeFromScreen() {
    if (this->_form != nullptr && this->_connected) {
        this->_form->close();
    }
}
  
void Front_Qt::connect() {
    if (this->_connector->connect()) {
        this->_connected = true;
        this->_form->hide(); 
        this->_screen = new Screen_Qt(this);
        this->_screen->show();
        this->_connector->listen();
    } 
    
    this->_form->setCursor(Qt::ArrowCursor);;
}

void Front_Qt::disconnect(std::string error) {
    
    if (this->_connector != nullptr) {
        this->_connector->drop_connexion();
    } 
    
    this->_form->set_IPField(this->_targetIP);
    this->_form->set_portField(this->_port);
    this->_form->set_PWDField(this->_pwd);
    this->_form->set_userNameField(this->_userName);
    this->_form->set_ErrorMsg(error);
    this->_form->show();
    
    this->_connected = false;
}
 
void Front_Qt::connexionReleased(){
    this->_form->setCursor(Qt::WaitCursor);
    this->_userName =  this->_form->get_userNameField();
    this->_targetIP =  this->_form->get_IPField();
    this->_pwd      =  this->_form->get_PWDField();
    this->_port     =  this->_form->get_portField();
    
    if (strcmp(this->_targetIP.c_str(), "") != 0){
        this->connect();
    }
}

void Front_Qt::mousePressEvent(QMouseEvent *e) {
    if (this->_callback != nullptr) {
        int flag(0); 
        switch (e->button()) {
            case 1: flag = MOUSE_FLAG_BUTTON1; break;
            case 2: flag = MOUSE_FLAG_BUTTON2; break; 
            case 4: flag = MOUSE_FLAG_BUTTON4; break;
            default: break; 
        }
        //std::cout << "mousePressed" << std::endl;
        this->_callback->rdp_input_mouse(flag | MOUSE_FLAG_DOWN, e->x(), e->y(), &(this->_keymap));
    } 
}
    
void Front_Qt::mouseReleaseEvent(QMouseEvent *e) {
    if (this->_callback != nullptr) {
        int flag(0); 
        switch (e->button()) {
            case 1: flag = MOUSE_FLAG_BUTTON1; break; 
            case 2: flag = MOUSE_FLAG_BUTTON2; break; 
            case 4: flag = MOUSE_FLAG_BUTTON4; break; 
            default: break;
        }
        //std::cout << "mouseRelease" << std::endl;
        this->_callback->rdp_input_mouse(flag, e->x(), e->y(), &(this->_keymap)); 
    }
}
    
void Front_Qt::keyPressEvent(QKeyEvent *e) { 
    this->_qtRDPKeymap.keyEvent(0       ,      e);
    if (this->_qtRDPKeymap.scanCode != 0) {
        this->send_rdp_scanCode(this->_qtRDPKeymap.scanCode, this->_qtRDPKeymap.flag);
    }
}
    
void Front_Qt::keyReleaseEvent(QKeyEvent *e) {
    this->_qtRDPKeymap.keyEvent(0x8000, e);
    if (this->_qtRDPKeymap.scanCode != 0) {
        this->send_rdp_scanCode(this->_qtRDPKeymap.scanCode, this->_qtRDPKeymap.flag);
    }
}
    
void Front_Qt::wheelEvent(QWheelEvent *e) {
    //std::cout << "wheel " << " delta=" << e->delta() << std::endl;
    int flag(MOUSE_FLAG_HWHEEL);
    if (e->delta() < 0) {
        flag = flag | MOUSE_FLAG_WHEEL_NEGATIVE;
    }
    if (this->_callback != nullptr) {
        //this->_callback->rdp_input_mouse(flag, e->x(), e->y(), &(this->_keymap));
    }
}
    
bool Front_Qt::eventFilter(QObject *obj, QEvent *e)  {
    if (e->type() == QEvent::MouseMove)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(e);
        //std::cout << "MouseMove " <<  mouseEvent->x() << " " <<  mouseEvent->y()<< std::endl;
        if (this->_callback != nullptr) {
            this->_callback->rdp_input_mouse(MOUSE_FLAG_MOVE, mouseEvent->x(), mouseEvent->y(), &(this->_keymap));  
        }
    }
    return false;
}
    
void Front_Qt::connexionPressed() {}

void Front_Qt::RefreshPressed() {
    this->refresh(0, 0, this->_info.width, this->_info.height);
}
    
void Front_Qt::RefreshReleased() {}
    
void Front_Qt::CtrlAltDelPressed() {
    int flag = Keymap2::KBDFLAGS_EXTENDED;

    this->send_rdp_scanCode(0x38, flag);  // ALT
    this->send_rdp_scanCode(0x1D, flag);  // CTRL
    this->send_rdp_scanCode(0x53, flag);  // DELETE       
}
    
void Front_Qt::CtrlAltDelReleased() {
    int flag = Keymap2::KBDFLAGS_EXTENDED | KBD_FLAG_UP;
    
    this->send_rdp_scanCode(0x38, flag);  // ALT
    this->send_rdp_scanCode(0x1D, flag);  // CTRL
    this->send_rdp_scanCode(0x53, flag);  // DELETE  
}
    
void Front_Qt::disconnexionPressed() {}
    
void Front_Qt::refresh(int x, int y, int w, int h) {
    Rect rect(x, y, w, h);
    this->_callback->rdp_input_invalidate(rect);
}

void Front_Qt::send_rdp_scanCode(int keyCode, int flag) {
    this->_keymap.event(flag, keyCode, this->_decoded_data, this->_ctrl_alt_delete); 
    if (this->_callback != nullptr) {
        this->_callback->rdp_input_scancode(keyCode, 0, flag, this->_timer, &(this->_keymap)); 
    }
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 

//---------------------------------------
//   GRAPHIC FUNCTIONS (factorization)
//---------------------------------------

void Front_Qt::draw_MemBlt(const Rect & drect, const Bitmap & bitmap, bool invert, int srcx, int srcy) {
    const int16_t mincx = bitmap.cx();
    const int16_t mincy = bitmap.cy();

    if (mincx <= 0 || mincy <= 0) {
        return;
    }        

    const unsigned char * row = bitmap.data();

    QImage::Format format(this->bpp_to_QFormat(bitmap.bpp(), false)); //bpp

    QImage qbitmap(row, mincx, mincy, format);

    qbitmap = qbitmap.mirrored(false, true);
    
    qbitmap = qbitmap.copy(srcx, srcy, drect.cx, drect.cy);
    
    if (bitmap.bpp() > this->_info.bpp) {
        qbitmap = qbitmap.convertToFormat(this->_imageFormatRGB);
    }
    
    if (invert) {
        qbitmap.invertPixels();
    }
    
    if (bitmap.bpp() == 24) {
        qbitmap = qbitmap.rgbSwapped();
    }
    
    const QRect trect(drect.x, drect.y, drect.cx, drect.cy);
    this->_screen->paintCache().drawImage(trect, qbitmap);

    //this->_screen->_scene.addPixmap(rect.x, rect.y, rect.cx, rect.cy, this->u32_to_qcolor(new_cmd24.color));
}


void Front_Qt::draw_bmp(const Rect & drect, const Bitmap & bitmap, bool invert) {
    const int16_t mincx = std::min<int16_t>(bitmap.cx(), std::min<int16_t>(this->_info.width - drect.x, drect.cx));
    const int16_t mincy = 1;

    if (mincx <= 0) {
        return;
    }        

    int rowYCoord(drect.y + drect.cy-1);
    int rowsize(bitmap.line_size()); //Bpp

    const unsigned char * row = bitmap.data();

    QImage::Format format(this->bpp_to_QFormat(bitmap.bpp(), false)); //bpp

    for (size_t k = 0 ; k < drect.cy; k++) {
        
        QImage qbitmap(row, mincx, mincy, format);
        
        if (bitmap.bpp() > this->_info.bpp) {
            qbitmap = qbitmap.convertToFormat(this->_imageFormatRGB);
        }
                    
        if (invert) {
            qbitmap.invertPixels();
        }
        
        if (bitmap.bpp() == 24) {
            qbitmap = qbitmap.rgbSwapped();
        }
        
        QRect trect(drect.x, rowYCoord, mincx, mincy);
        this->_screen->paintCache().drawImage(trect, qbitmap);

        row += rowsize;
        rowYCoord--;
    }
}


void Front_Qt::draw_RDPScrBlt(int srcx, int srcy, const Rect & drect, bool invert) {
    QImage qbitmap(this->_screen->getCache()->toImage().copy(srcx, srcy, drect.cx, drect.cy));
    if (invert) {
        qbitmap.invertPixels();
    }
    const QRect trect(drect.x, drect.y, drect.cx, drect.cy);
    this->_screen->paintCache().drawImage(trect, qbitmap);
}


QColor Front_Qt::u32_to_qcolor(uint32_t color){
    uint8_t b(color >> 16);
    uint8_t g(color >> 8);
    uint8_t r(color);
    return {r, g, b};
}
    
    
QImage::Format Front_Qt::bpp_to_QFormat(int bpp, bool alpha) {
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
    
    

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
    
//-----------------------------
//       DRAW FUNCTIONS 
//-----------------------------

void Front_Qt::draw(const RDPPatBlt & cmd, const Rect & clip) {
    if (this->verbose > 10) {
        LOG(LOG_INFO, "--------- FRONT ------------------------");
        cmd.log(LOG_INFO, clip);
        LOG(LOG_INFO, "========================================\n");
    }
    //std::cout << "RDPPatBlt " << (int) cmd.rop << std::endl;
    RDPPatBlt new_cmd24 = cmd;
    new_cmd24.back_color = color_decode_opaquerect(cmd.back_color, this->mod_bpp, this->mod_palette);
    new_cmd24.fore_color = color_decode_opaquerect(cmd.fore_color, this->mod_bpp, this->mod_palette);
    const Rect rect = clip.intersect(this->_info.width, this->_info.height).intersect(cmd.rect);
    
    if (cmd.brush.style == 0x03 && (cmd.rop == 0xF0 || cmd.rop == 0x5A)) { // external
        enum { BackColor, ForeColor };
        QColor backColor = this->u32_to_qcolor(new_cmd24.back_color);
        QColor foreColor = this->u32_to_qcolor(new_cmd24.fore_color);

        switch (cmd.rop) {

            // +------+-------------------------------+
            // | 0x5A | ROP: 0x005A0049 (PATINVERT)   |
            // |      | RPN: DPx                      |
            // +------+-------------------------------+
            case 0x5A:
                {
                    QBrush brush(backColor, Qt::Dense4Pattern);
                    this->_screen->paintCache().setBrush(brush);
                    this->_screen->paintCache().setCompositionMode(QPainter::RasterOp_SourceXorDestination);
                    this->_screen->paintCache().drawRect(rect.x, rect.y, rect.cx, rect.cy);
                    this->_screen->paintCache().setBrush(Qt::SolidPattern);
                    this->_screen->paintCache().setCompositionMode(QPainter::CompositionMode_SourceOver);
                }
                break;

            // +------+-------------------------------+
            // | 0xF0 | ROP: 0x00F00021 (PATCOPY)     |
            // |      | RPN: P                        |
            // +------+-------------------------------+
            case 0xF0:
                {
                    this->_screen->paintCache().setBrush(backColor);
                    this->_screen->paintCache().drawRect(rect.x, rect.y, rect.cx, rect.cy);
                }
                break;
            default:
                std::cout << "RDPPatBlt brush_style = 03 " << (int) cmd.rop << std::endl;
                break;
        }
        
    } else {
         switch (cmd.rop) {
             
                // +------+-------------------------------+
                // | 0x00 | ROP: 0x00000042 (BLACKNESS)   |
                // |      | RPN: 0                        |
                // +------+-------------------------------+
            case 0x00: // blackness
                this->_screen->paintCache().drawRect(rect.x, rect.y, rect.cx, rect.cy);
                break;
                // +------+-------------------------------+
                // | 0x05 | ROP: 0x000500A9               |
                // |      | RPN: DPon                     |
                // +------+-------------------------------+

                // +------+-------------------------------+
                // | 0x0F | ROP: 0x000F0001               |
                // |      | RPN: Pn                       |
                // +------+-------------------------------+

                // +------+-------------------------------+
                // | 0x50 | ROP: 0x00500325               |
                // |      | RPN: PDna                     |
                // +------+-------------------------------+

                // +------+-------------------------------+
                // | 0x55 | ROP: 0x00550009 (DSTINVERT)   |
                // |      | RPN: Dn                       |
                // +------+-------------------------------+
            case 0x55: // inversion
                //this->invert_color(rect);
                break;
                // +------+-------------------------------+
                // | 0x5A | ROP: 0x005A0049 (PATINVERT)   |
                // |      | RPN: DPx                      |
                // +------+-------------------------------+
            case 0x5A:
                this->_screen->paintCache().setCompositionMode(QPainter::RasterOp_SourceXorDestination);
                this->_screen->paintCache().drawRect(rect.x, rect.y, rect.cx, rect.cy);
                this->_screen->paintCache().setCompositionMode(QPainter::CompositionMode_SourceOver);
                break;
                // +------+-------------------------------+
                // | 0x5F | ROP: 0x005F00E9               |
                // |      | RPN: DPan                     |
                // +------+-------------------------------+

                // +------+-------------------------------+
                // | 0xA0 | ROP: 0x00A000C9               |
                // |      | RPN: DPa                      |
                // +------+-------------------------------+

                // +------+-------------------------------+
                // | 0xA5 | ROP: 0x00A50065               |
                // |      | RPN: PDxn                     |
                // +------+-------------------------------+

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

                // +------+-------------------------------+
                // | 0xF0 | ROP: 0x00F00021 (PATCOPY)     |
                // |      | RPN: P                        |
                // +------+-------------------------------+

                // +------+-------------------------------+
                // | 0xF5 | ROP: 0x00F50225               |
                // |      | RPN: PDno                     |
                // +------+-------------------------------+

                // +------+-------------------------------+
                // | 0xFA | ROP: 0x00FA0089               |
                // |      | RPN: DPo                      |
                // +------+-------------------------------+

                // +------+-------------------------------+
                // | 0xFF | ROP: 0x00FF0062 (WHITENESS)   |
                // |      | RPN: 1                        |
                // +------+-------------------------------+
            case 0xFF: // whiteness
                this->_screen->paintCache().drawRect(rect.x, rect.y, rect.cx, rect.cy);
                break;
            default:
                std::cout << "RDPPatBlt " << (int) cmd.rop << std::endl;
                break;
        }
    }
    
}

    
void Front_Qt::draw(const RDPOpaqueRect & cmd, const Rect & clip) {
    if (this->verbose > 10) {
        LOG(LOG_INFO, "--------- FRONT ------------------------");
        cmd.log(LOG_INFO, clip);
        LOG(LOG_INFO, "========================================\n");
    }
    //std::cout << "RDPOpaqueRect" << std::endl;
    RDPOpaqueRect new_cmd24 = cmd;
    new_cmd24.color = color_decode_opaquerect(cmd.color, this->mod_bpp, this->mod_palette);
    Rect rect(new_cmd24.rect.intersect(clip));
    this->_screen->paintCache().fillRect(rect.x, rect.y, rect.cx, rect.cy, this->u32_to_qcolor(new_cmd24.color));
    //this->_screen->_scene.addRect(rect.x, rect.y, rect.cx, rect.cy, this->u32_to_qcolor(new_cmd24.color));
}


void Front_Qt::draw(const RDPBitmapData & bitmap_data, const uint8_t * data,
    size_t size, const Bitmap & bmp) {
    if (this->verbose > 10) {
        LOG(LOG_INFO, "--------- FRONT ------------------------");
        bitmap_data.log(LOG_INFO, "FakeFront");
        LOG(LOG_INFO, "========================================\n");
    }
    //std::cout << "RDPBitmapData" << std::endl;
    if (!bmp.is_valid()){
        return;
    }

    Rect rectBmp( bitmap_data.dest_left, bitmap_data.dest_top, 
                            (bitmap_data.dest_right - bitmap_data.dest_left + 1), 
                            (bitmap_data.dest_bottom - bitmap_data.dest_top + 1));
    const Rect clipRect(0, 0, this->_info.width, this->_info.height);
    const Rect rect = rectBmp.intersect(clipRect);
    
    this->draw_bmp(rect, bmp, false);
}


void Front_Qt::draw(const RDPLineTo & cmd, const Rect & clip) {
    if (this->verbose > 10) {
        LOG(LOG_INFO, "--------- FRONT ------------------------");
        cmd.log(LOG_INFO, clip);
        LOG(LOG_INFO, "========================================\n");
    }
    //std::cout << "RDPLineTo" << std::endl; 
    RDPLineTo new_cmd24 = cmd;
    new_cmd24.back_color = color_decode_opaquerect(cmd.back_color, this->mod_bpp, this->mod_palette);
    new_cmd24.pen.color  = color_decode_opaquerect(cmd.pen.color,  this->mod_bpp, this->mod_palette);

    // TO DO clipping
    this->_screen->setPenColor(this->u32_to_qcolor(new_cmd24.back_color));
    this->_screen->paintCache().drawLine(new_cmd24.startx, new_cmd24.starty, new_cmd24.endx, new_cmd24.endy);
}


void Front_Qt::draw(const RDPScrBlt & cmd, const Rect & clip) {
    if (this->verbose > 10) {
        LOG(LOG_INFO, "--------- FRONT ------------------------");
        cmd.log(LOG_INFO, clip);
        LOG(LOG_INFO, "========================================\n");
    }

    //std::cout << "RDPScrBlt" << std::endl;
    
    const Rect drect = clip.intersect(this->_info.width, this->_info.height).intersect(cmd.rect);
    if (drect.isempty()) { 
        return;
    }

    int srcx(drect.x + cmd.srcx - cmd.rect.x);
    int srcy(drect.y + cmd.srcy - cmd.rect.y);
    
    switch (cmd.rop) {
            // +------+-------------------------------+
            // | 0x00 | ROP: 0x00000042 (BLACKNESS)   |
            // |      | RPN: 0                        |
            // +------+-------------------------------+
        case 0x00: this->_screen->paintCache().fillRect(drect.x, drect.y, drect.cx, drect.cy, Qt::black);
            break;
            // +------+-------------------------------+
            // | 0x11 | ROP: 0x001100A6 (NOTSRCERASE) |        
            // |      | RPN: DSon                     |
            // +------+-------------------------------+

            // +------+-------------------------------+
            // | 0x22 | ROP: 0x00220326               |
            // |      | RPN: DSna                     |
            // +------+-------------------------------+

            // +------+-------------------------------+
            // | 0x33 | ROP: 0x00330008 (NOTSRCCOPY)  |
            // |      | RPN: Sn                       |
            // +------+-------------------------------+

            // +------+-------------------------------+
            // | 0x44 | ROP: 0x00440328 (SRCERASE)    |
            // |      | RPN: SDna                     |
            // +------+-------------------------------+

            // +------+-------------------------------+
            // | 0x55 | ROP: 0x00550009 (DSTINVERT)   |
            // |      | RPN: Dn                       |
            // +------+-------------------------------+
        case 0x55: this->draw_RDPScrBlt(srcx, srcy, drect, true);
            break;
            // +------+-------------------------------+
            // | 0x66 | ROP: 0x00660046 (SRCINVERT)   |
            // |      | RPN: DSx                      |
            // +------+-------------------------------+

            // +------+-------------------------------+
            // | 0x77 | ROP: 0x007700E6               |
            // |      | RPN: DSan                     |
            // +------+-------------------------------+

            // +------+-------------------------------+
            // | 0x88 | ROP: 0x008800C6 (SRCAND)      |
            // |      | RPN: DSa                      |
            // +------+-------------------------------+

            // +------+-------------------------------+
            // | 0x99 | ROP: 0x00990066               |
            // |      | RPN: DSxn                     |
            // +------+-------------------------------+
;
            // +------+-------------------------------+
            // | 0xAA | ROP: 0x00AA0029               |
            // |      | RPN: D                        |
            // +------+-------------------------------+
        case 0xAA: // nothing to change
            break;
            // +------+-------------------------------+
            // | 0xBB | ROP: 0x00BB0226 (MERGEPAINT)  |
            // |      | RPN: DSno                     |
            // +------+-------------------------------+

            // +------+-------------------------------+
            // | 0xCC | ROP: 0x00CC0020 (SRCCOPY)     |
            // |      | RPN: S                        |
            // +------+-------------------------------+*/
        case 0xCC: this->draw_RDPScrBlt(srcx, srcy, drect, false);
            break;
            // +------+-------------------------------+
            // | 0xDD | ROP: 0x00DD0228               |
            // |      | RPN: SDno                     |
            // +------+-------------------------------+

            // +------+-------------------------------+
            // | 0xEE | ROP: 0x00EE0086 (SRCPAINT)    |
            // |      | RPN: DSo                      |
            // +------+-------------------------------+

            // +------+-------------------------------+
            // | 0xFF | ROP: 0x00FF0062 (WHITENESS)   |
            // |      | RPN: 1                        |
            // +------+-------------------------------+
        case 0xFF:
            this->_screen->paintCache().fillRect(drect.x, drect.y, drect.cx, drect.cy, Qt::white);
            break;
        default: std::cout << "RDPScrBlt (" << std::hex << (int)cmd.rop << ")" << std::endl;
            break;
    }
}
    

void Front_Qt::draw(const RDPMemBlt & cmd, const Rect & clip, const Bitmap & bitmap) {
    if (this->verbose > 10) {
        LOG(LOG_INFO, "--------- FRONT ------------------------");
        cmd.log(LOG_INFO, clip);
        LOG(LOG_INFO, "========================================\n");
    }
    //std::cout << "RDPMemBlt (" << std::hex << (int)cmd.rop << ")" << std::endl;
    Rect rectBmp(cmd.rect);         
    const Rect& drect = clip.intersect(rectBmp);
    if (drect.isempty()){
        return ;
    }
    
    switch (cmd.rop) {

        case 0x00:
            this->_screen->paintCache().fillRect(drect.x, drect.y, drect.cx, drect.cy, Qt::black);
            break;
            
        case 0x55: this->draw_MemBlt(drect, bitmap, true, cmd.srcx + (drect.x - cmd.rect.x), cmd.srcy + (drect.y - cmd.rect.y));
            break;
            
        case 0x99:  // nothing to change
            break;
            
        case 0xCC:  
            this->draw_MemBlt(drect, bitmap, false, cmd.srcx + (drect.x - cmd.rect.x), cmd.srcy + (drect.y - cmd.rect.y));
            break;
            
        case 0xEE: 
            this->draw_MemBlt(drect, bitmap, false, cmd.srcx + (drect.x - cmd.rect.x), cmd.srcy + (drect.y - cmd.rect.y));
            break;
            
        case 0xFF:
            this->_screen->paintCache().fillRect(drect.x, drect.y, drect.cx, drect.cy, Qt::white);
            break;
        default: std::cout << "RDPMemBlt (" << std::hex << (int)cmd.rop << ")" << std::endl;
            break;
    }
}


void Front_Qt::draw(const RDPMem3Blt & cmd, const Rect & clip, const Bitmap & bitmap) {
    if (this->verbose > 10) {
        LOG(LOG_INFO, "--------- FRONT ------------------------");
        cmd.log(LOG_INFO, clip);
        LOG(LOG_INFO, "========================================\n");
    }
    //std::cout << "RDPMem3Blt" << std::endl;
    Rect rectBmp(cmd.rect);
    const Rect& drect = clip.intersect(rectBmp);
    if (drect.isempty()){
        return ;
    }
    switch (cmd.rop) {
        case 0xB8: 
            {
                //uint8_t b(cmd.fore_color >> 16);
                //uint8_t g(cmd.fore_color >> 8);
                //uint8_t r(cmd.fore_color);
                QColor fore(this->u32_to_qcolor(cmd.fore_color));

                const int16_t mincx = std::min<int16_t>(bitmap.cx(), std::min<int16_t>(this->_info.width - drect.x, drect.cx));
                const int16_t mincy = 1;

                if (mincx <= 0) {
                    return;
                }        

                int rowYCoord(drect.y + drect.cy);
                int rowsize(bitmap.line_size()); //Bpp

                const unsigned char * row = bitmap.data();
                
                QColor white(Qt::white);
                QColor trans(0, 0, 0, 0);

                QImage::Format format(this->bpp_to_QFormat(bitmap.bpp(), true)); //bpp

                for (size_t k = 0 ; k < drect.cy; k++) {
                    
                    QImage img(row, mincx, mincy, format);
                    
                    if (bitmap.bpp() > this->_info.bpp) {
                        img = img.convertToFormat(this->_imageFormatARGB);
                    }
                    
                    for(int x= 0; x<img.width(); x++) {
                        for(int y = 0; y<img.height(); y++) {

                            if(img.pixel(x, y) == white.rgb()) {
                                img.setPixel(x, y, trans.rgba()) ;
                            } else {
                                img.setPixel(x, y, fore.rgb()) ;
                            }
                        }
                    }
                        
                    QRect trect(drect.x, rowYCoord, mincx, mincy);
                    this->_screen->paintCache().drawImage(trect, img);

                    row += rowsize;
                    rowYCoord--;
                }
            }
            break;

        default: std::cout << "RDPMem3Blt (" << std::hex << (int)cmd.rop << ")" << std::endl;
            break;
    }
}
    
void Front_Qt::draw(const RDPDestBlt & cmd, const Rect & clip) {
    if (this->verbose > 10) {
        LOG(LOG_INFO, "--------- FRONT ------------------------");
        cmd.log(LOG_INFO, clip);
        LOG(LOG_INFO, "========================================\n");
    }

    std::cout << "RDPDestBlt" << std::endl;
}

void Front_Qt::draw(const RDPMultiDstBlt & cmd, const Rect & clip) {
    if (this->verbose > 10) {
        LOG(LOG_INFO, "--------- FRONT ------------------------");
        cmd.log(LOG_INFO, clip);
        LOG(LOG_INFO, "========================================\n");
    }

    std::cout << "RDPMultiDstBlt" << std::endl;
}

void Front_Qt::draw(const RDPMultiOpaqueRect & cmd, const Rect & clip) {
    if (this->verbose > 10) {
        LOG(LOG_INFO, "--------- FRONT ------------------------");
        cmd.log(LOG_INFO, clip);
        LOG(LOG_INFO, "========================================\n");
    }

    std::cout << "RDPMultiOpaqueRect" << std::endl;
}

void Front_Qt::draw(const RDP::RDPMultiPatBlt & cmd, const Rect & clip) {
    if (this->verbose > 10) {
        LOG(LOG_INFO, "--------- FRONT ------------------------");
        cmd.log(LOG_INFO, clip);
        LOG(LOG_INFO, "========================================\n");
    }

    std::cout << "RDPMultiPatBlt" << std::endl;
}

void Front_Qt::draw(const RDP::RDPMultiScrBlt & cmd, const Rect & clip) {
    if (this->verbose > 10) {
        LOG(LOG_INFO, "--------- FRONT ------------------------");
        cmd.log(LOG_INFO, clip);
        LOG(LOG_INFO, "========================================\n");
    }

    std::cout << "RDPMultiScrBlt" << std::endl;
}

void Front_Qt::draw(const RDPGlyphIndex & cmd, const Rect & clip, const GlyphCache * gly_cache) {
    if (this->verbose > 10) {
        LOG(LOG_INFO, "--------- FRONT ------------------------");
        cmd.log(LOG_INFO, clip);
        LOG(LOG_INFO, "========================================\n");
    }
    
    std::cout << "RDPGlyphIndex" << std::endl;

    /* RDPGlyphIndex new_cmd24 = cmd;
    new_cmd24.back_color = color_decode_opaquerect(cmd.back_color, this->mod_bpp, this->mod_palette);
    new_cmd24.fore_color = color_decode_opaquerect(cmd.fore_color, this->mod_bpp, this->mod_palette);*/
    //this->gd.draw(new_cmd24, clip, gly_cache);
}

void Front_Qt::draw(const RDPPolygonSC & cmd, const Rect & clip) {
    if (this->verbose > 10) {
        LOG(LOG_INFO, "--------- FRONT ------------------------");
        cmd.log(LOG_INFO, clip);
        LOG(LOG_INFO, "========================================\n");
    }
    
    std::cout << "RDPPolygonSC" << std::endl;

    /*RDPPolygonSC new_cmd24 = cmd;
    new_cmd24.BrushColor  = color_decode_opaquerect(cmd.BrushColor,  this->mod_bpp, this->mod_palette);*/
    //this->gd.draw(new_cmd24, clip);
}

void Front_Qt::draw(const RDPPolygonCB & cmd, const Rect & clip) {
    if (this->verbose > 10) {
        LOG(LOG_INFO, "--------- FRONT ------------------------");
        cmd.log(LOG_INFO, clip);
        LOG(LOG_INFO, "========================================\n");
    }
    
    std::cout << "RDPPolygonCB" << std::endl;

    /*RDPPolygonCB new_cmd24 = cmd;
    new_cmd24.foreColor  = color_decode_opaquerect(cmd.foreColor,  this->mod_bpp, this->mod_palette);
    new_cmd24.backColor  = color_decode_opaquerect(cmd.backColor,  this->mod_bpp, this->mod_palette);*/
    //this->gd.draw(new_cmd24, clip);
}

void Front_Qt::draw(const RDPPolyline & cmd, const Rect & clip) {
    if (this->verbose > 10) {
        LOG(LOG_INFO, "--------- FRONT ------------------------");
        cmd.log(LOG_INFO, clip);
        LOG(LOG_INFO, "========================================\n");
    }
    
    std::cout << "RDPPolyline" << std::endl;

    /*RDPPolyline new_cmd24 = cmd;
    new_cmd24.PenColor  = color_decode_opaquerect(cmd.PenColor,  this->mod_bpp, this->mod_palette);*/
    //this->gd.draw(new_cmd24, clip);
}

void Front_Qt::draw(const RDPEllipseSC & cmd, const Rect & clip) {
    if (this->verbose > 10) {
        LOG(LOG_INFO, "--------- FRONT ------------------------");
        cmd.log(LOG_INFO, clip);
        LOG(LOG_INFO, "========================================\n");
    }
    
    std::cout << "RDPEllipseSC" << std::endl;

    /*RDPEllipseSC new_cmd24 = cmd;
    new_cmd24.color = color_decode_opaquerect(cmd.color, this->mod_bpp, this->mod_palette);*/
    //this->gd.draw(new_cmd24, clip);
}

void Front_Qt::draw(const RDPEllipseCB & cmd, const Rect & clip) {
    if (this->verbose > 10) {
        LOG(LOG_INFO, "--------- FRONT ------------------------");
        cmd.log(LOG_INFO, clip);
        LOG(LOG_INFO, "========================================\n");
    }
    
    std::cout << "RDPEllipseCB" << std::endl;
/*
    RDPEllipseCB new_cmd24 = cmd;
    new_cmd24.fore_color = color_decode_opaquerect(cmd.fore_color, this->mod_bpp, this->mod_palette);
    new_cmd24.back_color = color_decode_opaquerect(cmd.back_color, this->mod_bpp, this->mod_palette);*/
    //this->gd.draw(new_cmd24, clip);
}

void Front_Qt::draw(const RDP::FrameMarker & order) {
    if (this->verbose > 10) {
        LOG(LOG_INFO, "--------- FRONT ------------------------");
        order.log(LOG_INFO);
        LOG(LOG_INFO, "========================================\n");
    }
    
    std::cout << "FrameMarker" << std::endl;
    //this->gd.draw(order);
}

void Front_Qt::draw(const RDP::RAIL::NewOrExistingWindow & order) {
    if (this->verbose > 10) {
        LOG(LOG_INFO, "--------- FRONT ------------------------");
        order.log(LOG_INFO);
        LOG(LOG_INFO, "========================================\n");
    }

    std::cout << "NewOrExistingWindow" << std::endl;
    //this->gd.draw(order);
}

void Front_Qt::draw(const RDP::RAIL::WindowIcon & order) {
    if (this->verbose > 10) {
        LOG(LOG_INFO, "--------- FRONT ------------------------");
        order.log(LOG_INFO);
        LOG(LOG_INFO, "========================================\n");
    }
    
    std::cout << "WindowIcon" << std::endl;
    //this->gd.draw(order);
}

void Front_Qt::draw(const RDP::RAIL::CachedIcon & order) {
    if (this->verbose > 10) {
        LOG(LOG_INFO, "--------- FRONT ------------------------");
        order.log(LOG_INFO);
        LOG(LOG_INFO, "========================================\n");
    }
    
        std::cout << "CachedIcon" << std::endl;

    //this->gd.draw(order);
}

void Front_Qt::draw(const RDP::RAIL::DeletedWindow & order) {
    if (this->verbose > 10) {
        LOG(LOG_INFO, "--------- FRONT ------------------------");
        order.log(LOG_INFO);
        LOG(LOG_INFO, "========================================\n");
    }
    
    std::cout << "DeletedWindow" << std::endl;

    //this->gd.draw(order);
}
    
void Front_Qt::draw(const RDPColCache   & cmd) {}
    
void Front_Qt::draw(const RDPBrushCache & cmd) {}
    
    

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
    
//------------------------------
// Serveur non drawing exchange 
//------------------------------

int Front_Qt::server_resize(int width, int height, int bpp) {
    if (this->verbose > 10) {
        LOG(LOG_INFO, "--------- FRONT ------------------------");
        LOG(LOG_INFO, "server_resize(width=%d, height=%d, bpp=%d", width, height, bpp);
        LOG(LOG_INFO, "========================================\n");
    }
    //std::cout << "resize serveur" << std::endl;
    this->mod_bpp = bpp;
    //this->_info.bpp = bpp;
    this->_info.width = width;
    this->_info.height = height;
    
    //this->_screen->setUpdate();
    
    return 1;
}


void Front_Qt::server_set_pointer(const Pointer & cursor) {
    if (this->verbose > 10) {
        LOG(LOG_INFO, "--------- FRONT ------------------------");
        LOG(LOG_INFO, "server_set_pointer");
        LOG(LOG_INFO, "========================================\n");
    }
    
    //std::cout <<  cursor.pointer_type << std::endl;

}

void Front_Qt::flush() {
    if (this->verbose > 10) {
        LOG(LOG_INFO, "--------- FRONT ------------------------");
        LOG(LOG_INFO, "flush()");
        LOG(LOG_INFO, "========================================\n"); 
    }
}

const CHANNELS::ChannelDefArray & Front_Qt::get_channel_list(void) const { return cl; }

void Front_Qt::send_to_channel( const CHANNELS::ChannelDef & channel, uint8_t const * data, size_t length, size_t chunk_size, int flags) {
    if (this->verbose > 10) {
        LOG(LOG_INFO, "--------- FRONT ------------------------");
        LOG(LOG_INFO, "send_to_channel");
        LOG(LOG_INFO, "========================================\n");
    }

    const CHANNELS::ChannelDef * mod_channel = this->cl.get_by_name(channel.name);
    if (!mod_channel) {
        return;
    }
    
    if (!strcmp(channel.name, channel_names::cliprdr)) {
        std::unique_ptr<AsynchronousTask> out_asynchronous_task;
        std::cout << channel.name << " send_to_channel" << std::endl;
        this->_clipboard_channel.process_server_message(length, flags, data, chunk_size, out_asynchronous_task);
    }
}  

void Front_Qt::send_global_palette() {
    if (this->verbose > 10) {
        LOG(LOG_INFO, "--------- FRONT ------------------------");
        LOG(LOG_INFO, "send_global_palette()");
        LOG(LOG_INFO, "========================================\n");
    }
}

void Front_Qt::begin_update() {
    //if (this->verbose > 10) {
    //    LOG(LOG_INFO, "--------- FRONT ------------------------");
    //    LOG(LOG_INFO, "begin_update");
    //    LOG(LOG_INFO, "========================================\n");
    //}
}

void Front_Qt::end_update() {
    //if (this->verbose > 10) {
    //    LOG(LOG_INFO, "--------- FRONT ------------------------");
    //    LOG(LOG_INFO, "end_update");
    //    LOG(LOG_INFO, "========================================\n");
    //}
}

void Front_Qt::set_mod_palette(const BGRPalette & palette) {
    if (this->verbose > 10) {
        LOG(LOG_INFO, "--------- FRONT ------------------------");
        LOG(LOG_INFO, "set_mod_palette");
        LOG(LOG_INFO, "========================================\n");
    }
}    



/*
///////////////////////////////
// APPLICATION 
int main(int argc, char** argv){
    
    //" -name QA\\administrateur -pwd 'S3cur3!1nux' -ip 10.10.46.88 -p 3389";

    QApplication app(argc, argv);

    int verbose = 511;

    Front_Qt front(argv, argc, verbose);
    
    
    app.exec();
  
}*/
