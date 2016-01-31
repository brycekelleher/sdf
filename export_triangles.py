import sys
import maya.OpenMaya as om
import maya.OpenMayaMPx as OpenMayaMPx

def EmitTriangles(node):
	mesh = om.MFnMesh(node)

	# get the mesh vertices in object space
	# fixme: should be world space?
	vertices = om.MPointArray()
	mesh.getPoints(vertices, om.MSpace.kObject)
	
	# get the triangle indicies
	counts = om.MIntArray()
	indicies = om.MIntArray()
	mesh.getTriangles(counts, indicies)

	print '--- index data ----'
	#iterate through the triangles and dump the indicies
	print 'numtriangles %i' % (indicies.length() / 3)
	for i in range(indicies.length() / 3):
		print 'triangle %i %i %i %i' % (i, indicies[i * 3 + 0], indicies[i * 3 + 1], indicies[i * 3 + 2])

	print '--- vertex data ----'
	#iterate through the triangles and dump the indicies
	print 'numtriangles %i' % (indicies.length() / 3)
	for i in range(indicies.length()):
		v = vertices[indicies[i]]
		print '{ %f, %f },' % (v[0], v[1])

def IterateNodes():
	it = om.MItDag()
	while(not it.isDone()):
		obj = it.currentItem()

		if(obj.apiType() == om.MFn.kMesh):
			print 'processsing mesh: ' + om.MFnMesh(obj).name()
			EmitTriangles(obj)

		it.next()

def Main():
	om.MGlobal.displayInfo('iterating nodes')
	IterateNodes()

Main()

