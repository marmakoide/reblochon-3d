#ifndef REBLOCHON_RAY_TRAVERSAL_H
#define REBLOCHON_RAY_TRAVERSAL_H

#include <Eigen/Dense>



namespace reb {
	class Grid2d {
	public:
		Grid2d() :
			m_voxel_size(1),
			m_size(2, 2) {
			m_extent = (.5f * m_voxel_size) * m_size.cast<float>();
		}

		Grid2d(const Eigen::Vector2i& size,
		       float voxel_size) :
		m_voxel_size(voxel_size),
		m_size(size) {
			m_extent = (.5f * m_voxel_size) * m_size.cast<float>();	
		}

		inline float
		voxel_size() const {
			return m_voxel_size;
		}

		inline const Eigen::Vector2i&
		size() const {
			return m_size;
		}

		inline const Eigen::Vector2f&
		extent() const {
			return m_extent;
		}
		
		bool
		is_inside(const Eigen::Vector2f& X) const {
			for(int i = 0; i < 2; ++i)
				if (std::fabs(X(i)) > m_extent[i])
					return false;

			return true;
		}

	private:
		float m_voxel_size;
		Eigen::Vector2i m_size;
		Eigen::Vector2f m_extent;
	}; // class Grid2d



	class RayTraversal {
	public:
		RayTraversal(const Grid2d& grid,
		             const Eigen::Vector2f& origin,
		             const Eigen::Vector2f& direction) {
			// Size of the grid
			m_size = grid.size();

			// Sign of direction
			std::size_t sign[2];
			for(int i = 0; i < 2; ++i)
				sign[i] = direction[i] < 0;

			// Ray parameter delta
			m_t_delta = grid.voxel_size() * direction.cwiseAbs().cwiseInverse();

			// Index delta
			for(int i = 0; i < 2; ++i)
				if (std::fabs(direction[i]) > std::numeric_limits<float>::epsilon())
					m_index_delta[i] = direction[i] > 0 ? 1 : -1;
				else
					m_index_delta[i] = 0;

			// Do we start inside or outside the grid ?
			Eigen::Vector2f grid_bounds[2];
			grid_bounds[1] = grid.extent();
			grid_bounds[0] = -grid_bounds[1];

			Eigen::Vector2f bounds[2];
			if (grid.is_inside(origin)) {
				m_t_init = 0;

				for(int i = 0; i < 2; ++i) {
					float x = std::floor((origin[i] - grid_bounds[0][i]) / grid.voxel_size());
					m_index[i] = int(x);
					bounds[0][i] = x + grid_bounds[0][i];
				}
				bounds[1] = bounds[0].array() + 1;
			}
			else {
				// Do we intersects the grid at all ?
				Eigen::Vector2f inv_direction = direction.cwiseInverse();
				Eigen::Vector2f t_lo =  (grid_bounds[0] - origin).cwiseProduct(inv_direction);
				Eigen::Vector2f t_hi =  (grid_bounds[1] - origin).cwiseProduct(inv_direction);

				float t_min = std::fmin(t_lo.x(), t_hi.x());
				float t_max = std::fmax(t_lo.x(), t_hi.x());

				t_min = std::fmax(t_min, std::fmin(t_lo.y(), t_hi.y()));
				t_max = std::fmin(t_max, std::fmax(t_lo.y(), t_hi.y()));

				if ((t_min > t_max) || (t_max < 0)) {
					m_t_init = 0;
					m_index[0] = m_index[1] = -1; 
					return;
				}
			
				m_t_init = t_min;

				// Jump to the entering intersection
				Eigen::Vector2f start = origin + t_min * direction;
				for(int i = 0; i < 2; ++i)
					m_index[i] = int(std::floor((start(i) - grid_bounds[0][i]) / grid.voxel_size()));

				Eigen::Vector2f local_coord = start.cwiseAbs().cwiseQuotient(grid.extent());
				for(int i = 0; i < 2; ++i)
					if ((1. - local_coord[i]) <= std::numeric_limits<float>::epsilon())
						m_index[i] = start[i] < 0 ? 0 : m_size[i] - 1;

				bounds[0] = m_index.cast<float>() + grid_bounds[0] / grid.voxel_size();
				bounds[1] = bounds[0].array() + 1;
			}

			// Ray parameter
			for(int i = 0; i < 2; ++i)
				m_t[i] = (grid.voxel_size() * bounds[1 - sign[i]][i] - origin(i)) / direction(i);

			m_t_init = m_t.minCoeff();
		}

		inline float
		distance_init() const {
			return m_t_init;
		}

		float
		distance() const {
			Eigen::Index axis;
			m_t.minCoeff(&axis);

			return m_t[axis];
		}

		void
		next() {
			Eigen::Index axis;
			m_t.minCoeff(&axis);

			m_t[axis] += m_t_delta[axis];
			m_index[axis] += m_index_delta[axis];	
		}

		inline bool 
		has_next() const {
			for(int i = 0; i < 2; ++i)
				if ((m_index[i] < 0) || (m_index[i] >= m_size[i]))
					return false;

			return true;
		}

		inline int
		i() const {
			return m_index.x();
		}

		inline int
		j() const {
			return m_index.y();
		}

	private:
		float m_t_init;
		Eigen::Vector2f m_t;
		Eigen::Vector2f m_t_delta;
		Eigen::Vector2i m_size;
		Eigen::Vector2i m_index_delta;
		Eigen::Vector2i m_index;
	}; // class RayTraversal
} // namespace reb



#endif // REBLOCHON_RAY_TRAVERSAL_H
