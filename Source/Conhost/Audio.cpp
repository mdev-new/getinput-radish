#include "Audio.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

static HANDLE hAudioCommands = NULL;


enum sound_type {
	SOUND_EFFECT,
	SOUND_OBJ,
	SOUND_TRACK
};

struct sound {
	FMOD_SOUND* sound;
	FMOD_CHANNEL* chan;
	enum sound_type type;
	int fade_time;
};

struct sound_obj {
	int x;
	int y;
	struct sound sound;
	struct sound_obj* next;
	int id;
};


struct sound* get_sound_obj(struct param_store* param, int index, FMOD_VECTOR* vec) {
	struct sound_obj* cur = param->obj_list;
	int id = index_to_id(index);
	while (cur) {
		if (cur->id == id) {
			if (vec) {
				*vec = (FMOD_VECTOR) {cur->x, 0, cur->y};
			}
			return &cur->sound;
		}
		cur = cur->next;
	}
	return NULL;
}

int index_to_id(int index) {
	// check out of bounds
	return -index - 1;
}

void remove_sound_obj(struct param_store* param, int index) {
	struct sound_obj* prev = NULL;
	struct sound_obj* cur = param->obj_list;
	int id = index_to_id(index);
	while (cur) {
		if (cur->id == id) {
			if (cur == param->obj_list) {
				param->obj_list = cur->next;
			} else {
				prev->next = cur->next;
			}
			free(cur);
			return;
		}
		prev = cur;
		cur = cur->next;
	}
}

BOOL
create_object(struct param_store* param) {
	struct sound_obj* new_sound = malloc(sizeof(struct sound_obj));
	IF_ERR_EXIT(new_sound, "allocation error for sound object");
	// check out of bounds
	*new_sound = (struct sound_obj) {
			.x = param->args[1],
			.y = param->args[2],
			.sound = param->sound_store[param->args[0]],
			.next = NULL,
			.id = param->obj_id
	};
	param->obj_id++;
	if (!param->obj_list) {
		param->obj_list = new_sound;
	} else {
		struct sound_obj* prev = NULL;
		struct sound_obj* cur = param->obj_list;
		while (cur) {
			prev = cur;
			cur = cur->next;
		}
		prev->next = new_sound;
	}
	return TRUE;
}

BOOL
load_object(struct param_store* param) {
	FMOD_SOUND* sound = load_audio(param, FMOD_3D | FMOD_LOOP_NORMAL | FMOD_3D_LINEARROLLOFF, SOUND_OBJ);
	IF_ERR_EXIT(FMOD_Sound_Set3DMinMaxDistance(sound, 1.0, 3.0) == FMOD_OK, "fmod culd not set 3D minmax settings");
	return TRUE;
}

BOOL
load_effect(struct param_store* param) {
	load_audio(param, FMOD_DEFAULT, SOUND_EFFECT);
	return TRUE;
}

BOOL
load_track(struct param_store* param) {
	load_audio(param, FMOD_LOOP_NORMAL | FMOD_2D | FMOD_CREATESTREAM, SOUND_TRACK);
	return TRUE;
}

FMOD_SOUND*
load_audio(struct param_store* param, FMOD_MODE flags, enum sound_type type) {
	FMOD_SOUND* sound;
	IF_ERR_EXIT(FMOD_System_CreateSound(param->fmod, param->buffer + param->args[0], flags, NULL, &sound) == FMOD_OK, "fmod could not create sound");

	if (param->sound_store_cur == param->sound_store_max) {
		param->sound_store_max = !param->sound_store_max ? SOUND_STORE_START : param->sound_store_max * 2;
		param->sound_store = realloc(param->sound_store, sizeof(FMOD_SOUND*) * param->sound_store_max);
		IF_ERR_EXIT(param->sound_store, "allocation error for sound storage");
	}

	param->sound_store[param->sound_store_cur] = (struct sound) {
			.chan = NULL,
			.type = type,
			.sound = sound,
			.fade_time = 0
	};
	param->sound_store_cur++;

	return sound;
}

struct sound* get_sound_from_index(struct param_store* param, int index, FMOD_VECTOR* vec) {
	if (index < 0) {
		return get_sound_obj(param, index, vec);
	} else {
		// check out of bounds
		return &param->sound_store[index];
	}
}

// don't bother checking fmod for below since not crucial

BOOL
play_audio(struct param_store* param) {
	BOOL is_new = FALSE;

	FMOD_VECTOR vec;
	struct sound* sound = get_sound_from_index(param, param->args[0], &vec);

	FMOD_BOOL is_playing = FALSE;
	FMOD_Channel_IsPlaying(sound->chan, &is_playing);
	FMOD_BOOL is_virtual = FALSE;
	FMOD_Channel_IsVirtual(sound->chan, &is_virtual);
	if (sound->type == SOUND_EFFECT || !is_playing || is_virtual) {
		FMOD_System_PlaySound(param->fmod, sound->sound, NULL, TRUE, &sound->chan);
		if (sound->type == SOUND_EFFECT) {
			FMOD_Channel_SetVolume(sound->chan, param->args[3] / 100.0);
		} else if (sound->type == SOUND_OBJ) {
			FMOD_Channel_Set3DAttributes(sound->chan, &vec, NULL);
		}
		is_new = TRUE;
	}

	if (sound->type != SOUND_EFFECT) {
		FMOD_System_LockDSP(param->fmod);

		unsigned long long dspclock;
		FMOD_Channel_GetDSPClock(sound->chan, NULL, &dspclock);

		float volumes[2];
		unsigned long long points[2];
		BOOL in_trans = FALSE;
		if (!is_new) { // clear transition
			unsigned int has_trans = 0;
			FMOD_Channel_GetFadePoints(sound->chan, &has_trans, NULL, NULL);
			if (has_trans) {
				FMOD_Channel_GetFadePoints(sound->chan, &has_trans, points, volumes);
				in_trans = dspclock >= points[0] && dspclock < points[1];
				FMOD_Channel_RemoveFadePoints(sound->chan, dspclock, dspclock + (param->rate * sound->fade_time));
			}
		}

		if (in_trans) {
			float vol = volumes[0] + ((volumes[1] - volumes[0]) / (points[1] - points[0]) * (dspclock - points[0]));
			FMOD_Channel_AddFadePoint(sound->chan, dspclock, vol);
			FMOD_Channel_AddFadePoint(sound->chan, dspclock + (dspclock - points[0]), param->args[3] / 100.0);
		} else {
			FMOD_Channel_AddFadePoint(sound->chan, dspclock, param->args[1] / 100.0);
			FMOD_Channel_AddFadePoint(sound->chan, dspclock + (param->rate * param->args[2]), param->args[3] / 100.0);
		}

		sound->fade_time = param->args[2];
		FMOD_System_UnlockDSP(param->fmod);

	}

	if (is_new) {
		FMOD_Channel_SetPaused(sound->chan, FALSE);
	}
	return TRUE;
}

BOOL
stop_audio(struct param_store* param) {
	struct sound* sound = get_sound_from_index(param, param->args[0], NULL);
	if (sound->chan && sound->type != SOUND_EFFECT) {
		FMOD_Channel_Stop(sound->chan);
	}

	if (param->args[0] < 0) {
		remove_sound_obj(param, param->args[0]);
	}
	return TRUE;
}


DWORD CALLBACK AudioCommands(LPVOID data) {
	HANDLE hPipe = NULL;
	DWORD dwRead = 0;

	const int MAX_CMD_LEN = 512;

	char inbuf[MAX_CMD_LEN] = { 0 };

	hPipe = CreateNamedPipe(
		"\\\\.\\pipe\\GetinputCmd",
		PIPE_ACCESS_INBOUND,
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
		1,
		0,
		MAX_CMD_LEN,
		NMPWAIT_USE_DEFAULT_WAIT,
		NULL
	);

	while (hPipe != INVALID_HANDLE_VALUE) {
		BOOL pipeConnected = ConnectNamedPipe(hPipe, NULL);
		if (pipeConnected == FALSE) continue;

		while (ReadFile(hPipe, inbuf, MAX_CMD_LEN, &dwRead, NULL) != FALSE) {
			if (dwRead <= 0) continue;

			/* add terminating zero */
			inbuf[dwRead] = '\0';

		}

		DisconnectNamedPipe(hPipe);
	}

	return 0;

}

void AudioInit() {
	hAudioCommands = CreateThread(
		NULL,
		0,
		AudioCommands,
		NULL,
		0,
		NULL
	);
}

void AudioUpdate() {
	return;
}

void AudioDeinit() {
	return;
}
