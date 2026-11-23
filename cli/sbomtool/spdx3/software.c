/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 *​ Copyright (c) 2026 The FreeBSD Foundation
 *​
 *​ Portions of this software were developed by
 * Tuukka Pasanen <tuukka.pasanen@ilmi.fi> under sponsorship from
 * the FreeBSD Foundation
 */

#include <stdlib.h>
#include <string.h>
#include <libpkgconf/libpkgconf.h>
#include <libpkgconfcli/serialize.h>
#include "spdx3/simplelicensing.h"
#include "spdx3/software.h"
#include "spdx3/core.h"
#include "util.h"

static bool
serialize_copyright_lines_to_object(pkgconfcli_serialize_object_list_t *object_list, const pkgconf_list_t *copyright_lines)
{
	pkgconf_buffer_t copyright_buf = PKGCONF_BUFFER_INITIALIZER;
	const pkgconf_node_t *node;

	if (copyright_lines->head == NULL)
		return pkgconfcli_serialize_object_add_string(object_list, "software_copyrightText", "NOASSERTION") != NULL;

	PKGCONF_FOREACH_LIST_ENTRY(copyright_lines->head, node)
	{
		const pkgconf_bufferset_t *set = node->data;
		if (!pkgconf_buffer_join(&copyright_buf, '\n', pkgconf_buffer_str_or_empty(&set->buffer), NULL))
		{
			pkgconf_buffer_finalize(&copyright_buf);
			return false;
		}
	}

	bool ok = pkgconfcli_serialize_object_add_string(object_list, "software_copyrightText", pkgconf_buffer_str_or_empty(&copyright_buf)) != NULL;
	pkgconf_buffer_finalize(&copyright_buf);
	return ok;
}

static const char *
software_get_string(pkgconfcli_serialize_value_t *value, const char *key)
{
	pkgconfcli_serialize_value_t *element = pkgconfcli_serialize_value_object_find(value, key);

	if (!element)
		return NULL;

	if (pkgconfcli_serialize_is_value(element, PKGCONFCLI_SERIALIZE_TYPE_STRING))
		return pkgconfcli_serialize_value_get_string(element);

	return NULL;
}
static char *
software_add_license(pkgconf_client_t *client, sbomtool_spdx3_document_t *document, const char *expression)
{
	pkgconf_node_t *node = NULL;
	const char *cmp_expression = 0;
	char *spdx_id = NULL;
	bool found = false;

	PKGCONF_FOREACH_LIST_ENTRY(document->licenses->items.head, node)
	{
		pkgconfcli_serialize_value_t *item = node->data;
		if (pkgconfcli_serialize_is_value(item, PKGCONFCLI_SERIALIZE_TYPE_OBJECT))
		{
			cmp_expression = software_get_string(item, "simplelicensing_licenseExpression");

			if (!cmp_expression)
				break;

			if (!strcmp(expression, cmp_expression))
			{
				found = true;
				break;
			}
		}
	}

	pkgconfcli_serialize_value_t *license_obj = sbomtool_spdx3_simplelicensing_licenseExpression(client, document, expression);

	if (license_obj)
	{
		spdx_id = strdup (software_get_string(license_obj, "spdxId"));

		if (!found)
		{
			if (!pkgconfcli_serialize_array_add_take(document->licenses, license_obj))
			{
				free(spdx_id);
				spdx_id = NULL;
			}
		}
		else
			pkgconfcli_serialize_value_free(license_obj);
	}

	return spdx_id;
}

/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_value_t *sbomtool_spdx3_software_sbom(pkgconf_client_t *client, sbomtool_spdx3_document_t *document, const char *name)
 *
 *    Serialize /Software/Sbom as a JSON value tree.
 *
 *    :param pkgconf_client_t *client: Pkgconf Client
 *    :param sbomtool_spdx3_document_t *document: SPDX document structure which hold needed information
 *    :param const char *name: Name of SBOM this is related
 *    :return: pkgconfcli_serialize_value_t * representing the Sbom object.
 */
pkgconfcli_serialize_value_t *
sbomtool_spdx3_software_sbom(pkgconf_client_t *client, sbomtool_spdx3_document_t *document, const char *name)
{
	pkgconfcli_serialize_value_t *ret = NULL;
	pkgconfcli_serialize_object_list_t *object_list = NULL;
	pkgconfcli_serialize_array_t *sbom_type_array = NULL;
	pkgconfcli_serialize_array_t *root_element_array = NULL;
	pkgconfcli_serialize_array_t *element_array = NULL;
	char *spdx_id = NULL;

	spdx_id = sbomtool_util_get_sbom_id_string(client, "software_Sbom", name);
	if (!spdx_id)
		goto err;

	if (!pkgconfcli_serialize_array_add_string(document->root_elements, spdx_id))
		goto err;

	object_list = pkgconfcli_serialize_object_list_new();
	if (!object_list)
		goto err;

	sbom_type_array = pkgconfcli_serialize_array_new();
	if (!sbom_type_array)
		goto err;

	if (!pkgconfcli_serialize_array_add_string(sbom_type_array, "build"))
		goto err;

	root_element_array = pkgconfcli_serialize_array_new();
	if (!root_element_array)
		goto err;

	element_array = pkgconfcli_serialize_array_new();

	if (!element_array)
		goto err;

	if (!(pkgconfcli_serialize_object_add_string(object_list, "type", "software_Sbom") &&
	        pkgconfcli_serialize_object_add_string(object_list, "creationInfo", document->creation_info_id) &&
	        pkgconfcli_serialize_object_add_string(object_list, "spdxId", spdx_id)))
		goto err;

	/* object_add_array always takes ownership of the array (it is freed even on
	 * failure), so clear our reference before checking the result to avoid a
	 * double free at the error label.
	 */
	ret = pkgconfcli_serialize_object_add_array(object_list, "software_sbomType", sbom_type_array);
	sbom_type_array = NULL;
	if (!ret)
		goto err;

	ret = pkgconfcli_serialize_object_add_array(object_list, "rootElement", root_element_array);
	root_element_array = NULL;
	if (!ret)
		goto err;

	ret = pkgconfcli_serialize_object_add_array(object_list, "element", element_array);
	element_array = NULL;
	if (!ret)
		goto err;

	ret = pkgconfcli_serialize_value_object(object_list);
	object_list = NULL;

err:
	if (!ret)
		pkgconf_error(client, "sbomtool_spdx3_software_sbom: out of memory");

	free(spdx_id);
	pkgconfcli_serialize_object_list_free(object_list);
	pkgconfcli_serialize_array_free(sbom_type_array);
	pkgconfcli_serialize_array_free(root_element_array);
	pkgconfcli_serialize_array_free(element_array);
	return ret;
}

/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_value_t *sbomtool_spdx3_software_package(pkgconf_client_t *client, sbomtool_spdx3_document_t *document, pkgconf_pkg_t *pkg, pkgconfcli_serialize_value_t *sbom)
 *
 *    Serialize /Software/Package as a JSON value tree.
 *
 *    :param pkgconf_client_t *client: Pkgconf Client
 *    :param sbomtool_spdx3_document_t *document: SPDX document structure which hold needed information
 *    :param pkgconf_pkg_t *pkg: Package struct to be serialized.
 *    :pkgconfcli_serialize_value_t *sbom: SBOM object which relates to this package
 *    :return: pkgconfcli_serialize_value_t * representing the Package object.
 */
pkgconfcli_serialize_value_t *
sbomtool_spdx3_software_package(pkgconf_client_t *client, sbomtool_spdx3_document_t *document, pkgconf_pkg_t *pkg, pkgconfcli_serialize_value_t *sbom)
{
	char *spdx_id = NULL;
	char *license_spdx_id = NULL;
	const char *relation_spdx_id = NULL;
	const char *supplier_spdx_id = NULL;
	pkgconfcli_serialize_value_t *ret = NULL;
	pkgconfcli_serialize_object_list_t *object_list = NULL;
	pkgconfcli_serialize_array_t *originated_by = NULL;
	pkgconfcli_serialize_array_t *has_declared_license = NULL;
	pkgconfcli_serialize_array_t *has_concluded_license = NULL;
	pkgconfcli_serialize_array_t *depend_on_array = NULL;
	pkgconfcli_serialize_value_t *sbom_root_element = NULL;
	pkgconfcli_serialize_value_t *sbom_common_element = NULL;
	pkgconfcli_serialize_value_t *spdx_element = NULL;
	pkgconfcli_serialize_value_t *depencency_element = NULL;
	pkgconfcli_serialize_value_t *supplier_element = NULL;
	pkgconf_node_t *node = NULL;
	bool add_array = false;

	sbom_root_element = pkgconfcli_serialize_value_object_find(sbom, "rootElement");

	if (!sbom_root_element)
		goto err;

	sbom_common_element = pkgconfcli_serialize_value_object_find(sbom, "element");

	if (!sbom_common_element)
		goto err;

	spdx_id = sbomtool_util_get_sbom_id_string(client, "Package", pkg->id);
	if (!spdx_id)
		goto err;

	if (!pkgconfcli_serialize_array_add_take(pkgconfcli_serialize_value_get_array(sbom_root_element), pkgconfcli_serialize_value_string(spdx_id)))
		goto err;

	object_list = pkgconfcli_serialize_object_list_new();
	if (!object_list)
		goto err;

	originated_by = pkgconfcli_serialize_array_new();
	if (!originated_by)
		goto err;

	if (!pkgconfcli_serialize_array_add_string(originated_by, document->agent_spdx_id))
		goto err;

	if (!(pkgconfcli_serialize_object_add_string(object_list, "type", "software_Package") &&
	        pkgconfcli_serialize_object_add_string(object_list, "creationInfo", document->creation_info_id) &&
	        pkgconfcli_serialize_object_add_string(object_list, "spdxId", spdx_id) &&
	        pkgconfcli_serialize_object_add_string(object_list, "name", pkg->realname)))
		goto err;

	ret = pkgconfcli_serialize_object_add_array(object_list, "originatedBy", originated_by);
	originated_by = NULL;
	if (!ret)
		goto err;

	if (pkg->maintainer != NULL)
	{

		supplier_element = sbomtool_spdx3_core_agent(client, document, pkg->maintainer);

		if (!supplier_element)
			goto err;

		if (!pkgconfcli_serialize_array_add_take(document->maintainers, supplier_element))
			goto err;

		supplier_spdx_id = software_get_string(supplier_element, "spdxId");

		if (!pkgconfcli_serialize_object_add_string(object_list, "suppliedBy", supplier_spdx_id))
			goto err;

		if (!pkgconfcli_serialize_array_add_string(document->elements, supplier_spdx_id))
			goto err;

	}

	if (!serialize_copyright_lines_to_object(object_list, &pkg->copyright))
		goto err;

	if (!pkgconfcli_serialize_object_add_string(object_list, "software_homePage",
	        pkg->url ? pkg->url : ""))
		goto err;

	if (!pkgconfcli_serialize_object_add_string(object_list, "software_downloadLocation",
	        pkg->source ? pkg->source : ""))
		goto err;

	if (!pkgconfcli_serialize_object_add_string(object_list, "software_packageVersion", pkg->version))
		goto err;

	if (pkg->license.head)
	{
		has_declared_license = pkgconfcli_serialize_array_new();

		if (!has_declared_license)
			goto err;

		has_concluded_license = pkgconfcli_serialize_array_new();

		if (!has_concluded_license)
		{
			pkgconfcli_serialize_array_free(has_declared_license);
			goto err;
		}

		PKGCONF_FOREACH_LIST_ENTRY(pkg->license.head, node)
		{
			const pkgconf_license_t *license = node->data;
			if (license->type == PKGCONF_LICENSE_EXPRESSION)
			{

				license_spdx_id = software_add_license(client, document, license->data);

				if (!pkgconfcli_serialize_array_add_string(document->elements, license_spdx_id))
					goto err;

				if (!license_spdx_id)
					goto err;

				pkgconfcli_serialize_array_add_string(has_declared_license, license_spdx_id);
				pkgconfcli_serialize_array_add_string(has_concluded_license, license_spdx_id);


				if (!pkgconfcli_serialize_array_add_take(pkgconfcli_serialize_value_get_array(sbom_common_element), pkgconfcli_serialize_value_string(license_spdx_id)))
				{
					pkgconfcli_serialize_array_free(has_declared_license);
					pkgconfcli_serialize_array_free(has_concluded_license);
					goto err;
				}

				free(license_spdx_id);
			}
		}

		depencency_element = sbomtool_spdx3_core_relationship(client, document, spdx_id, has_declared_license, pkg->id, "hasDeclaredLicense", NULL);

		if (!depencency_element)
			goto err;

		relation_spdx_id = software_get_string(depencency_element, "spdxId");

		if (!relation_spdx_id)
			goto err;

		if (!pkgconfcli_serialize_array_add_string(document->elements, relation_spdx_id))
			goto err;

		if (!pkgconfcli_serialize_array_add_take(pkgconfcli_serialize_value_get_array(sbom_common_element), pkgconfcli_serialize_value_string(relation_spdx_id)))
			goto err;

		if (!pkgconfcli_serialize_array_add_take(document->relationships, depencency_element))
			goto err;

		depencency_element = sbomtool_spdx3_core_relationship(client, document, spdx_id, has_concluded_license, pkg->id, "hasConcludedLicense", NULL);

		if (!depencency_element)
			goto err;

		relation_spdx_id = software_get_string(depencency_element, "spdxId");

		if (!relation_spdx_id)
			goto err;

		if (!pkgconfcli_serialize_array_add_string(document->elements, relation_spdx_id))
			goto err;

		if (!pkgconfcli_serialize_array_add_take(pkgconfcli_serialize_value_get_array(sbom_common_element), pkgconfcli_serialize_value_string(relation_spdx_id)))
			goto err;

		if (!pkgconfcli_serialize_array_add_take(document->relationships, depencency_element))
			goto err;
	}

	depend_on_array = pkgconfcli_serialize_array_new();

	PKGCONF_FOREACH_LIST_ENTRY(pkg->required.head, node)
	{
		pkgconf_dependency_t *dep = node->data;
		pkgconf_pkg_t *match = dep->match;

		add_array = true;

		if (match == NULL)
			continue;

		char *spdx_id_relation = sbomtool_util_get_sbom_id_string(client, "Package", match->id);

		if (!spdx_id_relation)
			goto err;

		if (!pkgconfcli_serialize_array_add_string(document->elements, spdx_id_relation))
			goto err;


		if (!pkgconfcli_serialize_array_add_string(depend_on_array, spdx_id_relation))
		{
			free(spdx_id_relation);
			goto err;
		}

		free(spdx_id_relation);
	}

	if (add_array)
	{
		depencency_element = sbomtool_spdx3_core_relationship(client, document, spdx_id, depend_on_array, pkg->id, "dependsOn", NULL);

		if (!depencency_element)
			goto err;

		spdx_element = pkgconfcli_serialize_value_object_find(depencency_element, "spdxId");

		if (!spdx_element)
			goto err;

		relation_spdx_id = pkgconfcli_serialize_value_get_string(spdx_element);

		if (!relation_spdx_id)
			goto err;

		if (!pkgconfcli_serialize_array_add_string(document->elements, relation_spdx_id))
			goto err;

		if (!pkgconfcli_serialize_array_add_take(pkgconfcli_serialize_value_get_array(sbom_common_element), pkgconfcli_serialize_value_string(relation_spdx_id)))
			goto err;

		if (!pkgconfcli_serialize_array_add_take(document->relationships, depencency_element))
			goto err;
	}
	else
		pkgconfcli_serialize_array_free(depend_on_array);

	depend_on_array = pkgconfcli_serialize_array_new();
	add_array = false;

	PKGCONF_FOREACH_LIST_ENTRY(pkg->requires_private.head, node)
	{
		pkgconf_dependency_t *dep = node->data;
		pkgconf_pkg_t *match = dep->match;

		add_array = true;

		if (match == NULL)
			continue;

		char *spdx_id_relation = sbomtool_util_get_sbom_id_string(client, "Package", match->id);

		if (!spdx_id_relation)
			goto err;

		if (!pkgconfcli_serialize_array_add_string(document->elements, spdx_id_relation))
			goto err;

		if (!pkgconfcli_serialize_array_add_take(pkgconfcli_serialize_value_get_array(sbom_common_element), pkgconfcli_serialize_value_string(spdx_id_relation)))
			goto err;

		if (!pkgconfcli_serialize_array_add_string(depend_on_array, spdx_id_relation))
		{
			free(spdx_id_relation);
			goto err;
		}

		free(spdx_id_relation);

	}

	if (add_array)
	{
		depencency_element =  sbomtool_spdx3_core_relationship(client, document, spdx_id, depend_on_array, pkg->id, "dependsOn", "development");

		if (!depencency_element)
			goto err;

		spdx_element = pkgconfcli_serialize_value_object_find(depencency_element, "spdxId");

		if (!spdx_element)
			goto err;

		relation_spdx_id = pkgconfcli_serialize_value_get_string(spdx_element);

		if (!relation_spdx_id)
			goto err;

		if (!pkgconfcli_serialize_array_add_string(document->elements, relation_spdx_id))
			goto err;

		if (!pkgconfcli_serialize_array_add_take(pkgconfcli_serialize_value_get_array(sbom_common_element), pkgconfcli_serialize_value_string(relation_spdx_id)))
			goto err;

		if (!pkgconfcli_serialize_array_add_take(document->relationships, depencency_element))
			goto err;
	}
	else
		pkgconfcli_serialize_array_free(depend_on_array);

	ret = pkgconfcli_serialize_value_object(object_list);
	object_list = NULL;

err:
	if (!ret)
		pkgconf_error(client, "sbomtool_spdx3_software_package: out of memory");

	free(spdx_id);
	pkgconfcli_serialize_object_list_free(object_list);
	pkgconfcli_serialize_array_free(originated_by);
	return ret;
}

/*
 * !doc
 *
 * .. c:function:: void sbomtool_cyclondex_component_add_package(pkgconf_client_t *client, pkgconf_pkg_t *pkg, void *ptr, unsigned int iter_flags)
 *
 *    This should be passes to package traverse function as callback. Adds components to SPDX 3.x SBOM
 *
 *    :param pkgconf_client_t *client: Pkgconf client struct
 *    :param  pkgconf_pkg_t *pkg: Pkgconf Package struct
 *    :param void *ptr: This should be pointer to sbomtool_spdx3_document_t struct
 *    :param onst char *iter_flags: Flags which are passed to function. Currenlty not used.
 */
void
sbomtool_spdx3_software_add_package(pkgconf_client_t *client, pkgconf_pkg_t *pkg, void *ptr, unsigned int iter_flags)
{
	(void) iter_flags;

	sbomtool_spdx3_document_t *document = (sbomtool_spdx3_document_t *) ptr;
	pkgconfcli_serialize_value_t *sbom_obj = NULL;
	pkgconfcli_serialize_value_t *sbom_package = NULL;

	if (pkg->flags & PKGCONF_PKG_PROPF_VIRTUAL)
		return;

	if (!document->sbom_document)
	{
		document->sbom_document = sbomtool_spdx3_core_spdx_document(client, document);

		if (!document->sbom_document)
			goto err;

		if (!pkgconfcli_serialize_array_add_string(document->elements, document->agent_spdx_id))
			goto err;
	}

	sbom_obj = sbomtool_spdx3_software_sbom(client, document, pkg->id);

	if (!sbom_obj)
		goto err;

	sbom_package = sbomtool_spdx3_software_package(client, document, pkg, sbom_obj);

	if (!sbom_package)
		goto err;

	const char *ptr_spdx_id = software_get_string(sbom_obj, "spdxId");

	if (!pkgconfcli_serialize_array_add_string(document->elements, ptr_spdx_id))
		goto err;

	ptr_spdx_id = software_get_string(sbom_package, "spdxId");

	if (!pkgconfcli_serialize_array_add_string(document->elements, ptr_spdx_id))
		goto err;

	if (!pkgconfcli_serialize_array_add_take(document->software_sbom, sbom_obj))
	{
		sbom_obj = NULL;
		goto err;
	}

	sbom_obj = NULL;

	if (!pkgconfcli_serialize_array_add_take(document->packages, sbom_package))
	{
		sbom_package = NULL;
		goto err;
	}

	sbom_package = NULL;

	return;

err:
	pkgconfcli_serialize_value_free(sbom_obj);
	pkgconfcli_serialize_value_free(sbom_package);
}
