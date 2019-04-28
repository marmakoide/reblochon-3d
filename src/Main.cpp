#include <SDL.h>
#include "LoadPNG.h"
#include "Renderer.h"
#include "Macros.h"

using namespace reb;

const unsigned int SCREEN_WIDTH  = 640;
const unsigned int SCREEN_HEIGHT = 480;



bool
load_map(const char* path, Map& map) {
	// Load source picture
	SDL_Surface* src = load_png(path);
	if (!src)
		return false;

	// Setup map size
	map = Map(src->w, src->h);

	// Setup map elements
	std::fill(map.data().begin(), map.data().end(), 0);

	uint8_t const* scanline = (uint8_t const*)src->pixels;
	for(int i = 0; i < src->h; ++i, scanline += src->pitch)
		for(int j = 0; j < src->w; ++j) {
			switch(scanline[j]) {
				case 63:
					map(j, i) = 1;
					break;
				default:
					break;
			}
		}

	// Job done
	SDL_FreeSurface(src);
	return true;
}



class State {
public:
	State() :
		m_angle(0.f),
		m_angle_delta((M_PI / 180.f) * 1.f),
		m_pos(Eigen::Vector3f::Zero()),
		m_pos_delta(Eigen::Vector3f(0.f, .1f, 0.f)) { }

	void set(float angle,
		       const Eigen::Vector3f& pos) {
		m_angle = angle;
		m_pos = pos;
	}

	inline float
	angle() {
		return m_angle;
	}

	inline const Eigen::Vector3f&
	pos() const {
		return m_pos;
	}

	void
	move_forward() {
		Eigen::Matrix3f rot;
		rot = Eigen::AngleAxisf(m_angle, Eigen::Vector3f::UnitZ());
		m_pos += rot * m_pos_delta;
	}

	void
	move_backward() {
		Eigen::Matrix3f rot;
		rot = Eigen::AngleAxisf(m_angle, Eigen::Vector3f::UnitZ());
		m_pos -= rot * m_pos_delta;
	}

	void
	rotate_left() {
		m_angle += m_angle_delta;
	}

	void
	rotate_right() {
		m_angle -= m_angle_delta;
	}

private:
	float m_angle;
	float m_angle_delta;
	Eigen::Vector3f m_pos;
	Eigen::Vector3f m_pos_delta;
}; // class State



int
main(int UNUSED_PARAM(argc), char** UNUSED_PARAM(argv)) {
	State state;
	state.set((180.f / M_PI) * 30.f, Eigen::Vector3f(4.5f, 4.5f, 1.7f));

	Renderer view_renderer(SCREEN_WIDTH, SCREEN_HEIGHT, Renderer::focal_length_from_angle((180.f / M_PI) * 60.f));
	Map map;
	load_map("./data/test.png", map);

	// SDL initialization
	if (SDL_Init(SDL_INIT_VIDEO)) {
		SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Unable to initialize SDL: %s", SDL_GetError());
		return EXIT_FAILURE;
	}

	// Load the texture atlas
	SDL_Surface* texture_atlas = load_png("./data/texture-pack-16x16.png");
	if (!texture_atlas) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not load texture atlas: %s\n", SDL_GetError());
		SDL_Quit();
		return EXIT_FAILURE;
	}

	// Create a window
	SDL_Window* window = SDL_CreateWindow("reblochon-3d editor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
	if (!window) {
		SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Could not create window: %s\n", SDL_GetError());
		SDL_FreeSurface(texture_atlas);
		SDL_Quit();
		return EXIT_FAILURE;
	}

	// Create a renderer
	SDL_Surface* framebuffer = SDL_GetWindowSurface(window);
	SDL_Renderer* renderer = SDL_CreateSoftwareRenderer(framebuffer);
	if (!renderer) {
		SDL_DestroyWindow(window);
		SDL_FreeSurface(texture_atlas);
		SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Could not create SDL renderer : %s\n", SDL_GetError());
		return EXIT_FAILURE;
	}

	// Create an indexed color framebuffer	
	SDL_Surface* indexed_color_framebuffer =
		SDL_CreateRGBSurface(0,
		                     SCREEN_WIDTH, SCREEN_HEIGHT, 8,
		                     0x00000000,
		                     0x00000000,
		                     0x00000000,
		                     0x00000000);
	SDL_SetSurfacePalette(indexed_color_framebuffer, texture_atlas->format->palette);

	// Event processing & display loop
	bool quit = false; 
	while(!quit) {
		// Even read & process
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_QUIT:
					quit = true;
					break;

				case SDL_KEYDOWN:
					switch(event.key.keysym.sym) {
						case SDLK_UP:
							state.move_forward();
							break;

						case SDLK_DOWN:
							state.move_backward();
							break;

						case SDLK_LEFT:
							state.rotate_left();
							break;

						case SDLK_RIGHT:
							state.rotate_right();
							break;

						default:
							break;
					}
					break;

				default:
					break;
			}
		}

		// Update the display
		view_renderer.render(indexed_color_framebuffer, map, state.angle(), state.pos());
		SDL_BlitSurface(indexed_color_framebuffer, NULL, framebuffer, NULL);
		SDL_UpdateWindowSurface(window);

		// Sleep for a while
		SDL_Delay(20);
	}

	// Free ressources
	SDL_FreeSurface(indexed_color_framebuffer);
	SDL_FreeSurface(texture_atlas);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	// Job done
	return EXIT_SUCCESS;
}
