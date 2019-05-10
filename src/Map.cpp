#include "SDL.h"
#include "Map.h"

using namespace reb;



Map::Cell::Cell() :
	m_height(0),
	m_wall_texture_id(0),
	m_floor_texture_id(0) { }



Map::Map() :
	m_cell_array(1, 1) { }



Map::Map(int w, int h)
	: m_cell_array(w, h) { }



bool
Map::load(const char* path, Map& map) {
	std::uint32_t file_version_number = 1;
	const char* file_signature = "reblochon3d-map";
	const int file_signature_len = 15;

	// Open input file
	SDL_RWops* file = SDL_RWFromFile(path, "rb");
	if (file == NULL)
		return false;

	// Read and check the signature
	char signature[file_signature_len];
	memset(signature, 0, file_signature_len);
	if (SDL_RWread(file, signature, file_signature_len, 1) == 0)
		return false;

	if (strncmp(signature, file_signature, file_signature_len) != 0) {
		SDL_SetError("wrong file format signature");
		return false;
	}

	// Read and check the version number
	std::uint32_t version_number = SDL_ReadLE32(file);
	if (version_number != file_version_number) {
		SDL_SetError("unsupport file format version");
		return false;
	}

	// Read the size of the map
	std::uint16_t map_w = SDL_ReadLE16(file);	
	std::uint16_t map_h = SDL_ReadLE16(file);	
	map = Map(map_w, map_h);

	// Read the cells data
	for(std::uint16_t i = 0; i < map_w; ++i) {
		for(std::uint16_t j = 0; j < map_h; ++j) {
			// Read cell data
			std::uint32_t height = SDL_ReadLE32(file);
			std::uint8_t wall_texture_id = SDL_ReadU8(file);
			std::uint8_t floor_texture_id = SDL_ReadU8(file);

			// Write cell data
			Map::Cell& cell = map.cell_array()(i, j);
			cell.height() = height;
			cell.wall_texture_id() = wall_texture_id;
			cell.floor_texture_id() = floor_texture_id;
		}
	}

	// Close input file
	if (SDL_RWclose(file) != 0)
		return false;

	// Job done
	return true;
}
