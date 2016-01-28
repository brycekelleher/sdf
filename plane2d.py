from math import sqrt

#def plane2d(x0, y0, x1, y1):
#	x = x1 - x0
#	y = y1 - y0
#	l = sqrt((x * x) + (y * y))
#
#	nx = y / l
#	ny = x / l
#	d = -x0 * nx - y0 * ny
#
#	return nx, ny, d

def plane2d(a, b):
	x0, y0 = a
	x1, y1 = b

	x = x1 - x0
	y = y1 - y0
	l = sqrt((x * x) + (y * y))

	nx =  y / l
	ny = -x / l
	d = -x0 * nx - y0 * ny

	return nx, ny, d

#print plane2d((5.0, 0.0), (5.0, 5.0))
#print plane2d((5.0, 5.0), (5.0, 0.0))

def distance(abc, xy):
	a, b, c = abc
	x, y = xy
	return (a * x) + (b * y) + c
