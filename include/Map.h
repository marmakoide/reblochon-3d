#ifndef REBLOCHON_MAP_H
#define REBLOCHON_MAP_H

#include <cstdint>
#include "Array2dT.h"



namespace reb {
	class Map {
	public:
		class Cell {
		public:
			Cell();

			inline std::uint32_t
			height() const {
				return m_height;
			}

			inline std::uint32_t&
			height() {
				return m_height;
			}

			inline unsigned int
			wall_texture_id() const {
				return m_wall_texture_id;
			}

			inline unsigned int&
			wall_texture_id() {
				return m_wall_texture_id;
			}

			inline unsigned int
			floor_texture_id() const {
				return m_floor_texture_id;
			}

			inline unsigned int&
			floor_texture_id() {
				return m_floor_texture_id;
			}

		private:
			std::uint32_t m_height;
			unsigned int m_wall_texture_id;
			unsigned int m_floor_texture_id;
		}; // class Cell

		typedef reb::Array2dT<Cell> cell_array_type;



		Map();

		Map(int w, int h);

		inline const cell_array_type&
		cell_array() const {
			return m_cell_array;
		}

		inline cell_array_type&
		cell_array() {
			return m_cell_array;
		}

	private:
		cell_array_type m_cell_array;
	}; // class Map
} // namespace reb



#endif // REBLOCHON_MAP_H
