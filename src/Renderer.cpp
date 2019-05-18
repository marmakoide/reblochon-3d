#include "SDL.h"
#include "Renderer.h"

using namespace reb;



// --- Renderer::Column -------------------------------------------------------

Renderer::Column::Column() :
	m_y_start(0),
	m_y_end(0),
	m_z_start(0),
	m_z_end(0),
	m_u_start(0),
	m_u_end(0),
	m_v_start(0),
	m_v_end(0),
	m_texture_id(0) { }



Renderer::Column::Column(float y_start, float y_end,
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



void
Renderer::Column::clip(float y_lo, float y_hi) {
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



// --- Renderer::CoverageBuffer -----------------------------------------------

Renderer::CoverageBuffer::CoverageBuffer(int size) :
	m_size(size) {
	m_unoccluded_range_list.push_back(IntegerRange(0, m_size));
}



void
Renderer::CoverageBuffer::add(Column& column) {
	// If the column fragment is not within the viewport, we ignore it
	if ((column.y_end() <= 0) or (column.y_start() >= m_size))
		return;

	// Compute the integer range of the column
	IntegerRange column_range(
		(int)std::ceil(column.y_start() - .5f),
		(int)std::ceil(column.y_end()   - .5f));

	// Split and clip the non-occluded parts of the column fragment
	for(const IntegerRange& range : m_unoccluded_range_list) {
		if ((range.end() > column_range.start()) and (range.start() < column_range.end())) {
			Column clipped_column = column;
			clipped_column.clip(
				std::max(range.start(), column_range.start()),
				std::min(range.end(), column_range.end()));
			m_column_list.push_back(clipped_column);
		}
	}
	
	/*
	  Update the list of non-occluded ranges
	 */

	integer_range_list_type updated_unoccluded_range_list;

	{
		integer_range_list_type::const_iterator it = m_unoccluded_range_list.begin();

		// Keep anything before the column range
		for( ; it != m_unoccluded_range_list.end(); ++it)
			if ((*it).end() < column_range.start())
				updated_unoccluded_range_list.push_back(*it);
			else {
				if (((*it).start() < column_range.end()) and ((*it).start() < column_range.start()))
					updated_unoccluded_range_list.push_back(IntegerRange((*it).start(), column_range.start()));
				break;
			}
	
		// Skip everything within the column range into a single range
		for( ; it != m_unoccluded_range_list.end(); ++it)
			if ((*it).start() >= column_range.end())
				break;	
			else {
				if ((*it).end() > column_range.end())
					updated_unoccluded_range_list.push_back(IntegerRange(column_range.end(), (*it).end()));
			}

		// Keep anything after the column range
		for( ; it != m_unoccluded_range_list.end(); ++it)
			updated_unoccluded_range_list.push_back(*it);

		m_unoccluded_range_list = updated_unoccluded_range_list;
	}
}



void
Renderer::CoverageBuffer::clear() {
	m_column_list.clear();
	m_unoccluded_range_list.clear();
	m_unoccluded_range_list.push_back(IntegerRange(0, m_size));
}




// --- Renderer ---------------------------------------------------------------

Renderer::Renderer(int w,
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



float
Renderer::focal_length_from_angle(float angle) {
	return .5f / std::tan(.5f * angle);
}



void
Renderer::setup() {
	// Compute ray directions
	for(int i = 0; i < m_w; ++i) {
		Eigen::Vector2f U((i + .5f) / m_w - .5f, m_focal_length); 
		float U_norm = U.norm();
		U /= U_norm;
		m_ray_direction_list.row(i) << U.x(), U.y(), m_focal_length * U_norm;
	}
}



void
Renderer::fill_coverage_buffer(CoverageBuffer& coverage_buffer,
                        const Map& map,
                        const Grid2d& grid,
		                    const Eigen::Vector2f& ray_pos,
                        const Eigen::Vector2f& ray_dir,
                        float ray_norm,
                        float view_height) {
	bool column_completed = false;

	// Ray/grid intersection setup
	RayTraversal traversal(grid, ray_pos, ray_dir);
	float prev_dist = traversal.distance_init();
	float prev_axis = traversal.axis();
	
	// If the ray origin is inside the map render the piece of floor under it
	if (grid.is_inside(ray_pos)) {
		const Map::Cell& cell = map.cell_array()(traversal.i(), traversal.j());
		float cell_height = cell.height() / 256.f;

		if (cell_height < view_height) {
			float y_start = cell_height;
			float y_end   = cell_height;
					
			float u_start, v_start;
			u_start = ray_pos[1-prev_axis] + prev_dist * ray_dir[1-prev_axis];
			u_start = u_start - std::floor(u_start);
			v_start = ray_dir[prev_axis] > 0 ? 1 : 0;
			if (prev_axis == 1)
				std::swap(u_start, v_start);

			float dist = 2 * ray_norm * (view_height - cell_height);
			float u_end = ray_pos[1-prev_axis] + dist * ray_dir[1-prev_axis];
			u_end = u_end - std::floor(u_end);
			float v_end = ray_pos[prev_axis] + dist * ray_dir[prev_axis];
			v_end = v_end - std::floor(v_end);
			if (prev_axis == 1)
				std::swap(u_end, v_end);
			
			// Projection to screen space
			float k = -ray_norm / dist; 
			y_end   = m_h * (k * (y_end - view_height) + .5f); 

			k = -ray_norm / prev_dist; 
			y_start = m_h * (k * (y_start - view_height) + .5f);

			// Add the column fragment
			Column column(y_start, y_end, prev_dist, dist, u_start, u_end, v_start, v_end, cell.floor_texture_id() & 0xff);
			coverage_buffer.add(column);	
		}

		if (view_height < 0) {
			float y_start = 0;
			float y_end   = 0;

			float u_end, v_end;
			u_end = ray_pos[1-prev_axis] + prev_dist * ray_dir[1-prev_axis];
			u_end = u_end - std::floor(u_end);
			v_end = ray_dir[prev_axis] > 0 ? 1 : 0;
			if (prev_axis == 1)
				std::swap(u_end, v_end);			
			
			float dist = 2 * ray_norm * -view_height;
			float u_start = ray_pos[1-prev_axis] + dist * ray_dir[1-prev_axis];
			u_start = u_start - std::floor(u_start);
			float v_start = ray_pos[prev_axis] + dist * ray_dir[prev_axis];
			v_start = v_start - std::floor(v_start);
			if (prev_axis == 1)
				std::swap(u_start, v_start);

			// Projection to screen space
			float k = -ray_norm / prev_dist; 
			y_end = m_h * (k * (y_end - view_height) + .5f); 

			k = -ray_norm / dist; 
			y_start = m_h * (k * (y_start - view_height) + .5f);

			// Add the column fragment
			Column column(y_start, y_end, dist, prev_dist, u_start, u_end, v_start, v_end, cell.floor_texture_id() & 0xff);
			coverage_buffer.add(column);
		}
	}

	// For each intersection found with the grid
	for( ; traversal.has_next() and !column_completed; traversal.next()) {
		float dist = traversal.distance(); 
		int axis = traversal.axis();
		const Map::Cell& cell = map.cell_array()(traversal.i(), traversal.j());
		float cell_height = cell.height() / 256.f;

		// Generate a column for the cell top (ie. floor)
		if (cell_height < view_height) {
			float y_start = cell_height;
			float y_end   = cell_height;

			float u_start, v_start;
			u_start = ray_pos[1-axis] + dist * ray_dir[1-axis];
			u_start = u_start - std::floor(u_start);
			v_start = ray_dir[axis] > 0 ? 1 : 0;
			if (axis == 1)
				std::swap(u_start, v_start);			
					
			float u_end, v_end;
			u_end = ray_pos[1-prev_axis] + prev_dist * ray_dir[1-prev_axis];
			u_end = u_end - std::floor(u_end);
			v_end = ray_dir[prev_axis] > 0 ? 0 : 1;
			if (prev_axis == 1)
				std::swap(u_end, v_end);

			// Projection to screen space
			float k = -ray_norm / prev_dist; 
			y_end = m_h * (k * (y_end - view_height) + .5f);

			k = -ray_norm / dist; 
			y_start = m_h * (k * (y_start - view_height) + .5f); 

			// Add the column fragment
			Column column(y_start, y_end, dist, prev_dist, u_start, u_end, v_start, v_end, cell.floor_texture_id() & 0xff);
			coverage_buffer.add(column);
		}

		// Generate a column for the cell bottom (ie. ceiling)
		if (view_height < 0) {
			float y_start = 0;
			float y_end   = 0;

			float u_start, v_start;
			u_start = ray_pos[1-prev_axis] + prev_dist * ray_dir[1-prev_axis];
			u_start = u_start - std::floor(u_start);
			v_start = ray_dir[prev_axis] > 0 ? 0 : 1;
			if (prev_axis == 1)
				std::swap(u_start, v_start);			
					
			float u_end, v_end;
			u_end = ray_pos[1-axis] + dist * ray_dir[1-axis];
			u_end = u_end - std::floor(u_end);
			v_end = ray_dir[axis] > 0 ? 1 : 0;
			if (axis == 1)
				std::swap(u_end, v_end);

			// Projection to screen space
			float k = -ray_norm / prev_dist; 
			y_start = m_h * (k * (y_start - view_height) + .5f); 

			k = -ray_norm / dist; 
			y_end = m_h * (k * (y_end - view_height) + .5f);

			// Add the column fragment
			Column column(y_start, y_end, prev_dist, dist, u_start, u_end, v_start, v_end, cell.floor_texture_id() & 0xff);
			coverage_buffer.add(column);
		}

		// Generate a column for cell side (ie. wall)
		{
			// Compute the wall slice
			float y_start = cell_height;
			float y_end   = 0;

			float u_start = ray_pos[1 - prev_axis] + prev_dist * ray_dir[1 - prev_axis];
			u_start -= std::floor(u_start);
			float u_end = u_start;

			float v_start = 0;
			float v_end   = cell_height;

			// Projection to screen space
			float k = -ray_norm / prev_dist; 
			y_start = m_h * (k * (y_start - view_height) + .5f); 
			y_end   = m_h * (k * (y_end   - view_height) + .5f); 

			// Add the column fragment
			Column column(y_start, y_end, prev_dist, prev_dist, u_start, u_end, v_start, v_end, cell.wall_texture_id() & 0xff);
			coverage_buffer.add(column);
		}

		prev_axis = axis;
		prev_dist = dist;
	}
}



void
Renderer::render(SDL_Surface* dst,
		             const Map& map,
					       float angle,
		             const Eigen::Vector3f& pos) {
	Grid2d grid(Eigen::Vector2i(map.cell_array().w(), map.cell_array().h()), 1.);

	// Get 2d position offset
	Eigen::Vector2f ray_pos = pos.head(2);

	// Compute rotation matrix
	Eigen::Matrix2f rot_offset;
	rot_offset = Eigen::Rotation2Df(angle);

	// Clear the surface
	SDL_FillRect(dst, NULL, 149);

	// For each column
	CoverageBuffer coverage_buffer(m_h);
	for(int i = 0; i < m_w; ++i) {
		// Compute ray direction
		float ray_norm = m_ray_direction_list(i, 2);
		Eigen::Vector2f ray_dir = m_ray_direction_list.row(i).head(2);
		ray_dir = rot_offset * ray_dir;
		
		// Compute all the column fragments to render
		coverage_buffer.clear();
		fill_coverage_buffer(coverage_buffer, map, grid, ray_pos, ray_dir, ray_norm, pos.z());

		// Render the column fragments
		for(const Column& column : coverage_buffer.column_list())
			draw_column(dst, i, column);
	}
}



void
Renderer::draw_column(SDL_Surface* dst, int x, const Column& column) {
	if (column.z_start() == column.z_end())
		draw_wall_column(dst, x, column);
	else
		draw_floor_column(dst, x, column);
}



// Draws a vertical column (ie. constant Z) with only U texture coordinate being interpolated
void
Renderer::draw_wall_column(SDL_Surface* dst,
		                       int x, const Column& column) {
	float y_delta = column.y_end() - column.y_start();

	float v_delta = (column.v_end() - column.v_start()) / y_delta;
	float v_start = column.v_start() + .5f * v_delta;

	int u_offset = int(16 * column.u_start()) % 16;

	uint8_t const* src_pixel = (uint8_t const*)m_texture_atlas->pixels;
	src_pixel += 16 * (column.texture_id() % 16) + 16 * m_texture_atlas->pitch * (column.texture_id() / 16);
	src_pixel += u_offset;

	uint8_t* dst_pixel = (uint8_t*)dst->pixels;
	dst_pixel += ((int)std::floor(column.y_start())) * dst->pitch + x;

	int i_end = y_delta;
	for(int i = 0; i < i_end; ++i, dst_pixel += dst->pitch) {
		float v = i * v_delta + v_start;
		int v_offset = int(std::floor(16 * v)) & 15;

		*dst_pixel = src_pixel[v_offset * m_texture_atlas->pitch];
	}
}



// Draws an floor column
void 
Renderer::draw_floor_column(SDL_Surface* dst,
		                        int x, const Column& column) {
	float y_delta = column.y_end() - column.y_start();
	float inv_y_delta = 1.f / y_delta;

	float w_start = 1.f / column.z_start();
	float w_delta = ((1.f / column.z_end()) - (1.f / column.z_start())) * inv_y_delta;
	w_start += .5f * w_delta;

	float uw_start = column.u_start() / column.z_start();
	float uw_delta = (column.u_end() / column.z_end() - uw_start) * inv_y_delta;
	uw_start += .5f * uw_delta;

	float vw_start = column.v_start() / column.z_start();
	float vw_delta = (column.v_end() / column.z_end() - vw_start) * inv_y_delta;
	vw_start += .5f * vw_delta;

	uint8_t const* src_pixel = (uint8_t const*)m_texture_atlas->pixels;
	src_pixel += 16 * (column.texture_id() % 16) + 16 * m_texture_atlas->pitch * (column.texture_id() / 16);

	uint8_t* dst_pixel = (uint8_t*)dst->pixels;
	dst_pixel += ((int)std::floor(column.y_start())) * dst->pitch + x;

	int i_end = y_delta;
	for(int i = 0; i < i_end; ++i, dst_pixel += dst->pitch) {
		float z = 1. / (i * w_delta + w_start);
		float u = z * (i * uw_delta + uw_start);
		float v = z * (i * vw_delta + vw_start);

		int u_offset = int(std::floor(16 * u)) & 15;
		int v_offset = int(std::floor(16 * v)) & 15;

		*dst_pixel = src_pixel[v_offset * m_texture_atlas->pitch + u_offset];
	}
}
