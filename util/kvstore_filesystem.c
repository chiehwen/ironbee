/*****************************************************************************
 * Licensed to Qualys, Inc. (QUALYS) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * QUALYS licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ****************************************************************************/

#include "ironbee_config_auto.h"

#include <ironbee/kvstore_filesystem.h>

#include <ironbee/clock.h>
#include <ironbee/kvstore.h>

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

/**
 * Define the width for printing an expiration time.
 * This is related to EXPIRE_STR_FMT.
 * Both are 12 to accommodate the typical 10 digits and 2 buffer digits
 * for extreme future-time use.
 */
#define EXPIRE_FMT_WIDTH 13

/**
 * The sprintf format used for expiration times.
 */
#define EXPIRE_FMT "%012u"

/**
 * Define the width for printing a creation time
 * This is related to CREATE_STR_FMT.
 * Both are 20 to accommodate the typical 10+6 digits + and 2 buffer digits
 * for extreme future-time use.
 */
#define CREATE_FMT_WIDTH 20

/**
 * The sprintf format used for expiration times.
 */
#define CREATE_FMT "%012u-%06u"


/**
 * Malloc and populate a filesystem path for a key/value pair.
 *
 * The path will include the key value, the expiration value, and the type
 * of the pattern:
 * @c &lt;base_path&gt;/&lt;key&gt;/&lt;expiration&gt;-&lt;type&gt;.
 *
 * @param[in] kvstore Key-Value store.
 * @param[in] key The key to write to.
 * @param[in] expiration The expiration in seconds. This is ignored
 *            if the argument type is NULL.
 * @param[in] type The type of the file. If null then expiration is
 *            ignored and a shortened path is generated
 *            representing only the directory.
 * @param[in] type_len The type length of the type_len. This value is ignored
 *            if expiration is < 0.
 * @param[out] path The malloc'ed path. The caller must free this.
 *             The path variable will be set to NULL if a failure
 *             occurs after its memory has been allocated.
 *
 * @return
 *   - IB_OK on success
 *   - IB_EOTHER on system call failure. See @c errno.
 *   - IB_EALLOC if a memory allocation fails.
 */
static ib_status_t build_key_path(
    ib_kvstore_t *kvstore,
    const ib_kvstore_key_t *key,
    uint32_t expiration,
    const char *type,
    size_t type_len,
    char **path)
{
    assert(kvstore);
    assert(key);

    /* System return code. */
    int sys_rc;

    /* Used to compute expiration in absolute time. */
    ib_timeval_t ib_timeval;

    /* Epoch seconds after which this entry should expire. */
    uint32_t seconds;

    /* A stat struct. sb is the name used in the man page example code. */
    struct stat sb;

    ib_kvstore_filesystem_server_t *server =
        (ib_kvstore_filesystem_server_t *)(kvstore->server);

    size_t path_size =
        server->directory_length /* length of path */
        + 1                      /* path separator */
        + key->length            /* key length */
        + 1                      /* path separator */
        + EXPIRE_FMT_WIDTH       /* width to format the expiration time. */
        + 1                      /* dash. */
        + CREATE_FMT_WIDTH       /* width to format a creation time. */
        + 1                      /* dot. */
        + type_len               /* type. */
        + 1                      /* '\0' */;

    char *path_tmp = kvstore->malloc(
        kvstore,
        path_size+1,
        kvstore->malloc_cbdata);

    if ( ! path_tmp ) {
        return IB_EALLOC;
    }

    /* Push allocated path back to user. We now populate it. */
    *path = path_tmp;

    /* Append the path to the directory. */
    path_tmp = strncpy(path_tmp, server->directory, server->directory_length)
               + server->directory_length;

    /* Append the key. */
    path_tmp = strncpy(path_tmp, "/", 1) + 1;
    path_tmp = strncpy(path_tmp, key->key, key->length) + key->length;

    /* Momentarily tag the end of the path for the stat check. */
    *path_tmp = '\0';
    errno = 0;
    /* Check for a key directory. Make one if able.*/
    sys_rc = stat(*path, &sb);
    if (errno == ENOENT) {
        sys_rc = mkdir(*path, 0700);

        if (sys_rc) {
            goto eother_failure;
        }
    }
    else if (sys_rc) {
        goto eother_failure;
    }
    else if (!S_ISDIR(sb.st_mode)) {
        goto eother_failure;
    }

    if ( type ) {
        if ( expiration > 0 ) {
            ib_clock_gettimeofday(&ib_timeval);

            seconds = ib_timeval.tv_sec + expiration;
        }
        else {
            seconds = 0;
        }

        path_tmp = strncpy(path_tmp, "/", 1) + 1;
        path_tmp += snprintf(
            path_tmp,
            EXPIRE_FMT_WIDTH + 1 + CREATE_FMT_WIDTH,
            EXPIRE_FMT "-" CREATE_FMT,
            seconds, ib_timeval.tv_sec, ib_timeval.tv_usec);
        path_tmp = strncpy(path_tmp, ".", 1) + 1;
        path_tmp = strncpy(path_tmp, type, type_len) + type_len;
    }

    *path_tmp = '\0';

    return IB_OK;

eother_failure:
    kvstore->free(kvstore, *path, kvstore->free_cbdata);
    *path = NULL;
    return IB_EOTHER;
}

static ib_status_t kvconnect(
    ib_kvstore_t *kvstore,
    ib_kvstore_cbdata_t *cbdata)
{
    assert(kvstore);

    /* Nop. */

    return IB_OK;
}

static ib_status_t kvdisconnect(
    ib_kvstore_t *kvstore,
    ib_kvstore_cbdata_t *cbdata)
{
    assert(kvstore);

    /* Nop. */

    return IB_OK;
}

/**
 * Read a whole file into data.
 *
 * @param[in] kvstore The key-value store.
 * @param[in] path The path to the file.
 * @param[out] data A kvstore->malloc'ed array containing the file data.
 * @param[out] len The length of the data buffer.
 *
 * @returns
 *   - IB_OK on ok.
 *   - IB_EALLOC on memory allocation error.
 *   - IB_EOTHER on system call failure.
 */
static ib_status_t read_whole_file(
    ib_kvstore_t *kvstore,
    const char *path,
    void **data,
    size_t *len)
{
    assert(kvstore);
    assert(path);

    struct stat sb;
    int sys_rc;
    int fd = -1;
    char *dataptr;
    char *dataend;

    sys_rc = stat(path, &sb);
    if (sys_rc) {
        return IB_EOTHER;
    }

    dataptr = kvstore->malloc(
        kvstore,
        sb.st_size,
        kvstore->malloc_cbdata);
    if (!dataptr) {
        return IB_EALLOC;
    }
    *len = sb.st_size;
    *data = dataptr;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        goto eother_failure;
    }

    for (dataend = dataptr + sb.st_size; dataptr < dataend; )
    {
        ssize_t fr_tmp = read(fd, dataptr, dataend - dataptr);

        if (fr_tmp < 0) {
            goto eother_failure;
        }

        dataptr += fr_tmp;
    }

    close(fd);

    return IB_OK;

eother_failure:
    if (fd >= 0) {
        close(fd);
    }
    kvstore->free(kvstore, *data, kvstore->free_cbdata);
    *data = NULL;
    *len = 0;
    return IB_EOTHER;
}

static ib_status_t extract_type(
    ib_kvstore_t *kvstore,
    const char *path,
    char **type,
    size_t *type_length)
{
    assert(kvstore);
    assert(path);

    const char *pos;
    size_t len;

    pos = strrchr(path, '/');
    if (pos == NULL) {
        return IB_ENOENT;
    }
    pos = strchr(pos, '.');
    len = strlen(pos) - 8;  /* Ignore '.' and trailing .XXXXXX from mkstemp() */
    *type = kvstore->malloc(
        kvstore,
        len+1,
        kvstore->malloc_cbdata);
    if (!*type) {
        return IB_EALLOC;
    }
    strncpy(*type, pos, len);
    *type_length = len;

    return IB_OK;
}

/**
 * @returns
 *   - IB_OK on success.
 *   - IB_EALLOC on memory allocation failures.
 *   - IB_ENOENT if a / and . characters are not found to denote
 *               the location of the expiration decimals.
 */
static ib_status_t extract_time_info(
    ib_kvstore_t *kvstore,
    const char *path,
    uint32_t *expiration,
    ib_timeval_t *creation)
{
    assert(kvstore);
    assert(path);

    char *pos;

    pos = rindex(path, '/')+1;
    if (!pos) {
        return IB_ENOENT;
    }
    *expiration = strtoll(pos, &pos, 10);
    ++pos;                              /* Skip the '-' */
    creation->tv_sec = strtoll(pos, &pos, 10);
    ++pos;                              /* Skip the '-' */
    creation->tv_usec = strtoll(pos, &pos, 10);

    return IB_OK;
}

/**
 * @param[in] kvstore Key-value store.
 * @param[in] file The file name.
 * @param[out] value The value to be built.
 * @returns
 *   - IB_OK on success.
 *   - IB_EALLOC on memory failure from @c kvstore->malloc
 *   - IB_EOTHER on system call failure from file operations.
 *   - IB_ENOENT returned when a value is found to be expired.
 */
static ib_status_t load_kv_value(
    ib_kvstore_t *kvstore,
    const char *file,
    ib_kvstore_value_t **value)
{
    assert(kvstore);
    assert(file);

    ib_status_t rc;
    ib_timeval_t ib_timeval;

    ib_clock_gettimeofday(&ib_timeval);

    /* Use kvstore->malloc because of framework contractual requirements. */
    *value = kvstore->malloc(
        kvstore,
        sizeof(**value),
        kvstore->malloc_cbdata);
    if (!*value) {
        return IB_EALLOC;
    }

    /* Populate expiration. */
    rc = extract_time_info(kvstore, file,
                           &((*value)->expiration),
                           &((*value)->creation));
    if (rc != IB_OK) {
        kvstore->free(kvstore, *value, kvstore->free_cbdata);
        *value = NULL;
        return IB_EOTHER;
    }

    /* Remove expired file and signal there is no entry for that file. */
    if ((*value)->expiration < ib_timeval.tv_sec) {

        /* Remove the expired file. */
        unlink(file);

        /* Try to remove the key directory, though it may not be empty.
         * Failure is OK. */
        rmdir(file);
        kvstore->free(kvstore, *value, kvstore->free_cbdata);
        *value = NULL;
        return IB_ENOENT;
    }

    /* Populate type and type_length. */
    rc = extract_type(
        kvstore,
        file,
        &((*value)->type),
        &((*value)->type_length));

    if (rc != IB_OK) {
        kvstore->free(kvstore, *value, kvstore->free_cbdata);
        *value = NULL;
        return IB_EOTHER;
    }

    /* Populate value and value_length. */
    rc = read_whole_file(
        kvstore,
        file,
        &((*value)->value),
        &((*value)->value_length));

    if (rc != IB_OK) {
        kvstore->free(kvstore, (*value)->type, kvstore->free_cbdata);
        kvstore->free(kvstore, *value, kvstore->free_cbdata);
        *value = NULL;
        return IB_EOTHER;
    }

    return IB_OK;
}

typedef ib_status_t(*each_dir_t)(const char *path, const char *dirent, void *);

/**
 * Count the entries that do not begin with a dot.
 *
 * @param[in] path The directory path.
 * @param[in] dirent The directory entry in path.
 * @param[in,out] data An int* representing the current count of directories.
 *
 * @returns IB_OK.
 */
static ib_status_t count_dirent(
    const char *path,
    const char *dirent,
    void *data)
{
    assert(path);
    assert(dirent);

    size_t *i = (size_t *)data;

    if (strncmp(".", dirent, 1)) {
        ++(*i);
    }

    return IB_OK;
}

/**
 * Iterate through a directory in a standard way.
 *
 * @returns
 *   - IB_OK on success.
 *   - Other errors based on each_dir_t.
 */
static ib_status_t each_dir(const char *path, each_dir_t f, void* data)
{
    assert(path);
    assert(f);

    int tmp_errno; /* Holds errno until other system calls finish. */
    int sys_rc;
    ib_status_t rc;

    size_t dirent_sz;      /* Size of a dirent structure. */
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    struct dirent *result = NULL;

    dir = opendir(path);

    if (!dir) {
        rc = IB_EOTHER;
        goto rc_failure;
    }

    /* Man page-specified to portably allocate the dirent buffer for entry. */
    dirent_sz =
        (
            offsetof(struct dirent, d_name) +
            pathconf(path, _PC_NAME_MAX) + 1 + sizeof(long)
        ) & -sizeof(long);

    entry = malloc(dirent_sz);

    if (!entry) {
        rc = IB_EOTHER;
        goto rc_failure;
    }

    while (1) {
        sys_rc = readdir_r(dir, entry, &result);
        if (sys_rc) {
            rc = IB_EOTHER;
            goto rc_failure;
        }
        if (!result) {
            break;
        }
        rc = f(path, result->d_name, data);
        if (rc != IB_OK) {
            goto rc_failure;
        }
    }

    /* Clean exit. */
    closedir(dir);
    free(entry);
    return IB_OK;

rc_failure:
    tmp_errno = errno;
    if (dir) {
        closedir(dir);
    }
    if (entry) {
        free(entry);
    }
    errno = tmp_errno;
    return rc;
}

/**
 * Callback user data structure for build_value callback function.
 */
struct build_value_t {
    ib_kvstore_t *kvstore;        /**< Key value store. */
    ib_kvstore_value_t **values;  /**< Values array to be build. */
    size_t values_idx;         /**< Next value to be populated. */
    size_t values_len;         /**< Prevent new file causing array overflow. */
    size_t path_len;           /**< Cached path length value. */
};
typedef struct build_value_t build_value_t;

/**
 * Build an array of values.
 *
 * @param[in] path The directory path.
 * @param[in] file The file to load.
 * @param[out] data The data structure to be populated.
 * @returns
 *   - IB_OK
 *   - IB_EALLOC On a memory error.
 */
static ib_status_t build_value(const char *path, const char *file, void *data)
{
    assert(path);
    assert(file);
    assert(data);

    char *full_path;
    ib_status_t rc;
    build_value_t *bv = (build_value_t *)(data);

    /* Return if there is no space left in our array.
     * Partial results are not an error as an asynchronous write may
     * create a new file. */
    if (bv->values_idx >= bv->values_len) {
        return IB_OK;
    }

    if (*file != '.') {
        /* Build full path. */
        full_path = malloc(bv->path_len + strlen(file) + 2);
        if (!full_path) {
            return IB_EALLOC;
        }
        sprintf(full_path, "%s/%s", path, file);

        rc = load_kv_value(
            bv->kvstore,
            full_path,
            bv->values + bv->values_idx);

        /* If IB_ENOENT, a file was expired on get. */
        if (!rc) {
            bv->values_idx++;
        }

        free(full_path);
    }

    return IB_OK;
}

/**
 * Get implementations.
 *
 * @param[in] kvstore The key-value store.
 * @param[in] key The key to fetch.
 * @param[out] values A pointer to an array of pointers.
 * @param[out] values_length The length of *values.
 * @param[in,out] cbdata Callback data. Unused.
 */
static ib_status_t kvget(
    ib_kvstore_t *kvstore,
    const ib_kvstore_key_t *key,
    ib_kvstore_value_t ***values,
    size_t *values_length,
    ib_kvstore_cbdata_t *cbdata)
{
    assert(kvstore);
    assert(key);

    ib_status_t rc;
    build_value_t build_val;
    char *path = NULL;
    size_t dirent_count = 0;

    /* Build a path with no expiration value on it. */
    rc = build_key_path(kvstore, key, -1, NULL, 0, &path);
    if (rc != IB_OK) {
        goto failure1;
    }

    /* Count entries. */
    rc = each_dir(path, &count_dirent, &dirent_count);
    if (rc != IB_OK) {
        goto failure1;
    }
    if (dirent_count==0){
        rc = IB_ENOENT;
        goto failure1;
    }

    /* Initialize build_values user data. */
    build_val.kvstore = kvstore;
    build_val.path_len = strlen(path);
    build_val.values_idx = 0;
    build_val.values_len = dirent_count;
    build_val.values = (ib_kvstore_value_t**)kvstore->malloc(
        kvstore,
        sizeof(*build_val.values) * dirent_count,
        kvstore->malloc_cbdata);
    if (build_val.values == NULL) {
        rc = IB_EALLOC;
        goto failure1;
    }

    /* Build value array. */
    rc = each_dir(path, &build_value, &build_val);
    if (rc != IB_OK) {
        goto failure2;
    }

    /* Commit back results.
     * NOTE: values_idx does not need a +1 because it points at the
     *       next empty slot in the values array. */
    *values = build_val.values;
    *values_length = build_val.values_idx;

    /* Clean exit. */
    free(path);
    return IB_OK;

    /**
     * Reverse initialization error labels.
     */
failure2:
    kvstore->free(kvstore, build_val.values, kvstore->free_cbdata);
failure1:
    if (path) {
        free(path);
    }
    return rc;
}

/**
 * Set callback.
 *
 * @param[in] kvstore Key-value store.
 * @param[in] merge_policy This implementation does not merge on writes.
 *            Merging is done by the framework on reads.
 * @param[in] key The key to fetch all values for.
 * @param[in] value The value to write. The framework contract says that this
 *            is also an out-parameters, but in this implementation the
 *            merge_policy is not used so value is never merged
 *            and never written to.
 * @param[in,out] cbdata Callback data for the user.
 */
static ib_status_t kvset(
    ib_kvstore_t *kvstore,
    ib_kvstore_merge_policy_fn_t merge_policy,
    const ib_kvstore_key_t *key,
    ib_kvstore_value_t *value,
    ib_kvstore_cbdata_t *cbdata)
{
    assert(kvstore);
    assert(key);
    assert(value);

    ib_status_t rc;
    char *path = NULL;
    int fd = -1;
    ssize_t written;

    /* Build a path with no expiration value on it. */
    rc = build_key_path(
        kvstore,
        key,
        value->expiration,
        value->type,
        value->type_length,
        &path);
    if (rc != IB_OK) {
        goto cleanup;
    }

    path = realloc(path, strlen(path) + 7 + 1);
    if (!path) {
        rc = IB_EALLOC;
        goto cleanup;
    }
    strcat(path, ".XXXXXX");
    fd = mkstemp(path);
    if (fd < 0) {
        rc = IB_EOTHER;
        goto cleanup;
    }

    /* Write to the tmp file. */
    written = write(fd, value->value, value->value_length);
    if (written < (ssize_t)value->value_length ){
        rc = IB_EOTHER;
        goto cleanup;
    }

    rc = IB_OK;

cleanup:
    if (fd >= 0) {
        close(fd);
    }
    if (path != NULL) {
        free(path);
    }
    return rc;
}

/**
 * Remove all entries from key directory.
 *
 * @param[in] path The path to the key directory. @c rmdir is called on this.
 * @param[in] file The file inside of the directory pointed to by path
 *            which will be @c unlink'ed.
 * @param[in,out] data This is a size_t pointer containing
 *                the length of the path argument.
 * @returns
 *   - IB_OK
 *   - IB_EALLOC if a memory allocation fails concatenating path and file.
 */
static ib_status_t remove_file(
    const char *path,
    const char *file,
    void *data)
{
    assert(path);
    assert(file);
    assert(data);

    char *full_path;
    size_t path_len = *(size_t *)(data);

    if (*file != '.') {
        /* Build full path. */
        full_path = malloc(path_len + strlen(file) + 2);

        if (!full_path) {
            return IB_EALLOC;
        }

        sprintf(full_path, "%s/%s", path, file);

        unlink(full_path);

        free(full_path);
    }

    return IB_OK;
}

/**
 * Remove a key from the store.
 *
 * @param[in] kvstore
 * @param[in] key
 * @param[in,out] cbdata Callback data.
 * @returns
 *   - IB_OK on success.
 *   - IB_EOTHER on system call failure in build_key_path. See @c errno.
 *   - IB_EALLOC if a memory allocation fails.
 */
static ib_status_t kvremove(
    ib_kvstore_t *kvstore,
    const ib_kvstore_key_t *key,
    ib_kvstore_cbdata_t *cbdata)
{
    assert(kvstore);
    assert(key);

    ib_status_t rc;
    char *path = NULL;
    size_t path_len;

    /* Build a path with no expiration value on it. */
    rc = build_key_path(kvstore, key, -1, NULL, 0, &path);
    if (rc != IB_OK) {
        return rc;
    }

    path_len = strlen(path);

    /* Remove all files in the key directory. */
    each_dir(path, remove_file, &path_len);

    /* Attempt to remove the key path. This may fail, and that is OK.
     * Another process may write a new key while we were deleting old ones. */
    rmdir(path);

    free(path);

    return IB_OK;
}

/**
 * Destroy any allocated elements of the kvstore structure.
 * @param[out] kvstore to be destroyed. The contents on disk is untouched
 *             and another init of kvstore pointing at that directory
 *             will operate correctly.
 * @param[in] cbdata Unused.
 */
static void kvdestroy (ib_kvstore_t* kvstore, ib_kvstore_cbdata_t *cbdata)
{
    assert(kvstore);

    ib_kvstore_filesystem_server_t *server =
        (ib_kvstore_filesystem_server_t *)(kvstore->server);
    free((void *)server->directory);
    free(server);
    kvstore->server = NULL;

    return;
}

ib_status_t ib_kvstore_filesystem_init(
    ib_kvstore_t* kvstore,
    const char* directory)
{
    assert(kvstore);
    assert(directory);

    /* There is no callback data used for this implementation. */
    ib_kvstore_init(kvstore);

    ib_kvstore_filesystem_server_t *server = malloc(sizeof(*server));

    if ( server == NULL ) {
        return IB_EALLOC;
    }

    server->directory = strdup(directory);
    server->directory_length = strlen(directory);

    if ( server->directory == NULL ) {
        free(server);
        return IB_EALLOC;
    }

    kvstore->server = (ib_kvstore_server_t *) server;
    kvstore->get = kvget;
    kvstore->set = kvset;
    kvstore->remove = kvremove;
    kvstore->connect = kvconnect;
    kvstore->disconnect = kvdisconnect;
    kvstore->destroy = kvdestroy;

    kvstore->malloc_cbdata = NULL;
    kvstore->free_cbdata = NULL;
    kvstore->connect_cbdata = NULL;
    kvstore->disconnect_cbdata = NULL;
    kvstore->get_cbdata = NULL;
    kvstore->set_cbdata = NULL;
    kvstore->remove_cbdata = NULL;
    kvstore->merge_policy_cbdata = NULL;
    kvstore->destroy_cbdata = NULL;

    return IB_OK;
}
