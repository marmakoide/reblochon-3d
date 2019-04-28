#include <png.h>
#include <errno.h>
#include "LoadPNG.h"



namespace reb {
namespace internals {
	/*
	 * Sets the palette of a SDF_Surface instance from a libpng palette
	 */

	bool
	set_palette_from_png_data(SDL_Surface* surface,
	                          png_colorp palette_data,
	                          int palette_size) {
		// Convert png palette to SDL palette
		SDL_Color* colors = new SDL_Color[palette_size];
		for(int i = 0; i < palette_size; ++i) {
			colors[i].r = palette_data[i].red;
			colors[i].g = palette_data[i].green;
			colors[i].b = palette_data[i].blue;
			colors[i].a = 0xff;
		}

		// Set the SDL_Surface palette
		bool ret = true;

		SDL_Palette* palette = SDL_AllocPalette(palette_size);
		if (SDL_SetPaletteColors(palette, colors, 0, palette_size) == 0) {
			if (SDL_SetSurfacePalette(surface, palette) != 0)
				ret = false;
		}
		else
			ret = false;

		// Job done
		delete[] colors;
		SDL_FreePalette(palette);

		return ret;
	}



	/*
	 * Encapsulate the libpng reading routines to let the compiler take care of
	 * freeing ressources
	 */

	class PngReadContext {
	public:
		PngReadContext() :
			m_fp(0),
			m_png_ptr(0),
			m_info_ptr(0) { }

		~PngReadContext() {
			png_destroy_read_struct(&m_png_ptr, &m_info_ptr, NULL);

			if (m_fp)
				fclose(m_fp);
		}

		bool
		setup(const char* path) {
			// Open the input file
			m_fp = fopen(path, "r");
			if (!m_fp) {
				SDL_SetError("%s", strerror(errno));
				return false;
			}

			// Creates libpng structures
			m_png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
			if (!m_png_ptr) {
				SDL_SetError("png_create_write_struct failed");
				return false;
			}

			m_info_ptr = png_create_info_struct(m_png_ptr);
			if (!m_info_ptr) {
				SDL_SetError("png_create_info_struct failed");
				return false;
			}

			// Attemp to initialize libpng structures
			if (setjmp(png_jmpbuf(m_png_ptr))) {
				fclose(m_fp);
				SDL_SetError("png_init_io failed");
				return false;
			}

			png_init_io(m_png_ptr, m_fp);

			// Job done
			return true;
		}

		bool
		read_info() {
			if (setjmp(png_jmpbuf(m_png_ptr))) {
				SDL_SetError("png_read_info failed");
				return false;
			}

			png_read_info(m_png_ptr, m_info_ptr);

			return true;
		}

		inline png_uint_32
		get_image_width() {
			return png_get_image_width(m_png_ptr, m_info_ptr);
		}

		inline png_uint_32
		get_image_height() {
			return png_get_image_height(m_png_ptr, m_info_ptr);
		}

		inline png_byte
		get_color_type() {
			return png_get_color_type(m_png_ptr, m_info_ptr);
		}

		inline png_byte
		get_bit_depth() {
			return png_get_bit_depth(m_png_ptr, m_info_ptr);
		}

		inline bool
		get_color_palette(png_colorp& palette_data,
		                  int& palette_size) {
			if (!png_get_PLTE(m_png_ptr, m_info_ptr, &palette_data, &palette_size)) {
				SDL_SetError("no PLTE chunk found");
				return false;		
			}

			return true;
		}

		inline bool
		read_image(SDL_Surface* surface) {
			// Allocate row pointers
			png_bytep* row_pointers = new png_bytep[surface->h];
		  for(int i = 0; i < surface->h; i++)
  		  row_pointers[i] = ((png_bytep)surface->pixels) + i * surface->pitch;	

			// Read the data
			if (setjmp(png_jmpbuf(m_png_ptr))) {
				SDL_SetError("png_read_image failed");
				delete[] row_pointers;
				return false;
			}
			
			png_read_image(m_png_ptr, row_pointers);

			// Job done
			delete[] row_pointers;
			return true;
		}

	private:
		FILE* m_fp;
		png_structp m_png_ptr;
		png_infop m_info_ptr;
	}; // struct PngReadContext
} // namespace internals
} // namespace reb;



namespace reb {
	SDL_Surface*
	load_png(const char* path) {
		// Setup PNG read context
		internals::PngReadContext context;
		if (!context.setup(path))
			return NULL;

		// Read input metadata
		if (!context.read_info())
			return NULL;

		png_uint_32 width   = context.get_image_width();
  	png_uint_32 height  = context.get_image_height();
  	png_byte color_type = context.get_color_type();
		png_byte bit_depth  = context.get_bit_depth();

		// Gives up on anything which is not 8 bits depth
		if (bit_depth != 8) {
			SDL_SetError("bit depth is not 8 bits");
			return NULL;		
		}

		// Gives up on anything which is not indexed colors
		if (color_type != 3) {
			SDL_SetError("not an indexed color type");
			return NULL;		
		}

		// Read the palette data
		png_colorp palette_data; 
		int palette_size;
		if (!context.get_color_palette(palette_data, palette_size)) {
			SDL_SetError("no PLTE chunk found");
			return NULL;		
		}

		if (palette_size != 256) {
			SDL_SetError("color palette do not have 256 entries");
			return NULL;		
		}

		// Create the SDLSurface instance
		SDL_Surface* ret =
			SDL_CreateRGBSurface(0,
		                     width, height, 8,
		                     0x00000000,
		                     0x00000000,
		                     0x00000000,
		                     0x00000000);

		// Read input data
		if (!context.read_image(ret)) {
			SDL_FreeSurface(ret);
			return NULL;
		}

		// Set the palette of the surface
		if (!internals::set_palette_from_png_data(ret, palette_data, 256)) {
			SDL_FreeSurface(ret);
			return NULL;
		}

		// Job done
		return ret;
	}
} // namespace reb
