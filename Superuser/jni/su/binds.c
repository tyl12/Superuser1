/*
   Copyright 2016, Pierre-Hugues Husson <phh@phh.me>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <strings.h>
#include <stdint.h>
#include <pwd.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <sys/types.h>
#include <selinux/selinux.h>
#include <arpa/inet.h>
#include "binds.h"
#include "su.h"

int bind_foreach(bind_cb cb, void* arg, void* reserved) {
    int res = 0;
    char *str = NULL;
	int fd = open(BINDS_PATH, O_RDONLY); //## /data/xmsu/binds
    if(fd<0)
        return 1;

	off_t size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

    str = malloc(size);
    if(read(fd, str, size) != size)
        goto error;

    char *base = str;
    while(base < str+size) {
        char *parse_src, *parse_dst;
        int uid;

        char *ptr = memchr(base, 0, size-(base-str));
        if(ptr == NULL)
            goto error;
        sscanf(base, "%d", &uid);

        parse_src = strchr(base, ':');
        if(!parse_src)
            goto error;
        parse_src++;

        parse_dst = strchr(parse_src, ':');
        if(!parse_dst)
            goto error;
        *parse_dst = 0; // Split parse_src string
        parse_dst++;

		cb(arg, uid, parse_src, parse_dst, reserved);

        base = ptr+1;
    }

    res = 1;
error:
    if(str) free(str);
    close(fd);
    return res;
}

struct reserved1{
    char* _dst;
    int _res;
};

static void cb1(void *arg, int uid, const char *src, const char *dst, void* reserved) {
    struct reserved1* input=(struct reserved1 *)reserved;

    char* _dst=input->_dst;
    if(strcmp(dst, _dst) == 0)
        input->_res = 0;
}

int bind_uniq_dst(const char *dst) {
    static int _res;
    static const char *_dst;

	_res = 1;
    _dst = dst;

    //warp
    struct reserved1 obj;
    obj._dst=_dst;
    obj._res=_res;

    if(!bind_foreach(cb1, NULL, (void*)&obj))
        return 0;

    //dewarp
    _res = obj._res;

    return _res;
}

struct reserved2{
    int _uid;
};
static void cb2(void *arg, int uid, const char *src, const char *dst, void* reserved) {
    int _uid = ((struct reserved2*)reserved)->_uid;
    if(_uid == 0 || _uid == 2000 || _uid == uid) {
        fprintf(stderr, "%d %s => %s\n", uid, src, dst);
    }
}
void bind_ls(int uid) {
    static int _uid;
	_uid=uid;

    //warp
    struct reserved2 obj;
    obj._uid=_uid;

    bind_foreach(cb2, NULL, (void*)&obj);
}


struct reserved3{
    char* _path;
    int _uid;
    int _found;
    int _fd;
};

static void cb3(void *arg, int uid, const char *src, const char *dst, void* reserved) {
    struct reserved3* input = (struct reserved3*)reserved;
    char* _path = input->_path;
    int _uid = input->_uid;
    int _found = input->_found;
    int _fd = input->_fd;

    //The one we want to drop
    if(strcmp(dst, _path) == 0 &&
            (_uid == 0 || _uid == 2000 || _uid == uid)) {
        _found = 1;
        //
        input->_found = _found;
        return;
    }
    char *str = NULL;
    int len = asprintf(&str, "%d:%s:%s", uid, src, dst);
    write(_fd, str, len+1); //len doesn't include final \0 and we want to write it
    free(str);
}


int bind_remove(const char *path, int uid) {
	static int _found = 0;
    static const char *_path;
	static int _fd;
	static int _uid;


	_path = path;
	_found = 0;
	_uid = uid;

	unlink(BINDS_TMP_PATH);
	_fd = open(BINDS_TMP_PATH, O_WRONLY|O_CREAT, 0600);
	if(_fd<0)
		return 0;

    //warp
    struct reserved3 obj;
    obj._path=_path;
    obj._uid=_uid;
    obj._found=_found;
    obj._fd=_fd;

    bind_foreach(cb3, NULL, (void*)&obj);

    //dewarp
    _fd=obj._fd;
    _found=obj._found;

	close(_fd);
	unlink(BINDS_PATH);
	rename(BINDS_TMP_PATH, BINDS_PATH);
	return _found;
}

int init_foreach(init_cb icb, void* arg, void* reserved) {//## run a script which is passed by /data/xmsu/init
    int res = 0;
    char *str = NULL;
	int fd = open("/data/xmsu/init", O_RDONLY); //##@@
    if(fd<0)
        return 1;

	off_t size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

    str = malloc(size);
    if(read(fd, str, size) != size)
        goto error;

    char *base = str;
    while(base < str+size) {
        char *parsed;
        int uid;

        char *ptr = memchr(base, 0, size-(base-str)); //## "uid:script_file_path\0uid:script_file_path\0....."
        if(ptr == NULL)
            goto error;
        sscanf(base, "%d", &uid);

        parsed = strchr(base, ':');
        if(!parsed)
            goto error;
        parsed++;


        PLOGE("## %s: /data/xmsu/init: uid=%d, parsed=%s\n",__FUNCTION__, uid, parsed);
		icb(arg, uid, parsed, reserved);

        base = ptr+1;
    }

    res = 1;
error:
    if(str) free(str);
    close(fd);
    return res;
}

struct reserved5{
    char* _path;
    int _res;
};
static void cb5(void *arg, int uid, const char *path, void* reserved) {
    struct reserved5* input = (struct reserved5*) reserved;
    char* _path = input->_path;
    int _res=input->_res;

    if(strcmp(path, _path) == 0)
        _res = 0;

    input->_res = _res;
}
int init_uniq(const char *path) {
    static int _res;
    static const char *_path;

	_res = 1;
    _path = path;

    //warp
    struct reserved5 obj;
    obj._res=_res;
    obj._path=_path;

    if(!init_foreach(cb5, NULL, (void*)&obj))
        return 0;

    //dewarp
    _res = obj._res;

    return _res;
}


struct reserved6{
    char* _path;
    int _uid;
    int _found;
    int fd;
};

static void cb6(void *arg, int uid, const char *path, void* reserved) {
    struct reserved6* input = (struct reserved6*)reserved;
    char* _path=input->_path;
    int _uid=input->_uid;
    int _found=input->_found;
    int fd=input->fd;


    //The one we want to drop
    if(strcmp(path, _path) == 0 &&
            (_uid == 0 || _uid == 2000 || uid == _uid)) {
        _found = 1;
        //
        input->_found = _found;
        return;
    }
    char *str = NULL;
    int len = asprintf(&str, "%d:%s", uid, path);
    write(fd, str, len+1); //len doesn't include final \0 and we want to write it
    free(str);
}

int init_remove(const char *path, int uid) {
	static int _found = 0;
    static const char *_path;
	static int fd;
	static int _uid;

	_path = path;
	_found = 0;
	_uid = uid;

	unlink("/data/su/init.new");
	fd = open("/data/su/init.new", O_WRONLY|O_CREAT, 0600);
	if(fd<0)
		return 0;

    //warp
    struct reserved6 obj;
    obj._path = _path;
    obj._found = _found;
    obj._uid = _uid;
    obj.fd = fd;

    init_foreach(cb6, NULL, (void*)&obj);

    //dewarp
    _found = obj._found;

	close(fd);
	unlink("/data/su/init");
	rename("/data/su/init.new", "/data/su/init");
	return _found;
}

static void cb7(void *arg, int uid, const char *path, void* reserved) {
    int _uid = *(int*)reserved;

    if(_uid == 2000 || _uid == 0 || _uid == uid)
        fprintf(stderr, "%d %s\n", uid, path);
}

void init_ls(int uid) {
	static int _uid;
	_uid = uid;
    init_foreach(cb7, NULL, (void*)&_uid);
}
