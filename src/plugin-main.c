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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ev.h>
#include <json-c/json.h>
#include <obs-module.h>
#include <obs-frontend.h>

#define PORT 63515

struct http_server {
    struct ev_loop *loop;
    int listen_fd;
    json_object *data;
};

static struct http_server *server;

static void update_json_data() {
    json_object_object_add(server->data, "streaming", json_object_new_string(obs_frontend_streaming_active() ? "on" : "off"));
    json_object_object_add(server->data, "recording", json_object_new_string(obs_frontend_recording_active() ? "on" : "off"));
}

static void http_cb(struct ev_io *w, int revents) {
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);

    client_fd = accept(w->fd, (struct sockaddr *)&client_addr, &addrlen);
    if (client_fd < 0) {
        perror("accept");
        return;
    }

    // Create a new event watcher for the client socket
    struct ev_io client_watcher;
    ev_io_init(&client_watcher, client_cb, client_fd, EV_READ);
    ev_io_start(server->loop, &client_watcher);
}

static void client_cb(struct ev_io *w, int revents) {
    char buffer[1024];
    int bytes_read = recv(w->fd, buffer, sizeof(buffer), 0);
    if (bytes_read <= 0) {
        // Handle connection closure
        ev_io_stop(server->loop, w);
        close(w->fd);
        return;
    }

    // Parse HTTP request (simplified)
    char *request_path = strchr(buffer, ' ') + 1;
    request_path = strchr(request_path, '/') + 1;
    char *end = strchr(request_path, ' ');
    if (end) {
        *end = '\0';
    }

    // Generate HTTP response
    char response[1024];
    update_json_data(); // Update JSON data before sending response
    sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n%s", json_object_to_json_string(server->data));

    send(w->fd, response, strlen(response), 0);
}

bool obs_module_load(void) {
    server = calloc(1, sizeof(*server));
    server->loop = ev_default_loop(0);

    int server_socket;
    struct sockaddr_in server_addr;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error creating socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding socket");
        exit(1);
    }

    listen(server_socket, 5);

    struct ev_io watcher;
    ev_io_init(&watcher, http_cb, server_socket, EV_READ);
    ev_io_start(server->loop, &watcher);

    server->data = json_object_new_object();
    update_json_data(); // Initialize JSON data

    return true;
}

void obs_module_unload(void) {
    ev_break(server->loop, EVBREAK_ALL);
    ev_loop_destroy(server->loop);
    json_object_put(server->data);
    free(server);
}