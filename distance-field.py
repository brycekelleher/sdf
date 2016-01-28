from itertools import product
from plane2d import *

imagew = 256
imageh = 256

rows = range(imagew)
cols = range(imageh)
pixels = product(rows, cols)

def clamp(x): return max(-1.0, min(x, 1.0))
def dot(a, b): return (a[0] * b[0]) + (a[1] * b[1])
def pskew(v): return ( v[1], -v[0])
def nskew(v): return (-v[1],  v[0])
def edge(a, b): return (b[0] - a[0], b[1] - a[1])
def edgelength(a, b):
	x = b[0] - a[0]
	y = b[1] - a[1]
	return sqrt((x * x) + (y * y))

v0 = (0.2, 0.4)
v1 = (0.8, 0.2)
v2 = (0.5, 0.8)
abc0 = plane2d(v0, v1)
abc1 = plane2d(v1, v2)
abc2 = plane2d(v2, v0)

v00 = nskew(abc2[:2])
v01 = pskew(abc0[:2])
v10 = nskew(abc0[:2])
v11 = pskew(abc1[:2])
v20 = nskew(abc1[:2])
v21 = pskew(abc2[:2])

# distance field
def df(xy):
	#transform to 0..1 space
	xx, yy = xy
	xx = xx / float(imagew)
	yy = yy / float(imageh)
	xy = (xx, yy)

	f0 = distance(abc0, xy)
	f1 = distance(abc1, xy)
	f2 = distance(abc2, xy)
	return max(f0, f1, f2)

def df2(xy):
	#transform to 0..1 space
	xx, yy = xy
	xx = xx / float(imagew)
	yy = yy / float(imageh)
	xy = (xx, yy)

	v0xy = edge(v0, xy)
	v1xy = edge(v1, xy)
	v2xy = edge(v2, xy)

	f0 = distance(abc0, xy)
	f1 = distance(abc1, xy)
	f2 = distance(abc2, xy)

	#vertex 0
	if dot(v00, v0xy) > 0.0 and dot(v01, v0xy) > 0.0:
		return edgelength(v0, xy)
	if dot(v10, v1xy) > 0.0 and dot(v11, v1xy) > 0.0:
		return edgelength(v1, xy)
	if dot(v20, v2xy) > 0.0 and dot(v21, v2xy) > 0.0:
		return edgelength(v2, xy)

	return max(f0, f1, f2)

def pixelfunc(xy):
	f = df(xy)

	# bias, scale and clamp
	#f = 0.5 + 0.5 * f
	f = clamp(f)
	
	r = max(0.0,  f) * 256
	g = 0
	b = max(0.0, -f) * 256

	return (r, g, b)

print "P3"
print "256 256"
print "255"
for y  in rows:
	for x in cols:
		xy = x, y
		r, g, b = pixelfunc((xy))
		print int(r), int(g), int(b),
	print ""
