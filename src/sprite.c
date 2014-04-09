#include "sprite.h"

#include <assert.h>
#include <GL/glew.h>

#include "entitypool.h"
#include "dirs.h"
#include "mat3.h"
#include "saveload.h"
#include "transform.h"
#include "gfx.h"
#include "camera.h"
#include "texture.h"
#include "edit.h"

typedef struct Sprite Sprite;
struct Sprite
{
    EntityPoolElem pool_elem;

    Mat3 wmat; /* world transform matrix to send to shader */

    Vec2 cell;
    Vec2 size;

    int depth;
};

static EntityPool *pool;

/* ------------------------------------------------------------------------- */

void sprite_add(Entity ent)
{
    Sprite *sprite;

    if (entitypool_get(pool, ent))
        return; /* already has a sprite */

    transform_add(ent);

    sprite = entitypool_add(pool, ent);
    sprite->cell = vec2(32.0f, 32.0f);
    sprite->size = vec2(32.0f, 32.0f);
    sprite->depth = 0;
}
void sprite_remove(Entity ent)
{
    entitypool_remove(pool, ent);
}

void sprite_set_cell(Entity ent, Vec2 cell)
{
    Sprite *sprite = entitypool_get(pool, ent);
    assert(sprite);
    sprite->cell = cell;
}
Vec2 sprite_get_cell(Entity ent)
{
    Sprite *sprite = entitypool_get(pool, ent);
    assert(sprite);
    return sprite->cell;
}
void sprite_set_size(Entity ent, Vec2 size)
{
    Sprite *sprite = entitypool_get(pool, ent);
    assert(sprite);
    sprite->size = size;
}
Vec2 sprite_get_size(Entity ent)
{
    Sprite *sprite = entitypool_get(pool, ent);
    assert(sprite);
    return sprite->size;
}

void sprite_set_depth(Entity ent, int depth)
{
    Sprite *sprite = entitypool_get(pool, ent);
    assert(sprite);
    sprite->depth = depth;
}
int sprite_get_depth(Entity ent)
{
    Sprite *sprite = entitypool_get(pool, ent);
    assert(sprite);
    return sprite->depth;
}

/* ------------------------------------------------------------------------- */

static GLuint program;
static GLuint vao;
static GLuint vbo;

void sprite_init()
{
    Vec2 atlas_size;

    /* initialize pool */
    pool = entitypool_new(Sprite);

    /* create shader program, load atlas, bind parameters */
    program = gfx_create_program(data_path("sprite.vert"),
                                 data_path("sprite.geom"),
                                 data_path("sprite.frag"));
    glUseProgram(program);
    texture_load(data_path("test/atlas.png"));
    glUniform1i(glGetUniformLocation(program, "tex0"), 0);
    atlas_size = texture_get_size(data_path("test/atlas.png"));
    glUniform2fv(glGetUniformLocation(program, "atlas_size"), 1,
                 (const GLfloat *) &atlas_size);

    /* make vao, vbo, bind attributes */
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    gfx_bind_vertex_attrib(program, GL_FLOAT, 3, "wmat1", Sprite, wmat.m[0]);
    gfx_bind_vertex_attrib(program, GL_FLOAT, 3, "wmat2", Sprite, wmat.m[1]);
    gfx_bind_vertex_attrib(program, GL_FLOAT, 3, "wmat3", Sprite, wmat.m[2]);
    gfx_bind_vertex_attrib(program, GL_FLOAT, 2, "cell", Sprite, cell);
    gfx_bind_vertex_attrib(program, GL_FLOAT, 2, "size", Sprite, size);
}

void sprite_deinit()
{
    /* clean up GL stuff */
    glDeleteProgram(program);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    /* deinit pool */
    entitypool_free(pool);
}

void sprite_update_all()
{
    Sprite *sprite;
    static BBox bbox = { { -0.5, -0.5 }, { 0.5, 0.5 } };

    entitypool_remove_destroyed(pool, sprite_remove);

    /* update world transform matrices */
    entitypool_foreach(sprite, pool)
        sprite->wmat = transform_get_world_matrix(sprite->pool_elem.ent);

    /* update edit bbox */
    if (edit_get_enabled())
        entitypool_foreach(sprite, pool)
            edit_bboxes_update(sprite->pool_elem.ent, bbox);
}

static int _depth_compare(const void *a, const void *b)
{
    const Sprite *sa = a, *sb = b;

    /* descending, break ties by Entity id for stability */
    if (sb->depth == sa->depth)
        return ((int) sa->pool_elem.ent.id) - ((int) sb->pool_elem.ent.id);
    return sb->depth - sa->depth;
}

void sprite_draw_all()
{
    unsigned int nsprites;

    /* depth sort */
    entitypool_sort(pool, _depth_compare);

    /* bind program, update uniforms */
    glUseProgram(program);
    glUniformMatrix3fv(glGetUniformLocation(program, "inverse_view_matrix"),
                       1, GL_FALSE,
                       (const GLfloat *) camera_get_inverse_view_matrix_ptr());

    /* bind atlas */
    glActiveTexture(GL_TEXTURE0);
    texture_bind(data_path("test/atlas.png"));

    /* draw! */
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    nsprites = entitypool_size(pool);
    glBufferData(GL_ARRAY_BUFFER, nsprites * sizeof(Sprite),
                 entitypool_begin(pool), GL_STREAM_DRAW);
    glDrawArrays(GL_POINTS, 0, nsprites);
}

void sprite_save_all(Serializer *s)
{
    Sprite *sprite;

    entitypool_foreach(sprite, pool)
    {
        if (!entity_get_save_filter(sprite->pool_elem.ent))
            continue;
        loop_continue_save(s);

        entitypool_elem_save(pool, &sprite, s);
        mat3_save(&sprite->wmat, s);
        vec2_save(&sprite->cell, s);
        vec2_save(&sprite->size, s);
    }
    loop_end_save(s);
}
void sprite_load_all(Deserializer *s)
{
    Sprite *sprite;

    while (loop_continue_load(s))
    {
        entitypool_elem_load(pool, &sprite, s);
        mat3_load(&sprite->wmat, s);
        vec2_load(&sprite->cell, s);
        vec2_load(&sprite->size, s);
    }
}

