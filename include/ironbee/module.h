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

/* This is always re-included to allow for prefixing the symbol names. */
#include <ironbee/module_sym.h>

#ifndef _IB_MODULE_H_
#define _IB_MODULE_H_

/**
 * @file
 * @brief IronBee --- Module
 *
 * @author Brian Rectanus <brectanus@qualys.com>
 * @author Christopher Alfeld <calfeld@qualys.com>
 */

#include <ironbee/action.h>
#include <ironbee/build.h>
#include <ironbee/config.h>
#include <ironbee/operator.h>
#include <ironbee/rule_defs.h>
#include <ironbee/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup IronBeeModule Module
 * @ingroup IronBee
 *
 * Modules add functionality to IronBee.
 *
 * @{
 */

/**
 * Initialize values for dynamic modules created with ib_module_create().
 *
 * @param[in] m                   Module
 * @param[in] xfilename           Module code filename
 * @param[in] xdata               Module data
 * @param[in] xib                 Engine handle
 * @param[in] xname               Module name
 * @param[in] xgcdata             Global config data
 * @param[in] xgclen              Global config data length
 * @param[in] xfn_cfg_copy        Config copy function
 * @param[in] xcbdata_cfg_copy    Config copy function callback data
 * @param[in] xcm_init            Configuration field map
 * @param[in] xdm_init            Config directive map
 * @param[in] xfn_init            Initialize function
 * @param[in] xcbdata_init        Initialize function callback data
 * @param[in] xfn_fini            Finish function
 * @param[in] xcbdata_fini        Finish function callback data
 * @param[in] xfn_ctx_open        Context open function
 * @param[in] xcbdata_ctx_open    Context open function callback data
 * @param[in] xfn_ctx_close       Context close function
 * @param[in] xcbdata_ctx_close   Context close function callback data
 * @param[in] xfn_ctx_destroy     Context destroy function
 * @param[in] xcbdata_ctx_destroy Context destroy function callback data
 */
#define IB_MODULE_INIT_DYNAMIC(m, xfilename, xdata, xib, xname, xgcdata, xgclen, xfn_cfg_copy, xcbdata_cfg_copy, xcm_init, xdm_init, xfn_init, xcbdata_init, xfn_fini, xcbdata_fini, xfn_ctx_open, xcbdata_ctx_open, xfn_ctx_close, xcbdata_ctx_close, xfn_ctx_destroy, xcbdata_ctx_destroy) \
    do { \
        (m)->vernum             = IB_VERNUM; \
        (m)->abinum             = IB_ABINUM; \
        (m)->version            = IB_VERSION; \
        (m)->filename           = xfilename; \
        (m)->data               = xdata; \
        (m)->ib                 = xib; \
        (m)->rule               = NULL; \
        (m)->idx                = 0; \
        (m)->name               = xname; \
        (m)->gcdata             = xgcdata; \
        (m)->gclen              = xgclen; \
        (m)->fn_cfg_copy        = xfn_cfg_copy; \
        (m)->cbdata_cfg_copy    = xcbdata_cfg_copy; \
        (m)->cm_init            = xcm_init; \
        (m)->dm_init            = xdm_init; \
        (m)->fn_init            = xfn_init; \
        (m)->cbdata_init        = xcbdata_init; \
        (m)->fn_fini            = xfn_fini; \
        (m)->cbdata_fini        = xcbdata_fini; \
        (m)->fn_ctx_open        = xfn_ctx_open; \
        (m)->cbdata_ctx_open    = xcbdata_ctx_open; \
        (m)->fn_ctx_close       = xfn_ctx_close; \
        (m)->cbdata_ctx_close   = xcbdata_ctx_close; \
        (m)->fn_ctx_destroy     = xfn_ctx_destroy; \
        (m)->cbdata_ctx_destroy = xcbdata_ctx_destroy; \
    } while (0)

/** Defaults for all module structure headers */
#define IB_MODULE_HEADER_DEFAULTS     IB_VERNUM, \
                                      IB_ABINUM, \
                                      IB_VERSION, \
                                      __FILE__, \
                                      NULL, \
                                      NULL, \
                                      NULL, \
                                      0

/** Module config structure, size, and default handlers */
#define IB_MODULE_CONFIG(ptr)         (ptr), (sizeof(*(ptr))), NULL, NULL

/** Used to signify that there is no config structure for the module. */
#define IB_MODULE_CONFIG_NULL         NULL, 0, NULL, NULL

/**
 * Function which is exported in an IronBee module to return the address
 * to the module structure used to load the module.
 *
 * This module function is declared by IB_MODULE_DECLARE and defined
 * by IB_MODULE_INIT. The address of this function is looked up by
 * name (IB_MODULE_SYM) when the module is loaded and called to fetch
 * the address of the module structure built with IB_MODULE_INIT.
 *
 * @param ib Engine handle
 * @returns Address of the module structure
 */
typedef ib_module_t *(*ib_module_sym_fn)(ib_engine_t* ib);

/**
 * Function to handle copying configuration data.
 *
 * This is called when configuration data needs to be copied from a parent
 * context to a child context.  If NULL, it defaults to memcpy.
 *
 * @param[in] ib     Engine handle
 * @param[in] m      Module
 * @param[in] dst    Destination of data.
 * @param[in] src    Source of data.
 * @param[in] length Length of data.
 * @param[in] cbdata Callback data
 *
 * @returns Status code
 */
typedef ib_status_t (*ib_module_fn_cfg_copy_t)(
    ib_engine_t *ib,
    ib_module_t *m,
    void        *dst,
    const void  *src,
    size_t       length,
    void        *cbdata
);

/**
 * Function to initialize a module.
 *
 * This is called when the module is loaded.
 *
 * @param[in] ib     Engine handle
 * @param[in] m      Module
 * @param[in] cbdata Callback data
 *
 * @returns Status code
 */
typedef ib_status_t (*ib_module_fn_init_t)(
    ib_engine_t *ib,
    ib_module_t *m,
    void        *cbdata
);

/**
 * Function to finish a module.
 *
 * This is called when the module is unloaded.
 *
 * @param[in] ib     Engine handle
 * @param[in] m      Module
 * @param[in] cbdata Callback data
 *
 * @returns Status code
 */
typedef ib_status_t (*ib_module_fn_fini_t)(
    ib_engine_t *ib,
    ib_module_t *m,
    void        *cbdata
);

/**
 * Function called when a context is opened.
 *
 * This is called when a context is opened in the configuration file.
 *
 * @param[in] ib     Engine handle
 * @param[in] m      Module
 * @param[in] ctx    Config context
 * @param[in] cbdata Callback data
 *
 * @returns Status code
 */
typedef ib_status_t (*ib_module_fn_ctx_open_t)(
    ib_engine_t  *ib,
    ib_module_t  *m,
    ib_context_t *ctx,
    void         *cbdata
);

/**
 * Function called when a context is closed.
 *
 * This is called when ib_context_close() is called to close.
 * a configuration context. This should be used to initialize
 * any per-config-context resources.
 *
 * @param[in] ib     Engine handle
 * @param[in] m      Module
 * @param[in] ctx    Config context
 * @param[in] cbdata Callback data
 *
 * @returns Status code
 */
typedef ib_status_t (*ib_module_fn_ctx_close_t)(
    ib_engine_t  *ib,
    ib_module_t  *m,
    ib_context_t *ctx,
    void         *cbdata
);

/**
 * Function called when a context is destroyed.
 *
 * This is called when ib_context_destroy() is called to finish
 * a configuration context. This should be used to destroy
 * any per-config-context resources.
 *
 * @param[in] ib     Engine handle
 * @param[in] m      Module
 * @param[in] ctx    Config context
 * @param[in] cbdata Callback data.
 *
 * @returns Status code
 */
typedef ib_status_t (*ib_module_fn_ctx_destroy_t)(
    ib_engine_t  *ib,
    ib_module_t  *m,
    ib_context_t *ctx,
    void         *cbdata
);

/**
 * Additional functionality for IronBee.
 *
 * A module provides additional functionality to IronBee.  It can register
 * configuration values and directives, hook into events, and provide
 * functions for other modules.
 */
struct ib_module_t {
    /* Header */
    uint32_t     vernum;      /**< Engine version number */
    uint32_t     abinum;      /**< Engine ABI Number */
    const char  *version;     /**< Engine version string */
    const char  *filename;    /**< Module code filename */
    void        *data;        /**< Module data */
    ib_engine_t *ib;          /**< Engine */
    ib_rule_t   *rule;        /**< Access to Rule Engine and env. Init=NULL*/
    size_t       idx;         /**< Module index */


    /* Module Config */
    const char *name; /**< Module name */

    void                    *gcdata;          /**< Global config data */
    size_t                   gclen;           /**< Global config data length */
    ib_module_fn_cfg_copy_t  fn_cfg_copy;     /**< Config copy handler */
    void                    *cbdata_cfg_copy; /**< Config copy data */
    const ib_cfgmap_init_t  *cm_init;         /**< Module config mapping */

    const ib_dirmap_init_t *dm_init; /**< Module directive mapping */

    /* Functions */
    ib_module_fn_init_t         fn_init;         /**< Module init */
    void                       *cbdata_init;     /**< fn_init callback data */
    ib_module_fn_fini_t         fn_fini;         /**< Module finish */
    void                       *cbdata_fini;     /**< fn_init callback data */
    ib_module_fn_ctx_open_t     fn_ctx_open;     /**< Context open */
    void                       *cbdata_ctx_open; /**< fn_init callback data */
    ib_module_fn_ctx_close_t    fn_ctx_close;    /**< Context close */
    void                       *cbdata_ctx_close;/**< fn_init callback data */
    ib_module_fn_ctx_destroy_t  fn_ctx_destroy;  /**< Context destroy */
    void                       *cbdata_ctx_destroy; /**< fn_init callback data */
};

/**
 * Initialize an engine module.
 *
 * Use this to initialize a static module.
 *
 * @param[in] m  Module handle (already loaded)
 * @param[in] ib Engine handle
 *
 * @returns Status code
 */
ib_status_t DLL_PUBLIC ib_module_init(
    ib_module_t *m,
    ib_engine_t *ib
);

/**
 * Create a module structure.
 *
 * Use this to dynamically build modules.
 *
 * @param[out] pm Address which module structure will be written
 * @param[in]  ib Engine handle
 *
 * @returns Status code
 */
ib_status_t DLL_PUBLIC ib_module_create(
    ib_module_t **pm,
    ib_engine_t  *ib
);

/**
 * Load and initialize an engine module.
 *
 * This causes the module init() function to be called.
 *
 * @param[out] pm   Address which module handle is written
 * @param[in]  ib   Engine handle
 * @param[in]  file Filename of the module
 *
 * @returns Status code
 */
ib_status_t DLL_PUBLIC ib_module_load(
    ib_module_t **pm,
    ib_engine_t  *ib,
    const char   *file
);

/**
 * Unload an engine module.
 *
 * @param[in] m Module
 *
 * @returns Status code
 */
ib_status_t DLL_PUBLIC ib_module_unload(
     ib_module_t *m
);


/**
 * Register a module with a configuration context.
 *
 * @param[in] m Module
 * @param[in] ctx Configuration context
 *
 * @returns Status code
 */
ib_status_t DLL_PUBLIC ib_module_register_context(
     ib_module_t  *m,
     ib_context_t *ctx
);

/**
 * Create an IronBee rule action for use by this module.
 *
 * Actions created by this method will still need an
 * ib_rule_exec_t from the ib_tx_t object to execute.
 *
 * Actions created by this method use the main IronBee context by default.
 *
 * Actions created by this method should be destroyed
 * with ib_action_inst_destroy(ib_action_inst_t *).
 *
 * @param[in] module The module.
 * @param[in] mpool The memory pool to use to create this object.
 *            If NULL, then the main engine memory pool will be used.
 * @param[in] action_name The name of the action.
 * @param[in] action_parameters The parameter value for the action.
 * @param[in] flags Flags to create the action with. Typically 0.
 * @param[out] action_instance Where to put the newly created action.
 * @returns Values returned by ib_action_inst_create.
 *   - IB_OK success.
 *   - IB_EINVAL if the named action does not exist.
 *   - IB_EALLOC if memory could not be allocated.
 */
ib_status_t DLL_PUBLIC ib_module_action_inst_create(
    ib_module_t *module,
    ib_mpool_t *mpool,
    const char *action_name,
    const char *action_parameters,
    ib_flags_t flags,
    ib_action_inst_t **action_instance);

/**
 * Create an IronBee rule operator for use by this module.
 *
 * Operators created by this method will still need a ib_rule_exec_t
 * which can be found in the ib_tx_t structure.
 *
 * Operators built by this method use the module rule for read-only access.
 *
 * Operators built by this method must be destroyed using
 * ib_operator_inst_destroy(ib_operator_inst_t *).
 *
 * @param[in] module The module.
 * @param[in] mpool The memory pool to use to create this object.
 *            If NULL, then the main engine memory pool will be used.
 * @param[in] operator_name The name of the operator, such as "rx".
 * @param[in] parameters The parameters to pass to the operator.
 * @param[in] flags Flags for the operator.
 * @param[out] operator_instance The created operator instance.
 * @returns Values returned by ib_operator_inst_create.
 *  - IB_OK on success
 *  - IB_ENOENT if the named operator does not exist
 */
ib_status_t DLL_PUBLIC ib_module_operator_inst_create(
    ib_module_t *module,
    ib_mpool_t *mpool,
    const char *operator_name,
    const char *parameters,
    ib_flags_t flags,
    ib_operator_inst_t **operator_instance);


/**
 * @} IronBeeModule
 */

#ifdef __cplusplus
}
#endif

#endif /* _IB_MODULE_H_ */
