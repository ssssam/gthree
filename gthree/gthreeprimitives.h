#ifndef __GTHREE_PRIMITIVES_H__
#define __GTHREE_PRIMITIVES_H__

#if !defined (__GTHREE_H_INSIDE__) && !defined (GTHREE_COMPILATION)
#error "Only <gthree/gthree.h> can be included directly."
#endif

#include <gthree/gthreegeometry.h>

G_BEGIN_DECLS

GTHREE_API
GthreeGeometry *gthree_geometry_new_box           (float width,
                                                   float height,
                                                   float depth,
                                                   int   width_segments,
                                                   int   height_segments,
                                                   int   depth_segments);
GTHREE_API
GthreeGeometry *gthree_geometry_new_sphere        (float radius,
                                                   int   widthSegments,
                                                   int   heightSegments);
GTHREE_API
GthreeGeometry *gthree_geometry_new_sphere_full   (float radius,
                                                   int   widthSegments,
                                                   int   heightSegments,
                                                   float phiStart,
                                                   float phiLength,
                                                   float thetaStart,
                                                   float thetaLength);
GTHREE_API
GthreeGeometry *gthree_geometry_new_cylinder      (float radius,
                                                   float length);
GTHREE_API
GthreeGeometry *gthree_geometry_new_cylinder_full (float    radiusTop,
                                                   float    radiusBottom,
                                                   float    height,
                                                   int      radialSegments,
                                                   int      heightSegments,
                                                   gboolean openEnded,
                                                   float    thetaStart,
                                                   float    thetaLength);
GTHREE_API
GthreeGeometry *gthree_geometry_new_torus         (float    radius,
                                                   float    tube);
GTHREE_API
GthreeGeometry *gthree_geometry_new_torus_full    (float    radius,
                                                   float    tube,
                                                   int      radialSegments,
                                                   int      tubularSegments,
                                                   float    arc);
GTHREE_API
GthreeGeometry *gthree_geometry_new_plane          (float width,
                                                    float height,
                                                    int   width_segments,
                                                    int   height_segments);

G_END_DECLS

#endif /* __GTHREE_PRIMITIVES_H__ */
