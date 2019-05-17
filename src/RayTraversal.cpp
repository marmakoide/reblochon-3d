#include "RayTraversal.h"

using namespace reb;



RayTraversal::RayTraversal(const Grid2d& grid,
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
			m_index[i] = std::floor(x);
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
