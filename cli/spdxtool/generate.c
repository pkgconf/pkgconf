/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 *​ Copyright (c) 2025 The FreeBSD Foundation
 *​
 *​ Portions of this software were developed by
 * Tuukka Pasanen <tuukka.pasanen@ilmi.fi> under sponsorship from
 * the FreeBSD Foundation
 */

#include <libpkgconf/stdinc.h>
#include <libpkgconf/libpkgconf.h>
#include "util.h"
#include "core.h"
#include "software.h"
#include "serialize.h"
#include "simplelicensing.h"
#include "generate.h"

typedef struct {
	spdxtool_core_spdx_document_t *document;
	bool failed;
} generate_spdx_ctx_t;

static bool
generate_spdx_document_add_sbom(pkgconf_client_t *client, spdxtool_core_spdx_document_t *document, spdxtool_software_sbom_t *sbom)
{
	pkgconf_node_t *node = calloc(1, sizeof(pkgconf_node_t));
	if (!node)
	{
		pkgconf_error(client, "generate_spdx_document_add_sbom: out of memory");
		return false;
	}

	pkgconf_node_insert_tail(node, sbom, &document->rootElement);
	return true;
}

// NOTE: this function is passed to pkgconf_pkg_traverse
static void
generate_spdx_package(pkgconf_client_t *client, pkgconf_pkg_t *pkg, void *ptr, unsigned int iter_flags)
{
	(void) iter_flags;

	generate_spdx_ctx_t *ctx = ptr;
	spdxtool_core_spdx_document_t *document = ctx->document;
	pkgconf_node_t *iter = NULL;
	spdxtool_software_sbom_t *sbom = NULL;
	char *package_spdx = NULL;
	char *spdx_id_string = NULL;
	pkgconf_buffer_t spdx_id_buf = PKGCONF_BUFFER_INITIALIZER;
	pkgconf_buffer_t concluded_buf = PKGCONF_BUFFER_INITIALIZER;
	char sep = spdxtool_util_get_uri_separator(client);

	if (pkg->flags & PKGCONF_PKG_PROPF_VIRTUAL)
		return;

	if (ctx->failed)
		return;

	spdx_id_string = spdxtool_util_get_spdx_id_string(client, "software_Sbom", pkg->id);
	if (!spdx_id_string)
		goto err;

	sbom = spdxtool_software_sbom_new(client, spdx_id_string, document->creation_info, "build");
	free(spdx_id_string);
	spdx_id_string = NULL;
	if (!sbom)
		goto err;

	sbom->spdx_document = document;
	sbom->rootElement = pkg;

	package_spdx = spdxtool_util_get_spdx_id_string(client, "Package", pkg->id);
	if (!package_spdx)
		goto err;

	if (pkgconf_tuple_add(client, &pkg->vars, "spdxId", package_spdx, false, 0) == NULL)
		goto err;
	free(package_spdx);
	package_spdx = NULL;

	if (pkgconf_tuple_add(client, &pkg->vars, "creationInfo", document->creation_info, false, 0) == NULL ||
		pkgconf_tuple_add(client, &pkg->vars, "agent", document->agent, false, 0) == NULL)
	{
		goto err;
	}

	if (pkg->maintainer != NULL)
	{
		const char *supplier = spdxtool_core_spdx_document_add_maintainer(client, document, pkg->maintainer);
		if (!supplier)
			goto err;

		if (pkgconf_tuple_add(client, &pkg->vars, "suppliedBy", supplier, false, 0) == NULL)
			goto err;
	}

	if (pkg->license.head != NULL)
	{
		if (!pkgconf_buffer_append_fmt(&spdx_id_buf, "%s%chasDeclaredLicense", pkg->id, sep))
			goto err;

		char *spdx_id_name = pkgconf_buffer_freeze(&spdx_id_buf);
		if (!spdx_id_name)
			goto err;

		package_spdx = spdxtool_util_get_spdx_id_string(client, "Relationship", spdx_id_name);
		free(spdx_id_name);
		if (!package_spdx)
			goto err;

		if (pkgconf_tuple_add(client, &pkg->vars, "hasDeclaredLicense", package_spdx, false, 0) == NULL)
			goto err;
		free(package_spdx);
		package_spdx = NULL;

		if (!pkgconf_buffer_append_fmt(&concluded_buf, "%s%chasConcludedLicense", pkg->id, sep))
			goto err;

		spdx_id_name = pkgconf_buffer_freeze(&concluded_buf);
		if (!spdx_id_name)
			goto err;

		package_spdx = spdxtool_util_get_spdx_id_string(client, "Relationship", spdx_id_name);
		free(spdx_id_name);
		if (!package_spdx)
			goto err;

		if (pkgconf_tuple_add(client, &pkg->vars, "hasConcludedLicense", package_spdx, false, 0) == NULL)
			goto err;
		free(package_spdx);
		package_spdx = NULL;

		PKGCONF_FOREACH_LIST_ENTRY(pkg->license.head, iter)
		{
			const pkgconf_license_t *license = iter->data;
			if (license->type == PKGCONF_LICENSE_EXPRESSION)
			{
				if (!spdxtool_core_spdx_document_add_license(client, document, license->data))
					goto err;
			}
		}
	}

	if (!generate_spdx_document_add_sbom(client, document, sbom))
		goto err;

	sbom = NULL;
	return;

err:
	ctx->failed = true;
	pkgconf_error(client, "generate_spdx_package: failed for %s", pkg->id);
	free(package_spdx);
	free(spdx_id_string);
	pkgconf_buffer_finalize(&spdx_id_buf);
	pkgconf_buffer_finalize(&concluded_buf);
	spdxtool_software_sbom_free(sbom);
}

bool
spdxtool_generate(pkgconf_client_t *client, pkgconf_pkg_t *world, FILE *out, int maxdepth,
	const char *creation_time, const char *creation_id, const char *agent_name)
{
	const char *agent_name_string = agent_name ? agent_name : "Default";
	const char *creation_id_string = creation_id ? creation_id : "_:creationinfo_1";
	const char *tool_short_name_string = "spdxtool";
	const char *tool_name_string = "spdxtool";

	spdxtool_core_agent_t *agent = spdxtool_core_agent_new(client, creation_id_string, agent_name_string);
	if (!agent)
	{
		pkgconf_error(client, "Could not create agent struct");
		return false;
	}

	spdxtool_core_tool_t *tool = spdxtool_core_tool_new(client, creation_id_string, tool_short_name_string, tool_name_string);
	if (!tool)
	{
		pkgconf_error(client, "Could not create tool struct");
		spdxtool_core_agent_free(agent);
		return false;
	}

	spdxtool_core_creation_info_t *creation = spdxtool_core_creation_info_new(client, agent->spdx_id, tool->spdx_id, creation_id_string, creation_time);
	if (!creation)
	{
		pkgconf_error(client, "Could not create creation info struct");
		spdxtool_core_tool_free(tool);
		spdxtool_core_agent_free(agent);
		return false;
	}

	char *spdx_id_int = spdxtool_util_get_spdx_id_int(client, "spdxDocument");
	spdxtool_core_spdx_document_t *document = spdxtool_core_spdx_document_new(client, spdx_id_int, creation_id_string, agent->spdx_id);
	free(spdx_id_int);
	if (!document)
	{
		pkgconf_error(client, "Could not create document");
		spdxtool_core_creation_info_free(creation);
		spdxtool_core_agent_free(agent);
		return false;
	}

	generate_spdx_ctx_t ctx = {
		.document = document,
	};
	int eflag = pkgconf_pkg_traverse(client, world, generate_spdx_package, &ctx, maxdepth, 0);
	if (eflag != PKGCONF_PKG_ERRF_OK || ctx.failed)
	{
		spdxtool_core_spdx_document_free(document);
		spdxtool_core_creation_info_free(creation);
		spdxtool_core_agent_free(agent);
		return false;
	}

	pkgconfcli_serialize_value_t *root = spdxtool_generate_sbom(client, agent, tool, creation, document);
	if (!root)
	{
		spdxtool_core_spdx_document_free(document);
		spdxtool_core_creation_info_free(creation);
		spdxtool_core_agent_free(agent);
		return false;
	}

	pkgconf_buffer_t buffer = PKGCONF_BUFFER_INITIALIZER;
	bool ret = pkgconfcli_serialize_value_to_buf(&buffer, root, 0);
	pkgconfcli_serialize_value_free(root);

	if (ret)
	{
		ret = pkgconf_output_file_fmt(out, "%s\n", pkgconf_buffer_str(&buffer));
		if (!ret)
			pkgconf_error(client, "spdxtool: Could not output to file: %s", strerror(errno));
	}
	else
		pkgconf_error(client, "spdxtool: Could not serialize SPDX document");

	pkgconf_buffer_finalize(&buffer);

	spdxtool_core_spdx_document_free(document);
	spdxtool_core_creation_info_free(creation);
	spdxtool_core_tool_free(tool);
	spdxtool_core_agent_free(agent);

	return ret;
}

/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_value_t *spdxtool_generate_sbom(pkgconf_client_t *client, spdxtool_core_agent_t *agent, spdxtool_core_creation_info_t *creation, spdxtool_core_spdx_document_t *spdx)
 *
 *    Serialize a complete SPDX SBOM document to a JSON-LD value tree. Iterates
 *    all SBOMs, packages, relationships, and license expressions registered on
 *    the document. The SpdxDocument object is emitted last to ensure all element
 *    IDs have been populated by prior iteration. This function must be called
 *    after pkgconf_pkg_traverse has completed so that all packages and their
 *    dependencies are registered on spdx.
 *
 *    :param pkgconf_client_t *client: The pkgconf client being accessed.
 *    :param spdxtool_core_agent_t *agent: Agent struct to include in the document.
 *    :param spdxtool_core_creation_info_t *creation: CreationInfo struct to include in the document.
 *    :param spdxtool_core_spdx_document_t *spdx: SpdxDocument struct containing all registered SBOMs, packages, relationships, and licenses.
 *    :return: pkgconfcli_serialize_value_t * representing the complete JSON-LD document, or a null string value on allocation failure.
 */
pkgconfcli_serialize_value_t *
spdxtool_generate_sbom(pkgconf_client_t *client, spdxtool_core_agent_t *agent, spdxtool_core_tool_t *tool, spdxtool_core_creation_info_t *creation, spdxtool_core_spdx_document_t *spdx)
{
	const char *errstr = "out of memory";
	pkgconfcli_serialize_value_t *ret = NULL;
	pkgconfcli_serialize_array_t *graph = NULL;
	pkgconfcli_serialize_object_list_t *root = pkgconfcli_serialize_object_list_new();
	if (!root)
		goto err;

	if (!pkgconfcli_serialize_object_add_string(root, "@context", "https://spdx.org/rdf/3.0.1/spdx-context.jsonld"))
		goto err;

	graph = pkgconfcli_serialize_array_new();
	if (!graph)
		goto err;

	if (!pkgconfcli_serialize_array_add_take(graph, spdxtool_core_agent_to_object(client, agent)))
		goto err;

	if (!pkgconfcli_serialize_array_add_take(graph, spdxtool_core_tool_to_object(client, tool)))
		goto err;

	if (!pkgconfcli_serialize_array_add_take(graph, spdxtool_core_creation_info_to_object(client, creation)))
		goto err;

	pkgconf_node_t *iter = NULL;
	PKGCONF_FOREACH_LIST_ENTRY(spdx->maintainers.head, iter)
	{
		spdxtool_core_agent_t *maintainer = iter->data;
		if (!maintainer)
		{
			errstr = "maintainers list corrupted";
			goto err;
		}
		if (!pkgconfcli_serialize_array_add_take(graph, spdxtool_core_agent_to_object(client, maintainer)))
			goto err;
	}

	PKGCONF_FOREACH_LIST_ENTRY(spdx->licenses.head, iter)
	{
		spdxtool_simplelicensing_license_expression_t *expression = iter->data;
		if (!expression)
		{
			errstr = "licenses list corrupted";
			goto err;
		}
		if (!pkgconfcli_serialize_array_add_take(graph, spdxtool_simplelicensing_licenseExpression_to_object(client, spdx->creation_info, expression)))
			goto err;
	}

	PKGCONF_FOREACH_LIST_ENTRY(spdx->rootElement.head, iter)
	{
		spdxtool_software_sbom_t *current_sbom = iter->data;
		if (!current_sbom)
		{
			errstr = "sbom list corrupted";
			goto err;
		}
		if (!pkgconfcli_serialize_array_add_take(graph, spdxtool_software_sbom_to_object(client, current_sbom)))
			goto err;
	}

	PKGCONF_FOREACH_LIST_ENTRY(spdx->packages.head, iter)
	{
		pkgconf_pkg_t *pkg = iter->data;
		if (!pkg)
		{
			errstr = "pkg list corrupted";
			goto err;
		}
		if (!pkgconfcli_serialize_array_add_take(graph, spdxtool_software_package_to_object(client, pkg, spdx)))
			goto err;
	}

	PKGCONF_FOREACH_LIST_ENTRY(spdx->relationships.head, iter)
	{
		spdxtool_core_relationship_t *relationship = iter->data;
		if (!relationship)
		{
			errstr = "relationship list corrupted";
			goto err;
		}
		if (!pkgconfcli_serialize_array_add_take(graph, spdxtool_core_relationship_to_object(client, relationship)))
			goto err;
	}

	// SpdxDocument last — spdx->element must be fully populated first
	if (!pkgconfcli_serialize_array_add_take(graph, spdxtool_core_spdx_document_to_object(client, spdx)))
		goto err;

	bool ok = pkgconfcli_serialize_object_add_array(root, "@graph", graph);
	graph = NULL;
	if (!ok)
		goto err;

	ret = pkgconfcli_serialize_value_object(root);
	root = NULL;

	err:
	if (!ret)
		pkgconf_error(client, "spdxtool_generate_sbom: %s", errstr);

	pkgconfcli_serialize_object_list_free(root);
	pkgconfcli_serialize_array_free(graph);
	return ret;
}
