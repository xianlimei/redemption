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
*   Copyright (C) Wallix 2010-2015
*   Author(s): Jonathan Poelen
*/

#pragma once

#include <type_traits>
#include <utility>

#include <cassert>
#include <cstdint>
#include <iostream>

#include "utils/sugar/noncopyable.hpp"
#include "core/RDP/orders/RDPOrdersCommon.hpp"
#include "core/RDP/orders/RDPOrdersPrimaryGlyphIndex.hpp"
#include "core/RDP/caches/glyphcache.hpp"


struct BGRPalette;
class RDPDestBlt;
class RDPMultiDstBlt;
class RDPPatBlt;
class RDPOpaqueRect;
class RDPMultiOpaqueRect;
class RDPScrBlt;
class RDPMemBlt;
class RDPMem3Blt;
class RDPLineTo;
class RDPGlyphIndex;
class RDPPolygonSC;
class RDPPolygonCB;
class RDPPolyline;
class RDPEllipseSC;
class RDPEllipseCB;

struct RDPBitmapData;
struct Pointer;
struct Rect;
class Bitmap;

class RDPColCache;
class RDPBrushCache;

namespace RDP {
    class RDPMultiPatBlt;
    class RDPMultiScrBlt;
    class FrameMarker;

    namespace RAIL {
        class NewOrExistingWindow;
        class WindowIcon;
        class CachedIcon;
        class DeletedWindow;
        class NewOrExistingNotificationIcons;
        class DeletedNotificationIcons;
        class ActivelyMonitoredDesktop;
        class NonMonitoredDesktop;
    }
}

namespace gdi {

struct GraphicDepth
{
    static constexpr GraphicDepth unspecified() { return {0}; }
    static constexpr GraphicDepth depth8() { return {1}; }
    static constexpr GraphicDepth depth15() { return {2}; }
    static constexpr GraphicDepth depth16() { return {3}; }
    static constexpr GraphicDepth depth24() { return {4}; }

    static constexpr GraphicDepth from_bpp(uint8_t bpp) {
        return {
            bpp == 8  ? depth8() :
            bpp == 15 ? depth15() :
            bpp == 16 ? depth16() :
            bpp == 24 ? depth24() :
            bpp == 32 ? depth24() : // TODO useless ?
            unspecified()
        };
    }

    GraphicDepth() = default;

    constexpr GraphicDepth(GraphicDepth const &) = default;
    GraphicDepth & operator=(GraphicDepth const &) = default;

private:
    struct bpp_table { uint8_t table[5] = {0, 8, 15, 16, 24}; };
public:
    constexpr uint8_t to_bpp() const {
        return bpp_table{}.table[unsigned(this->depth_)];
    }

    constexpr bool is_defined() const { return !this->is_unspecified(); }
    constexpr bool is_unspecified() const { return this->depth_ == unspecified().depth_; }
    constexpr bool is_depth8() const { return this->depth_ == depth8().depth_; }
    constexpr bool is_depth15() const { return this->depth_ == depth15().depth_; }
    constexpr bool is_depth16() const { return this->depth_ == depth16().depth_; }
    constexpr bool is_depth24() const { return this->depth_ == depth24().depth_; }

    constexpr bool contains(GraphicDepth depth) const {
        return this->is_unspecified() || depth.is_unspecified() || (this->depth_ == depth.depth_);
    }

    constexpr GraphicDepth const & depth_or(GraphicDepth const & default_depth) const {
        return this->is_unspecified() ? default_depth : *this;
    }

    constexpr unsigned id() const { return this->depth_; }

private:
    enum class PrivateDepth { unspecified_, depth8_, depth15_, depth16_, depth24_, };
public:
    // for switch/case, == and !=
    constexpr operator PrivateDepth () const { return static_cast<PrivateDepth>(this->depth_); }

private:
    constexpr GraphicDepth(uint8_t depth) : depth_(depth) {}

    uint8_t depth_;
};

constexpr bool operator < (GraphicDepth const & depth1, GraphicDepth const & depth2) {
    return depth1.id() < depth2.id();
}

constexpr bool operator > (GraphicDepth const & depth1, GraphicDepth const & depth2) {
    return (depth2 < depth1);
}

constexpr bool operator <= (GraphicDepth const & depth1, GraphicDepth const & depth2) {
    return !(depth2 < depth1);
}

constexpr bool operator >= (GraphicDepth const & depth1, GraphicDepth const & depth2) {
    return !(depth1 < depth2);
}


struct GraphicColorCtx
{
    GraphicColorCtx(GraphicDepth depth, BGRPalette const * palette)
    : depth(depth)
    , palette(palette)
    {
        assert(depth == GraphicDepth::depth8() ? bool(palette) : true);
    }

    static GraphicColorCtx depth8(BGRPalette && palette) = delete;
    static GraphicColorCtx depth8(BGRPalette const & palette) { return {GraphicDepth::depth8(), &palette}; }

    static GraphicColorCtx depth15() { return {GraphicDepth::depth15(), nullptr}; }
    static GraphicColorCtx depth16() { return {GraphicDepth::depth16(), nullptr}; }
    static GraphicColorCtx depth24() { return {GraphicDepth::depth24(), nullptr}; }

    static GraphicColorCtx from_bpp(uint8_t bpp, BGRPalette const * palette)
    { return {GraphicDepth::from_bpp(bpp), palette}; }

    static GraphicColorCtx from_bpp(uint8_t bpp, BGRPalette const & palette)
    { return {GraphicDepth::from_bpp(bpp), &palette}; }

    static GraphicColorCtx from_bpp(uint8_t bpp, BGRPalette const && palette) = delete;

    GraphicDepth depth;
    BGRPalette const * palette;
};


struct GraphicApi : private noncopyable
{
    GraphicApi() {}

    virtual ~GraphicApi() = default;

    virtual GraphicDepth const & order_depth() const = 0;

    virtual void set_pointer(Pointer      const &) {}
    virtual void set_palette(BGRPalette   const &) {}

    virtual void draw(RDP::FrameMarker    const & cmd) = 0;
    virtual void draw(RDPDestBlt          const & cmd, Rect clip) = 0;
    virtual void draw(RDPMultiDstBlt      const & cmd, Rect clip) = 0;
    virtual void draw(RDPScrBlt           const & cmd, Rect clip) = 0;
    virtual void draw(RDP::RDPMultiScrBlt const & cmd, Rect clip) = 0;
    virtual void draw(RDPMemBlt           const & cmd, Rect clip, Bitmap const & bmp) = 0;
    virtual void draw(RDPBitmapData       const & cmd, Bitmap const & bmp) = 0;

    virtual void draw(RDPPatBlt           const & cmd, Rect clip, GraphicColorCtx color_ctx) = 0;
    virtual void draw(RDP::RDPMultiPatBlt const & cmd, Rect clip, GraphicColorCtx color_ctx) = 0;
    virtual void draw(RDPOpaqueRect       const & cmd, Rect clip, GraphicColorCtx color_ctx) = 0;
    virtual void draw(RDPMultiOpaqueRect  const & cmd, Rect clip, GraphicColorCtx color_ctx) = 0;
    virtual void draw(RDPLineTo           const & cmd, Rect clip, GraphicColorCtx color_ctx) = 0;
    virtual void draw(RDPPolygonSC        const & cmd, Rect clip, GraphicColorCtx color_ctx) = 0;
    virtual void draw(RDPPolygonCB        const & cmd, Rect clip, GraphicColorCtx color_ctx) = 0;
    virtual void draw(RDPPolyline         const & cmd, Rect clip, GraphicColorCtx color_ctx) = 0;
    virtual void draw(RDPEllipseSC        const & cmd, Rect clip, GraphicColorCtx color_ctx) = 0;
    virtual void draw(RDPEllipseCB        const & cmd, Rect clip, GraphicColorCtx color_ctx) = 0;
    virtual void draw(RDPMem3Blt          const & cmd, Rect clip, GraphicColorCtx color_ctx, Bitmap const & bmp) = 0;
    virtual void draw(RDPGlyphIndex       const & cmd, Rect clip, GraphicColorCtx color_ctx, GlyphCache const & gly_cache) = 0;

    // NOTE maybe in an other interface
    virtual void draw(const RDP::RAIL::NewOrExistingWindow            &) {}
    virtual void draw(const RDP::RAIL::WindowIcon                     &) {}
    virtual void draw(const RDP::RAIL::CachedIcon                     &) {}
    virtual void draw(const RDP::RAIL::DeletedWindow                  &) {}
    virtual void draw(const RDP::RAIL::NewOrExistingNotificationIcons &) {}
    virtual void draw(const RDP::RAIL::DeletedNotificationIcons       &) {}
    virtual void draw(const RDP::RAIL::ActivelyMonitoredDesktop       &) {}
    virtual void draw(const RDP::RAIL::NonMonitoredDesktop            &) {}

    // TODO The 2 methods below should not exist and cache access be done before calling drawing orders
    virtual void draw(RDPColCache   const &) {}
    virtual void draw(RDPBrushCache const &) {}

    virtual void begin_update() {}
    virtual void end_update() {}

    virtual void sync() {}

    // TODO berk, data within size
    virtual void set_row(std::size_t rownum, const uint8_t * data) { (void)rownum; (void)data; }

    virtual GraphicDepth const & order_depth() const = 0;

    virtual void set_depths(GraphicDepth const & depths) = 0;
};


class BlackoutGraphic final : public GraphicApi
{
public:
    void draw(RDP::FrameMarker    const & cmd) override { this->draw_impl( cmd); }
    void draw(RDPDestBlt          const & cmd, Rect clip) override { this->draw_impl(cmd, clip); }
    void draw(RDPMultiDstBlt      const & cmd, Rect clip) override { this->draw_impl(cmd, clip); }
    void draw(RDPPatBlt           const & cmd, Rect clip, GraphicColorCtx color_ctx) override { this->draw_impl(cmd, clip, color_ctx); }
    void draw(RDP::RDPMultiPatBlt const & cmd, Rect clip, GraphicColorCtx color_ctx) override { this->draw_impl(cmd, clip, color_ctx); }
    void draw(RDPOpaqueRect       const & cmd, Rect clip, GraphicColorCtx color_ctx) override { this->draw_impl(cmd, clip, color_ctx); }
    void draw(RDPMultiOpaqueRect  const & cmd, Rect clip, GraphicColorCtx color_ctx) override { this->draw_impl(cmd, clip, color_ctx); }
    void draw(RDPScrBlt           const & cmd, Rect clip) override { this->draw_impl(cmd, clip); }
    void draw(RDP::RDPMultiScrBlt const & cmd, Rect clip) override { this->draw_impl(cmd, clip); }
    void draw(RDPLineTo           const & cmd, Rect clip, GraphicColorCtx color_ctx) override { this->draw_impl(cmd, clip, color_ctx); }
    void draw(RDPPolygonSC        const & cmd, Rect clip, GraphicColorCtx color_ctx) override { this->draw_impl(cmd, clip, color_ctx); }
    void draw(RDPPolygonCB        const & cmd, Rect clip, GraphicColorCtx color_ctx) override { this->draw_impl(cmd, clip, color_ctx); }
    void draw(RDPPolyline         const & cmd, Rect clip, GraphicColorCtx color_ctx) override { this->draw_impl(cmd, clip, color_ctx); }
    void draw(RDPEllipseSC        const & cmd, Rect clip, GraphicColorCtx color_ctx) override { this->draw_impl(cmd, clip, color_ctx); }
    void draw(RDPEllipseCB        const & cmd, Rect clip, GraphicColorCtx color_ctx) override { this->draw_impl(cmd, clip, color_ctx); }
    void draw(RDPBitmapData       const & cmd, Bitmap const & bmp) override { this->draw_impl(cmd, bmp); }
    void draw(RDPMemBlt           const & cmd, Rect clip, Bitmap const & bmp) override { this->draw_impl(cmd, clip, bmp);}
    void draw(RDPMem3Blt          const & cmd, Rect clip, GraphicColorCtx color_ctx, Bitmap const & bmp) override { this->draw_impl(cmd, clip, color_ctx, bmp); }
    void draw(RDPGlyphIndex       const & cmd, Rect clip, GraphicColorCtx color_ctx, GlyphCache const & gly_cache) override { this->draw_impl(cmd, clip, color_ctx, gly_cache); }

    void draw(const RDP::RAIL::NewOrExistingWindow            & cmd) override { this->draw_impl(cmd); }
    void draw(const RDP::RAIL::WindowIcon                     & cmd) override { this->draw_impl(cmd); }
    void draw(const RDP::RAIL::CachedIcon                     & cmd) override { this->draw_impl(cmd); }
    void draw(const RDP::RAIL::DeletedWindow                  & cmd) override { this->draw_impl(cmd); }
    void draw(const RDP::RAIL::NewOrExistingNotificationIcons & cmd) override { this->draw_impl(cmd); }
    void draw(const RDP::RAIL::DeletedNotificationIcons       & cmd) override { this->draw_impl(cmd); }
    void draw(const RDP::RAIL::ActivelyMonitoredDesktop       & cmd) override { this->draw_impl(cmd); }
    void draw(const RDP::RAIL::NonMonitoredDesktop            & cmd) override { this->draw_impl(cmd); }

    void draw(RDPColCache   const & cmd) override { this->draw_impl(cmd); }
    void draw(RDPBrushCache const & cmd) override { this->draw_impl(cmd); }

private:
    template<class... Args>
    void draw_impl(Args const & ...) {}

public:

    void set_depths(gdi::GraphicDepth const & depths) override {
        this->order_depth_ = depths;
    }

    GraphicDepth const & order_depth() const override {
        return this->order_depth_;
    }

    gdi::GraphicDepth order_depth_;

    BlackoutGraphic(gdi::GraphicDepth const & depths)
        : order_depth_(depths) {}
};


inline gdi::GraphicApi & null_gd() {
    static gdi::BlackoutGraphic gd(gdi::GraphicDepth::unspecified());
    return gd;
}


struct TextMetrics
{
    int width = 0;
    int height = 0;

    TextMetrics(const Font & font, const char * unicode_text)
    {
        UTF8toUnicodeIterator unicode_iter(unicode_text);
        uint16_t height_max = 0;
        for (; uint32_t c = *unicode_iter; ++unicode_iter) {
            const FontChar & font_item = font.glyph_or_unknown(c);
            this->width += font_item.incby;
            height_max = std::max(height_max, font_item.height);
        }
        this->height = height_max;
    }
};


// TODO implementation of the server_draw_text function below is a small subset of possibilities text can be packed (detecting duplicated strings). See MS-RDPEGDI 2.2.2.2.1.1.2.13 GlyphIndex (GLYPHINDEX_ORDER)
static inline void server_draw_text(
    GraphicApi & drawable, Font const & font,
    int16_t x, int16_t y, const char * text,
    uint32_t fgcolor, uint32_t bgcolor,
    GraphicColorCtx color_ctx,
    Rect clip
) {
    // BUG TODO static not const is a bad idea
    static GlyphCache mod_glyph_cache;

    UTF8toUnicodeIterator unicode_iter(text);

    while (*unicode_iter) {
        int total_width = 0;
        int total_height = 0;
        uint8_t data[256];
        auto data_begin = std::begin(data);
        const auto data_end = std::end(data)-2;

        const int cacheId = 7;
        int distance_from_previous_fragment = 0;
        while (data_begin != data_end) {
            const uint32_t charnum = *unicode_iter;
            if (!charnum) {
                break ;
            }
            ++unicode_iter;

            int cacheIndex = 0;
            FontChar const * font_item = font.glyph_at(charnum);
            if (!font_item) {
                LOG(LOG_WARNING, "server_draw_text() - character not defined >0x%02x<", charnum);
                font_item = &font.unknown_glyph();
            }

            // TODO avoid passing parameters by reference to get results
            const GlyphCache::t_glyph_cache_result cache_result =
                mod_glyph_cache.add_glyph(*font_item, cacheId, cacheIndex);
            (void)cache_result; // supress warning

            *data_begin = cacheIndex;
            ++data_begin;
            *data_begin = distance_from_previous_fragment;
            ++data_begin;
            distance_from_previous_fragment = font_item->incby;
            total_width += font_item->incby;
            total_height = std::max(uint16_t(total_height), font_item->height);
        }

        const Rect bk(x, y, total_width + 1, total_height + 1);

        RDPGlyphIndex glyphindex(
            cacheId,            // cache_id
            0x03,               // fl_accel
            0x0,                // ui_charinc
            1,                  // f_op_redundant,
            fgcolor,            // BackColor (text color)
            bgcolor,            // ForeColor (color of the opaque rectangle)
            bk,                 // bk
            bk,                 // op
            // brush
            RDPBrush(0, 0, 3, 0xaa,
                reinterpret_cast<const uint8_t *>("\xaa\x55\xaa\x55\xaa\x55\xaa\x55")),
            x,                  // glyph_x
            y + total_height,   // glyph_y
            data_begin - data,  // data_len in bytes
            data                // data
        );

        x += total_width;

        drawable.draw(glyphindex, clip, color_ctx, mod_glyph_cache);
    }
}

}
