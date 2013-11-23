/*
 * (C) Copyright 2013 Ye Yan and others.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Lesser General Public License
 * (LGPL) version 2.1 which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/lgpl-2.1.html
 *
 * This library/program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * Contributors:
 */

#include <cairo/cairo.h>
#include <boost/shared_ptr.hpp>

#include <string>
#include <sstream>  

class ImageBuilder
{
public:
    ImageBuilder (int width, int height):
        surface_(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height)),
        ctx_(cairo_create(surface_))
    {
    }

    virtual ~ImageBuilder ()
    {
        cairo_destroy(ctx_);
        cairo_surface_destroy(surface_);
    }

    void DrawFilledRect(double red, double green, double blue,
            int x, int y, int width, int height)
    {
        cairo_save(ctx_);

        cairo_set_source_rgb(ctx_, red, green, blue);
        cairo_set_line_width(ctx_, 1);

        cairo_rectangle(ctx_, x, y, width, height);
        cairo_stroke_preserve(ctx_);
        cairo_fill(ctx_);

        cairo_restore(ctx_);
    }

    void WriteText(double x, double y, 
            const std::string &font, double size, double rowPitch, const std::string &text)
            //const char *font, double size, const char *text)
    {
        cairo_text_extents_t extents;

        cairo_save(ctx_);

        cairo_select_font_face(ctx_, font.c_str(), 
                CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

        cairo_set_font_size(ctx_, size);
        cairo_text_extents(ctx_, text.c_str(), &extents);

        std::istringstream is(text);
        std::string line;

        while(!is.eof())
        {
            y = y - extents.y_bearing + rowPitch;
            cairo_move_to(ctx_, x - extents.x_bearing, y);
            std::getline(is, line);
            cairo_show_text(ctx_, line.c_str());
        }

        cairo_restore(ctx_);
    }

    template<class U>
    void WriteImage(U &frameData, int offsetX, int offsetY, int tgtWidth, int tgtHeight)
    {
        WriteImage(
                frameData.Data(), frameData.Width(), frameData.Height(),
                offsetX, offsetY, tgtWidth, tgtHeight);
    }

    void WriteImage(
            // source image data in RGB24 format
            uint8_t *srcData, int srcWidth, int srcHeight, 
            // offset
            int offsetX, int offsetY,
            // resize
            int tgtWidth, int tgtHeight)
    {
        typedef boost::shared_ptr<cairo_surface_t> SurfacePtr;

        cairo_save(ctx_);
        cairo_translate(ctx_, offsetX, offsetY);
        cairo_scale(ctx_, 
                static_cast<double>(tgtWidth)/srcWidth, 
                static_cast<double>(tgtHeight)/srcHeight);

        {
            int stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, srcWidth);
            SurfacePtr srcSurface(
                    cairo_image_surface_create_for_data(
                        srcData, CAIRO_FORMAT_RGB24, srcWidth, srcHeight, stride),
                    cairo_surface_destroy);
            cairo_set_source_surface(ctx_, srcSurface.get(), 0, 0);
        }

        cairo_paint(ctx_);
        cairo_restore(ctx_);
    }

    void SaveToPng(const std::string &fileName)//const char* fileName)
    {
        cairo_surface_write_to_png(surface_, fileName.c_str());
    }
private:
    /* data */
    cairo_surface_t *surface_;
    cairo_t *ctx_;
};












