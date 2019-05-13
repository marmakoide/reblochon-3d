#ifndef REBLOCHON_RENDERER_H
#define REBLOCHON_RENDERER_H

#include <Eigen/Geometry>
#include "Map.h"
#include "RayTraversal.h"
#include <list>



namespace reb {
	// Represents the integer range [start, end[
	class IntegerRange {
	public:
		inline IntegerRange() :
			m_start(0),
			m_end(0) { }

		inline IntegerRange(int start, int end) :
			m_start(start),
			m_end(end) { }

		inline IntegerRange(const IntegerRange& other) :
			m_start(other.m_start),
			m_end(other.m_end) { }

		inline IntegerRange& operator = (const IntegerRange& other) {
			m_start = other.m_start;
			m_end = other.m_end;
			return *this;
		}

		inline int start() const { return m_start; }

		inline int end() const { return m_end; }

		inline int length() const { return m_start - m_end; }

	private:
		int m_start, m_end;
	}; // class IntegerRange



	class Renderer {
	public:
		// Represents a piece of vertical column to be rendered
		class Column {
		public:
			Column();

			Column(float y_start, float y_end,
			       float z_start, float z_end,
			       float u_start, float u_end,
			       float v_start, float v_end,
			       unsigned int texture_id);

			inline float
			y_start() const {
				return m_y_start;
			}

			inline float
			y_end() const {
				return m_y_end;
			}

			inline float
			z_start() const {
				return m_z_start;
			}

			inline float
			z_end() const {
				return m_z_end;
			}

			inline float
			u_start() const {
				return m_u_start;
			}

			inline float
			u_end() const {
				return m_u_end;
			}

			inline float
			v_start() const {
				return m_v_start;
			}

			inline float
			v_end() const {
				return m_v_end;
			}

			inline unsigned int
			texture_id() const {
				return m_texture_id;
			}

			void
			clip(float y_lo, float y_hi);

		private:
			float m_y_start, m_y_end;
			float m_z_start, m_z_end;
			float m_u_start, m_u_end;
			float m_v_start, m_v_end;
			unsigned int m_texture_id;
		}; // class Column



		// Represents a full column of the screen
		class CoverageBuffer {
		public:
			typedef std::list<Column> column_list_type;
			typedef std::list<IntegerRange> integer_range_list_type;



			CoverageBuffer(int size);

			inline const column_list_type&
			column_list() const {
				return m_column_list;
			}

			void clear();

			void add(Column& column);

		private:
			int m_size;
			column_list_type m_column_list;
			integer_range_list_type m_unoccluded_range_list;
		}; // class CoverageBuffer



		Renderer(int w, int h,
						 SDL_Surface* texture_atlas,
		         float focal_length);

		void
		render(SDL_Surface* dst,
		       const Map& map,
					 float angle,
		       const Eigen::Vector3f& pos);

		static float focal_length_from_angle(float angle);

	private:
		void fill_coverage_buffer(CoverageBuffer& coverage_buffer,
		                          const Map& map,
                              const Grid2d& grid,
		                          const Eigen::Vector2f& ray_pos,
                              const Eigen::Vector2f& ray_dir,
		                          float ray_norm,
                              float view_height);

		void draw_column(SDL_Surface* dst, int x, const Column& column);

		void draw_wall_column(SDL_Surface* dst, int x, const Column& column);

		void draw_floor_column(SDL_Surface* dst, int x, const Column& column);

		void setup();

	
		int m_w, m_h;
		float m_focal_length;
		SDL_Surface* m_texture_atlas;
		Eigen::Matrix<float, Eigen::Dynamic, 3> m_ray_direction_list;
	}; //  class Renderer
} // namespace reb



#endif // REBLOCHON_RENDERER_H
