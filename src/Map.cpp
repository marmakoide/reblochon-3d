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
	: m_spawn_point(0, 0), 
    m_cell_array(w, h) { }



bool
Map::load(const char* path, Map& map) {
	const char* spawn_tag = "spawn";
	const char* map_tag = "_map_";
	const int tag_len = 5;

	std::uint32_t file_version_number = 1;
	const char* file_signature = "reblochon3d-map";
	const int file_signature_len = 15;

	// Open input file
	SDL_RWops* file = SDL_RWFromFile(path, "rb");
	if (file == NULL)
		return false;

	// Read and check the signature
	{
		char signature[file_signature_len];
		memset(signature, 0, file_signature_len);
		if (SDL_RWread(file, signature, file_signature_len, 1) == 0)
			return false;

		if (strncmp(signature, file_signature, file_signature_len) != 0) {
			SDL_SetError("wrong file format signature");
			return false;
		}
	}

	// Read and check the version number
	{
		std::uint32_t version_number = SDL_ReadLE32(file);
		if (version_number != file_version_number) {
			SDL_SetError("unsupported file format version");
			return false;	
		}
	}

	// While there are tags
	bool map_tag_found = false;
	bool spawn_tag_found = false;
	Eigen::Vector2f spawn_point(0, 0);

	while(true) {
		// Read the tag
		char tag[tag_len];
		memset(tag, 0, tag_len);
		if (SDL_RWread(file, tag, tag_len, 1) == 0)
			break;

		// Found a map tag
		if ((strncmp(tag, map_tag, tag_len) == 0) and !map_tag_found) {
			map_tag_found = true;

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
		}
		// Found a spawn tag
		else if ((strncmp(tag, spawn_tag, tag_len) == 0) and !spawn_tag_found) {
			spawn_tag_found = true;
			std::int32_t x = static_cast<std::int32_t>(SDL_ReadLE32(file));
			std::int32_t y = static_cast<std::int32_t>(SDL_ReadLE32(file));
			spawn_point = Eigen::Vector2f(x / 256.f, y / 256.f);
		}
		// Unknown tag
		else {
			SDL_SetError("unsupported tag");			
			return false;
		}
	}

	// Check that we found all the required tags
	if (!map_tag_found) {
		SDL_SetError("no map defined");
		return false;
	}

	if (!spawn_tag_found) {
		SDL_SetError("no spawn point defined");
		return false;
	}

	// Setup the spawn point
	map.spawn_point() = spawn_point;

	// Close input file
	if (SDL_RWclose(file) != 0)
		return false;

	// Job done
	return true;
}
