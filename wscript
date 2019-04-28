#! /usr/bin/env python

# the following two variables are used by the target "waf dist"
VERSION = '1.0.0'
APPNAME = 'reblochon-3d'

# these variables are mandatory ('/' are converted automatically)
top, out = '.', 'build'



def options(context):
	context.load('compiler_cxx')



def configure(context):
	context.load('compiler_cxx')
	context.env.CXXFLAGS = ['-std=c++14', '-Wall', '-Wextra', '-O3', '-g', '-frounding-math']
	context.check_cfg(package = 'eigen3', uselib_store = 'eigen', args = ['eigen3 >= 3.3', '--cflags'])
	context.check_cfg(package = 'sdl2', uselib_store = 'sdl2', args = ['--cflags', '--libs'])
	context.check_cfg(package='libpng', atleast_version='1.2.0', uselib_store='png', args='--cflags --libs', mandatory=1)



def build(context):
	context.program(
		target = 'reblochon-editor',
		includes = 'include',
		source = context.path.ant_glob('src/*.cpp'),
		lib    = ['m'],
		use    = ['sdl2', 'png', 'eigen']
	)
