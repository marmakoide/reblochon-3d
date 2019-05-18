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
		             const Eigen::Vector2f& direction);

		inline float
		distance_init() const {
			return m_t_init;
		}

		int
		axis_init() const {
			return m_axis_init;
		}

		int
		axis() const {
			Eigen::Index axis;
			m_t.minCoeff(&axis);
			return axis;
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
		int m_axis_init;
		Eigen::Vector2f m_t;
		Eigen::Vector2f m_t_delta;
		Eigen::Vector2i m_size;
		Eigen::Vector2i m_index_delta;
		Eigen::Vector2i m_index;
	}; // class RayTraversal
} // namespace reb



#endif // REBLOCHON_RAY_TRAVERSAL_H
