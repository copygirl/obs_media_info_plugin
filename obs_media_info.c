#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <obs/obs-module.h>
#include <obs/util/bmem.h>
#include <obs/util/platform.h>
#include <obs/util/threading.h>
#include <obs/graphics/graphics.h>
#include "track_info.h"

extern TrackInfo current_track; //TODO: wrapper to get current_track of interest
void mpris_init();
int mpris_process();

typedef struct source {
    bool live;
    uint32_t width;
    uint32_t height;
    pthread_t update_thread;
    bool end_update_thread;
    pthread_mutex_t* texture_mutex;
    gs_texture_t* texture;
} obsmed_source;

const char* obsmed_get_name(void* type_data) {
    return "Media infos";
}


void* update_func(void* arg) {
    obsmed_source* source = arg;
    char* last_track_url = strdup("");
    time_t last_update_time = time(NULL);

    while (! source->end_update_thread) {
        mpris_process(); // Get new data into current_track

        if (current_track.update_time > last_update_time) {
            last_update_time = current_track.update_time;

            if (current_track.album_art_url != NULL && strcmp(last_track_url, current_track.album_art_url) != 0) {
                if (last_track_url != NULL) free(last_track_url);

                pthread_mutex_lock(source->texture_mutex);
                obs_enter_graphics();
                if (source->texture != NULL) gs_texture_destroy(source->texture);
                source->texture = gs_texture_create_from_file(current_track.album_art_url); //TODO: texture from http only works with obs's ffmpeg backend not with imageMagic.
                if (source->texture == NULL) puts("error loading texture");
                obs_leave_graphics();
                pthread_mutex_unlock(source->texture_mutex);

                last_track_url = strdup(current_track.album_art_url);
            }

            char text2[200];
            snprintf(text2, 199, "%s\n%s\n%s", current_track.title, current_track.album, current_track.artist);
            obs_source_t* text = obs_get_source_by_name("toto");
            obs_data_t* tdata = obs_data_create();
            obs_data_set_string(tdata, "text", text2);
            obs_source_update(text, tdata);
            obs_data_release(tdata);
            obs_source_release(text);
        }
        os_sleep_ms(500);
    }
    pthread_exit(NULL);
}

void* obsmed_create(obs_data_t *settings, obs_source_t *source) {
    obsmed_source* data = bmalloc(sizeof(obsmed_source));
    mpris_init();

    data->width = 300;
    data->height = 300;
    data->texture = NULL;

    data->end_update_thread = false;
    data->texture_mutex = bmalloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(data->texture_mutex, NULL);
    if(pthread_create(&(data->update_thread), NULL, update_func, data)) {
        fprintf(stderr, "Error creating thread\n");
        return NULL;
    }

    return data;
}

void obsmed_destroy(void* d) {
    obsmed_source* data = d;
    data->end_update_thread = true;
    pthread_join(data->update_thread, NULL);
    pthread_mutex_destroy(data->texture_mutex);
    bfree(data->texture_mutex);
    gs_texture_destroy(data->texture);
    bfree(data);
}

uint32_t obsmed_get_width(void* data) {
    obsmed_source* d = data;
    return d->width;
}

uint32_t obsmed_get_height(void* data) {
    obsmed_source* d = data;
    return d->height;
}


static obs_properties_t* obsmed_get_properties(void *data)
{
    obsmed_source* d = data;
    obs_properties_t *props = obs_properties_create();

    obs_properties_add_font(props, "font", obs_module_text("Font"));
    obs_properties_add_text(props, "text", obs_module_text("Text"), OBS_TEXT_MULTILINE);

    return props;
}


void obsmed_video_render(void *data, gs_effect_t *effect) {
     obsmed_source* d = data;

     if (pthread_mutex_trylock(d->texture_mutex) == 0) {
         obs_source_draw(d->texture, 0, 0, d->width, d->height, false);
         pthread_mutex_unlock(d->texture_mutex);
     }
}


struct obs_source_info obs_media_info = {
    .id = "obs_media_info",
    .type = OBS_SOURCE_TYPE_INPUT,
    .output_flags = OBS_SOURCE_VIDEO,
    .create = obsmed_create,
    .destroy = obsmed_destroy,
    .video_render = obsmed_video_render,
    .get_name = obsmed_get_name,
    .get_width = obsmed_get_width,
    .get_height = obsmed_get_height,
    /*.update = obsmed_update,
    .get_defaults = obsmed_get_defaults,*/
    .get_properties = obsmed_get_properties,
    .icon_type = OBS_ICON_TYPE_CUSTOM,
};



OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs_media_info", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
    return "Plugin to display the cover artwork and basic informations on the currently playing sound";
}

bool obs_module_load(void)
{
    obs_register_source(&obs_media_info);
    return true;
}