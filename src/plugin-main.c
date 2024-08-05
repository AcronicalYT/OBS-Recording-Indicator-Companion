/*
Recording Indicator Integration
Copyright (C) 2024 Acronical

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <plugin-support.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

bool obs_module_load(void)
{
	bool recording = obs_get_recording_status();
	bool streaming = obs_get_streaming_status();

	json_t *root = json_object();
	json_object_set_new(root, "recording", json_boolean(recording));
	json_object_set_new(root, "streaming", json_boolean(streaming));

	http_server_t *server = http_server_create("localhost", 63515);
	http_server_set_response(server, json_dumps(root, 0));

	http_server_start(server);

	json_decref(root);
	obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	http_server_stop();
	obs_log(LOG_INFO, "plugin unloaded");
}
