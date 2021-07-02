#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include "track_info.h"
#include "list.h"
#include "utils.h"

typedef struct track_info_per_player {
    TrackInfoPlayer player;
    TrackInfo track;
    bool playing;
} TrackInfoPerPlayer;

list players;

static int players_name_cmp(TrackInfoPerPlayer* a, TrackInfoPerPlayer* b) {
    return strcmp(a->player.name, b->player.name);
}

void track_info_init() {
    list_init(&players);
}
TrackInfo* track_info_get_best_cantidate() {
    struct list_element* curr = players;
    TrackInfo* best_candidate = NULL;

    while (curr != NULL) {
        TrackInfoPerPlayer* e = curr->element;

        if (e->playing) {
            if (best_candidate == NULL) {
                best_candidate = &(e->track);
            } else {
                if (best_candidate->update_time < e->track.update_time) {
                    best_candidate = &(e->track);
                }
            }
        }
        curr = curr->next;
    }
    return best_candidate;
}

TrackInfoPlayer** track_info_get_players(int* ret_nb) {
    struct list_element* curr = players;
    int nb_player = list_size(players);

    TrackInfoPlayer** ret = malloc(sizeof(TrackInfoPlayer*) * nb_player);
    allocfail_return_null(ret);

    *ret_nb = 0;
    while (curr != NULL) {
        TrackInfoPerPlayer* e = curr->element;
        ret[*ret_nb] = &(e->player);
        curr = curr->next;
        *ret_nb = *ret_nb + 1;
    }
    assert(nb_player == *ret_nb);
    return ret;
}

static void track_info_dup(TrackInfo t, TrackInfo* ret) {
    ret->album = strdup(t.album);
    ret->artist = strdup(t.artist);
    ret->title = strdup(t.title);
    ret->album_art_url = strdup(t.album_art_url);
    ret->update_time = t.update_time;
}

void track_info_free(TrackInfo* ti) {
    efree(ti->artist);
    efree(ti->album);
    efree(ti->title);
    efree(ti->album_art_url);
}

void track_info_register_player(const char* name, const char* fancy_name){
    TrackInfoPerPlayer* track_info = malloc(sizeof(TrackInfoPerPlayer));;
    allocfail_return(track_info);
    track_info->player.name = strdup(name);
    allocfail_return(track_info->player.name);
    track_info->player.fancy_name = strdup(fancy_name);
    allocfail_return(track_info->player.fancy_name);
    list_prepend(&players, track_info, sizeof(TrackInfoPerPlayer));
}

void track_info_unregister_player(const char* name) {
    TrackInfoPerPlayer h = {.player.name = name};
    list_remove(&players, &h, (list_cmpfunc) players_name_cmp);
}

static TrackInfoPerPlayer* track_info_get_for_player(const char* name) {
    TrackInfoPerPlayer* track_info = NULL;
    TrackInfoPerPlayer h = {.player.name = name};
    if (list_search(&players, &h, (list_cmpfunc) players_name_cmp, (void**) &track_info)) {
        printf("Found already registered player %s (%s)\n", name, track_info->player.fancy_name);
    } else {
        printf("Unknown player %s\n", name);
    }
    return track_info;
}

void track_info_register_track_change(const char* name, TrackInfo track) {
    TrackInfoPerPlayer* track_info = track_info_get_for_player(name);
    if (track_info == NULL) return;

    track_info_dup(track, &(track_info->track));
}

void track_info_register_state_change(const char* name, bool playing) {
    TrackInfoPerPlayer* track_info = track_info_get_for_player(name);
    if (track_info == NULL) return;

    track_info->playing = playing;
}

void track_info_print_players() {
    struct list_element* curr = players;

    int i = 0;
    while (curr != NULL) {
        TrackInfoPerPlayer* e = curr->element;
        printf("Player %d: %s (%s)\n", i, e->player.name, e->player.fancy_name);
        printf("> ");
        track_info_print(e->track);
        printf("playing: %d\n\n", e->playing);

        i = i + 1;
        curr = curr->next;
    }
}

void track_info_print(TrackInfo ti) {
    printf("%s - \"%s\" from %s, art: %s\n", ti.artist, ti.title, ti.album, ti.album_art_url);
}
