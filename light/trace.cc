/*  Copyright (C) 1996-1997  Id Software, Inc.

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

#include <light/light.hh>
#include <light/trace.hh>
#include <light/ltface.hh>
#include <common/bsputils.hh>
#ifdef HAVE_EMBREE
#include <light/trace_embree.hh>
#endif
#include <cassert>

/*
==============
Light_PointInLeaf
 
from hmap2
==============
*/
const mleaf_t *Light_PointInLeaf( const mbsp_t *bsp, const qvec3d &point )
{
    int num = 0;
    
    while( num >= 0 )
        num = bsp->dnodes[num].children[bsp->dplanes[bsp->dnodes[num].planenum].distance_to_fast(point) < 0];
    
    return &bsp->dleafs[-1 - num];
}

/*
==============
Light_PointContents

from hmap2
==============
*/
int Light_PointContents( const mbsp_t *bsp, const qvec3d &point )
{
    return Light_PointInLeaf(bsp, point)->contents;
}

/*
 * ============================================================================
 * FENCE TEXTURE TESTING
 * ============================================================================
 */

/**
 * Given a float texture coordinate, returns a pixel index to sample in [0, width-1].
 * This assumes the texture repeats and nearest filtering
 */
uint32_t clamp_texcoord(vec_t in, uint32_t width)
{
    if (in >= 0.0f) {
        return (uint32_t)in % width;
    } else {
        vec_t in_abs = ceil(fabs(in));
        uint32_t in_abs_mod = (uint32_t)in_abs % width;
        return (width - in_abs_mod) % width;
    }
}

qvec4b SampleTexture(const mface_t *face, const mbsp_t *bsp, const qvec3d &point)
{
    const auto *texture = Face_Texture(bsp, face);

    if (texture == nullptr || !texture->meta.width) {
        return {};
    }

    const mtexinfo_t *tex = &bsp->texinfo[face->texinfo];

    qvec2d texcoord = WorldToTexCoord(point, tex);

    const uint32_t x = clamp_texcoord(texcoord[0], texture->meta.width);
    const uint32_t y = clamp_texcoord(texcoord[1], texture->meta.height);

    return texture->pixels[(texture->meta.width * y) + x];
}

hitresult_t TestSky(const qvec3d &start, const qvec3d &dirn, const modelinfo_t *self, const mface_t **face_out)
{
    return Embree_TestSky(start, dirn, self, face_out);
}

hitresult_t TestLight(const qvec3d &start, const qvec3d &stop, const modelinfo_t *self)
{
    return Embree_TestLight(start, stop, self);
}

raystream_intersection_t *MakeIntersectionRayStream(int maxrays)
{
    return Embree_MakeIntersectionRayStream(maxrays);
}
raystream_occlusion_t *MakeOcclusionRayStream(int maxrays)
{
    return Embree_MakeOcclusionRayStream(maxrays);
}

void MakeTnodes(const mbsp_t *bsp)
{
    Embree_TraceInit(bsp);
}
