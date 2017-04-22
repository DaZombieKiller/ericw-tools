/*  Copyright (C) 2017 Eric Wasylishen

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

    See file, 'COPYING', for details.
*/

#ifndef __COMMON_MESH_HH__
#define __COMMON_MESH_HH__

#include <vector>
#include <glm/glm.hpp> // FIXME: switch to qvec

class mesh_t {
public:
    std::vector<glm::vec3> verts;
    std::vector<std::vector<int>> faces;
};

// Welds vertices at exactly the same position
mesh_t buildMesh(const std::vector<std::vector<glm::vec3>> &faces);
std::vector<std::vector<glm::vec3>> meshToFaces(const mesh_t &mesh);

// Preserves the number and order of faces.
// doesn't merge verts.
// adds verts to fix t-juncs
void cleanupMesh(mesh_t &mesh);

#endif /* __COMMON_MESH_HH__ */
