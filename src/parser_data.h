/**
 * @file parser_data.h
 * @author Radek Krejci <rkrejci@cesnet.cz>
 * @brief Data parsers for libyang
 *
 * Copyright (c) 2015-2020 CESNET, z.s.p.o.
 *
 * This source code is licensed under BSD 3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef LY_PARSER_DATA_H_
#define LY_PARSER_DATA_H_

#include "tree_data.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ly_in;

/**
 * @page howtoDataParsers Parsing Data
 *
 * Data parser allows to read instances from a specific format. libyang supports the following data formats:
 *
 * - XML
 *
 *   Original data format used in NETCONF protocol. XML mapping is part of the YANG specification
 *   ([RFC 6020](http://tools.ietf.org/html/rfc6020)).
 *
 * - JSON
 *
 *   The alternative data format available in RESTCONF protocol. Specification of JSON encoding of data modeled by YANG
 *   can be found in [RFC 7951](http://tools.ietf.org/html/rfc7951). The specification does not cover RPCs, actions and
 *   Notifications, so the representation of these data trees is proprietary and corresponds to the representation of these
 *   trees in XML.
 *
 * While the parsers themselves process the input data only syntactically, all the parser functions actually incorporate
 * the [common validator](@ref howtoDataValidation) checking the input data semantically. Therefore, the parser functions
 * accepts two groups of options - @ref dataparseroptions and @ref datavalidationoptions.
 *
 * In contrast to the schema parser, data parser also accepts empty input data if such an empty data tree is valid
 * according to the schemas in the libyang context (i.e. there are no top level mandatory nodes).
 *
 * There are individual functions to process different types of the data instances trees:
 * - ::lyd_parse_data() is intended for standard configuration data trees. According to the given
 *   [parser options](@ref dataparseroptions), the caller can further specify which kind of data tree is expected:
 *   - *complete :running datastore*: this is the default case, possibly with the use of (some of) the
 *     ::LYD_PARSE_STRICT, ::LYD_PARSE_OPAQ or ::LYD_VALIDATE_PRESENT options.
 *   - *complete configuration-only datastore* (such as :startup): in this case it is necessary to except all state data
 *     using ::LYD_PARSE_NO_STATE option.
 *   - *incomplete datastore*: there are situation when the data tree is incomplete or invalid by specification. For
 *     example the *:operational* datastore is not necessarily valid and results of the NETCONF's \<get\> or \<get-config\>
 *     oprations used with filters will be incomplete (and thus invalid). This can be allowed using ::LYD_PARSE_ONLY,
 *     the ::LYD_PARSE_NO_STATE should be used for the data returned by \<get-config\> operation.
 * - ::lyd_parse_rpc() is used for parsing RPCs/Actions, optionally including the RPC envelopes known from the NETCONF
 *   protocol.
 * - ::lyd_parse_reply() processes reply to a previous RPC/Action, which must be provided.
 * - ::lyd_parse_notif() is able to process complete Notification message.
 *
 * Further information regarding the processing input instance data can be found on the following pages.
 * - @subpage howtoDataValidation
 * - @subpage howtoDataWD
 *
 * Functions List
 * --------------
 * - ::lyd_parse_data()
 * - ::lyd_parse_data_mem()
 * - ::lyd_parse_data_fd()
 * - ::lyd_parse_data_path()
 * - ::lyd_parse_rpc()
 * - ::lyd_parse_reply()
 * - ::lyd_parse_notif()
 */

/**
 * @page howtoDataValidation Validating Data
 *
 * Data validation is performed implicitly to the input data processed by the [parser](@ref howtoDataParsers) and
 * on demand via the lyd_validate_*() functions. The explicit validation process is supposed to be used when a (complex or
 * simple) change is done on the data tree (via [data manipulation](@ref howtoDataManipulation) functions) and the data
 * tree is expected to be valid (it doesn't make sense to validate modified result of filtered \<get\> operation).
 *
 * Similarly to the [data parser](@ref howtoDataParsers), there are individual functions to validate standard data tree
 * (::lyd_validate_all()) and RPC, Action and Notification (::lyd_validate_op()). For the standard data trees, it is possible
 * to modify the validation process by @ref datavalidationoptions. This way the state data can be prohibited
 * (::LYD_VALIDATE_NO_STATE) and checking for mandatory nodes can be limited to the YANG modules with already present data
 * instances (::LYD_VALIDATE_PRESENT). Validation of the standard data tree can be also limited with ::lyd_validate_module()
 * function, which scopes only to a specified single YANG module.
 *
 * Since the operation data trees (RPCs, Actions or Notifications) can reference (leafref, instance-identifier, when/must
 * expressions) data from a datastore tree, ::lyd_validate_op() may require additional data tree to be provided. This is a
 * difference in contrast to the parsing process, when the data are loaded from an external source and invalid reference
 * outside the operation tree is acceptable.
 *
 * Functions List
 * --------------
 * - ::lyd_validate_all()
 * - ::lyd_validate_module()
 * - ::lyd_validate_op()
 */

/**
 * @addtogroup datatree
 * @{
 */

/**
 * @ingroup datatree
 * @defgroup dataparseroptions Data parser options
 *
 * Various options to change the data tree parsers behavior.
 *
 * Default parser behavior:
 * - complete input file is always parsed. In case of XML, even not well-formed XML document (multiple top-level
 * elements) is parsed in its entirety,
 * - parser silently ignores data without matching schema node definition,
 * - list instances are checked whether they have all the keys, error is raised if not.
 *
 * Default parser validation behavior:
 * - the provided data are expected to provide complete datastore content (both the configuration and state data)
 * and performs data validation according to all YANG rules, specifics follow,
 * - list instances are expected to have all the keys (it is not checked),
 * - instantiated (status) obsolete data print a warning,
 * - all types are fully resolved (leafref/instance-identifier targets, unions) and must be valid (lists have
 * all the keys, leaf(-lists) correct values),
 * - when statements on existing nodes are evaluated, if not satisfied, a validation error is raised,
 * - if-feature statements are evaluated,
 * - invalid multiple data instances/data from several cases cause a validation error,
 * - implicit nodes (NP containers and default values) are added.
 * @{
 */
/* note: keep the lower 16bits free for use by LYD_VALIDATE_ flags. They are not supposed to be combined together,
 * but since they are used (as a separate parameter) together in some functions, we want to keep them in a separated
 * range to be able detect that the caller put wrong flags into the parser/validate options parameter. */
#define LYD_PARSE_ONLY      0x010000        /**< Data will be only parsed and no validation will be performed. When statements
                                                 are kept unevaluated, union types may not be fully resolved, if-feature
                                                 statements are not checked, and default values are not added (only the ones
                                                 parsed are present). */
#define LYD_PARSE_TRUSTED   0x020000        /**< Data are considered trusted so they will be parsed as validated. If the parsed
                                                 data are not valid, using this flag may lead to some unexpected behavior!
                                                 This flag can be used only with #LYD_PARSE_ONLY. */
#define LYD_PARSE_STRICT    0x040000        /**< Instead of silently ignoring data without schema definition raise an error.
                                                 Do not combine with #LYD_PARSE_OPAQ (except for ::LYD_LYB). */
#define LYD_PARSE_OPAQ      0x080000        /**< Instead of silently ignoring data without definition, parse them into
                                                 an opaq node. Do not combine with #LYD_PARSE_STRICT (except for ::LYD_LYB). */
#define LYD_PARSE_NO_STATE  0x100000        /**< Forbid state data in the parsed data. */

#define LYD_PARSE_LYB_MOD_UPDATE  0x200000  /**< Only for LYB format, allow parsing data printed using a specific module
                                                 revision to be loaded even with a module with the same name but newer
                                                 revision. */

#define LYD_PARSE_OPTS_MASK 0xFFFF0000      /**< Mask for all the LYD_PARSE_ options. */

/** @} dataparseroptions */

/**
 * @ingroup datatree
 * @defgroup datavalidationoptions Data validation options
 *
 * Various options to change data validation behaviour, both for the parser and separate validation.
 *
 * Default separate validation behavior:
 * - the provided data are expected to provide complete datastore content (both the configuration and state data)
 * and performs data validation according to all YANG rules, specifics follow,
 * - instantiated (status) obsolete data print a warning,
 * - all types are fully resolved (leafref/instance-identifier targets, unions) and must be valid (lists have
 * all the keys, leaf(-lists) correct values),
 * - when statements on existing nodes are evaluated. Depending on the previous when state (from previous validation
 * or parsing), the node is silently auto-deleted if the state changed from true to false, otherwise a validation error
 * is raised if it evaluates to false,
 * - if-feature statements are evaluated,
 * - data from several cases behave based on their previous state (from previous validation or parsing). If there existed
 * already a case and another one was added, the previous one is silently auto-deleted. Otherwise (if data from 2 or
 * more cases were created) a validation error is raised,
 * - default values are added.
 *
 * @{
 */
#define LYD_VALIDATE_NO_STATE   0x0001      /**< Consider state data not allowed and raise an error if they are found. */
#define LYD_VALIDATE_PRESENT    0x0002      /**< Validate only modules whose data actually exist. */

#define LYD_VALIDATE_OPTS_MASK  0x0000FFFF  /**< Mask for all the LYD_VALIDATE_* options. */

/** @} datavalidationoptions */

/**
 * @ingroup datatree
 * @defgroup datavalidateop Operation to validate
 *
 * Operation provided to ::lyd_validate_op() to validate.
 *
 * The operation cannot be determined automatically since RPC/action and a reply to it share the common top level node
 * referencing the RPC/action schema node and may not have any input/output children to use for distinction.
 */
typedef enum {
    LYD_VALIDATE_OP_RPC = 1,   /**< Validate RPC/action request (input parameters). */
    LYD_VALIDATE_OP_REPLY,     /**< Validate RPC/action reply (output parameters). */
    LYD_VALIDATE_OP_NOTIF      /**< Validate Notification operation. */
} LYD_VALIDATE_OP;

/** @} datavalidateop */

/**
 * @brief Parse (and validate) data from the input handler as a YANG data tree.
 *
 * @param[in] ctx Context to connect with the tree being built here.
 * @param[in] in The input handle to provide the dumped data in the specified @p format to parse (and validate).
 * @param[in] format Format of the input data to be parsed. Can be 0 to try to detect format from the input handler.
 * @param[in] parse_options Options for parser, see @ref dataparseroptions.
 * @param[in] validate_options Options for the validation phase, see @ref datavalidationoptions.
 * @param[out] tree Resulting data tree built from the input data. Note that NULL can be a valid result as a representation of an empty YANG data tree.
 * The returned data are expected to be freed using ::lyd_free_all().
 * @return LY_SUCCESS in case of successful parsing (and validation).
 * @return LY_ERR value in case of error. Additional error information can be obtained from the context using ly_err* functions.
 */
LY_ERR lyd_parse_data(const struct ly_ctx *ctx, struct ly_in *in, LYD_FORMAT format, uint32_t parse_options,
        uint32_t validate_options, struct lyd_node **tree);

/**
 * @brief Parse (and validate) input data as a YANG data tree.
 *
 * Wrapper around ::lyd_parse_data() hiding work with the input handler.
 *
 * @param[in] ctx Context to connect with the tree being built here.
 * @param[in] data The input data in the specified @p format to parse (and validate).
 * @param[in] format Format of the input data to be parsed. Can be 0 to try to detect format from the input handler.
 * @param[in] parse_options Options for parser, see @ref dataparseroptions.
 * @param[in] validate_options Options for the validation phase, see @ref datavalidationoptions.
 * @param[out] tree Resulting data tree built from the input data. Note that NULL can be a valid result as a representation of an empty YANG data tree.
 * The returned data are expected to be freed using ::lyd_free_all().
 * @return LY_SUCCESS in case of successful parsing (and validation).
 * @return LY_ERR value in case of error. Additional error information can be obtained from the context using ly_err* functions.
 */
LY_ERR lyd_parse_data_mem(const struct ly_ctx *ctx, const char *data, LYD_FORMAT format, uint32_t parse_options,
        uint32_t validate_options, struct lyd_node **tree);

/**
 * @brief Parse (and validate) input data as a YANG data tree.
 *
 * Wrapper around ::lyd_parse_data() hiding work with the input handler.
 *
 * @param[in] ctx Context to connect with the tree being built here.
 * @param[in] fd File descriptor of a regular file (e.g. sockets are not supported) containing the input data in the specified @p format to parse (and validate).
 * @param[in] format Format of the input data to be parsed. Can be 0 to try to detect format from the input handler.
 * @param[in] parse_options Options for parser, see @ref dataparseroptions.
 * @param[in] validate_options Options for the validation phase, see @ref datavalidationoptions.
 * @param[out] tree Resulting data tree built from the input data. Note that NULL can be a valid result as a representation of an empty YANG data tree.
 * The returned data are expected to be freed using ::lyd_free_all().
 * @return LY_SUCCESS in case of successful parsing (and validation).
 * @return LY_ERR value in case of error. Additional error information can be obtained from the context using ly_err* functions.
 */
LY_ERR lyd_parse_data_fd(const struct ly_ctx *ctx, int fd, LYD_FORMAT format, uint32_t parse_options, uint32_t validate_options,
        struct lyd_node **tree);

/**
 * @brief Parse (and validate) input data as a YANG data tree.
 *
 * Wrapper around ::lyd_parse_data() hiding work with the input handler.
 *
 * @param[in] ctx Context to connect with the tree being built here.
 * @param[in] path Path to the file with the input data in the specified @p format to parse (and validate).
 * @param[in] format Format of the input data to be parsed. Can be 0 to try to detect format from the input handler.
 * @param[in] parse_options Options for parser, see @ref dataparseroptions.
 * @param[in] validate_options Options for the validation phase, see @ref datavalidationoptions.
 * @param[out] tree Resulting data tree built from the input data. Note that NULL can be a valid result as a representation of an empty YANG data tree.
 * The returned data are expected to be freed using ::lyd_free_all().
 * @return LY_SUCCESS in case of successful parsing (and validation).
 * @return LY_ERR value in case of error. Additional error information can be obtained from the context using ly_err* functions.
 */
LY_ERR lyd_parse_data_path(const struct ly_ctx *ctx, const char *path, LYD_FORMAT format, uint32_t parse_options,
        uint32_t validate_options, struct lyd_node **tree);

/**
 * @brief Parse (and validate) data from the input handler as a YANG RPC/action invocation.
 *
 * In case o LYD_XML @p format, the \<rpc\> envelope element is accepted if present. It is [checked](https://tools.ietf.org/html/rfc6241#section-4.1), an opaq
 * data node (lyd_node_opaq) is created and all its XML attributes are parsed and inserted into the node. As a content of the envelope, an RPC data or
 * \<action\> envelope element is expected. The \<action\> envelope element is also [checked](https://tools.ietf.org/html/rfc7950#section-7.15.2) and parsed as
 * the \<rpc\> envelope. Inside the \<action\> envelope, only an action data are expected.
 *
 * Similarly, in the case of LYD_JSON @p format, the same envelopes in form of JSON objects are accepted. Nothing
 * corresponding to the XML attributes is accepted in this case.
 *
 * @param[in] ctx Context to connect with the tree being built here.
 * @param[in] in The input handle to provide the dumped data in the specified @p format to parse (and validate).
 * @param[in] format Format of the input data to be parsed.
 * @param[out] tree Resulting full RPC/action tree built from the input data. The returned data are expected to be freed using ::lyd_free_all().
 * In contrast to YANG data tree, result of parsing RPC/action cannot be NULL until an error occurs.
 * @param[out] op Optional pointer to the actual operation node inside the full action @p tree, useful only for action.
 * @return LY_SUCCESS in case of successful parsing (and validation).
 * @return LY_ERR value in case of error. Additional error information can be obtained from the context using ly_err* functions.
 */
LY_ERR lyd_parse_rpc(const struct ly_ctx *ctx, struct ly_in *in, LYD_FORMAT format, struct lyd_node **tree,
        struct lyd_node **op);

/**
 * @brief Parse (and validate) data from the input handler as a YANG RPC/action reply.
 *
 * In case o LYD_XML @p format, the \<rpc-reply\> envelope element is accepted if present. It is [checked](https://tools.ietf.org/html/rfc6241#section-4.2), an opaq
 * data node (lyd_node_opaq) is created and all its XML attributes are parsed and inserted into the node.
 *
 * Similarly, in the case of LYD_JSON @p format, the same envelopes in form of JSON objects are accepted. Nothing
 * corresponding to the XML attributes is processed in this case.
 *
 * The reply data are strictly expected to be related to the provided RPC/action @p request.
 *
 * @param[in] request The RPC/action tree (result of ::lyd_parse_rpc()) of the request for the reply being parsed.
 * @param[in] in The input handle to provide the dumped data in the specified @p format to parse (and validate).
 * @param[in] format Format of the input data to be parsed.
 * @param[out] tree Resulting full RPC/action reply tree built from the input data. The returned data are expected to be freed using ::lyd_free_all().
 * The reply tree always includes duplicated operation node (and its parents) of the @p request, so in contrast to YANG data tree,
 * the result of parsing RPC/action reply cannot be NULL until an error occurs. At least one of the @p tree and @p op output variables must be provided.
 * @param[out] op Pointer to the actual operation node inside the full action reply @p tree, useful only for action. At least one of the @p op
 * and @p tree output variables must be provided.
 * @return LY_SUCCESS in case of successful parsing (and validation).
 * @return LY_ERR value in case of error. Additional error information can be obtained from the request's context using ly_err* functions.
 */
LY_ERR lyd_parse_reply(const struct lyd_node *request, struct ly_in *in, LYD_FORMAT format, struct lyd_node **tree,
        struct lyd_node **op);

/**
 * @brief Parse XML string as YANG notification.
 *
 * In case o LYD_XML @p format, the \<notification\> envelope element in combination with the child \<eventTime\> element are accepted if present. They are
 * [checked](https://tools.ietf.org/html/rfc5277#page-25), opaq data nodes (lyd_node_opaq) are created and all their XML attributes are parsed and inserted into the nodes.
 *
 * Similarly, in the case of LYD_JSON @p format, the same envelopes in form of JSON objects are accepted. Nothing
 * corresponding to the XML attributes is accepted in this case.
 *
 * @param[in] ctx Context to connect with the tree being built here.
 * @param[in] in The input handle to provide the dumped data in the specified @p format to parse (and validate).
 * @param[in] format Format of the input data to be parsed.
 * @param[out] tree Resulting full Notification tree built from the input data. The returned data are expected to be freed using ::lyd_free_all().
 * In contrast to YANG data tree, result of parsing Notification cannot be NULL until an error occurs.
 * @param[out] ntf Optional pointer to the actual notification node inside the full Notification @p tree, useful for nested notifications.
 * @return LY_SUCCESS in case of successful parsing (and validation).
 * @return LY_ERR value in case of error. Additional error information can be obtained from the context using ly_err* functions.
 */
LY_ERR lyd_parse_notif(const struct ly_ctx *ctx, struct ly_in *in, LYD_FORMAT format, struct lyd_node **tree,
        struct lyd_node **ntf);

/**
 * @brief Fully validate a data tree.
 *
 * @param[in,out] tree Data tree to recursively validate. May be changed by validation.
 * @param[in] ctx libyang context. Can be NULL if @p tree is set.
 * @param[in] val_opts Validation options (@ref datavalidationoptions).
 * @param[out] diff Optional diff with any changes made by the validation.
 * @return LY_SUCCESS on success.
 * @return LY_ERR error on error.
 */
LY_ERR lyd_validate_all(struct lyd_node **tree, const struct ly_ctx *ctx, uint32_t val_opts, struct lyd_node **diff);

/**
 * @brief Fully validate a data tree of a module.
 *
 * @param[in,out] tree Data tree to recursively validate. May be changed by validation.
 * @param[in] module Module whose data (and schema restrictions) to validate.
 * @param[in] val_opts Validation options (@ref datavalidationoptions).
 * @param[out] diff Optional diff with any changes made by the validation.
 * @return LY_SUCCESS on success.
 * @return LY_ERR error on error.
 */
LY_ERR lyd_validate_module(struct lyd_node **tree, const struct lys_module *module, uint32_t val_opts, struct lyd_node **diff);

/**
 * @brief Validate an RPC/action, notification, or RPC/action reply.
 *
 * @param[in,out] op_tree Operation tree with any parents. It can point to the operation itself or any of
 * its parents, only the operation subtree is actually validated.
 * @param[in] tree Tree to be used for validating references from the operation subtree.
 * @param[in] op Operation to validate (@ref datavalidateop), the given @p op_tree must correspond to this value. Note that
 * it isn't possible to detect the operation simply from the @p op_tree since RPC/action and their reply share the same
 * RPC/action data node and in case one of the input and output do not define any data node children, it is not possible
 * to get know what is here given for validation and if it is really valid.
 * @param[out] diff Optional diff with any changes made by the validation.
 * @return LY_SUCCESS on success.
 * @return LY_ERR error on error.
 */
LY_ERR lyd_validate_op(struct lyd_node *op_tree, const struct lyd_node *tree, LYD_VALIDATE_OP op, struct lyd_node **diff);

/** @} datatree */

#ifdef __cplusplus
}
#endif

#endif /* LY_PARSER_DATA_H_ */