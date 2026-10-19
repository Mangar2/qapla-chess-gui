/**
 * @license
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Volker Böhm
 * @copyright Copyright (c) 2025 Volker Böhm
 */

/**
 * @file macos-draw.mm
 * @brief Recording and painting of QaplaUi drawing, and the fonts, on Core Graphics and Core Text.
 */

#import "macos-backend.h"

#import <CoreText/CoreText.h>

#include "data/chess-font.h"
#include "data/ibm-plex-mono-font.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <numbers>

namespace QaplaMac {

    // ------------------------------------------------------------------------------------------
    // Colors
    // ------------------------------------------------------------------------------------------

    NSColor* nsColor(Color32 color) {
        const CGFloat red = static_cast<CGFloat>(color & 0xFFU) / 255.0;
        const CGFloat green = static_cast<CGFloat>((color >> 8U) & 0xFFU) / 255.0;
        const CGFloat blue = static_cast<CGFloat>((color >> 16U) & 0xFFU) / 255.0;
        const CGFloat alpha = static_cast<CGFloat>((color >> 24U) & 0xFFU) / 255.0;
        return [NSColor colorWithSRGBRed:red green:green blue:blue alpha:alpha];
    }

    NSColor* nsColor(const Vec4& color) {
        return [NSColor colorWithSRGBRed:color.x green:color.y blue:color.z alpha:color.w];
    }

    // ------------------------------------------------------------------------------------------
    // Fonts
    // ------------------------------------------------------------------------------------------

    namespace {
        // Made from the embedded font files without registering them with the system: they are
        // the application's, and no other program needs to see them.
        CTFontDescriptorRef chessFont = nullptr;
        CTFontDescriptorRef monoFont = nullptr;

        CTFontDescriptorRef descriptorFromData(const void* data, size_t size) {
            NSData* bytes = [NSData dataWithBytes:data length:size];
            return CTFontManagerCreateFontDescriptorFromData((__bridge CFDataRef)bytes);
        }

        NSFont* fontFrom(CTFontDescriptorRef descriptor, CGFloat size) {
            if (descriptor == nullptr) {
                return nil;
            }
            return (__bridge_transfer NSFont*)CTFontCreateWithFontDescriptor(descriptor, size, nullptr);
        }

        struct FontKey {
            int font;
            int size;
            bool operator<(const FontKey& other) const {
                return font != other.font ? font < other.font : size < other.size;
            }
        };
    }

    void registerFonts() {
        chessFont = descriptorFromData(chessFontData, chessFontSize);
        monoFont = descriptorFromData(ibmPlexMonoFontData, ibmPlexMonoFontSize);
    }

    float baseFontSize(QaplaUi::Font font) {
        switch (font) {
        case QaplaUi::Font::Chess: return 32.0F;
        case QaplaUi::Font::Mono: return 13.0F;
        case QaplaUi::Font::System:
        case QaplaUi::Font::Inter: return 13.0F;
        }
        return 13.0F;
    }

    NSFont* nsFont(QaplaUi::Font font, float size) {
        static std::map<FontKey, NSFont*> cache;
        // Quantised to quarter points: sizes computed from a board's width vary continuously,
        // and a font per pixel fraction would fill the cache for nothing.
        const FontKey key{ static_cast<int>(font), static_cast<int>(std::lround(size * 4.0F)) };
        auto found = cache.find(key);
        if (found != cache.end()) {
            return found->second;
        }
        const CGFloat points = std::max(1.0F, static_cast<float>(key.size) / 4.0F);
        NSFont* result = nil;
        switch (font) {
        case QaplaUi::Font::Chess:
            result = fontFrom(chessFont, points);
            break;
        case QaplaUi::Font::Mono:
            result = fontFrom(monoFont, points);
            if (result == nil) {
                result = [NSFont monospacedSystemFontOfSize:points weight:NSFontWeightRegular];
            }
            break;
        case QaplaUi::Font::System:
        case QaplaUi::Font::Inter:
            result = [NSFont systemFontOfSize:points];
            break;
        }
        if (result == nil) {
            result = [NSFont systemFontOfSize:points];
        }
        cache.emplace(key, result);
        return result;
    }

    namespace {
        NSString* toNSString(std::string_view text) {
            NSString* result = [[NSString alloc] initWithBytes:text.data() length:text.size()
                                                      encoding:NSUTF8StringEncoding];
            return result != nil ? result : @"";
        }

        NSDictionary* attributes(QaplaUi::Font font, float size, NSColor* color, bool wrap) {
            NSMutableDictionary* result = [NSMutableDictionary dictionary];
            result[NSFontAttributeName] = nsFont(font, size);
            if (color != nil) {
                result[NSForegroundColorAttributeName] = color;
            }
            if (wrap) {
                NSMutableParagraphStyle* style = [[NSMutableParagraphStyle alloc] init];
                style.lineBreakMode = NSLineBreakByWordWrapping;
                result[NSParagraphStyleAttributeName] = style;
            }
            return result;
        }
    }

    float lineHeight(QaplaUi::Font font, float size) {
        NSFont* nsfont = nsFont(font, size);
        return static_cast<float>(std::ceil(nsfont.ascender - nsfont.descender + nsfont.leading));
    }

    Vec2 measureText(QaplaUi::Font font, float size, std::string_view text, float wrapWidth) {
        if (text.empty()) {
            return { 0.0F, lineHeight(font, size) };
        }
        NSString* string = toNSString(text);
        NSDictionary* attrs = attributes(font, size, nil, wrapWidth > 0.0F);
        if (wrapWidth > 0.0F) {
            NSRect bounds = [string boundingRectWithSize:NSMakeSize(wrapWidth, CGFLOAT_MAX)
                                                 options:NSStringDrawingUsesLineFragmentOrigin
                                              attributes:attrs];
            return { static_cast<float>(std::ceil(bounds.size.width)),
                     static_cast<float>(std::ceil(bounds.size.height)) };
        }
        NSSize result = [string sizeWithAttributes:attrs];
        return { static_cast<float>(std::ceil(result.width)),
                 std::max(static_cast<float>(std::ceil(result.height)), lineHeight(font, size)) };
    }

    // ------------------------------------------------------------------------------------------
    // Recording
    // ------------------------------------------------------------------------------------------

    Vec2 RecordingDrawList::local(Vec2 point) const {
        return container_ != nullptr ? point - containerOrigin(*container_) : point;
    }

    void RecordingDrawList::push(DrawCmd&& cmd) {
        if (container_ != nullptr) {
            appendDrawCmd(*container_, std::move(cmd));
        }
    }

    void RecordingDrawList::addLine(Vec2 from, Vec2 to, Color32 color, float thickness) {
        auto a = local(from);
        auto b = local(to);
        DrawCmd cmd;
        cmd.type = DrawCmd::Type::Line;
        cmd.coords = { a.x, a.y, b.x, b.y, 0, 0 };
        cmd.color = color;
        cmd.thickness = thickness;
        push(std::move(cmd));
    }

    void RecordingDrawList::addRect(Vec2 min, Vec2 max, Color32 color, float rounding, float thickness) {
        auto a = local(min);
        auto b = local(max);
        DrawCmd cmd;
        cmd.type = DrawCmd::Type::Rect;
        cmd.coords = { a.x, a.y, b.x, b.y, 0, 0 };
        cmd.color = color;
        cmd.rounding = rounding;
        cmd.thickness = thickness;
        push(std::move(cmd));
    }

    void RecordingDrawList::addRectFilled(Vec2 min, Vec2 max, Color32 color, float rounding) {
        auto a = local(min);
        auto b = local(max);
        DrawCmd cmd;
        cmd.type = DrawCmd::Type::RectFilled;
        cmd.coords = { a.x, a.y, b.x, b.y, 0, 0 };
        cmd.color = color;
        cmd.rounding = rounding;
        push(std::move(cmd));
    }

    void RecordingDrawList::addCircle(Vec2 center, float radius, Color32 color,
        [[maybe_unused]] int segments, float thickness) {
        auto c = local(center);
        DrawCmd cmd;
        cmd.type = DrawCmd::Type::Circle;
        cmd.coords = { c.x, c.y, radius, 0, 0, 0 };
        cmd.color = color;
        cmd.thickness = thickness;
        push(std::move(cmd));
    }

    void RecordingDrawList::addCircleFilled(Vec2 center, float radius, Color32 color,
        [[maybe_unused]] int segments) {
        auto c = local(center);
        DrawCmd cmd;
        cmd.type = DrawCmd::Type::CircleFilled;
        cmd.coords = { c.x, c.y, radius, 0, 0, 0 };
        cmd.color = color;
        push(std::move(cmd));
    }

    void RecordingDrawList::addTriangleFilled(Vec2 first, Vec2 second, Vec2 third, Color32 color) {
        auto a = local(first);
        auto b = local(second);
        auto c = local(third);
        DrawCmd cmd;
        cmd.type = DrawCmd::Type::Triangle;
        cmd.coords = { a.x, a.y, b.x, b.y, c.x, c.y };
        cmd.color = color;
        push(std::move(cmd));
    }

    void RecordingDrawList::addText(Vec2 position, Color32 color, std::string_view text) {
        addText(QaplaUi::getFont(), QaplaUi::getFontSize(), position, color, text);
    }

    void RecordingDrawList::addText(QaplaUi::Font font, float size, Vec2 position, Color32 color,
        std::string_view text) {
        addWrappedText(font, size, position, color, text, 0.0F);
    }

    void RecordingDrawList::addWrappedText(QaplaUi::Font font, float size, Vec2 position,
        Color32 color, std::string_view text, float wrapWidth) {
        if (text.empty()) return;
        auto p = local(position);
        DrawCmd cmd;
        cmd.type = DrawCmd::Type::Text;
        cmd.coords = { p.x, p.y, 0, 0, 0, 0 };
        cmd.color = color;
        cmd.font = font;
        cmd.fontSize = size;
        cmd.wrapWidth = wrapWidth;
        cmd.text = std::string(text);
        push(std::move(cmd));
    }

    void RecordingDrawList::pathArcTo(Vec2 center, float radius, float fromAngle, float toAngle,
        int segments) {
        if (segments <= 0) {
            segments = std::max(8, static_cast<int>(std::abs(toAngle - fromAngle) * radius / 3.0F));
        }
        for (int index = 0; index <= segments; ++index) {
            const float angle = fromAngle + (toAngle - fromAngle) * static_cast<float>(index)
                / static_cast<float>(segments);
            path_.push_back(local({ center.x + std::cos(angle) * radius,
                                    center.y + std::sin(angle) * radius }));
        }
    }

    void RecordingDrawList::pathStroke(Color32 color, bool closed, float thickness) {
        if (path_.size() >= 2) {
            DrawCmd cmd;
            cmd.type = DrawCmd::Type::Polyline;
            cmd.points = path_;
            cmd.color = color;
            cmd.closed = closed;
            cmd.thickness = thickness;
            push(std::move(cmd));
        }
        path_.clear();
    }

    void RecordingDrawList::pushClipRect(Vec2 min, Vec2 max, bool intersectWithCurrent) {
        auto a = local(min);
        auto b = local(max);
        DrawCmd cmd;
        cmd.type = DrawCmd::Type::PushClip;
        cmd.coords = { a.x, a.y, b.x, b.y, 0, 0 };
        cmd.closed = intersectWithCurrent;
        push(std::move(cmd));
    }

    void RecordingDrawList::popClipRect() {
        DrawCmd cmd;
        cmd.type = DrawCmd::Type::PopClip;
        push(std::move(cmd));
    }

    // ------------------------------------------------------------------------------------------
    // Painting
    // ------------------------------------------------------------------------------------------

    namespace {
        void setColor(CGContextRef context, Color32 color, float alpha, bool fill) {
            const CGFloat red = static_cast<CGFloat>(color & 0xFFU) / 255.0;
            const CGFloat green = static_cast<CGFloat>((color >> 8U) & 0xFFU) / 255.0;
            const CGFloat blue = static_cast<CGFloat>((color >> 16U) & 0xFFU) / 255.0;
            const CGFloat a = static_cast<CGFloat>((color >> 24U) & 0xFFU) / 255.0 * alpha;
            if (fill) {
                CGContextSetRGBFillColor(context, red, green, blue, a);
            } else {
                CGContextSetRGBStrokeColor(context, red, green, blue, a);
            }
        }

        CGRect rectOf(const DrawCmd& cmd) {
            return CGRectMake(cmd.coords[0], cmd.coords[1], cmd.coords[2] - cmd.coords[0],
                cmd.coords[3] - cmd.coords[1]);
        }

        CGPathRef roundedPath(CGRect rect, float rounding) {
            const CGFloat radius = std::min<CGFloat>({ rounding, rect.size.width / 2.0,
                rect.size.height / 2.0 });
            if (radius <= 0.0) {
                return CGPathCreateWithRect(rect, nullptr);
            }
            return CGPathCreateWithRoundedRect(rect, radius, radius, nullptr);
        }
    }

    void paint(const std::vector<DrawCmd>& commands, CGContextRef context, float alpha) {
        int clipDepth = 0;
        for (const auto& cmd : commands) {
            switch (cmd.type) {
            case DrawCmd::Type::Line: {
                setColor(context, cmd.color, alpha, false);
                CGContextSetLineWidth(context, cmd.thickness);
                CGContextSetLineCap(context, kCGLineCapButt);
                CGContextMoveToPoint(context, cmd.coords[0], cmd.coords[1]);
                CGContextAddLineToPoint(context, cmd.coords[2], cmd.coords[3]);
                CGContextStrokePath(context);
                break;
            }
            case DrawCmd::Type::Rect: {
                setColor(context, cmd.color, alpha, false);
                CGContextSetLineWidth(context, cmd.thickness);
                // Stroked half a line inside, as ImGui does, so a border stays within its box.
                CGRect rect = CGRectInset(rectOf(cmd), cmd.thickness * 0.5, cmd.thickness * 0.5);
                CGPathRef path = roundedPath(rect, cmd.rounding);
                CGContextAddPath(context, path);
                CGContextStrokePath(context);
                CGPathRelease(path);
                break;
            }
            case DrawCmd::Type::RectFilled: {
                setColor(context, cmd.color, alpha, true);
                CGPathRef path = roundedPath(rectOf(cmd), cmd.rounding);
                CGContextAddPath(context, path);
                CGContextFillPath(context);
                CGPathRelease(path);
                break;
            }
            case DrawCmd::Type::Circle: {
                setColor(context, cmd.color, alpha, false);
                CGContextSetLineWidth(context, cmd.thickness);
                const float r = cmd.coords[2];
                CGContextStrokeEllipseInRect(context,
                    CGRectMake(cmd.coords[0] - r, cmd.coords[1] - r, r * 2.0F, r * 2.0F));
                break;
            }
            case DrawCmd::Type::CircleFilled: {
                setColor(context, cmd.color, alpha, true);
                const float r = cmd.coords[2];
                CGContextFillEllipseInRect(context,
                    CGRectMake(cmd.coords[0] - r, cmd.coords[1] - r, r * 2.0F, r * 2.0F));
                break;
            }
            case DrawCmd::Type::Triangle: {
                setColor(context, cmd.color, alpha, true);
                CGContextMoveToPoint(context, cmd.coords[0], cmd.coords[1]);
                CGContextAddLineToPoint(context, cmd.coords[2], cmd.coords[3]);
                CGContextAddLineToPoint(context, cmd.coords[4], cmd.coords[5]);
                CGContextClosePath(context);
                CGContextFillPath(context);
                break;
            }
            case DrawCmd::Type::Polyline: {
                setColor(context, cmd.color, alpha, false);
                CGContextSetLineWidth(context, cmd.thickness);
                CGContextSetLineJoin(context, kCGLineJoinRound);
                CGContextMoveToPoint(context, cmd.points[0].x, cmd.points[0].y);
                for (size_t index = 1; index < cmd.points.size(); ++index) {
                    CGContextAddLineToPoint(context, cmd.points[index].x, cmd.points[index].y);
                }
                if (cmd.closed) {
                    CGContextClosePath(context);
                }
                CGContextStrokePath(context);
                break;
            }
            case DrawCmd::Type::Text: {
                const Color32 color = cmd.color;
                const CGFloat a = static_cast<CGFloat>((color >> 24U) & 0xFFU) / 255.0 * alpha;
                NSColor* textColor = [nsColor(color) colorWithAlphaComponent:a];
                NSString* string = toNSString(cmd.text);
                NSDictionary* attrs = attributes(cmd.font, cmd.fontSize, textColor, cmd.wrapWidth > 0.0F);
                NSGraphicsContext* previous = [NSGraphicsContext currentContext];
                [NSGraphicsContext setCurrentContext:
                    [NSGraphicsContext graphicsContextWithCGContext:context flipped:YES]];
                if (cmd.wrapWidth > 0.0F) {
                    [string drawWithRect:NSMakeRect(cmd.coords[0], cmd.coords[1], cmd.wrapWidth, CGFLOAT_MAX)
                                 options:NSStringDrawingUsesLineFragmentOrigin
                              attributes:attrs];
                } else {
                    [string drawAtPoint:NSMakePoint(cmd.coords[0], cmd.coords[1]) withAttributes:attrs];
                }
                [NSGraphicsContext setCurrentContext:previous];
                break;
            }
            case DrawCmd::Type::PushClip: {
                CGContextSaveGState(context);
                ++clipDepth;
                CGContextClipToRect(context, rectOf(cmd));
                break;
            }
            case DrawCmd::Type::PopClip: {
                if (clipDepth > 0) {
                    CGContextRestoreGState(context);
                    --clipDepth;
                }
                break;
            }
            }
        }
        while (clipDepth-- > 0) {
            CGContextRestoreGState(context);
        }
    }

} // namespace QaplaMac

// ----------------------------------------------------------------------------------------------
// Views
// ----------------------------------------------------------------------------------------------

@implementation QPContainerView

- (BOOL)isFlipped {
    return YES;
}

- (BOOL)acceptsFirstMouse:(NSEvent*)event {
    // The first click into an inactive window already counts: a board square, a tab.
    return YES;
}

- (void)mouseDown:(NSEvent*)event {
    // A click beside a text field ends its editing, as it does anywhere else on the Mac. The
    // click itself reaches the application through the event monitor.
    NSView* content = self.window.contentView;
    if (content.acceptsFirstResponder) {
        [self.window makeFirstResponder:content];
    }
}

- (void)rightMouseDown:(NSEvent*)event {
}

- (void)drawRect:(NSRect)dirtyRect {
    if (self.hasCustomBackground && self.customBackground != nil) {
        [self.customBackground setFill];
        if (self.cornerRadius > 0.0) {
            [[NSBezierPath bezierPathWithRoundedRect:self.bounds xRadius:self.cornerRadius
                                             yRadius:self.cornerRadius] fill];
        } else {
            NSRectFill(dirtyRect);
        }
    }
}

@end

@implementation QPLayerView {
    std::vector<QaplaMac::DrawCmd> commands_;
}

- (BOOL)isFlipped {
    return YES;
}

- (NSView*)hitTest:(NSPoint)point {
    // Drawing never takes the mouse: the container below it does, as the item it shows.
    return nil;
}

- (void)setCommands:(const std::vector<QaplaMac::DrawCmd>&)commands {
    if (commands == commands_) {
        return;
    }
    commands_ = commands;
    [self setNeedsDisplay:YES];
}

- (void)drawRect:(NSRect)dirtyRect {
    CGContextRef context = [NSGraphicsContext currentContext].CGContext;
    QaplaMac::paint(commands_, context);
}

@end
