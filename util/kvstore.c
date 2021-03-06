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

/**
 * @file
 * @brief IronBee Key-Value Store Implementation --- Key-Value Store Implementation
 * @author Sam Baskinger <sbaskinger@qualys.com>
 */

#include "ironbee_config_auto.h"

#include <ironbee/kvstore.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/**
 * Default malloc implementation that wraps malloc.
 * @param[in] kvstore Key-value store.
 * @param[in] size Size in bytes.
 * @param[in] cbdata Callback data. Unused.
 * @returns
 *   - Pointer to the new memory segment.
 *   - Null on error.
 */
static void* kvstore_malloc(
    ib_kvstore_t *kvstore,
    size_t size,
    ib_kvstore_cbdata_t *cbdata)
{
    assert(kvstore);

    void *r = malloc(size);

    return r;
}

/**
 * Default malloc implementation that wraps free.
 *
 * @param[in] kvstore Key-value store.
 * @param[in] ptr Pointer to free.
 * @param[in] cbdata Callback data. Unused.
 */
static void kvstore_free(
    ib_kvstore_t *kvstore,
    void *ptr,
    ib_kvstore_cbdata_t *cbdata)
{
    assert(kvstore);

    free(ptr);

    return;
}

/**
 * @param[in] kvstore The Key Value store.
 * @param[in] value The value that will be duplicated.
 * @returns Pointer to the duplicate value or NULL.
 */
static ib_kvstore_value_t * kvstore_value_dup(
    ib_kvstore_t *kvstore,
    ib_kvstore_value_t *value)
{
    assert(kvstore);
    assert(value);

    ib_kvstore_value_t *new_value = kvstore->malloc(
        kvstore,
        sizeof(*new_value),
        kvstore->malloc_cbdata);

    if (!new_value) {
        return NULL;
    }

    if (value->value) {
        new_value->value = kvstore->malloc(
            kvstore,
            value->value_length,
            kvstore->malloc_cbdata);
        if (!new_value->value) {
            goto failure;
        }

        new_value->value_length = value->value_length;
        memcpy(new_value->value, value->value, value->value_length);
    }
    else {
        new_value->value = NULL;
        new_value->value_length = 0;
    }

    if (value->type) {
        new_value->type = kvstore->malloc(
            kvstore,
            value->type_length+1,
            kvstore->malloc_cbdata);
        if (!new_value->type) {
            goto failure;
        }

        new_value->type_length = value->type_length;
        memcpy(new_value->type, value->type, value->type_length);
        new_value->type[new_value->type_length] = '\0';
    }
    else {
        new_value->type = NULL;
        new_value->type_length = 0;
    }

    /* Copy in all data. */
    new_value->expiration = value->expiration;

    return new_value;

failure:
    if (new_value) {
        if (new_value->value) {
            kvstore->free(kvstore, new_value->value, kvstore->free_cbdata);
        }

        if (new_value->type) {
            kvstore->free(kvstore, new_value->type, kvstore->free_cbdata);
        }

        kvstore->free(kvstore, new_value, kvstore->free_cbdata);
    }
    return NULL;
}

/**
 * Trivial merge policy that returns the first value in the list
 * if the list is size 1 or greater.
 *
 * If the list size is 0, this does nothing.
 *
 * @param[in] kvstore Key-value store.
 * @param[in] values Array of @ref ib_kvstore_value_t pointers.
 * @param[in] value_size The length of values.
 * @param[out] resultant_value Pointer to values[0] if value_size > 0.
 * @param[in,out] cbdata Context callback data.
 * @returns IB_OK
 */
static ib_status_t default_merge_policy(
    ib_kvstore_t *kvstore,
    ib_kvstore_value_t **values,
    size_t value_size,
    ib_kvstore_value_t **resultant_value,
    ib_kvstore_cbdata_t *cbdata)
{
    assert(kvstore);
    assert(values);

    if ( value_size > 0 ) {
        *resultant_value = values[0];
    }

    return IB_OK;
}

ib_status_t ib_kvstore_init(ib_kvstore_t *kvstore)
{
    assert(kvstore);

    kvstore->malloc = &kvstore_malloc;
    kvstore->free = &kvstore_free;
    kvstore->default_merge_policy = &default_merge_policy;

    return IB_OK;
}

ib_status_t ib_kvstore_connect(ib_kvstore_t *kvstore) {
    assert(kvstore);

    ib_status_t rc =  kvstore->connect(kvstore, kvstore->connect_cbdata);

    return rc;
}

ib_status_t ib_kvstore_disconnect(ib_kvstore_t *kvstore) {
    assert(kvstore);

    ib_status_t rc = kvstore->disconnect(kvstore, kvstore->disconnect_cbdata);

    return rc;
}

ib_status_t ib_kvstore_get(
    ib_kvstore_t *kvstore,
    ib_kvstore_merge_policy_fn_t merge_policy,
    const ib_kvstore_key_t *key,
    ib_kvstore_value_t **val)
{
    assert(kvstore);
    assert(key);

    ib_kvstore_value_t *merged_value = NULL;
    ib_kvstore_value_t **values = NULL;
    size_t values_length;
    ib_status_t rc;
    size_t i;

    if ( merge_policy == NULL ) {
        merge_policy = kvstore->default_merge_policy;
    }

    rc = kvstore->get(
        kvstore,
        key,
        &values,
        &values_length,
        kvstore->get_cbdata);

    if (rc != IB_OK) {
        *val = NULL;
        return rc;
    }

    /* Merge any values. */
    if (values_length > 1) {
        rc = merge_policy(
            kvstore,
            values,
            values_length,
            &merged_value,
            kvstore->merge_policy_cbdata);

        if (rc != IB_OK) {
            goto exit_get;
        }

        *val = kvstore_value_dup(kvstore, merged_value);
    }
    else if (values_length == 1 ) {
        *val = kvstore_value_dup(kvstore, values[0]);
    }
    else {
        *val = NULL;
        rc = IB_ENOENT;
    }

exit_get:
    for (i=0; i < values_length; ++i) {
        /* If the merge policy returns a pointer to a value array element,
         * null it to avoid a double free. */
        if ( merged_value == values[i] ) {
            merged_value = NULL;
        }
        ib_kvstore_free_value(kvstore, values[i]);
    }

    if (values) {
        kvstore->free(kvstore, values, kvstore->free_cbdata);
    }

    /* Never free the user's value. Only free what we allocated. */
    if (merged_value) {
        ib_kvstore_free_value(kvstore, merged_value);
    }

    return rc;
}

ib_status_t ib_kvstore_set(
    ib_kvstore_t *kvstore,
    ib_kvstore_merge_policy_fn_t merge_policy,
    const ib_kvstore_key_t *key,
    ib_kvstore_value_t *val)
{
    assert(kvstore);
    assert(key);
    assert(val);

    ib_status_t rc;

    if ( merge_policy == NULL ) {
        merge_policy = kvstore->default_merge_policy;
    }

    rc = kvstore->set(kvstore, merge_policy, key, val, kvstore->set_cbdata);

    return rc;
}

ib_status_t ib_kvstore_remove(
    ib_kvstore_t *kvstore,
    const ib_kvstore_key_t *key)
{
    assert(kvstore);
    assert(key);

    ib_status_t rc = kvstore->remove(kvstore, key, kvstore->remove_cbdata);

    return rc;
}


void ib_kvstore_free_value(ib_kvstore_t *kvstore, ib_kvstore_value_t *value) {
    assert(kvstore);
    assert(value);

    if (value->value) {
        kvstore->free(kvstore, value->value, kvstore->free_cbdata);
    }

    if (value->type) {
        kvstore->free(kvstore, value->type, kvstore->free_cbdata);
    }

    kvstore->free(kvstore, value, kvstore->free_cbdata);

    return;
}

void ib_kvstore_free_key(ib_kvstore_t *kvstore, ib_kvstore_key_t *key) {
    assert(kvstore);
    assert(key);

    if (key->key) {
        kvstore->free(kvstore, (char *)key->key, kvstore->free_cbdata);
    }

    kvstore->free(kvstore, key, kvstore->free_cbdata);

    return;
}

void ib_kvstore_destroy(ib_kvstore_t *kvstore) {
    kvstore->destroy(kvstore, kvstore->destroy_cbdata);
}
