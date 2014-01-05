#include "transform.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "entitymap.h"
#include "saveload.h"

typedef struct Transform Transform;
struct Transform
{
    Vec2 position;
    float rotation;
    Vec2 scale;

    Mat3 worldmat_cache; /* remember to update this! */
};

static EntityMap *emap;
static unsigned int n_transforms = 0;

/* ------------------------------------------------------------------------- */

static void _update_cache(Transform *transform)
{
    transform->worldmat_cache = mat3_scaling_rotation_translation(
            transform->scale,
            transform->rotation,
            transform->position
            );
}

void transform_add(Entity ent)
{
    Transform *transform;

    if (entitymap_get(emap, ent))
        return; /* already has a transform */

    transform = malloc(sizeof(Transform));
    transform->position = vec2(0.0f, 0.0f);
    transform->rotation = 0.0f;
    transform->scale = vec2(1.0f, 1.0f);

    entitymap_set(emap, ent, transform);

    ++n_transforms;
}
void transform_remove(Entity ent)
{
    free(entitymap_get(emap, ent));
    entitymap_set(emap, ent, NULL);

    --n_transforms;
}

void transform_set_position(Entity ent, Vec2 pos)
{
    Transform *transform = entitymap_get(emap, ent);
    assert(transform);
    transform->position = pos;
    _update_cache(transform);
}
Vec2 transform_get_position(Entity ent)
{
    Transform *transform = entitymap_get(emap, ent);
    assert(transform);
    return transform->position;
}
void transform_translate(Entity ent, Vec2 trans)
{
    Transform *transform = entitymap_get(emap, ent);
    assert(transform);
    transform->position = vec2_add(transform->position, trans);
    _update_cache(transform);
}

void transform_set_rotation(Entity ent, float rot)
{
    Transform *transform = entitymap_get(emap, ent);
    assert(transform);
    transform->rotation = rot;
    _update_cache(transform);
}
float transform_get_rotation(Entity ent)
{
    Transform *transform = entitymap_get(emap, ent);
    assert(transform);
    return transform->rotation;
}
void transform_rotate(Entity ent, float rot)
{
    Transform *transform = entitymap_get(emap, ent);
    assert(transform);
    transform->rotation += rot;
    _update_cache(transform);
}

void transform_set_scale(Entity ent, Vec2 scale)
{
    Transform *transform = entitymap_get(emap, ent);
    assert(transform);
    transform->scale = scale;
    _update_cache(transform);
}
Vec2 transform_get_scale(Entity ent)
{
    Transform *transform = entitymap_get(emap, ent);
    assert(transform);
    return transform->scale;
}

Mat3 transform_get_world_matrix(Entity ent)
{
    Transform *transform = entitymap_get(emap, ent);
    assert(transform);
    return transform->worldmat_cache;
}

/* ------------------------------------------------------------------------- */

void transform_init()
{
    emap = entitymap_new(NULL);
}
void transform_deinit()
{
    unsigned int i, max;
    Transform *transform;

    max = entitymap_get_max(emap);
    for (i = 0; i < max; ++i)
        free(entitymap_get(emap, i));

    entitymap_free(emap);
}

void transform_update_all()
{
    unsigned int max;
    Entity ent;
    Transform *curr;

    max = entitymap_get_max(emap);
    for (ent = 0; ent < max; ++ent)
        if ((curr = entitymap_get(emap, ent))
                && entity_destroyed(ent))
            transform_remove(ent);
}

void transform_save_all(FILE *file)
{
    unsigned int max;
    Entity ent;
    Transform *curr;

    uint_save(&n_transforms, file);

    max = entitymap_get_max(emap);
    for (ent = 0; ent < max; ++ent)
        if ((curr = entitymap_get(emap, ent)))
        {
            entity_save(&ent, file);
            vec2_save(&curr->position, file);
            scalar_save(&curr->rotation, file);
            vec2_save(&curr->scale, file);
            mat3_save(&curr->worldmat_cache, file);
        }
}
void transform_load_all(FILE *file)
{
    unsigned int max, i;
    Entity ent;
    Transform *curr;

    /* clear old */
    max = entitymap_get_max(emap);
    for (i = 0; i < max; ++i)
        free(entitymap_get(emap, i));
    entitymap_clear(emap);

    /* load new */
    uint_load(&n_transforms, file);
    for (i = 0; i < n_transforms; ++i)
    {
        entity_load(&ent, file);

        curr = malloc(sizeof(Transform));
        vec2_load(&curr->position, file);
        scalar_load(&curr->rotation, file);
        vec2_load(&curr->scale, file);
        mat3_load(&curr->worldmat_cache, file);

        entitymap_set(emap, ent, curr);
    }
}
