//
//  tcp_server.cpp
//  uv-test
//
//  Created by 李伟 on 2018/7/31.
//  Copyright © 2018年 李伟. All rights reserved.
//

#include "tcp_server.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "uv.h"

#include "net_iobuffer.h"
#include "data_struct.hpp"

#define DEFAULT_PORT 7000
#define DEFAULT_BACKLOG 1024

static uv_loop_t *loop;

static void on_new_connection(uv_stream_t *server, int status);
static void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
static void echo_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf);
static void echo_write(uv_write_t *req, int status);

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    buf->base = (char*) malloc(1024);
    buf->len = 1024;
}

void echo_write(uv_write_t *req, int status) {
    if (status) {
        fprintf(stderr, "write error %s\n", uv_strerror(status));
    }
    
    free(req);
}

void echo_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    if (nread < 0) {
        if (nread != UV_EOF) {
            fprintf(stderr, "read error %s\n", uv_err_name((int)nread));
        }
        uv_close((uv_handle_t*) client, NULL);
    } else if (nread > 0) {
        
        printf("%s\n", buf->base);
        
        uv_write_t *req = (uv_write_t *) malloc(sizeof(uv_write_t));
        uv_buf_t wrbuf;
        std::string http_respone("HTTP/1.1 200 OK\r\n"
                                 "Content-Type:text/html;charset=utf-8\r\n"
                                 "Content-Length:13\r\n"
                                 "\r\n"
                                 "{\"what\":\"ok\"}");
        wrbuf.base = (char*)http_respone.c_str();
        wrbuf.len = http_respone.size()+1;
        uv_write(req, client, &wrbuf, 1, echo_write);
    }
    
    if (buf->base) {
        free(buf->base);
    }
}

void on_new_connection(uv_stream_t *server, int status) {
    if (status < 0) {
        fprintf(stderr, "New connection error %s\n", uv_strerror(status));
        // error!
        return;
    }
    
    uv_tcp_t *client = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
    uv_tcp_init(loop, client);
    if (uv_accept(server, (uv_stream_t*) client) == 0) {
        uv_read_start((uv_stream_t*) client, alloc_buffer, echo_read);
    }
    else {
        uv_close((uv_handle_t*) client, NULL);
    }
    printf("on new connection, status:%d\r\n", status);
}

static void signal_handler(uv_signal_t* handle, int signum) {
    printf("pid %d get a signal: %d, process exit\n", getpid(), signum);
    uv_signal_stop(handle);
    
    exit(0);
}

int http_server_run(int argc, char** args)
{
    loop = uv_loop_new();
    
    uv_signal_t sign;
    uv_signal_init(loop, &sign);
    uv_signal_start(&sign, signal_handler, SIGINT);
    
    uv_tcp_t server;
    uv_tcp_init(loop, &server);
    
    struct sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", DEFAULT_PORT, &addr);
    uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);
    
    int r = uv_listen((uv_stream_t*) &server, DEFAULT_BACKLOG, on_new_connection);
    if (r) {
        fprintf(stderr, "Listen error %s\n", uv_strerror(r));
        return 1;
    }
    
    printf("listening %d\n", DEFAULT_PORT);
    
    r = uv_run(loop, UV_RUN_DEFAULT);
    
    uv_loop_close(loop);
    if (r != 0) {
        
    }
    
    free(loop);
    
    return r;
}
