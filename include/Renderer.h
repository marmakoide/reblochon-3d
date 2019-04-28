#ifndef REBLOCHON_RENDERER_H
#define REBLOCHON_RENDERER_H

#include <Eigen/Geometry>
#include "Map.h"
#include "RayTraversal.h"



namespace reb {
	class Renderer {
	public:
		Renderer(int w,
		         int h,
		         float focal_length) :
			m_w(w),
			m_h(h),
			m_focal_length(focal_length),
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
			SDL_FillRect(dst, NULL, 151);

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

				for( ; traversal.has_next() and !column_completed; traversal.next()) {
					if (map(traversal.i(), traversal.j()) != 0) {
						// Compute the wall slice
						float y_end = wall_height;
						float y_start = 0.;

						y_start -= view_height;
						y_end -= view_height;

						float k = ray_norm / prev_dist; 
						y_start *= k; 
						y_end *= k; 

						y_start = (y_start + .5f) * m_h;
						y_end = (y_end + .5f) * m_h;

						// Render the wall slice
						if ((y_end > 0) and (y_start < m_h)) {
							// Clip the vertical line
							if (y_start < 0)
								y_start = 0;

							if (y_end > m_h)
								y_end = m_h;

							// Draw the vertical line
							draw_vertical_line(dst, i, int(y_start), int(y_end));

							// Done drawing
							column_completed = true;
						}
					}

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
		draw_vertical_line(SDL_Surface* dst,
		                   int x, int y_start, int y_end) {
			uint8_t* pixel = (uint8_t*)dst->pixels;
			pixel += y_start * dst->pitch + x;

			for(int i = y_end - y_start; i != 0; --i, pixel += dst->pitch)
				*pixel = 63;
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
		Eigen::Matrix<float, Eigen::Dynamic, 3> m_ray_direction_list;
	}; //  class Renderer
} // namespace reb



#endif // REBLOCHON_RENDERER_H
