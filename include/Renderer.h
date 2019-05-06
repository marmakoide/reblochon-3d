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
				m_z_start(0),
				m_z_end(0),
				m_u_start(0),
				m_u_end(0),
				m_v_start(0),
				m_v_end(0),
				m_texture_id(0) { }

			Column(float y_start, float y_end,
			       float z_start, float z_end,
			       float u_start, float u_end,
			       float v_start, float v_end,
			       unsigned int texture_id) :
				m_y_start(y_start),
				m_y_end(y_end),
				m_z_start(z_start),
				m_z_end(z_end),
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
			clip(float y_lo, float y_hi) {
				float y_delta = m_y_end - m_y_start;
				float y_lo_diff = y_lo - m_y_start;
				float y_hi_diff = y_hi - m_y_start;

				// Update y_start and y_end
				m_y_start = y_lo;
				m_y_end   = y_hi;

				// Clip Z
				float w_start = 1. / m_z_start;
				float w_end   = 1. / m_z_end;
				float w_delta = (w_end - w_start) / y_delta;
				float w_start_clipped = w_start + w_delta * y_lo_diff;
				float w_end_clipped   = w_start + w_delta * y_hi_diff;
				m_z_start = 1. / w_start_clipped;
				m_z_end   = 1. / w_end_clipped;

				// Clip U texture coordinates, linear in 1/Z
				float uw_start = w_start * m_u_start;
				float uw_delta = (w_end * m_u_end - uw_start) / y_delta;
				m_u_start = (uw_start + uw_delta * y_lo_diff) * m_z_start;
				m_u_end   = (uw_start + uw_delta * y_hi_diff) * m_z_end;

				// Clip V texture coordinates, linear in 1/Z
				float vw_start = w_start * m_v_start;
				float vw_delta = (w_end * m_v_end - vw_start) / y_delta;
				m_v_start = (vw_start + vw_delta * y_lo_diff) * m_z_start;
				m_v_end   = (vw_start + vw_delta * y_hi_diff) * m_z_end;
			}

		private:
			float m_y_start, m_y_end;
			float m_z_start, m_z_end;
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
							Column column(y_start, y_end, dist, prev_dist, u_start, u_end, v_start, v_end, 16);

							column.clip(std::fmax(std::ceil(y_start - .5f), 0.f),
								          std::fmin(std::ceil(y_end - .5f), m_h));
							draw_floor_column(dst, i, column);
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
							Column column(y_start, y_end, prev_dist, prev_dist, u_start, u_end, v_start, v_end, 0);

							column.clip(std::fmax(std::ceil(y_start - .5f), 0.f),
								          std::fmin(std::ceil(y_end - .5f), m_h));
							draw_wall_column(dst, i, column);

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
		// Draws a vertical column (ie. constant Z) with only U texture coordinate being interpolated
		void
		draw_wall_column(SDL_Surface* dst,
		                 int x, const Column& column) {
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

		// Draws an floor column
		void 
		draw_floor_column(SDL_Surface* dst,
		                  int x, const Column& column) {
			float y_delta = column.y_end() - column.y_start();

			float w_start = 1.f / column.z_start();
			float w_delta = ((1.f / column.z_end()) - (1.f / column.z_start())) / y_delta;

			float uw_start = column.u_start() / column.z_start();
			float uw_delta = (column.u_end() / column.z_end() - column.u_start() / column.z_start()) / y_delta;

			float vw_start = column.v_start() / column.z_start();
			float vw_delta = (column.v_end() / column.z_end() - column.v_start() / column.z_start()) / y_delta;

			uint8_t const* src_pixel = (uint8_t const*)m_texture_atlas->pixels;
			src_pixel += 16 * (column.texture_id() % 16) + 16 * m_texture_atlas->pitch * (column.texture_id() / 16);

			uint8_t* dst_pixel = (uint8_t*)dst->pixels;
			dst_pixel += ((int)std::floor(column.y_start())) * dst->pitch + x;


			int i_end = y_delta;
			for(int i = 0; i < i_end; ++i, dst_pixel += dst->pitch) {
				float w = w_start + (i + .5f) * w_delta;
				float u = (uw_start + (i + .5f) * uw_delta) / w;
				float v = (vw_start + (i + .5f) * vw_delta) / w;

				int u_offset = int(std::floor(16 * u)) & 15;
				int v_offset = int(std::floor(16 * v)) & 15;

				*dst_pixel = src_pixel[v_offset * m_texture_atlas->pitch + u_offset];
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
