/* common/polylib.h */

#ifndef __COMMON_POLYLIB_H__
#define __COMMON_POLYLIB_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int numpoints;
    vec3_t p[4];                /* variable sized */
} winding_t;

#define MAX_POINTS_ON_WINDING 64
#define ON_EPSILON 0.1

winding_t *AllocWinding(int points);
vec_t WindingArea(const winding_t * w);
void WindingCenter(const winding_t * w, vec3_t center);
void WindingBounds (const winding_t *w, vec3_t mins, vec3_t maxs);
void ClipWinding(const winding_t * in, vec3_t normal, vec_t dist,
                 winding_t ** front, winding_t ** back);
winding_t *ChopWinding(winding_t * in, vec3_t normal, vec_t dist);
winding_t *CopyWinding(const winding_t * w);
winding_t *BaseWindingForPlane(const vec3_t normal, float dist);
void CheckWinding(const winding_t * w);
void WindingPlane(const winding_t * w, vec3_t normal, vec_t *dist);
void RemoveColinearPoints(winding_t * w);

#ifdef __cplusplus
}
#endif

#endif /* __COMMON_POLYLIB_H__ */
