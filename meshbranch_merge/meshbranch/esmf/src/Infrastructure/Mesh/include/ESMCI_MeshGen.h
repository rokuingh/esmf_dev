<<<<<<< ESMCI_MeshGen.h
=======
//
// Earth System Modeling Framework
// Copyright 2002-2010, University Corporation for Atmospheric Research, 
// Massachusetts Institute of Technology, Geophysical Fluid Dynamics 
// Laboratory, University of Michigan, National Centers for Environmental 
// Prediction, Los Alamos National Laboratory, Argonne National Laboratory, 
// NASA Goddard Space Flight Center.
// Licensed under the University of Illinois-NCSA License.

//
//-----------------------------------------------------------------------------
>>>>>>> 1.5
#ifndef ESMCI_MeshGen_h
#define ESMCI_MeshGen_h

#include <ostream>

namespace ESMCI {

class Mesh;
class MeshObjTopo;

// Generate a hyper cube on proc 0
void HyperCube(Mesh &mesh, const MeshObjTopo *topo);

<<<<<<< ESMCI_MeshGen.h
// Create a DG mesh from the given mesh (i.e. duplicate nodes)
void DGMesh(const Mesh &srcmesh, Mesh &dgmesh);

=======
// Generate a 2D non periodic cartesian mesh on proc 0
void Cart2D(Mesh &mesh, const int X, const int Y,
                        const double xA, const double xB,
                        const double yA, const double yB);

// Generate a 3D non periodic spherical shell mesh on proc 0
void SphShell(Mesh &mesh, const int lat, const int lon,
                        const double latA, const double latB,
                        const double lonA, const double lonB);

>>>>>>> 1.5
} // namespace

#endif
