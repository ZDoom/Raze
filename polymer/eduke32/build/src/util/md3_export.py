#!BPY

"""
Name: 'Quake3 (.md3)...'
Blender: 242
Group: 'Export'
Tooltip: 'Export to Quake3 file format. (.md3)'
"""
__author__ = "PhaethonH, Bob Holcomb, Damien McGinnes, Robert (Tr3B) Beckebans"
__url__ = ("http://xreal.sourceforge.net")
__version__ = "0.7 2006-11-12"

__bpydoc__ = """\
This script exports a Quake3 file (MD3).

Supported:<br>
    Surfaces, Materials and Tags.

Missing:<br>
    None.

Known issues:<br>
    None.

Notes:<br>
    TODO
"""

import sys, os, os.path, struct, string, math

import Blender
from Blender import *
from Blender.Draw import *
from Blender.BGL import *
from Blender.Window import *

import types

import textwrap

import logging
reload(logging)

import sys, struct, string, math
from types import *

import os
from os import path

GAMEDIR = 'D:/Games/XreaL_testing/base/'
#GAMEDIR = '/opt/XreaL/base/'
MAX_QPATH = 64

import sys, struct, string, math
from types import *

import os
from os import path

import q_shared
from q_shared import *

MD3_IDENT = "IDP3"
MD3_VERSION = 15
MD3_MAX_TAGS = 16
MD3_MAX_SURFACES = 32
MD3_MAX_FRAMES = 1024
MD3_MAX_SHADERS = 256
MD3_MAX_VERTICES = 4096
MD3_MAX_TRIANGLES = 8192
MD3_XYZ_SCALE = (1.0 / 64.0)
MD3_BLENDER_SCALE = (1.0 / 1.0)


class md3Vert:
	xyz = []
	normal = 0
	binaryFormat = "<3hh"

	def __init__(self):
		self.xyz = [0, 0, 0]
		self.normal = 0

	def GetSize(self):
		return struct.calcsize(self.binaryFormat)

	# copied from PhaethonH <phaethon@linux.ucla.edu> md3.py
	def Decode(self, latlng):
		lat = (latlng >> 8) & 0xFF;
		lng = (latlng) & 0xFF;
		lat *= math.pi / 128;
		lng *= math.pi / 128;
		x = math.cos(lat) * math.sin(lng)
		y = math.sin(lat) * math.sin(lng)
		z =                 math.cos(lng)
		retval = [ x, y, z ]
		return retval

	# copied from PhaethonH <phaethon@linux.ucla.edu> md3.py
	def Encode(self, normal):
		x, y, z = normal

		# normalise
		l = math.sqrt((x*x) + (y*y) + (z*z))
		if l == 0:
			return 0
		x = x/l
		y = y/l
		z = z/l

		if (x == 0.0) & (y == 0.0) :
			if z > 0.0:
				return 0
			else:
				return (128 << 8)

		# Encode a normal vector into a 16-bit latitude-longitude value
		#lng = math.acos(z)
		#lat = math.acos(x / math.sin(lng))
		#retval = ((lat & 0xFF) << 8) | (lng & 0xFF)
		lng = math.acos(z) * 255 / (2 * math.pi)
		lat = math.atan2(y, x) * 255 / (2 * math.pi)
		retval = ((int(lat) & 0xFF) << 8) | (int(lng) & 0xFF)
		return retval

	def Load(self, file):
		tmpData = file.read(struct.calcsize(self.binaryFormat))
		data = struct.unpack(self.binaryFormat, tmpData)
		self.xyz[0] = data[0] * MD3_XYZ_SCALE
		self.xyz[1] = data[1] * MD3_XYZ_SCALE
		self.xyz[2] = data[2] * MD3_XYZ_SCALE
		self.normal = data[3]
		return self

	def Save(self, file):
		tmpData = [0] * 4
		tmpData[0] = self.xyz[0] / MD3_XYZ_SCALE
		tmpData[1] = self.xyz[1] / MD3_XYZ_SCALE
		tmpData[2] = self.xyz[2] / MD3_XYZ_SCALE
		tmpData[3] = self.normal
		data = struct.pack(self.binaryFormat, tmpData[0], tmpData[1], tmpData[2], tmpData[3])
		file.write(data)
		#print "Wrote MD3 Vertex: ", data

	def Dump(self):
		log.info("MD3 Vertex")
		log.info("X: %s", self.xyz[0])
		log.info("Y: %s", self.xyz[1])
		log.info("Z: %s", self.xyz[2])
		log.info("Normal: %s", self.normal)
		log.info("")

class md3TexCoord:
	u = 0.0
	v = 0.0

	binaryFormat = "<2f"

	def __init__(self):
		self.u = 0.0
		self.v = 0.0

	def GetSize(self):
		return struct.calcsize(self.binaryFormat)

	def Load(self, file):
		tmpData = file.read(struct.calcsize(self.binaryFormat))
		data = struct.unpack(self.binaryFormat, tmpData)
		# for some reason quake3 texture maps are upside down, flip that
		self.u = data[0]
		self.v = 1.0 - data[1]
		return self

	def Save(self, file):
		tmpData = [0] * 2
		tmpData[0] = self.u
		tmpData[1] = 1.0 - self.v
		data = struct.pack(self.binaryFormat, tmpData[0], tmpData[1])
		file.write(data)
		#print "wrote MD3 texture coordinate structure: ", data

	def Dump(self):
		log.info("MD3 Texture Coordinates")
		log.info("U: %s", self.u)
		log.info("V: %s", self.v)
		log.info("")


class md3Triangle:
	indexes = []

	binaryFormat = "<3i"

	def __init__(self):
		self.indexes = [ 0, 0, 0 ]

	def GetSize(self):
		return struct.calcsize(self.binaryFormat)

	def Load(self, file):
		tmpData = file.read(struct.calcsize(self.binaryFormat))
		data = struct.unpack(self.binaryFormat, tmpData)
		self.indexes[0] = data[0]
		self.indexes[1] = data[2] # reverse
		self.indexes[2] = data[1] # reverse
		return self

	def Save(self, file):
		tmpData = [0] * 3
		tmpData[0] = self.indexes[0]
		tmpData[1] = self.indexes[2] # reverse
		tmpData[2] = self.indexes[1] # reverse
		data = struct.pack(self.binaryFormat,tmpData[0], tmpData[1], tmpData[2])
		file.write(data)
		#print "wrote MD3 face structure: ",data

	def Dump(self, log):
		log.info("MD3 Triangle")
		log.info("Indices: %s", self.indexes)
		log.info("")


class md3Shader:
	name = ""
	index = 0

	binaryFormat = "<%dsi" % MAX_QPATH

	def __init__(self):
		self.name = ""
		self.index = 0

	def GetSize(self):
		return struct.calcsize(self.binaryFormat)

	def Load(self, file):
		tmpData = file.read(struct.calcsize(self.binaryFormat))
		data = struct.unpack(self.binaryFormat, tmpData)
		self.name = asciiz(data[0])
		self.index = data[1]
		return self

	def Save(self, file):
		tmpData = [0] * 2
		tmpData[0] = self.name
		tmpData[1] = self.index
		data = struct.pack(self.binaryFormat, tmpData[0], tmpData[1])
		file.write(data)
		#print "wrote MD3 shader structure: ",data

	def Dump(self, log):
		log.info("MD3 Shader")
		log.info("Name: %s", self.name)
		log.info("Index: %s", self.index)
		log.info("")


class md3Surface:
	ident = ""
	name = ""
	flags = 0
	numFrames = 0
	numShaders = 0
	numVerts = 0
	numTriangles = 0
	ofsTriangles = 0
	ofsShaders = 0
	ofsUV = 0
	ofsVerts = 0
	ofsEnd = 0
	shaders = []
	triangles = []
	uv = []
	verts = []

	binaryFormat = "<4s%ds10i" % MAX_QPATH  # 1 int, name, then 10 ints

	def __init__(self):
		self.ident = ""
		self.name = ""
		self.flags = 0
		self.numFrames = 0
		self.numShaders = 0
		self.numVerts = 0
		self.numTriangles = 0
		self.ofsTriangles = 0
		self.ofsShaders = 0
		self.ofsUV = 0
		self.ofsVerts = 0
		self.ofsEnd
		self.shaders = []
		self.triangles = []
		self.uv = []
		self.verts = []

	def GetSize(self):
		sz = struct.calcsize(self.binaryFormat)
		self.ofsTriangles = sz
		for t in self.triangles:
			sz += t.GetSize()
		self.ofsShaders = sz
		for s in self.shaders:
			sz += s.GetSize()
		self.ofsUV = sz
		for u in self.uv:
			sz += u.GetSize()
		self.ofsVerts = sz
		for v in self.verts:
			sz += v.GetSize()
		self.ofsEnd = sz
		return self.ofsEnd

	def Load(self, file, log):
		# where are we in the file (for calculating real offsets)
		ofsBegin = file.tell()
		tmpData = file.read(struct.calcsize(self.binaryFormat))
		data = struct.unpack(self.binaryFormat, tmpData)
		self.ident = data[0]
		self.name = asciiz(data[1])
		self.flags = data[2]
		self.numFrames = data[3]
		self.numShaders = data[4]
		self.numVerts = data[5]
		self.numTriangles = data[6]
		self.ofsTriangles = data[7]
		self.ofsShaders = data[8]
		self.ofsUV = data[9]
		self.ofsVerts = data[10]
		self.ofsEnd = data[11]

		# load the tri info
		file.seek(ofsBegin + self.ofsTriangles, 0)
		for i in range(0, self.numTriangles):
			self.triangles.append(md3Triangle())
			self.triangles[i].Load(file)
			#self.triangles[i].Dump(log)

		# load the shader info
		file.seek(ofsBegin + self.ofsShaders, 0)
		for i in range(0, self.numShaders):
			self.shaders.append(md3Shader())
			self.shaders[i].Load(file)
			#self.shaders[i].Dump(log)

		# load the uv info
		file.seek(ofsBegin + self.ofsUV, 0)
		for i in range(0, self.numVerts):
			self.uv.append(md3TexCoord())
			self.uv[i].Load(file)
			#self.uv[i].Dump(log)

		# load the verts info
		file.seek(ofsBegin + self.ofsVerts, 0)
		for i in range(0, self.numFrames):
			for j in range(0, self.numVerts):
				self.verts.append(md3Vert())
				#i*self.numVerts+j=where in the surface vertex list the vert position for this frame is
				self.verts[(i * self.numVerts) + j].Load(file)
				#self.verts[j].Dump(log)

		# go to the end of this structure
		file.seek(ofsBegin+self.ofsEnd, 0)

		return self

	def Save(self, file):
		self.GetSize()
		tmpData = [0] * 12
		tmpData[0] = self.ident
		tmpData[1] = self.name
		tmpData[2] = self.flags
		tmpData[3] = self.numFrames
		tmpData[4] = self.numShaders
		tmpData[5] = self.numVerts
		tmpData[6] = self.numTriangles
		tmpData[7] = self.ofsTriangles
		tmpData[8] = self.ofsShaders
		tmpData[9] = self.ofsUV
		tmpData[10] = self.ofsVerts
		tmpData[11] = self.ofsEnd
		data = struct.pack(self.binaryFormat, tmpData[0],tmpData[1],tmpData[2],tmpData[3],tmpData[4],tmpData[5],tmpData[6],tmpData[7],tmpData[8],tmpData[9],tmpData[10],tmpData[11])
		file.write(data)

		# write the tri data
		for t in self.triangles:
			t.Save(file)

		# save the shader coordinates
		for s in self.shaders:
			s.Save(file)

		# save the uv info
		for u in self.uv:
			u.Save(file)

		# save the verts
		for v in self.verts:
			v.Save(file)

	def Dump(self, log):
		log.info("MD3 Surface")
		log.info("Ident: %s", self.ident)
		log.info("Name: %s", self.name)
		log.info("Flags: %s", self.flags)
		log.info("Number of Frames: %s", self.numFrames)
		log.info("Number of Shaders: %s", self.numShaders)
		log.info("Number of Verts: %s", self.numVerts)
		log.info("Number of Triangles: %s", self.numTriangles)
		log.info("Offset to Triangles: %s", self.ofsTriangles)
		log.info("Offset to Shaders: %s", self.ofsShaders)
		log.info("Offset to UV: %s", self.ofsUV)
		log.info("Offset to Verts: %s", self.ofsVerts)
		log.info("Offset to end: %s", self.ofsEnd)
		log.info("")


class md3Tag:
	name = ""
	origin = []
	axis = []

	binaryFormat="<%ds3f9f" % MAX_QPATH

	def __init__(self):
		self.name = ""
		self.origin = [0, 0, 0]
		self.axis = [0, 0, 0, 0, 0, 0, 0, 0, 0]

	def GetSize(self):
		return struct.calcsize(self.binaryFormat)

	def Load(self, file):
		tmpData = file.read(struct.calcsize(self.binaryFormat))
		data = struct.unpack(self.binaryFormat, tmpData)
		self.name = asciiz(data[0])
		self.origin[0] = data[1]
		self.origin[1] = data[2]
		self.origin[2] = data[3]
		self.axis[0] = data[4]
		self.axis[1] = data[5]
		self.axis[2] = data[6]
		self.axis[3] = data[7]
		self.axis[4] = data[8]
		self.axis[5] = data[9]
		self.axis[6] = data[10]
		self.axis[7] = data[11]
		self.axis[8] = data[12]
		return self

	def Save(self, file):
		tmpData = [0] * 13
		tmpData[0] = self.name
		tmpData[1] = float(self.origin[0])
		tmpData[2] = float(self.origin[1])
		tmpData[3] = float(self.origin[2])
		tmpData[4] = float(self.axis[0])
		tmpData[5] = float(self.axis[1])
		tmpData[6] = float(self.axis[2])
		tmpData[7] = float(self.axis[3])
		tmpData[8] = float(self.axis[4])
		tmpData[9] = float(self.axis[5])
		tmpData[10] = float(self.axis[6])
		tmpData[11] = float(self.axis[7])
		tmpData[12] = float(self.axis[8])
		data = struct.pack(self.binaryFormat, tmpData[0],tmpData[1],tmpData[2],tmpData[3],tmpData[4],tmpData[5],tmpData[6], tmpData[7], tmpData[8], tmpData[9], tmpData[10], tmpData[11], tmpData[12])
		file.write(data)
		#print "wrote MD3 Tag structure: ",data

	def Dump(self, log):
		log.info("MD3 Tag")
		log.info("Name: %s", self.name)
		log.info("Origin: %s", self.origin)
		log.info("Axis: %s", self.axis)
		log.info("")

class md3Frame:
	mins = 0
	maxs = 0
	localOrigin = 0
	radius = 0.0
	name = ""

	binaryFormat="<3f3f3ff16s"

	def __init__(self):
		self.mins = [0, 0, 0]
		self.maxs = [0, 0, 0]
		self.localOrigin = [0, 0, 0]
		self.radius = 0.0
		self.name = ""

	def GetSize(self):
		return struct.calcsize(self.binaryFormat)

	def Load(self, file):
		tmpData = file.read(struct.calcsize(self.binaryFormat))
		data = struct.unpack(self.binaryFormat, tmpData)
		self.mins[0] = data[0]
		self.mins[1] = data[1]
		self.mins[2] = data[2]
		self.maxs[0] = data[3]
		self.maxs[1] = data[4]
		self.maxs[2] = data[5]
		self.localOrigin[0] = data[6]
		self.localOrigin[1] = data[7]
		self.localOrigin[2] = data[8]
		self.radius = data[9]
		self.name = asciiz(data[10])
		return self

	def Save(self, file):
		tmpData = [0] * 11
		tmpData[0] = self.mins[0]
		tmpData[1] = self.mins[1]
		tmpData[2] = self.mins[2]
		tmpData[3] = self.maxs[0]
		tmpData[4] = self.maxs[1]
		tmpData[5] = self.maxs[2]
		tmpData[6] = self.localOrigin[0]
		tmpData[7] = self.localOrigin[1]
		tmpData[8] = self.localOrigin[2]
		tmpData[9] = self.radius
		tmpData[10] = self.name
		data = struct.pack(self.binaryFormat, tmpData[0],tmpData[1],tmpData[2],tmpData[3],tmpData[4],tmpData[5],tmpData[6],tmpData[7], tmpData[8], tmpData[9], tmpData[10])
		file.write(data)
		#print "wrote MD3 frame structure: ",data

	def Dump(self, log):
		log.info("MD3 Frame")
		log.info("Min Bounds: %s", self.mins)
		log.info("Max Bounds: %s", self.maxs)
		log.info("Local Origin: %s", self.localOrigin)
		log.info("Radius: %s", self.radius)
		log.info("Name: %s", self.name)
		log.info("")

class md3Object:
	# header structure
	ident = ""			# this is used to identify the file (must be IDP3)
	version = 0			# the version number of the file (Must be 15)
	name = ""
	flags = 0
	numFrames = 0
	numTags = 0
	numSurfaces = 0
	numSkins = 0
	ofsFrames = 0
	ofsTags = 0
	ofsSurfaces = 0
	ofsEnd = 0
	frames = []
	tags = []
	surfaces = []

	binaryFormat="<4si%ds9i" % MAX_QPATH  # little-endian (<), 17 integers (17i)

	def __init__(self):
		self.ident = 0
		self.version = 0
		self.name = ""
		self.flags = 0
		self.numFrames = 0
		self.numTags = 0
		self.numSurfaces = 0
		self.numSkins = 0
		self.ofsFrames = 0
		self.ofsTags = 0
		self.ofsSurfaces = 0
		self.ofsEnd = 0
		self.frames = []
		self.tags = []
		self.surfaces = []

	def GetSize(self):
		self.ofsFrames = struct.calcsize(self.binaryFormat)
		self.ofsTags = self.ofsFrames
		for f in self.frames:
			self.ofsTags += f.GetSize()
		self.ofsSurfaces += self.ofsTags
		for t in self.tags:
			self.ofsSurfaces += t.GetSize()
		self.ofsEnd = self.ofsSurfaces
		for s in self.surfaces:
			self.ofsEnd += s.GetSize()
		return self.ofsEnd

	def Load(self, file, log):
		tmpData = file.read(struct.calcsize(self.binaryFormat))
		data = struct.unpack(self.binaryFormat, tmpData)

		self.ident = data[0]
		self.version = data[1]

		if(self.ident != "IDP3" or self.version != 15):
			log.error("Not a valid MD3 file")
			log.error("Ident: %s", self.ident)
			log.error("Version: %s", self.version)
			Exit()

		self.name = asciiz(data[2])
		self.flags = data[3]
		self.numFrames = data[4]
		self.numTags = data[5]
		self.numSurfaces = data[6]
		self.numSkins = data[7]
		self.ofsFrames = data[8]
		self.ofsTags = data[9]
		self.ofsSurfaces = data[10]
		self.ofsEnd = data[11]

		# load the frame info
		file.seek(self.ofsFrames, 0)
		for i in range(0, self.numFrames):
			self.frames.append(md3Frame())
			self.frames[i].Load(file)
			#self.frames[i].Dump(log)

		# load the tags info
		file.seek(self.ofsTags, 0)
		for i in range(0, self.numFrames):
			for j in range(0, self.numTags):
				tag = md3Tag()
				tag.Load(file)
				#tag.Dump(log)
				self.tags.append(tag)

		# load the surface info
		file.seek(self.ofsSurfaces, 0)
		for i in range(0, self.numSurfaces):
			self.surfaces.append(md3Surface())
			self.surfaces[i].Load(file, log)
			self.surfaces[i].Dump(log)
		return self

	def Save(self, file):
		self.GetSize()
		tmpData = [0] * 12
		tmpData[0] = self.ident
		tmpData[1] = self.version
		tmpData[2] = self.name
		tmpData[3] = self.flags
		tmpData[4] = self.numFrames
		tmpData[5] = self.numTags
		tmpData[6] = self.numSurfaces
		tmpData[7] = self.numSkins
		tmpData[8] = self.ofsFrames
		tmpData[9] = self.ofsTags
		tmpData[10] = self.ofsSurfaces
		tmpData[11] = self.ofsEnd

		data = struct.pack(self.binaryFormat, tmpData[0],tmpData[1],tmpData[2],tmpData[3],tmpData[4],tmpData[5],tmpData[6],tmpData[7], tmpData[8], tmpData[9], tmpData[10], tmpData[11])
		file.write(data)

		for f in self.frames:
			f.Save(file)

		for t in self.tags:
			t.Save(file)

		for s in self.surfaces:
			s.Save(file)

	def Dump(self, log):
		log.info("Header Information")
		log.info("Ident: %s", self.ident)
		log.info("Version: %s", self.version)
		log.info("Name: %s", self.name)
		log.info("Flags: %s", self.flags)
		log.info("Number of Frames: %s",self.numFrames)
		log.info("Number of Tags: %s", self.numTags)
		log.info("Number of Surfaces: %s", self.numSurfaces)
		log.info("Number of Skins: %s", self.numSkins)
		log.info("Offset Frames: %s", self.ofsFrames)
		log.info("Offset Tags: %s", self.ofsTags)
		log.info("Offset Surfaces: %s", self.ofsSurfaces)
		log.info("Offset end: %s", self.ofsEnd)
		log.info("")

def asciiz(s):
	n = 0
	while(ord(s[n]) != 0):
		n = n + 1
	return s[0:n]

# strips the slashes from the back of a string
def StripPath(path):
	for c in range(len(path), 0, -1):
		if path[c-1] == "/" or path[c-1] == "\\":
			path = path[c:]
			break
	return path

# strips the model from path
def StripModel(path):
	for c in range(len(path), 0, -1):
		if path[c-1] == "/" or path[c-1] == "\\":
			path = path[:c]
			break
	return path

# strips file type extension
def StripExtension(name):
	n = 0
	best = len(name)
	while(n != -1):
		n = name.find('.',n+1)
		if(n != -1):
			best = n
	name = name[0:best]
	return name

# strips gamedir
def StripGamePath(name):
	gamepath = GAMEDIR.replace( '\\', '/' )
	namepath = name.replace( '\\', '/' )
	if namepath[0:len(gamepath)] == gamepath:
		namepath= namepath[len(gamepath):len(namepath)]
	return namepath

import sys, struct, string, math

def ANGLE2SHORT(x):
		return int((x * 65536 / 360) & 65535)

def SHORT2ANGLE(x):
		return x * (360.0 / 65536.0)

def DEG2RAD(a):
	return (a * math.pi) / 180.0

def RAD2DEG(a):
	return (a * 180.0) / math.pi

def DotProduct(x, y):
	return x[0] * y[0] + x[1] * y[1] + x[2] * y[2]

def CrossProduct(a,b):
	return [a[1]*b[2] - a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]]

def VectorLength(v):
	return math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2])

def VectorSubtract(a, b):
	return [a[0] - b[0], a[1] - b[1], a[2] - b[2]]

def VectorAdd(a, b):
	return [a[0] + b[0], a[1] + b[1], a[2] + b[2]]

def VectorCopy(v):
	return [v[0], v[1], v[2]]

def VectorInverse(v):
	return [-v[0], -v[1], -v[2]]

#define VectorCopy(a,b)			((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2])
#define	VectorScale(v, s, o)	((o)[0]=(v)[0]*(s),(o)[1]=(v)[1]*(s),(o)[2]=(v)[2]*(s))
#define	VectorMA(v, s, b, o)	((o)[0]=(v)[0]+(b)[0]*(s),(o)[1]=(v)[1]+(b)[1]*(s),(o)[2]=(v)[2]+(b)[2]*(s))

def RadiusFromBounds(mins, maxs):
	corner = [0, 0, 0]
	a = 0
	b = 0

	for i in range(0, 3):
		a = abs(mins[i])
		b = abs(maxs[i])
		if a > b:
			corner[i] = a
		else:
			corner[i] = b

	return VectorLength(corner)


# NOTE: Tr3B - matrix is in column-major order
def MatrixIdentity():
	return [[1.0, 0.0, 0.0, 0.0],
			[0.0, 1.0, 0.0, 0.0],
			[0.0, 0.0, 1.0, 0.0],
			[0.0, 0.0, 0.0, 1.0]]

def MatrixFromAngles(pitch, yaw, roll):
	sp = math.sin(DEG2RAD(pitch))
	cp = math.cos(DEG2RAD(pitch))

	sy = math.sin(DEG2RAD(yaw))
	cy = math.cos(DEG2RAD(yaw))

	sr = math.sin(DEG2RAD(roll))
	cr = math.cos(DEG2RAD(roll))

#	return [[cp * cy, (sr * sp * cy + cr * -sy), (cr * sp * cy + -sr * -sy), 0.0],
#			[cp * sy, (sr * sp * sy + cr * cy),  (cr * sp * sy + -sr * cy),  0.0],
#			[-sp,      sr * cp,                   cr * cp,                   0.0],
#			[0.0,      0.0,                       0.0,                       1.0]]

	return [[cp * cy,                     cp * sy,                 -sp,       0.0],
			[(sr * sp * cy + cr * -sy),  (sr * sp * sy + cr * cy),  sr * cp,  0.0],
			[(cr * sp * cy + -sr * -sy), (cr * sp * sy + -sr * cy), cr * cp,  0.0],
			[0.0,                         0.0,                      0.0,      1.0]]

def MatrixTransformPoint(m, p):
	return [m[0][0] * p[0] + m[1][0] * p[1] + m[2][0] * p[2] + m[3][0],
			m[0][1] * p[0] + m[1][1] * p[1] + m[2][1] * p[2] + m[3][1],
			m[0][2] * p[0] + m[1][2] * p[1] + m[2][2] * p[2] + m[3][2]]


def MatrixTransformNormal(m, p):
	return [m[0][0] * p[0] + m[1][0] * p[1] + m[2][0] * p[2],
			m[0][1] * p[0] + m[1][1] * p[1] + m[2][1] * p[2],
			m[0][2] * p[0] + m[1][2] * p[1] + m[2][2] * p[2]]

def MatrixMultiply(b, a):
	return [[
			a[0][0] * b[0][0] + a[0][1] * b[1][0] + a[0][2] * b[2][0],
			a[0][0] * b[0][1] + a[0][1] * b[1][1] + a[0][2] * b[2][1],
			a[0][0] * b[0][2] + a[0][1] * b[1][2] + a[0][2] * b[2][2],
			0.0,
			],[
			a[1][0] * b[0][0] + a[1][1] * b[1][0] + a[1][2] * b[2][0],
			a[1][0] * b[0][1] + a[1][1] * b[1][1] + a[1][2] * b[2][1],
			a[1][0] * b[0][2] + a[1][1] * b[1][2] + a[1][2] * b[2][2],
			0.0,
			],[
			a[2][0] * b[0][0] + a[2][1] * b[1][0] + a[2][2] * b[2][0],
			a[2][0] * b[0][1] + a[2][1] * b[1][1] + a[2][2] * b[2][1],
			a[2][0] * b[0][2] + a[2][1] * b[1][2] + a[2][2] * b[2][2],
			0.0,
			],[
			a[3][0] * b[0][0] + a[3][1] * b[1][0] + a[3][2] * b[2][0] + b[3][0],
			a[3][0] * b[0][1] + a[3][1] * b[1][1] + a[3][2] * b[2][1] + b[3][1],
			a[3][0] * b[0][2] + a[3][1] * b[1][2] + a[3][2] * b[2][2] + b[3][2],
			1.0,
			]]

def MatrixSetupTransform(forward, left, up, origin):
	return [[forward[0], forward[1], forward[2], origin[0]],
			[left[0],    left[1],   left[2],     origin[1]],
			[up[0],      up[1],     up[2],       origin[2]],
			[0.0,        0.0,       0.0,         1.0]]

# our own logger class. it works just the same as a normal logger except
# all info messages get show.
class Logger(logging.Logger):
	def __init__(self, name,level = logging.NOTSET):
		logging.Logger.__init__(self, name, level)

		self.has_warnings = False
		self.has_errors = False
		self.has_critical = False

	def info(self, msg, *args, **kwargs):
		apply(self._log,(logging.INFO, msg, args), kwargs)

	def warning(self, msg, *args, **kwargs):
		logging.Logger.warning(self, msg, *args, **kwargs)
		self.has_warnings = True

	def error(self, msg, *args, **kwargs):
		logging.Logger.error(self, msg, *args, **kwargs)
		self.has_errors = True

	def critical(self, msg, *args, **kwargs):
		logging.Logger.critical(self, msg, *args, **kwargs)
		self.has_errors = True

# should be able to make this print to stdout in realtime and save MESSAGES
# as well. perhaps also have a log to file option
class LogHandler(logging.StreamHandler):
	def __init__(self):
		logging.StreamHandler.__init__(self, sys.stdout)

		if "md3_export_log" not in Blender.Text.Get():
			self.outtext = Blender.Text.New("md3_export_log")
		else:
			self.outtext = Blender.Text.Get('md3_export_log')
			self.outtext.clear()

		self.lastmsg = ''

	def emit(self, record):
		# print to stdout and  to a new blender text object
		msg = self.format(record)

		if msg == self.lastmsg:
			return

		self.lastmsg = msg
		self.outtext.write("%s\n" %msg)

		logging.StreamHandler.emit(self, record)

logging.setLoggerClass(Logger)
log = logging.getLogger('md3_export')

handler = LogHandler()
formatter = logging.Formatter('%(levelname)s %(message)s')
handler.setFormatter(formatter)

log.addHandler(handler)
# set this to minimum output level. eg. logging.DEBUG, logging.WARNING, logging.ERROR
# logging.CRITICAL. logging.INFO will make little difference as these always get
# output'd
log.setLevel(logging.WARNING)


class BlenderGui:
	def __init__(self):
		text = """A log has been written to a blender text window. Change this window type to
a text window and you will be able to select the file md3_export_log."""

		text = textwrap.wrap(text,40)
		text += ['']

		if log.has_critical:
			text += ['There were critical errors!!!!']

		elif log.has_errors:
			text += ['There were errors!']

		elif log.has_warnings:
			text += ['There were warnings']

		# add any more text before here
		text.reverse()

		self.msg = text

		Blender.Draw.Register(self.gui, self.event, self.button_event)

	def gui(self,):
		quitbutton = Blender.Draw.Button("Exit", 1, 0, 0, 100, 20, "Close Window")

		y = 35

		for line in self.msg:
			BGL.glRasterPos2i(10,y)
			Blender.Draw.Text(line)
			y+=15

	def event(self,evt, val):
		if evt == Blender.Draw.ESCKEY:
			Blender.Draw.Exit()
			return

	def button_event(self,evt):
		if evt == 1:
			Blender.Draw.Exit()
			return


def ApplyTransform(vert, matrix):
	return vert * matrix


def UpdateFrameBounds(v, f):
	for i in range(0, 3):
		f.mins[i] = min(v[i], f.mins[i])
	for i in range(0, 3):
		f.maxs[i] = max(v[i], f.maxs[i])


def UpdateFrameRadius(f):
	f.radius = RadiusFromBounds(f.mins, f.maxs)


def ProcessSurface(scene, blenderObject, md3, pathName, modelName):
	# because md3 doesnt suppoort faceUVs like blender, we need to duplicate
	# any vertex that has multiple uv coords

	vertDict = {}
	indexDict = {} # maps a vertex index to the revised index after duplicating to account for uv
	vertList = [] # list of vertices ordered by revised index
	numVerts = 0
	uvList = [] # list of tex coords ordered by revised index
	faceList = [] # list of faces (they index into vertList)
	numFaces = 0

	scene.makeCurrent()
	Blender.Set("curframe", 1)
	Blender.Window.Redraw()

	# get the object (not just name) and the Mesh, not NMesh
	mesh = blenderObject.getData(False, True)
	matrix = blenderObject.getMatrix('worldspace')

	surf = md3Surface()
	surf.numFrames = md3.numFrames
	surf.name = blenderObject.getName()
	surf.ident = MD3_IDENT

	# create shader for surface
	surf.shaders.append(md3Shader())
	surf.numShaders += 1
	surf.shaders[0].index = 0

	log.info("Materials: %s", mesh.materials)
	# :P
	#shaderpath=Blender.Draw.PupStrInput("shader path for "+blenderObject.name+":", "", MAX_QPATH )
        shaderpath=""
	if 	shaderpath == "" :
		if not mesh.materials:
			surf.shaders[0].name = pathName + blenderObject.name
		else:
			surf.shaders[0].name = pathName + mesh.materials[0].name
	else:
		if not mesh.materials:
			surf.shaders[0].name = shaderpath + blenderObject.name
		else:
			surf.shaders[0].name = shaderpath + mesh.materials[0].name

	# process each face in the mesh
	for face in mesh.faces:

		tris_in_this_face = []  #to handle quads and up...

		# this makes a list of indices for each tri in this face. a quad will be [[0,1,1],[0,2,3]]
		for vi in range(1, len(face.v)-1):
			tris_in_this_face.append([0, vi, vi + 1])

		# loop across each tri in the face, then each vertex in the tri
		for this_tri in tris_in_this_face:
			numFaces += 1
			tri = md3Triangle()
			tri_ind = 0
			for i in this_tri:
				# get the vertex index, coords and uv coords
				index = face.v[i].index
				v = face.v[i].co
				if mesh.faceUV == True:
					uv = (face.uv[i][0], face.uv[i][1])
				elif mesh.vertexUV:
					uv = (face.v[i].uvco[0], face.v[i].uvco[1])
				else:
					uv = (0.0, 0.0) # handle case with no tex coords


				if vertDict.has_key((index, uv)):
					# if we've seen this exact vertex before, simply add it
					# to the tris list of vertex indices
					tri.indexes[tri_ind] = vertDict[(index, uv)]
				else:
					# havent seen this tri before
					# (or its uv coord is different, so we need to duplicate it)

					vertDict[(index, uv)] = numVerts

					# put the uv coord into the list
					# (uv coord are directly related to each vertex)
					tex = md3TexCoord()
					tex.u = uv[0]
					tex.v = uv[1]
					uvList.append(tex)

					tri.indexes[tri_ind] = numVerts

					# now because we have created a new index,
					# we need a way to link it to the index that
					# blender returns for NMVert.index
					if indexDict.has_key(index):
						# already there - each of the entries against
						# this key represents  the same vertex with a
						# different uv value
						ilist = indexDict[index]
						ilist.append(numVerts)
						indexDict[index] = ilist
					else:
						# this is a new one
						indexDict[index] = [numVerts]

					numVerts += 1
				tri_ind +=1
			faceList.append(tri)

	# we're done with faces and uv coords
	for t in uvList:
		surf.uv.append(t)

	for f in faceList:
		surf.triangles.append(f)

	surf.numTriangles = len(faceList)
	surf.numVerts = numVerts

	# now vertices are stored as frames -
	# all vertices for frame 1, all vertices for frame 2...., all vertices for frame n
	# so we need to iterate across blender's frames, and copy out each vertex
	for	frameNum in range(1, md3.numFrames + 1):
		Blender.Set("curframe", frameNum)
		Blender.Window.Redraw()

		m = NMesh.GetRawFromObject(blenderObject.name)

		vlist = [0] * numVerts
		for vertex in m.verts:
			try:
				vindices = indexDict[vertex.index]
			except:
				log.warning("Found a vertex in %s that is not part of a face", blenderObject.name)
				continue

			vTx = ApplyTransform(vertex.co, matrix)
			nTx = ApplyTransform(vertex.no, matrix)
			UpdateFrameBounds(vTx, md3.frames[frameNum - 1])
			vert = md3Vert()
			#vert.xyz = vertex.co[0:3]
			#vert.normal = vert.Encode(vertex.no[0:3])
			vert.xyz = vTx[0:3]
			vert.normal = vert.Encode(vertex.no[0:3])
                        #print vertex.no
			for ind in vindices:  # apply the position to all the duplicated vertices
				vlist[ind] = vert

		UpdateFrameRadius(md3.frames[frameNum - 1])

		for vl in vlist:
			surf.verts.append(vl)

	surf.Dump(log)
	md3.surfaces.append(surf)
	md3.numSurfaces += 1


def Export(fileName):
	if(fileName.find('.md3', -4) <= 0):
		fileName += '.md3'

	log.info("Starting ...")

	log.info("Exporting MD3 format to: %s", fileName)

	pathName = StripGamePath(StripModel(fileName))
	log.info("Shader path name: %s", pathName)

	modelName = StripExtension(StripPath(fileName))
	log.info("Model name: %s", modelName)

	md3 = md3Object()
	md3.ident = MD3_IDENT
	md3.version = MD3_VERSION

	tagList = []

	# get the scene
	scene = Blender.Scene.getCurrent()
	context = scene.getRenderingContext()

	scene.makeCurrent()
	md3.numFrames = Blender.Get("curframe")
	Blender.Set("curframe", 1)

	# create a bunch of blank frames, they'll be filled in by 'ProcessSurface'
	for i in range(1, md3.numFrames + 1):
		frame = md3Frame()
		frame.name = "frame_" + str(i)
		md3.frames.append(frame)

	# export all selected objects
	objlist = Blender.Object.GetSelected()

	# process each object for the export
	for obj in objlist:
		# check if it's a mesh object
		if obj.getType() == "Mesh":
			log.info("Processing surface: %s", obj.name)
			if len(md3.surfaces) == MD3_MAX_SURFACES:
				log.warning("Hit md3 limit (%i) for number of surfaces, skipping ...", MD3_MAX_SURFACES, obj.getName())
			else:
				ProcessSurface(scene, obj, md3, pathName, modelName)
		elif obj.getType() == "Empty":   # for tags, we just put em in a list so we can process them all together
			if obj.name[0:4] == "tag_":
				log.info("Processing tag: %s", obj.name)
				tagList.append(obj)
				md3.numTags += 1
		else:
			log.info("Skipping object: %s", obj.name)


	# work out the transforms for the tags for each frame of the export
	for i in range(1, md3.numFrames + 1):

		# needed to update IPO's value, but probably not the best way for that...
		scene.makeCurrent()
		Blender.Set("curframe", i)
		Blender.Window.Redraw()
		for tag in tagList:
			t = md3Tag()
			matrix = tag.getMatrix('worldspace')
			t.origin[0] = matrix[3][0]
			t.origin[1] = matrix[3][1]
			t.origin[2] = matrix[3][2]

			t.axis[0] = matrix[0][0]
			t.axis[1] = matrix[0][1]
			t.axis[2] = matrix[0][2]

			t.axis[3] = matrix[1][0]
			t.axis[4] = matrix[1][1]
			t.axis[5] = matrix[1][2]

			t.axis[6] = matrix[2][0]
			t.axis[7] = matrix[2][1]
			t.axis[8] = matrix[2][2]
			t.name = tag.name
			#t.Dump(log)
			md3.tags.append(t)

	# export!
	file = open(fileName, "wb")
	md3.Save(file)
	file.close()
	md3.Dump(log)

def FileSelectorCallback(fileName):
	Export(fileName)

	BlenderGui()

Blender.Window.FileSelector(FileSelectorCallback, "Export Quake3 MD3")