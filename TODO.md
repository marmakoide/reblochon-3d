# reblochon-3d TODO list

## Bugs

* Fullscreen mode doesn't stretch and center the view
* Going up or down triggers rendering bugs : rendering of a column should not
stop just when a wall is hit when casting a ray, it should stop either when 
the end of the map is reached or the column is full. 
* When leaving the map boundaries, the outer walls are not rendered

## Features

* Being able to select a wall or a floor tile, and modify its attributes like
height or texture
* Being able to save the changes made to the map
* Rendering billboards sprites
* Rendering voxel sprites
* Multiple cubes per tiles can be defined
* No hard-coded 16x16 textures
* No harded coded 256 textures tops
* Depth shading

