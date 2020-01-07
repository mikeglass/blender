/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2006 by Nicholas Bishop
 * All rights reserved.
 */

/** \file
 * \ingroup edsculpt
 */

#ifndef __SCULPT_INTERN_H__
#define __SCULPT_INTERN_H__

#include "DNA_listBase.h"
#include "DNA_vec_types.h"
#include "DNA_key_types.h"

#include "BLI_bitmap.h"
#include "BLI_threads.h"

#include "BKE_pbvh.h"

struct KeyBlock;
struct Object;
struct SculptUndoNode;
struct bContext;
struct PoseIKChainSegment;

bool sculpt_mode_poll(struct bContext *C);
bool sculpt_mode_poll_view3d(struct bContext *C);
/* checks for a brush, not just sculpt mode */
bool sculpt_poll(struct bContext *C);
bool sculpt_poll_view3d(struct bContext *C);

/* Updates */

typedef enum SculptUpdateType {
  SCULPT_UPDATE_COORDS = 1 << 0,
  SCULPT_UPDATE_MASK = 1 << 1,
} SculptUpdateType;

/* Stroke */

typedef struct SculptCursorGeometryInfo {
  float location[3];
  float normal[3];
  float active_vertex_co[3];
} SculptCursorGeometryInfo;

bool sculpt_stroke_get_location(struct bContext *C, float out[3], const float mouse[2]);
bool sculpt_cursor_geometry_info_update(bContext *C,
                                        SculptCursorGeometryInfo *out,
                                        const float mouse[2],
                                        bool use_sampled_normal);
void sculpt_geometry_preview_lines_update(bContext *C, struct SculptSession *ss, float radius);
void sculpt_pose_calc_pose_data(struct Sculpt *sd,
                                struct Object *ob,
                                struct SculptSession *ss,
                                float initial_location[3],
                                float radius,
                                float pose_offset,
                                float *r_pose_origin,
                                float *r_pose_factor);

struct PoseIKChain *sculpt_pose_ik_chain_init(struct Sculpt *sd,
                                              struct Object *ob,
                                              struct SculptSession *ss,
                                              struct Brush *br,
                                              const float initial_location[3],
                                              const float radius);

void sculpt_pose_ik_chain_free(struct PoseIKChain *ik_chain);

/* Sculpt PBVH abstraction API */
const float *sculpt_vertex_co_get(struct SculptSession *ss, int index);

/* Dynamic topology */
void sculpt_pbvh_clear(Object *ob);
void sculpt_dyntopo_node_layers_add(struct SculptSession *ss);
void sculpt_dynamic_topology_disable(bContext *C, struct SculptUndoNode *unode);

/* Undo */

typedef enum {
  SCULPT_UNDO_COORDS,
  SCULPT_UNDO_HIDDEN,
  SCULPT_UNDO_MASK,
  SCULPT_UNDO_DYNTOPO_BEGIN,
  SCULPT_UNDO_DYNTOPO_END,
  SCULPT_UNDO_DYNTOPO_SYMMETRIZE,
  SCULPT_UNDO_GEOMETRY,
} SculptUndoType;

typedef struct SculptUndoNode {
  struct SculptUndoNode *next, *prev;

  SculptUndoType type;

  char idname[MAX_ID_NAME]; /* name instead of pointer*/
  void *node;               /* only during push, not valid afterwards! */

  float (*co)[3];
  float (*orig_co)[3];
  short (*no)[3];
  float *mask;
  int totvert;

  /* non-multires */
  int maxvert; /* to verify if totvert it still the same */
  int *index;  /* to restore into right location */
  BLI_bitmap *vert_hidden;

  /* multires */
  int maxgrid;  /* same for grid */
  int gridsize; /* same for grid */
  int totgrid;  /* to restore into right location */
  int *grids;   /* to restore into right location */
  BLI_bitmap **grid_hidden;

  /* bmesh */
  struct BMLogEntry *bm_entry;
  bool applied;

  /* shape keys */
  char shapeName[sizeof(((KeyBlock *)0))->name];

  /* geometry modification operations and bmesh enter data */
  CustomData geom_vdata;
  CustomData geom_edata;
  CustomData geom_ldata;
  CustomData geom_pdata;
  int geom_totvert;
  int geom_totedge;
  int geom_totloop;
  int geom_totpoly;

  /* pivot */
  float pivot_pos[3];
  float pivot_rot[4];

  size_t undo_size;
} SculptUndoNode;

/* Factor of brush to have rake point following behind
 * (could be configurable but this is reasonable default). */
#define SCULPT_RAKE_BRUSH_FACTOR 0.25f

struct SculptRakeData {
  float follow_dist;
  float follow_co[3];
};

/* Single struct used by all BLI_task threaded callbacks, let's avoid adding 10's of those... */
typedef struct SculptThreadedTaskData {
  struct bContext *C;
  struct Sculpt *sd;
  struct Object *ob;
  const struct Brush *brush;
  struct PBVHNode **nodes;
  int totnode;

  struct VPaint *vp;
  struct VPaintData *vpd;
  struct WPaintData *wpd;
  struct WeightPaintInfo *wpi;
  unsigned int *lcol;
  struct Mesh *me;
  /* For passing generic params. */
  void *custom_data;

  /* Data specific to some callbacks. */

  /* Note: even if only one or two of those are used at a time,
   *       keeping them separated, names help figuring out
   *       what it is, and memory overhead is ridiculous anyway. */
  float flippedbstrength;
  float angle;
  float strength;
  bool smooth_mask;
  bool has_bm_orco;

  struct SculptProjectVector *spvc;
  float *offset;
  float *grab_delta;
  float *cono;
  float *area_no;
  float *area_no_sp;
  float *area_co;
  float (*mat)[4];
  float (*vertCos)[3];

  int filter_type;
  float filter_strength;

  bool use_area_cos;
  bool use_area_nos;
  bool any_vertex_sampled;

  float *prev_mask;

  float *pose_factor;
  float *pose_initial_co;
  int pose_chain_segment;

  float multiplane_scrape_angle;
  float multiplane_scrape_planes[2][4];

  float max_distance_squared;
  float nearest_vertex_search_co[3];

  int mask_expand_update_it;
  bool mask_expand_invert_mask;
  bool mask_expand_use_normals;
  bool mask_expand_keep_prev_mask;

  float transform_mats[8][4][4];

  float dirty_mask_min;
  float dirty_mask_max;
  bool dirty_mask_dirty_only;

  ThreadMutex mutex;

} SculptThreadedTaskData;

/*************** Brush testing declarations ****************/
typedef struct SculptBrushTest {
  float radius_squared;
  float location[3];
  float dist;
  int mirror_symmetry_pass;

  /* For circle (not sphere) projection. */
  float plane_view[4];

  /* Some tool code uses a plane for it's calculateions. */
  float plane_tool[4];

  /* View3d clipping - only set rv3d for clipping */
  struct RegionView3D *clip_rv3d;
} SculptBrushTest;

typedef bool (*SculptBrushTestFn)(SculptBrushTest *test, const float co[3]);

typedef struct {
  struct Sculpt *sd;
  struct SculptSession *ss;
  float radius_squared;
  const float *center;
  bool original;
  bool ignore_fully_masked;
} SculptSearchSphereData;

typedef struct {
  struct Sculpt *sd;
  struct SculptSession *ss;
  float radius_squared;
  bool original;
  bool ignore_fully_masked;
  struct DistRayAABB_Precalc *dist_ray_to_aabb_precalc;
} SculptSearchCircleData;

void sculpt_brush_test_init(struct SculptSession *ss, SculptBrushTest *test);
bool sculpt_brush_test_sphere(SculptBrushTest *test, const float co[3]);
bool sculpt_brush_test_sphere_sq(SculptBrushTest *test, const float co[3]);
bool sculpt_brush_test_sphere_fast(const SculptBrushTest *test, const float co[3]);
bool sculpt_brush_test_cube(SculptBrushTest *test, const float co[3], float local[4][4]);
bool sculpt_brush_test_circle_sq(SculptBrushTest *test, const float co[3]);
bool sculpt_search_sphere_cb(PBVHNode *node, void *data_v);
bool sculpt_search_circle_cb(PBVHNode *node, void *data_v);

SculptBrushTestFn sculpt_brush_test_init_with_falloff_shape(SculptSession *ss,
                                                            SculptBrushTest *test,
                                                            char falloff_shape);
const float *sculpt_brush_frontface_normal_from_falloff_shape(SculptSession *ss,
                                                              char falloff_shape);

float tex_strength(struct SculptSession *ss,
                   const struct Brush *br,
                   const float point[3],
                   const float len,
                   const short vno[3],
                   const float fno[3],
                   const float mask,
                   const int vertex_index,
                   const int thread_id);

/* just for vertex paint. */
bool sculpt_pbvh_calc_area_normal(const struct Brush *brush,
                                  Object *ob,
                                  PBVHNode **nodes,
                                  int totnode,
                                  bool use_threading,
                                  float r_area_no[3]);

/* Cache stroke properties. Used because
 * RNA property lookup isn't particularly fast.
 *
 * For descriptions of these settings, check the operator properties.
 */

typedef struct StrokeCache {
  /* Invariants */
  float initial_radius;
  float scale[3];
  int flag;
  float clip_tolerance[3];
  float initial_mouse[2];

  /* Variants */
  float radius;
  float radius_squared;
  float true_location[3];
  float true_last_location[3];
  float location[3];
  float last_location[3];

  /* Original pixel radius with the pressure curve applied for dyntopo detail size */
  float dyntopo_pixel_radius;

  bool is_last_valid;

  bool pen_flip;
  bool invert;
  float pressure;
  float mouse[2];
  float bstrength;
  float normal_weight; /* from brush (with optional override) */

  /* The rest is temporary storage that isn't saved as a property */

  bool first_time; /* Beginning of stroke may do some things special */

  /* from ED_view3d_ob_project_mat_get() */
  float projection_mat[4][4];

  /* Clean this up! */
  struct ViewContext *vc;
  const struct Brush *brush;

  float special_rotation;
  float grab_delta[3], grab_delta_symmetry[3];
  float old_grab_location[3], orig_grab_location[3];

  /* screen-space rotation defined by mouse motion */
  float rake_rotation[4], rake_rotation_symmetry[4];
  bool is_rake_rotation_valid;
  struct SculptRakeData rake_data;

  /* Symmetry index between 0 and 7 bit combo 0 is Brush only;
   * 1 is X mirror; 2 is Y mirror; 3 is XY; 4 is Z; 5 is XZ; 6 is YZ; 7 is XYZ */
  int symmetry;
  int mirror_symmetry_pass; /* the symmetry pass we are currently on between 0 and 7*/
  float true_view_normal[3];
  float view_normal[3];

  /* sculpt_normal gets calculated by calc_sculpt_normal(), then the
   * sculpt_normal_symm gets updated quickly with the usual symmetry
   * transforms */
  float sculpt_normal[3];
  float sculpt_normal_symm[3];

  /* Used for area texture mode, local_mat gets calculated by
   * calc_brush_local_mat() and used in tex_strength(). */
  float brush_local_mat[4][4];

  float plane_offset[3]; /* used to shift the plane around when doing tiled strokes */
  int tile_pass;

  float last_center[3];
  int radial_symmetry_pass;
  float symm_rot_mat[4][4];
  float symm_rot_mat_inv[4][4];
  bool original;
  float anchored_location[3];

  /* Pose brush */
  struct PoseIKChain *pose_ik_chain;

  float vertex_rotation; /* amount to rotate the vertices when using rotate brush */
  struct Dial *dial;

  char saved_active_brush_name[MAX_ID_NAME];
  char saved_mask_brush_tool;
  int saved_smooth_size; /* smooth tool copies the size of the current tool */
  bool alt_smooth;

  float plane_trim_squared;

  bool supports_gravity;
  float true_gravity_direction[3];
  float gravity_direction[3];

  float *automask;

  float stroke_local_mat[4][4];
  float multiplane_scrape_sampled_angle;

  rcti previous_r; /* previous redraw rectangle */
  rcti current_r;  /* current redraw rectangle */

} StrokeCache;

typedef struct FilterCache {
  bool enabled_axis[3];
  int random_seed;

  /* unmasked nodes */
  PBVHNode **nodes;
  int totnode;

  /* mask expand iteration caches */
  int mask_update_current_it;
  int mask_update_last_it;
  int *mask_update_it;
  float *normal_factor;
  float *edge_factor;
  float *prev_mask;
  float mask_expand_initial_co[3];
} FilterCache;

void sculpt_cache_calc_brushdata_symm(StrokeCache *cache,
                                      const char symm,
                                      const char axis,
                                      const float angle);
void sculpt_cache_free(StrokeCache *cache);

SculptUndoNode *sculpt_undo_push_node(Object *ob, PBVHNode *node, SculptUndoType type);
SculptUndoNode *sculpt_undo_get_node(PBVHNode *node);
void sculpt_undo_push_begin(const char *name);
void sculpt_undo_push_end(void);

void sculpt_vertcos_to_key(Object *ob, KeyBlock *kb, const float (*vertCos)[3]);

void sculpt_update_object_bounding_box(struct Object *ob);

bool sculpt_get_redraw_rect(struct ARegion *ar, struct RegionView3D *rv3d, Object *ob, rcti *rect);

#endif
