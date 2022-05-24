#ifndef __SCENE_H
#define __SCENE_H

typedef void (*void_function)(void);
typedef bool (*bool_function)(void);

typedef struct scene {
    void_function init;
    void_function setup;
    void_function render;
    void_function update;
    void_function events;
    void_function exit;
    bool_function is_enabled;

} t_scene;

#endif /* __SCENE_H */