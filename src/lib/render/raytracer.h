#pragma once

#include "lib/core/types.h"
#include "lib/globals/raytracing.h"
#include "lib/shapes/line.h"
#include "lib/shapes/bbox.h"
#include "lib/shapes/helix.h"
#include "lib/input/keyboard.h"
#include "lib/controllers/fps.h"
#include "lib/controllers/orb.h"
#include "lib/controllers/camera_controller.h"
#include "lib/nodes/camera.h"
#include "lib/memory/allocators.h"

#include "lib/render/shaders/closest_hit/debug.h"
#include "lib/render/shaders/closest_hit/classic.h"
#include "lib/render/shaders/closest_hit/surface.h"
#include "lib/render/shaders/intersection/cube.h"
#include "lib/render/shaders/intersection/plane.h"
#include "lib/render/shaders/intersection/sphere.h"
#include "lib/render/shaders/intersection/tetrahedra.h"
#include "lib/render/shaders/ray_generation/primary_rays.h"

#include "BVH.h"
#include "SSB.h"

#ifdef __CUDACC__
__device__
__host__
__forceinline__
#else
inline
#endif
void tracePrimaryRay(Ray *ray, Scene *scene, GeometryBounds *bounds, Masks *masks, u16 x, u16 y) {
    ray->hit.uv.x = ray->hit.uv.y = 1;
    ray->hit.distance = MAX_DISTANCE;
    ray->masks = *masks;
    setRayVisibilityMasksFromBounds(&ray->masks, masks, bounds, x, y);

    hitPlanes(scene->planes, ray);
    if (ray->masks.visibility.cubes) hitCubes(scene->cubes, scene->cube_indices, ray, false);
    if (ray->masks.visibility.spheres) hitSpheres(scene->spheres, ray, false);
    if (ray->masks.visibility.tetrahedra) hitTetrahedra(scene->tetrahedra, scene->tetrahedron_indices, ray, false);
}

#ifdef __CUDACC__
__device__
__host__
__forceinline__
#else
inline
#endif
void renderBeauty(Ray *ray, Scene *scene, BVHNode *bvh_nodes, GeometryBounds *bounds, Masks *masks, u16 x, u16 y, Pixel* pixel) {
    tracePrimaryRay(ray, scene, bounds, masks, x, y);

    vec3 color;
    fillVec3(&color, 0);
//    shadeSurface(scene, bvh, masks, ray->hit.material_id, ray->direction,  &ray->hit.position, &ray->hit.normal, &color);
    shadeLambert(scene, bvh_nodes, masks, ray->direction, &ray->hit.position, &ray->hit.normal, &color);
//    shadePhong(scene, bvh, masks, ray->direction, &ray->hit.position, &ray->hit.normal &color);
//    shadeBlinn(scene, bvh, masks, ray->direction, &ray->hit.position, &ray->hit.normal, &color);
    setPixelColor(pixel, color);
}

#ifdef __CUDACC__
__device__
__host__
__forceinline__
#else
inline
#endif
void renderNormals(Ray *ray, Scene *scene, BVHNode *bvh_nodes, GeometryBounds *bounds, Masks *masks, u16 x, u16 y, Pixel* pixel) {
    tracePrimaryRay(ray, scene, bounds, masks, x, y);

    vec3 color;
    shadeDirection(&ray->hit.normal, &color);
    setPixelColor(pixel, color);
}

#ifdef __CUDACC__
__device__
__host__
__forceinline__
#else
inline
#endif
void renderDepth(Ray *ray, Scene *scene, BVHNode *bvh_nodes, GeometryBounds *bounds, Masks *masks, u16 x, u16 y, Pixel* pixel) {
    tracePrimaryRay(ray, scene, bounds, masks, x, y);

    vec3 color;
    shadeDepth(ray->hit.distance, &color);
    setPixelColor(pixel, color);
}

#ifdef __CUDACC__
__device__
__host__
__forceinline__
#else
inline
#endif
void renderUVs(Ray *ray, Scene *scene, BVHNode *bvh_nodes, GeometryBounds *bounds, Masks *masks, u16 x, u16 y, Pixel* pixel) {
    tracePrimaryRay(ray, scene, bounds, masks, x, y);

    vec3 color;
    shadeUV(ray->hit.uv, &color);
    setPixelColor(pixel, color);
}

#ifdef __CUDACC__

#define initShader(C, W) \
    initKernel(C, W);    \
                         \
    Pixel *pixel = (Pixel *)&d_pixels[i]; \
                         \
    Ray ray; \
    ray.direction = &d_ray_directions[i]; \
    ray.origin = &Ro; \
    ray.hit.distance = MAX_DISTANCE; \
                         \
    Scene scene;         \
    scene.geo_counts = d_geo_counts; \
    scene.materials = d_materials; \
    scene.point_lights = d_point_lights; \
    scene.tetrahedra = d_tetrahedra; \
    scene.spheres = d_spheres; \
    scene.planes = d_planes; \
    scene.cubes = d_cubes; \
    scene.ambient_light = d_ambient_light;\
    scene.cube_indices = d_cube_indices;\
    scene.tetrahedron_indices = d_tetrahedron_indices

__global__ void d_renderUVs(     u16 W, u32 C, vec3 Ro) { initShader(C, W); renderUVs(     &ray, &scene, d_bvh_nodes, d_ssb_bounds, d_masks, x, y, pixel); }
__global__ void d_renderDepth(   u16 W, u32 C, vec3 Ro) { initShader(C, W); renderDepth(   &ray, &scene, d_bvh_nodes, d_ssb_bounds, d_masks, x, y, pixel); }
__global__ void d_renderBeauty(  u16 W, u32 C, vec3 Ro) { initShader(C, W); renderBeauty(  &ray, &scene, d_bvh_nodes, d_ssb_bounds, d_masks, x, y, pixel); }
__global__ void d_renderNormals( u16 W, u32 C, vec3 Ro) { initShader(C, W); renderNormals( &ray, &scene, d_bvh_nodes, d_ssb_bounds, d_masks, x, y, pixel); }

void renderOnGPU(Scene *scene, Camera *camera) {
    u32 count = frame_buffer.size;
    setupKernel(count)

    vec3 Ro = camera->transform.position;

    switch (render_mode) {
        case Beauty    : d_renderBeauty<<< blocks, threads>>>(frame_buffer.width, count, Ro); break;
        case Depth     : d_renderDepth<<<  blocks, threads>>>(frame_buffer.width, count, Ro); break;
        case Normals   : d_renderNormals<<<blocks, threads>>>(frame_buffer.width, count, Ro); break;
        case UVs       : d_renderUVs<<<    blocks, threads>>>(frame_buffer.width, count, Ro); break;
    }

    copyPixelsFromGPUtoCPU((u32*)frame_buffer.pixels, count);
}
#endif

#define runShaderOnCPU(shader) { \
    for (u16 y = 0; y < frame_buffer.height; y++) { \
        for (u16 x = 0; x < frame_buffer.width; x++, pixel++, ray.direction++, ray.direction_rcp++) { \
            shader(&ray, scene, ray_tracer.bvh.nodes, &ray_tracer.ssb.bounds, &ray_tracer.masks, x, y, pixel); \
        }  \
    }  \
}

void renderOnCPU(Scene *scene, Camera *camera) {
    Pixel* pixel = frame_buffer.pixels;
    Ray ray;
    ray.origin = &camera->transform.position;
    ray.direction = ray_tracer.ray_directions;
    ray.direction_rcp = ray_tracer.ray_directions_rcp;

    switch (render_mode) {
        case Beauty    : runShaderOnCPU(renderBeauty)  break;
        case Depth     : runShaderOnCPU(renderDepth)   break;
        case Normals   : runShaderOnCPU(renderNormals) break;
        case UVs       : runShaderOnCPU(renderUVs)     break;
    }
}

void onZoom() {
    generateRayDirections();
    current_camera_controller->moved = true;
    current_camera_controller->zoomed = false;
}

void onTurn() {
    generateRayDirections();
    transposeMat3(&current_camera_controller->camera->transform.rotation_matrix,
                  &current_camera_controller->camera->transform.rotation_matrix_inverted);
    current_camera_controller->turned = false;
    current_camera_controller->moved = true;
}

void onMove(Scene* scene) {
    vec3 *geo_position_in_view_space;
    vec3 *cam_position = &current_camera_controller->camera->transform.position;
    mat3 *cam_rotation = &current_camera_controller->camera->transform.rotation_matrix_inverted;
    Node *node, **node_ptr;
    u8 geo_count;

    for (u8 geo_type = 0; geo_type < GEO_TYPE_COUNT; geo_type++) {
        switch (geo_type) {
            case GeoTypeCube:
                geo_count = CUBE_COUNT;
                geo_position_in_view_space = ray_tracer.ssb.view_positions.cubes;
                node_ptr = scene->node_ptrs.cubes;
                break;
            case GeoTypeSphere:
                geo_count = SPHERE_COUNT;
                geo_position_in_view_space = ray_tracer.ssb.view_positions.spheres;
                node_ptr = scene->node_ptrs.spheres;
                break;
            case GeoTypeTetrahedron:
                geo_count = TETRAHEDRON_COUNT;
                geo_position_in_view_space = ray_tracer.ssb.view_positions.tetrahedra;
                node_ptr = scene->node_ptrs.tetrahedra;
                break;
            default:
                continue;
        }
        for (u8 i = 0; i < geo_count; i++, geo_position_in_view_space++, node_ptr++) {
            node = *node_ptr;
            subVec3(&node->position, cam_position, geo_position_in_view_space);
            imulVec3Mat3(geo_position_in_view_space, cam_rotation);
        }
    }
    updateSceneMasks(scene, &ray_tracer.ssb, &ray_tracer.masks, current_camera_controller->camera->focal_length);

    current_camera_controller->moved = false;
}

void onResize(Scene *scene) {
    generateRayDirections();
    onMove(scene);
}

void draw3DLineSegment(vec3 *start, vec3 *end, Camera *camera, Pixel *pixel) {
    f32 x_factor = camera->focal_length;
    f32 y_factor = camera->focal_length * frame_buffer.width_over_height;

    vec3 *cam_pos = &camera->transform.position;
    mat3 *cam_rot = &camera->transform.rotation_matrix_inverted;

    vec3 projected_start, projected_end;

    subVec3(start, cam_pos, &projected_start);
    subVec3(end, cam_pos, &projected_end);
    imulVec3Mat3(&projected_start, cam_rot);
    imulVec3Mat3(&projected_end, cam_rot);

    projectEdge(&projected_start,
                &projected_end,
                x_factor,
                y_factor);

    drawLine2D((i32)projected_start.x,
               (i32)projected_start.y,
               (i32)projected_end.x,
               (i32)projected_end.y,
               pixel);
}

void draw3DShape(vec3 *vertices, u8 vertex_count, Camera *camera, Pixel *pixel) {
    for (u8 v = 0; v < vertex_count; v++)
        draw3DLineSegment(vertices + v, vertices + ((v + 1) % vertex_count), camera, pixel);
}

void drawCube(Cube *cube, QuadIndices *indices, Camera *camera, Pixel *pixel) {
    vec3 quad[4];
    for (u8 q = 0; q < 6; q++) {
        quad[0] = cube->vertices[indices[q].v1];
        quad[1] = cube->vertices[indices[q].v2];
        quad[2] = cube->vertices[indices[q].v3];
        quad[3] = cube->vertices[indices[q].v4];
        draw3DShape(quad, 4, camera, pixel);
    }
}

void onRender(Scene *scene, Camera *camera) {

#ifdef __CUDACC__
    if (use_GPU) {
        if (last_rendered_on_CPU) generateRayDirections();

        renderOnGPU(scene, camera);

        last_rendered_on_CPU = false;
    } else {
        if (!last_rendered_on_CPU) generateRayDirections();

        renderOnCPU(scene, camera);

        last_rendered_on_CPU = true;
    }
#else
    renderOnCPU(scene, camera);
#endif
    Pixel pixel;
    pixel.color.R = MAX_COLOR_VALUE;
    pixel.color.G = MAX_COLOR_VALUE;
    pixel.color.B = 0;
    pixel.color.A = 0;

    if (show_BVH) drawBVH(&ray_tracer.bvh, camera);
    if (show_SSB) drawSSB(&ray_tracer.ssb, &pixel);

//    for (u8 c = 0; c < CUBE_COUNT; c++)
//        drawCube(scene->cubes + c, scene->cube_indices, camera, &pixel);
}


void initRayTracer(Scene *scene) {
    initBVH(&ray_tracer.bvh, 3);
    updateBVH(&ray_tracer.bvh, scene);

    ray_tracer.rays_per_pixel = 1;
    ray_tracer.ray_count = ray_tracer.rays_per_pixel * MAX_WIDTH * MAX_HEIGHT;
    ray_tracer.ray_directions     = AllocN(vec3, ray_tracer.ray_count);
    ray_tracer.ray_directions_rcp = AllocN(vec3, ray_tracer.ray_count);

    Node *node, **node_ptr;
    u8 node_id, geo_count, *shadowing, *visibility, *transparency;
    for (u8 geo_type = 0; geo_type < GEO_TYPE_COUNT; geo_type++) {
        node_id = 1;

        switch (geo_type) {
            case GeoTypeCube:
                geo_count = CUBE_COUNT;
                shadowing    = &ray_tracer.masks.shadowing.cubes;
                visibility   = &ray_tracer.masks.visibility.cubes;
                transparency = &ray_tracer.masks.transparency.cubes;
                node_ptr = scene->node_ptrs.cubes;
                break;
            case GeoTypeSphere:
                geo_count = SPHERE_COUNT;
                shadowing    = &ray_tracer.masks.shadowing.spheres;
                visibility   = &ray_tracer.masks.visibility.spheres;
                transparency = &ray_tracer.masks.transparency.spheres;
                node_ptr = scene->node_ptrs.spheres;
                break;
            case GeoTypeTetrahedron:
                geo_count = TETRAHEDRON_COUNT;
                shadowing    = &ray_tracer.masks.shadowing.tetrahedra;
                visibility   = &ray_tracer.masks.visibility.tetrahedra;
                transparency = &ray_tracer.masks.transparency.tetrahedra;
                node_ptr = scene->node_ptrs.tetrahedra;
                break;
            default:
                continue;
        }

        *shadowing    = 0;
        *visibility   = 0;
        *transparency = 0;

        for (u8 i = 0; i < geo_count; i++, node_id <<= (u8)1, node_ptr++) {
            node = *node_ptr;
            if (scene->materials[node->geo.material_id].uses & (u8) TRANSPARENCY)
                *transparency |= node_id;
            *visibility |= node_id;
        }
    }

    ray_tracer.masks.shadowing.spheres &= (u8)(~((u8)((u8)1 << (u8)3)));
    ray_tracer.masks.shadowing.tetrahedra = FULL_MASK;
}


//#ifdef __CUDACC__
//__device__
//__host__
//__forceinline__
//#else
//inline
//#endif
//u8 renderBeauty(Pixel* pixel, u16 x, u16 y, Ray *ray, Scene *scene) {
//    u8 sphere_visibility_mask = getSphereVisibilityMask(scene->spheres, scene->masks->visibility, x, y);
//
//    vec3 *Rd = ray->direction,
//            *N = &ray->hit.normal,
//            *P = &ray->hit.position;
//
//    ray->hit.distance = MAX_DISTANCE;
//    tracePrimaryRay(ray, scene, sphere_visibility_mask);
//
//    vec3 color, RLd;
//    fillVec3(&color, 0);
//    Material* hit_material = &scene->materials[ray->hit.material_id];
//
//    f32 NdotRd = dotVec3(N, Rd);
//    bool from_behind = NdotRd > 0;
//    NdotRd = -saturate(from_behind ? NdotRd : -NdotRd);
//    if (from_behind) iscaleVec3(N, -1);
//    reflect(Rd, N, NdotRd, &RLd);
//
//    shadeSurface(scene, hit_material, Rd,  &RLd, P, N, &color);

//    vec3 *N, *Rd, *P, L, H, color, scaled_light_color, ambient_color = scene->aux->ambient_color;
//    Material *material;
//    u8 uses, exp;
//    bool from_behind,
//         using_phong,
//         using_blinn,
//         has_diffuse,
//         has_specular,
//         has_reflection,
//         has_refraction,
//         has_rfl_or_rfr;
//    f32 di, si, li, NdotRd,
//        reflection_amount,
//        light_distance_squared,
//        light_distance,
//        diffuse_light,
//        specular_light;
//
//    // Shade primary rays
//    hitPlanes(scene->planes, &hit);
//    if (sphere_visibility_mask)
//        hitSpheresSimple(scene->spheres, true, sphere_visibility_mask, &hit);
//
//    RayHit hit;
//    initRayHit(hit, ray_origin);
//    hit.ray_direction = ray_direction;
//
//    bool reflected = false;
//    bool refracted = false;
//    RayHit refracted_hit,
//           reflected_hit;
//    vec3 *RLd = &reflected_hit.ray_direction;
//    vec3 *RRd = &refracted_hit.ray_direction;
//
//    for (u8 hit_depth = 0; hit_depth < MAX_HIT_DEPTH; hit_depth++) {
//        hitPlanes(scene->planes, &hit);
//        if (sphere_visibility_mask)
//            hitSpheresSimple(scene->spheres, !hit_depth, sphere_visibility_mask, &hit);
//
//        material = &scene->materials[hit.material_id];
//        uses = material->uses;
//
//        using_phong = uses & (u8)PHONG;
//        using_blinn = uses & (u8)BLINN;
//        has_diffuse = uses & (u8)LAMBERT;
//        has_specular = using_phong || using_blinn;
//        has_reflection = uses & (u8)REFLECTION;
//        has_refraction = uses & (u8)REFRACTION;
//
//        di = material->diffuse_intensity;
//        si = material->specular_intensity;
//        exp = material->specular_exponent * (using_blinn ? (u8)4 : (u8)1);
//
//        N = &hit.normal,
//        Rd = &hit.ray_direction,
//        P = &hit.position;
//
//        if (has_reflection || has_refraction || using_phong) {
//            NdotRd = dotVec3(N, Rd);
//            from_behind = NdotRd > 0;
//            NdotRd = -saturate(from_behind ? NdotRd : -NdotRd);
//            if (from_behind) iscaleVec3(N, -1);
//            reflect(Rd, N, NdotRd, RLd);
//        }
//
//        if (has_diffuse || has_specular){
//            color = ambient_color;
//        }
//
//        if (has_reflection || has_refraction) {
//            color.x = color.y = color.z = 0;
//
//            if (has_reflection) {
//                reflected = true;
//                initRayHit(reflected_hit, hit.position);
//            }
//
//            if (has_refraction) {
//                refracted = true;
//                initRayHit(refracted_hit, hit.position);
//                refract(Rd, N, NdotRd, from_behind ? scene->aux->n2_over_n1 : scene->aux->n1_over_n2, RRd);
//            }
//
//            if (has_reflection && has_refraction) {
//                reflection_amount = schlickFresnel(from_behind ? IOR_GLASS : IOR_AIR, from_behind ? IOR_AIR : IOR_GLASS, NdotRd);
////                iscaleVec3(&reflected_color, reflection_amount);
////                iscaleVec3(&refracted_color, 1 - reflection_amount);
//            }
//        } else break;
//    }
//
//
//    if (has_reflection) iaddVec3(out_color, &reflected_color);
//    if (has_refraction) iaddVec3(out_color, &refracted_color);
//
//    vec3 color; color.x = color.y = color.z = 0;
//    if (scene->materials[hit->material_id].uses & (u8)PHONG)
//        shadePhong(scene, hit, &color);
//    else
//        shadeLambert(scene, hit, &color);
////    else shade(scene, &hit, &color);
////                hitCubes(&hit);
//
////                perfStart(&aux_timer);
////                if (shade_normals)
////                    hitImplicitTetrahedra(scene->tetrahedra, &hit);
////                else
////                    hitTetrahedra(scene->tetrahedra, &hit);
////                perfEnd(&aux_timer, aux_timer.accumulated_ticks >= ticks_per_second, i == frame_buffer.size);
//
////        shadeLambert(scene, &hit, &color);
//
//    setPixelColor(pixel, color);
//    return sphere_visibility_mask;
//}