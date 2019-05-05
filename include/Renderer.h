#ifndef REBLOCHON_RENDERER_H
#define REBLOCHON_RENDERER_H

#include <Eigen/Geometry>
#include "Map.h"
#include "RayTraversal.h"
#include <list>
#include <iostream>



namespace reb {
	class Renderer {
	public:
		Renderer(int w,
		         int h,
							SDL_Surface* texture_atlas,
		         float focal_length) :
			m_w(w),
			m_h(h),
			m_focal_length(focal_length),
			m_texture_atlas(texture_atlas),
			m_ray_direction_list(m_w, 3) { 
			setup();
		}



		class Column {
		public:
			Column() :
				m_y_start(0),
				m_y_end(0),
				m_u_start(0),
				m_u_end(0),
				m_v_start(0),
				m_v_end(0),
				m_texture_id(0) { }

			Column(float y_start, float y_end,
			       float u_start, float u_end,
			       float v_start, float v_end,
			       unsigned int texture_id) :
				m_y_start(y_start),
				m_y_end(y_end),
				m_u_start(u_start),
				m_u_end(u_end),
				m_v_start(v_start),
				m_v_end(v_end),
				m_texture_id(texture_id) { }

			inline float
			y_start() const {
				return m_y_start;
			}

			inline float
			y_end() const {
				return m_y_end;
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
			clip(float y_lo, float y_hi) {
				float y_delta = m_y_end - m_y_start;

				// Clip U texture coordinates
				float u_delta = (m_u_end - m_u_start) / y_delta;
				float u_start_clipped = m_u_start + u_delta * (y_lo - m_y_start);
				float u_end_clipped   = m_u_start + u_delta * (y_hi - m_y_start);
				m_u_start = u_start_clipped;
				m_u_end = u_end_clipped;

				// Clip V texture coordinates
				float v_delta = (m_v_end - m_v_start) / y_delta;
				float v_start_clipped = m_v_start + v_delta * (y_lo - m_y_start);
				float v_end_clipped   = m_v_start + v_delta * (y_hi - m_y_start);
				m_v_start = v_start_clipped;
				m_v_end = v_end_clipped;

				// Update y_start and y_end
				m_y_start = y_lo;
				m_y_end = y_hi;
			}

		private:
			float m_y_start, m_y_end;
			float m_u_start, m_u_end;
			float m_v_start, m_v_end;
			unsigned int m_texture_id;
		}; // class Column



		void
		render(SDL_Surface* dst,
		       const Map& map,
					 float angle,
		       const Eigen::Vector3f& pos) {
			// Get 2d position offset
			Eigen::Vector2f pos_offset = pos.head(2);

			// Compute rotation matrix
			Eigen::Matrix2f rot_offset;
			rot_offset = Eigen::Rotation2Df(angle);

			Grid2d grid(Eigen::Vector2i(map.w(), map.h()), 1.);

			// Clear the surface
			SDL_FillRect(dst, NULL, 149);

			// Setup constants
			float wall_height = 2.5f;
			float view_height = pos.z();

			// For each column
			for(int i = 0; i < m_w; ++i) {
				// Compute ray direction
				float ray_norm = m_ray_direction_list(i, 2);
				Eigen::Vector2f ray_dir = m_ray_direction_list.row(i).head(2);
				ray_dir = rot_offset * ray_dir;
				
				// Cast a ray
				bool column_completed = false;

				RayTraversal traversal(grid, pos_offset, ray_dir);
				float prev_dist = traversal.distance_init();
				float prev_axis = traversal.axis();

				for( ; traversal.has_next() and !column_completed; traversal.next()) {
					float dist = traversal.distance(); 
					int axis = traversal.axis();

					if (map(traversal.i(), traversal.j()) == 0) {
						// Generate a floor column
						float y_start = 0;
						float y_end = 0;

						float u_start, v_start;
						u_start = pos_offset[1-axis] + dist * ray_dir[1-axis];
						u_start = u_start - std::floor(u_start);
						v_start = ray_dir[axis] > 0 ? 1 : 0;
						if (axis == 1)
							std::swap(u_start, v_start);			
					
						float u_end, v_end;
						u_end = pos_offset[1-prev_axis] + prev_dist * ray_dir[1-prev_axis];
						u_end = u_end - std::floor(u_end);
						v_end = ray_dir[prev_axis] > 0 ? 0 : 1;
						if (prev_axis == 1)
							std::swap(u_end, v_end);

						// Projection to screen space
						float k = -ray_norm / prev_dist; 
						y_end = m_h * (k * (y_end - view_height) + .5f);

						k = -ray_norm / dist; 
						y_start   = m_h * (k * (y_start - view_height) + .5f); 

						// Render the floor slice
						if ((y_end > 0) and (y_start < m_h)) {
							Column column(y_start, y_end, u_start, u_end, v_start, v_end, 16);

							column.clip(std::fmax(std::ceil(y_start - .5f), 0.f),
								          std::fmin(std::ceil(y_end - .5f), m_h));
							draw_column(dst, i, column);
						}
					}
					// Generate a wall column
					else {
						// Compute the wall slice
						float y_start = wall_height;
						float y_end   = 0;

						float u_start = pos_offset[1 - prev_axis] + prev_dist * ray_dir[1 - prev_axis];
						u_start -= std::floor(u_start);
						float u_end = u_start;

						float v_start = 0;
						float v_end   = wall_height;

						// Projection to screen space
						float k = -ray_norm / prev_dist; 
						y_start = m_h * (k * (y_start - view_height) + .5f); 
						y_end   = m_h * (k * (y_end   - view_height) + .5f); 

						// Render the wall slice
						if ((y_end > 0) and (y_start < m_h)) {
							Column column(y_start, y_end, u_start, u_end, v_start, v_end, 0);

							column.clip(std::fmax(std::ceil(y_start - .5f), 0.f),
								          std::fmin(std::ceil(y_end - .5f), m_h));
							draw_column(dst, i, column);

							// Done drawing
							column_completed = true;
						}
					}

					prev_axis = axis;
					prev_dist = dist;
				}
			}
		}

		static float
		focal_length_from_angle(float angle) {
			return .5f / std::tan(.5f * angle);
		}

	private:
		void 
		draw_column(SDL_Surface* dst,
		            int x, const Column& column) {
			// Constant z column
			if (column.u_end() == column.u_start()) {
				float y_delta = column.y_end() - column.y_start();
				float v_delta = (column.v_end() - column.v_start()) / y_delta;

				int u_offset = int(16 * column.u_start()) % 16;

				uint8_t const* src_pixel = (uint8_t const*)m_texture_atlas->pixels;
				src_pixel += 16 * (column.texture_id() % 16) + 16 * m_texture_atlas->pitch * (column.texture_id() / 16);
				src_pixel += u_offset;

				uint8_t* dst_pixel = (uint8_t*)dst->pixels;
				dst_pixel += ((int)std::floor(column.y_start())) * dst->pitch + x;

				int i_end = y_delta;
				for(int i = 0; i < i_end; ++i, dst_pixel += dst->pitch) {
					int v_offset = std::floor(16 * (v_delta * (i + .5f) + column.v_start()));
					v_offset &= 15;
					*dst_pixel = src_pixel[v_offset * m_texture_atlas->pitch];
				}
			}
			else {
				float y_delta = column.y_end() - column.y_start();
				float u_delta = (column.u_end() - column.u_start()) / y_delta;
				float v_delta = (column.v_end() - column.v_start()) / y_delta;

				uint8_t const* src_pixel = (uint8_t const*)m_texture_atlas->pixels;
				src_pixel += 16 * (column.texture_id() % 16) + 16 * m_texture_atlas->pitch * (column.texture_id() / 16);

				uint8_t* dst_pixel = (uint8_t*)dst->pixels;
				dst_pixel += ((int)std::floor(column.y_start())) * dst->pitch + x;

				int i_end = y_delta;
				for(int i = 0; i < i_end; ++i, dst_pixel += dst->pitch) {
					int u_offset = std::floor(16 * (u_delta * (i + .5f) + column.u_start()));
					int v_offset = std::floor(16 * (v_delta * (i + .5f) + column.v_start()));
					
					u_offset &= 15;
					v_offset &= 15;

					*dst_pixel = src_pixel[v_offset * m_texture_atlas->pitch + u_offset];
				}
			}
		}

		void
		setup() {
			// Compute ray directions
			for(int i = 0; i < m_w; ++i) {
				Eigen::Vector2f U((i + .5f) / m_w - .5f, m_focal_length); 
				float U_norm = U.norm();
				U /= U_norm;
				m_ray_direction_list.row(i) << U.x(), U.y(), m_focal_length * U_norm;
			}
		}

	
		int m_w, m_h;
		float m_focal_length;
		SDL_Surface* m_texture_atlas;
		Eigen::Matrix<float, Eigen::Dynamic, 3> m_ray_direction_list;
	}; //  class Renderer
} // namespace reb



#endif // REBLOCHON_RENDERER_H
