#ifndef REBLOCHON_RENDERER_H
#define REBLOCHON_RENDERER_H

#include <Eigen/Geometry>
#include "Map.h"
#include "RayTraversal.h"
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
				float prev_axis = 1 - traversal.axis();

				for( ; traversal.has_next() and !column_completed; traversal.next()) {
					if (map(traversal.i(), traversal.j()) != 0) {
						// Compute the wall slice
						float y_start = wall_height;
						float y_end = 0;

						float v_start = 0;
						float v_end = 2.5f;

						y_start -= view_height;
						y_end -= view_height;

						float k = -ray_norm / prev_dist; 
						y_start *= k; 
						y_end *= k; 

						y_start = (y_start + .5f) * m_h;
						y_end = (y_end + .5f) * m_h;

						// Render the wall slice
						if ((y_end > 0) and (y_start < m_h)) {
							// Clip the vertical line position
							float y_start_clipped = std::fmax(std::ceil(y_start - .5f), 0.f);
							float y_end_clipped   = std::fmin(std::ceil(y_end - .5f), m_h);

							// Clip the V texture coordinates
							float v_start_clipped = v_start + (y_start_clipped - y_start) * ((v_end - v_start) / (y_end - y_start));
							float v_end_clipped = v_start + (y_end_clipped - y_start) * ((v_end - v_start) / (y_end - y_start));

							// U texture coordinates
							float u = pos_offset[prev_axis] + prev_dist * ray_dir[prev_axis];
							u = u - std::floor(u);

							// Draw the vertical line
							//draw_vertical_line_single_color(dst, i, int(y_start_clipped), int(y_end_clipped), 63);
							if (y_start_clipped < y_end_clipped)
								draw_vertical_line_textured(dst, i, int(y_start_clipped), int(y_end_clipped), u, v_start_clipped, v_end_clipped, 4);

							// Done drawing
							column_completed = true;
						}
					}

					prev_axis = 1 - traversal.axis();
					prev_dist = traversal.distance(); 
				}
			}
		}

		static float
		focal_length_from_angle(float angle) {
			return .5f / std::tan(.5f * angle);
		}

	private:
		void 
		draw_vertical_line_textured(SDL_Surface* dst,
		                            int x, int y_start, int y_end,
		                            float u, float v_start, float v_end,
		                            int texture_id) {
			float k = (v_end - v_start) / (y_end - y_start);
			int u_offset = int(16 * u) % 16;

			uint8_t const* src_pixel = (uint8_t const*)m_texture_atlas->pixels;
			src_pixel += 16 * (texture_id % 16) + 16 * m_texture_atlas->pitch * (texture_id / 16);
			src_pixel += u_offset;

			uint8_t* dst_pixel = (uint8_t*)dst->pixels;
			dst_pixel += ((int)std::floor(y_start)) * dst->pitch + x;

			int i_end = y_end  - y_start;
			for(int i = 0; i < i_end; ++i, dst_pixel += dst->pitch) {
				int v_offset = std::floor(16 * (k * (i + .5f) + v_start));
				v_offset &= 15;
				*dst_pixel = src_pixel[v_offset * m_texture_atlas->pitch];
			}
		}

		void 
		draw_vertical_line_single_color(SDL_Surface* dst,
		                                int x,
		                                int y_start, int y_end,
		                                int color_id) {
			uint8_t* pixel = (uint8_t*)dst->pixels;
			pixel += y_start * dst->pitch + x;

			for(int i = y_end - y_start; i != 0; --i, pixel += dst->pitch)
				*pixel = color_id;
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
