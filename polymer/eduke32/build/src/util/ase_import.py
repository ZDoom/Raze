#!BPY 

""" 
Name: 'ASCII Scene (.ase) v0.15' 
Blender: 242 
Group: 'Import' 
Tooltip: 'ASCII Scene import (*.ase)' 
""" 
__author__ = "Goofos & Plagman" 
__version__ = "0.15" 

# goofos at epruegel.de 
# 
# ***** BEGIN GPL LICENSE BLOCK ***** 
# 
# This program is free software; you can redistribute it and/or 
# modify it under the terms of the GNU General Public License 
# as published by the Free Software Foundation; either version 2 
# of the License, or (at your option) any later version. 
# 
# This program is distributed in the hope that it will be useful, 
# but WITHOUT ANY WARRANTY; without even the implied warranty of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
# GNU General Public License for more details. 
# 
# You should have received a copy of the GNU General Public License 
# along with this program; if not, write to the Free Software Foundation, 
# Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. 
# 
# ***** END GPL LICENCE BLOCK ***** 

import string, time, sys as osSys 
import Blender 
from Blender import Draw, Mesh, Window, Object, Scene, NMesh, Key, Ipo, IpoCurve
#import meshtools 


def read_main(filename): 

    global counts 
    counts = {'verts': 0, 'tris': 0} 

    start = time.clock() 
    file = open(filename, "r") 

    print_boxed("----------------start-----------------") 
    print 'Import Patch: ', filename 

    editmode = Window.EditMode()    # are we in edit mode?  If so ... 
    if editmode: Window.EditMode(0) # leave edit mode before getting the mesh 

    lines= file.readlines() 
    read_file(file, lines) 

    Blender.Window.DrawProgressBar(1.0, '')  # clear progressbar 
    file.close() 
    print "----------------end-----------------" 
    end = time.clock() 
    seconds = " in %.2f %s" % (end-start, "seconds") 
    totals = "Verts: %i Tris: %i " % (counts['verts'], counts['tris']) 
    print_boxed(totals) 
    message = "Successfully imported " + Blender.sys.basename(filename) + seconds 
    #meshtools.print_boxed(message) 
    print_boxed(message) 


def print_boxed(text): #Copy/Paste from meshtools, only to remove the beep :) 
    lines = text.splitlines() 
    maxlinelen = max(map(len, lines)) 
    if osSys.platform[:3] == "win": 
        print chr(218)+chr(196) + chr(196)*maxlinelen + chr(196)+chr(191) 
        for line in lines: 
            print chr(179) + ' ' + line.ljust(maxlinelen) + ' ' + chr(179) 
        print chr(192)+chr(196) + chr(196)*maxlinelen + chr(196)+chr(217) 
    else: 
        print '+-' + '-'*maxlinelen + '-+' 
        for line in lines: print '| ' + line.ljust(maxlinelen) + ' |' 
        print '+-' + '-'*maxlinelen + '-+' 
    #print '\a\r', # beep when done 


class ase_obj: 

    def __init__(self): 
        self.name = 'Name' 
        self.objType = None 
        self.row0x = None 
        self.row0y = None 
        self.row0z = None 
        self.row1x = None 
        self.row1y = None 
        self.row1z = None 
        self.row2x = None 
        self.row2y = None 
        self.row2z = None 
        self.row3x = None 
        self.row3y = None 
        self.row3z = None 
        self.parent = None 
        self.obj = None 
        self.objName = 'Name' 

class ase_mesh: 

    def __init__(self): 
        self.name = '' 
        self.vCount = 0 
        self.fCount = 0 
        self.verts = [] 
        self.faces = [] 
        
class mesh_vert: 

    def __init__(self): 
        self.x = 0.0 
        self.y = 0.0 
        self.z = 0.0 
        self.u = 0.0
        self.v = 0.0
        self.nx = 0.0 
        self.ny = 0.0 
        self.nz = 0.0 
    def make_tuple(self):
        return (self.x, self.y, self.z, self.u, self.v, self.nx, self.ny, self.nz)
      
class mesh_face: 

    def __init__(self): 
        self.v1 = mesh_vert() 
        self.v2 = mesh_vert() 
        self.v3 = mesh_vert() 
        self.i1 = 0
        self.i2 = 0
        self.i3 = 0

def read_file(file, lines): 

    objects = [] 
    objIdx = 0 
    objCheck = -1 #needed to skip helper objects 
    PBidx = 0.0 
    lineCount = float(len(lines)) 
    processed_indices = []
    curFaceID = 0
    faceVertID = 0

    print 'Read file' 
    Blender.Window.DrawProgressBar(0.0, "Read File...") 

    for line in lines: 
        words = string.split(line) 

        if (PBidx % 10000) == 0.0: 
            Blender.Window.DrawProgressBar(PBidx / lineCount, "Read File...") 

        if not words: 
            continue 
        elif words[0] == '*GEOMOBJECT': 
            objCheck = 0 
            newObj = ase_obj() 
            objects.append(newObj) 
            obj = objects[objIdx] 
            objIdx += 1 
        elif words[0] == '*NODE_NAME' and objCheck != -1: 
            if objCheck == 0: 
                obj.name = words[1] 
                objCheck = 1 
            elif objCheck == 1: 
                obj.objName = words[1] 
        elif words[0] == '*TM_ROW0' and objCheck != -1: 
            obj.row0x = float(words[1]) 
            obj.row0y = float(words[2]) 
            obj.row0z = float(words[3]) 
        elif words[0] == '*TM_ROW1' and objCheck != -1: 
            obj.row1x = float(words[1]) 
            obj.row1y = float(words[2]) 
            obj.row1z = float(words[3]) 
        elif words[0] == '*TM_ROW2' and objCheck != -1: 
            obj.row2x = float(words[1]) 
            obj.row2y = float(words[2]) 
            obj.row2z = float(words[3]) 
        elif words[0] == '*TM_ROW3' and objCheck != -1: 
            obj.row3x = float(words[1]) 
            obj.row3y = float(words[2]) 
            obj.row3z = float(words[3]) 
            objCheck = -1 
        elif words[0] == '*MESH': 
            obj.objType = 'Mesh' 
            obj.obj = ase_mesh() 
            me = obj.obj 
        elif words[0] == '*MESH_NUMVERTEX': 
            me.vCount = int(words[1]) 
            for i in range(me.vCount):
                me.verts.append(mesh_vert())
        elif words[0] == '*MESH_NUMFACES': 
            me.fCount = int(words[1]) 
            for i in range(me.fCount):
                me.faces.append(mesh_face())
        elif words[0] == '*MESH_VERTEX':
            i = int(words[1])
            me.verts[i].x = float(words[2]);
            me.verts[i].y = float(words[3]);
            me.verts[i].z = float(words[4]);
        elif words[0] == '*MESH_FACE': 
            i = int(words[1].rstrip(":")) # looks like "13:"
            v1 = int(words[3]);
            v2 = int(words[5]);
            v3 = int(words[7]);
            me.faces[i].v1.x = me.verts[v1].x;
            me.faces[i].v1.y = me.verts[v1].y;
            me.faces[i].v1.z = me.verts[v1].z;
            
            me.faces[i].v2.x = me.verts[v2].x;
            me.faces[i].v2.y = me.verts[v2].y;
            me.faces[i].v2.z = me.verts[v2].z;
            
            me.faces[i].v3.x = me.verts[v3].x;
            me.faces[i].v3.y = me.verts[v3].y;
            me.faces[i].v3.z = me.verts[v3].z;
        elif words[0] == '*MESH_NUMTVERTEX': 
            del me.verts[:]
            uvCount = int(words[1]) 
            for i in range(uvCount):
                me.verts.append(mesh_vert())
        elif words[0] == '*MESH_TVERT': 
            i = int(words[1])
            me.verts[i].u = float(words[2]);
            me.verts[i].v = float(words[3]);
        #elif words[0] == '*MESH_NUMTVFACES': 
        elif words[0] == '*MESH_TFACE':
            i = int(words[1])
            uv1 = int(words[2]);
            uv2 = int(words[3]);
            uv3 = int(words[4]);
            
            me.faces[i].v1.u = me.verts[uv1].u;
            me.faces[i].v1.v = me.verts[uv1].v;

            me.faces[i].v2.u = me.verts[uv2].u;
            me.faces[i].v2.v = me.verts[uv2].v;

            me.faces[i].v3.u = me.verts[uv3].u;
            me.faces[i].v3.v = me.verts[uv3].v;
        #elif words[0] == '*MESH_NUMCVERTEX': 
            ##
        #elif words[0] == '*MESH_VERTCOL': 
            ##
        #elif words[0] == '*MESH_CFACE': 
            ##
        elif words[0] == '*MESH_FACENORMAL': 
            curFaceID = int(words[1]) # global, vertexnormal needs this
            faceVertID = 0 # same
        elif words[0] == '*MESH_VERTEXNORMAL': 
            nx = float(words[2])
            ny = float(words[3])
            nz = float(words[4])

            if (faceVertID == 0):
                me.faces[curFaceID].v1.nx = nx;
                me.faces[curFaceID].v1.ny = ny;
                me.faces[curFaceID].v1.nz = nz;
            elif (faceVertID == 1):
                me.faces[curFaceID].v2.nx = nx;
                me.faces[curFaceID].v2.ny = ny;
                me.faces[curFaceID].v2.nz = nz;
            elif (faceVertID == 2):
                me.faces[curFaceID].v3.nx = nx;
                me.faces[curFaceID].v3.ny = ny;
                me.faces[curFaceID].v3.nz = nz;
                
            faceVertID = faceVertID + 1;
        PBidx += 1.0 

    spawn_main(objects) 

    Blender.Redraw() 

def spawn_main(objects): 

    PBidx = 0.0 
    objCount = float(len(objects)) 

    print 'Import Objects' 
    Blender.Window.DrawProgressBar(0.0, "Importing Objects...") 

    for obj in objects: 

        Blender.Window.DrawProgressBar(PBidx / objCount, "Importing Objects...") 

        if obj.objType == 'Mesh': 
            spawn_mesh(obj) 

        PBidx += 1.0 

import random

def spawn_mesh(obj): 

    objMe = obj.obj 
    #normal_flag = 1 

    row0 = obj.row0x, obj.row0y, obj.row0z 
    row1 = obj.row1x, obj.row1y, obj.row1z 
    row2 = obj.row2x, obj.row2y, obj.row2z 
    row3 = obj.row3x, obj.row3y, obj.row3z 

    newMatrix = Blender.Mathutils.Matrix(row0, row1, row2, row3) 
    newMatrix.resize4x4() 

    newObj = Blender.Object.New(obj.objType, obj.name) 
    newObj.setMatrix(newMatrix) 
    Blender.Scene.getCurrent().link(newObj) 


    newMesh = Blender.Mesh.New(obj.objName) 
    newMesh.getFromObject(newObj.name) 
    
    newMesh.vertexUV = 1
    newObj.link(newMesh) 
    
    del objMe.verts[:]
    objMe.vCount = 0
    
    vertDict = {}
    
    #for face in objMe.faces:
        #objMe.verts.append(face.v1)
        #objMe.verts.append(face.v2)
        #objMe.verts.append(face.v3)
        #face.i1 = objMe.vCount
        #objMe.vCount = objMe.vCount + 1
        #face.i2 = objMe.vCount
        #objMe.vCount = objMe.vCount + 1
        #face.i3 = objMe.vCount
        #objMe.vCount = objMe.vCount + 1
    
    for face in objMe.faces:
        if not face.v1.make_tuple() in vertDict:
            vertDict[face.v1.make_tuple()] = objMe.vCount
            objMe.verts.append(face.v1)
            objMe.vCount = objMe.vCount + 1
        if not face.v2.make_tuple() in vertDict:
            vertDict[face.v2.make_tuple()] = objMe.vCount
            objMe.verts.append(face.v2)
            objMe.vCount = objMe.vCount + 1
        if not face.v3.make_tuple() in vertDict:
            vertDict[face.v3.make_tuple()] = objMe.vCount
            objMe.verts.append(face.v3)
            objMe.vCount = objMe.vCount + 1
        face.i1 = vertDict[face.v1.make_tuple()]
        face.i2 = vertDict[face.v2.make_tuple()]
        face.i3 = vertDict[face.v3.make_tuple()]

    # Verts 
    for i in range(objMe.vCount):
        xyz = Blender.Mathutils.Vector(objMe.verts[i].x, objMe.verts[i].y, objMe.verts[i].z)
        newMesh.verts.extend(xyz)

    frameCount = 100
    

    #animate
    for frame in range(frameCount):
        for i in range(objMe.vCount):
            xyz = Blender.Mathutils.Vector(objMe.verts[i].x, objMe.verts[i].y, objMe.verts[i].z)
            uv = Blender.Mathutils.Vector(objMe.verts[i].u, objMe.verts[i].v)
            norm = Blender.Mathutils.Vector(objMe.verts[i].nx, objMe.verts[i].ny, objMe.verts[i].nz)
            newMesh.verts[i].co  = xyz * (1.0 + frame * 0.1);
            newMesh.verts[i].uvco = uv;
            newMesh.verts[i].no = norm;
        newObj.insertShapeKey()

    # Faces 
    for i in range(objMe.fCount):
        face = [objMe.faces[i].i1, objMe.faces[i].i2, objMe.faces[i].i3]
        newMesh.faces.extend(face) 

    # UV 
    #if guiTable['UV'] == 1 and objMe.hasFUV == 1: 
        #newMesh.faceUV = 1 
        #for f in objMe.uvFaces: 
            #uv1 = Blender.Mathutils.Vector(float(objMe.uvVerts[f.uv1].u), float(objMe.uvVerts[f.uv1].v)) 
            #uv2 = Blender.Mathutils.Vector(float(objMe.uvVerts[f.uv2].u), float(objMe.uvVerts[f.uv2].v)) 
            #uv3 = Blender.Mathutils.Vector(float(objMe.uvVerts[f.uv3].u), float(objMe.uvVerts[f.uv3].v)) 
            #newMesh.faces[f.index].uv = [uv1, uv2, uv3] 
    ## normals
    #vertices = [coords for n, coords in sorted(objMe.normals)]
   
    #random.seed()

    #i = 0
    #for v in newMesh.verts:
        #no = Blender.Mathutils.Vector(vertices[i][0], vertices[i][1], vertices[i][2])
        #v.no = no
        #print 'vertice ', i, 'normal : ', v.no
        ##v.no[0] = vertices[i][0]
        ##v.no[1] = vertices[i][1]
        ##v.no[2] = vertices[i][2]
        #i = i + 1
        
    for key in Key.Get() :
        key.ipo = Ipo.New('Key', "bleh" + "_ipo")
    index = 1
    for curveName in key.ipo.curveConsts :
        # print curveName
        key.ipo.addCurve(curveName)
        key.ipo[curveName].interpolation = IpoCurve.InterpTypes.CONST
        key.ipo[curveName].addBezier((0, 0))
        key.ipo[curveName].addBezier((index, 1))
        key.ipo[curveName].addBezier((index + 1, 0))
        index+=1

    newMesh.transform((newObj.getMatrix('worldspace').invert()), 1) 

    counts['verts'] += objMe.vCount 
    counts['tris'] += objMe.fCount 
    print 'Imported Mesh-Object: ', obj.name 



def read_ui(filename): 
    Window.WaitCursor(1) 

    read_main(filename) 

    Window.WaitCursor(0) 


Blender.Window.FileSelector(read_ui, "Import ASE")