/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 *​ Copyright (c) 2025 The FreeBSD Foundation
 *​
 *​ Portions of this software were developed by
 * Tuukka Pasanen <tuukka.pasanen@ilmi.fi> under sponsorship from
 * the FreeBSD Foundation
 */

#include <stdlib.h>
#include <string.h>
#include <libpkgconf/libpkgconf.h>
#include "util.h"
#include "serialize.h"
#include "software.h"
#include "core.h"

/*
 * !doc
 *
 * .. c:function:: spdxtool_software_sbom_t *spdxtool_software_sbom_new(pkgconf_client_t *client, const char *spdx_id, const char *creation_id, const char *sbom_type)
 *
 *    Create new /Software/Sbom struct
 *
 *    :param pkgconf_client_t *client: The pkgconf client being accessed.
 *    :param const char *spdx_id: spdxId for this SBOM element
 *    :param const char *creation_id: id for CreationInfo
 *    :param const char *sbom_type: Sbom types can be found SPDX documention
 *    :return: NULL if some problem occurs and Sbom struct if not
 */
spdxtool_software_sbom_t *
spdxtool_software_sbom_new(pkgconf_client_t *client, const char *spdx_id, const char *creation_id, const char *sbom_type)
{
	spdxtool_software_sbom_t *sbom = NULL;

	if(!client || !spdx_id || !creation_id || !sbom_type)
	{
		return NULL;
	}

	sbom = calloc(1, sizeof(spdxtool_software_sbom_t));
	if(!sbom)
	{
		pkgconf_error(client, "Memory exhausted! Can't create sbom struct.");
		return NULL;
	}

	sbom->type="software_Sbom";
	sbom->spdx_id = strdup(spdx_id);
	sbom->creation_info = strdup(creation_id);
	sbom->sbom_type = strdup(sbom_type);

	if(!sbom->spdx_id || !sbom->creation_info || !sbom->sbom_type)
	{
		pkgconf_error(client, "Memory exhausted! Can't create sbom struct.");
		spdxtool_software_sbom_free(sbom);
		return NULL;
	}

	return sbom;
}

/*
 * !doc
 *
 * .. c:function:: void spdxtool_software_sbom_free(spdxtool_software_sbom_t *sbom)
 *
 *    Free /Software/Sbom struct
 *
 *    :param spdxtool_software_sbom_t *sbom: Sbom struct to be freed.
 *    :return: nothing
 */
void
spdxtool_software_sbom_free(spdxtool_software_sbom_t *sbom)
{

	if(!sbom)
	{
		return;
	}

	free(sbom->spdx_id);
	free(sbom->creation_info);
	free(sbom->sbom_type);

	free(sbom);
}

/*
 * !doc
 *
 * .. c:function:: spdxtool_serialize_value_t spdxtool_software_sbom_to_object(pkgconf_client_t *client, spdxtool_software_sbom_t *sbom)
 *
 *    Serialize /Software/Sbom struct to a JSON value tree. As a side effect,
 *    the package associated with the SBOM's rootElement is registered on the
 *    document via spdxtool_core_spdx_document_add_package, and relationship
 *    element IDs are registered via spdxtool_core_spdx_document_add_element.
 *
 *    :param pkgconf_client_t *client: The pkgconf client being accessed.
 *    :param spdxtool_software_sbom_t *sbom: Sbom struct to be serialized.
 *    :return: spdxtool_serialize_value_t representing the Sbom object.
 */
spdxtool_serialize_value_t
spdxtool_software_sbom_to_object(pkgconf_client_t *client, spdxtool_software_sbom_t *sbom)
{
	char *spdx_id = spdxtool_util_tuple_lookup(client, &sbom->rootElement->vars, "spdxId");
	char sep = spdxtool_util_get_uri_separator(client);
	if (!spdx_id)
	{
		pkgconf_error(client, "spdxtool_software_sbom_to_object: out of memory");
		return spdxtool_serialize_null();
	}

	spdxtool_serialize_object_list_t *object = spdxtool_serialize_object_list_new();
	if (!object)
	{
		pkgconf_error(client, "spdxtool_software_sbom_to_object: out of memory");
		free(spdx_id);
		return spdxtool_serialize_null();
	}

	// software_sbomType array
	spdxtool_serialize_array_t *sbom_type_array = spdxtool_serialize_array_new();
	if (!sbom_type_array)
	{
		pkgconf_error(client, "spdxtool_software_sbom_to_object: out of memory");
		spdxtool_serialize_object_list_free(object);
		free(spdx_id);
		return spdxtool_serialize_null();
	}
	spdxtool_serialize_array_add_string(sbom_type_array, sbom->sbom_type);

	// rootElement array
	spdxtool_serialize_array_t *root_element_array = spdxtool_serialize_array_new();
	if (!root_element_array)
	{
		pkgconf_error(client, "spdxtool_software_sbom_to_object: out of memory");
		spdxtool_serialize_object_list_free(object);
		free(spdx_id);
		return spdxtool_serialize_null();
	}
	spdxtool_serialize_array_add_string(root_element_array, spdx_id);

	// element array
	spdxtool_serialize_array_t *element_array = spdxtool_serialize_array_new();
	if (!element_array)
	{
		pkgconf_error(client, "spdxtool_software_sbom_to_object: out of memory");
		spdxtool_serialize_object_list_free(object);
		free(spdx_id);
		return spdxtool_serialize_null();
	}

	pkgconf_node_t *node = NULL;
	PKGCONF_FOREACH_LIST_ENTRY(sbom->rootElement->required.head, node)
	{
		pkgconf_dependency_t *dep = node->data;
		pkgconf_pkg_t *match = dep->match;
		pkgconf_buffer_t relationship_buf = PKGCONF_BUFFER_INITIALIZER;

		pkgconf_buffer_append_fmt(&relationship_buf, "%s%cdependsOn%c%s", sbom->rootElement->id, sep, sep, match->id);
		char *relationship_str = pkgconf_buffer_freeze(&relationship_buf);
		if (!relationship_str)
		{
			pkgconf_error(client, "spdxtool_software_sbom_to_object: out of memory");
			spdxtool_serialize_object_list_free(object);
			free(spdx_id);
			return spdxtool_serialize_null();
		}

		char *spdx_id_relation = spdxtool_util_get_spdx_id_string(client, "Relationship", relationship_str);
		free(relationship_str);
		if (!spdx_id_relation)
		{
			pkgconf_error(client, "spdxtool_software_sbom_to_object: out of memory");
			spdxtool_serialize_object_list_free(object);
			free(spdx_id);
			return spdxtool_serialize_null();
		}

		spdxtool_serialize_array_add_string(element_array, spdx_id_relation);
		spdxtool_core_spdx_document_add_element(client, sbom->spdx_document, spdx_id_relation);
		free(spdx_id_relation);
	}

	char *value = spdxtool_util_tuple_lookup(client, &sbom->rootElement->vars, "hasDeclaredLicense");
	if (value)
	{
		spdxtool_serialize_array_add_string(element_array, value);
		spdxtool_core_spdx_document_add_element(client, sbom->spdx_document, value);
		free(value);
	}

	value = spdxtool_util_tuple_lookup(client, &sbom->rootElement->vars, "hasConcludedLicense");
	if (value)
	{
		spdxtool_serialize_array_add_string(element_array, value);
		spdxtool_core_spdx_document_add_element(client, sbom->spdx_document, value);
		free(value);
	}

	spdxtool_serialize_object_add_string(object, "type", sbom->type);
	spdxtool_serialize_object_add_string(object, "creationInfo", sbom->creation_info);
	spdxtool_serialize_object_add_string(object, "spdxId", sbom->spdx_id);
	spdxtool_serialize_object_add_array(object, "software_sbomType", sbom_type_array);
	spdxtool_serialize_object_add_array(object, "rootElement", root_element_array);
	spdxtool_serialize_object_add_array(object, "element", element_array);

	// register package for serialization as a sibling in the graph
	spdxtool_core_spdx_document_add_package(client, sbom->spdx_document, sbom->rootElement);

	free(spdx_id);
	return spdxtool_serialize_object(object);
}

/*
 * !doc
 *
 * .. c:function:: spdxtool_serialize_value_t spdxtool_software_package_to_object(pkgconf_client_t *client, pkgconf_pkg_t *pkg, spdxtool_core_spdx_document_t *spdx)
 *
 *    Serialize /Software/Package struct to a JSON value tree. As a side effect,
 *    any license and dependency relationships generated during serialization are
 *    added to the document via spdxtool_core_spdx_document_add_relationship.
 *
 *    :param pkgconf_client_t *client: The pkgconf client being accessed.
 *    :param pkgconf_pkg_t *pkg: Package struct to be serialized.
 *    :param spdxtool_core_spdx_document_t *spdx: SpdxDocument to which generated relationships are added.
 *    :return: spdxtool_serialize_value_t representing the Package object.
 */
spdxtool_serialize_value_t
spdxtool_software_package_to_object(pkgconf_client_t *client, pkgconf_pkg_t *pkg, spdxtool_core_spdx_document_t *spdx)
{
	char *creation_info = spdxtool_util_tuple_lookup(client, &pkg->vars, "creationInfo");
	char *spdx_id = spdxtool_util_tuple_lookup(client, &pkg->vars, "spdxId");
	char *agent = spdxtool_util_tuple_lookup(client, &pkg->vars, "agent");
	char *spdx_id_license = NULL;
	pkgconf_list_t relations = PKGCONF_LIST_INITIALIZER;
	pkgconf_list_t *cpy_relations = NULL;
	pkgconf_node_t *node = NULL;
	char sep = spdxtool_util_get_uri_separator(client);

	if (!creation_info || !spdx_id || !agent)
	{
		pkgconf_error(client, "spdxtool_software_package_to_object: could not gather object info");
		free(creation_info);
		free(spdx_id);
		free(agent);
		return spdxtool_serialize_null();
	}

	spdxtool_serialize_object_list_t *object = spdxtool_serialize_object_list_new();
	if (!object)
	{
		pkgconf_error(client, "spdxtool_software_package_to_object: out of memory");
		free(creation_info);
		free(spdx_id);
		free(agent);
		return spdxtool_serialize_null();
	}

	spdxtool_serialize_array_t *originated_by = spdxtool_serialize_array_new();
	if (!originated_by)
	{
		pkgconf_error(client, "spdxtool_software_package_to_object: out of memory");
		free(creation_info);
		free(spdx_id);
		free(agent);
		return spdxtool_serialize_null();
	}

	spdxtool_serialize_array_add_string(originated_by, agent);
	spdxtool_serialize_object_add_string(object, "type", "software_Package");
	spdxtool_serialize_object_add_string(object, "creationInfo", creation_info);
	spdxtool_serialize_object_add_string(object, "spdxId", spdx_id);
	spdxtool_serialize_object_add_string(object, "name", pkg->realname);
	spdxtool_serialize_object_add_array(object, "originatedBy", originated_by);

	if (pkg->copyright != NULL)
		spdxtool_serialize_object_add_string(object, "software_copyrightText", pkg->copyright);
	else
		spdxtool_serialize_object_add_string(object, "software_copyrightText", "NOASSERTION");

	if (pkg->url != NULL)
		spdxtool_serialize_object_add_string(object, "software_homePage", pkg->url);
	else
		spdxtool_serialize_object_add_string(object, "software_homePage", "");

	if (pkg->source)
		spdxtool_serialize_object_add_string(object, "software_downloadLocation", pkg->source);
	else
		spdxtool_serialize_object_add_string(object, "software_downloadLocation", "");

	spdxtool_serialize_object_add_string(object, "software_packageVersion", pkg->version);

	PKGCONF_FOREACH_LIST_ENTRY(pkg->license.head, node)
	{
		const pkgconf_license_t *license = node->data;
		if (license->type == PKGCONF_LICENSE_EXPRESSION)
		{
			spdx_id_license = spdxtool_util_get_spdx_id_string(client, "simplelicensing_LicenseExpression", license->data);
			if (!spdx_id_license)
			{
				pkgconf_error(client, "spdxtool_software_package_to_object: could not get license");
				free(creation_info);
				free(spdx_id);
				free(agent);
				return spdxtool_serialize_null();
			}
			pkgconf_license_insert(client, &relations, PKGCONF_LICENSE_UNKNOWN, spdx_id_license);
			free(spdx_id_license);
		}
	}

	char *tuple_license = spdxtool_util_tuple_lookup(client, &pkg->vars, "hasDeclaredLicense");
	if (tuple_license)
	{
		cpy_relations = calloc(1, sizeof(pkgconf_list_t));
		pkgconf_license_copy_list(client, cpy_relations, &relations);
		spdxtool_core_relationship_t *relationship = spdxtool_core_relationship_new(client, creation_info, tuple_license, spdx_id, cpy_relations, "hasDeclaredLicense");
		free(tuple_license);
		if (relationship)
			spdxtool_core_spdx_document_add_relationship(client, spdx, relationship);
		cpy_relations = NULL;
	}

	tuple_license = spdxtool_util_tuple_lookup(client, &pkg->vars, "hasConcludedLicense");
	if (tuple_license)
	{
		cpy_relations = calloc(1, sizeof(pkgconf_list_t));
		pkgconf_license_copy_list(client, cpy_relations, &relations);
		spdxtool_core_relationship_t *relationship = spdxtool_core_relationship_new(client, creation_info, tuple_license, spdx_id, cpy_relations, "hasConcludedLicense");
		free(tuple_license);
		if (relationship)
			spdxtool_core_spdx_document_add_relationship(client, spdx, relationship);
		cpy_relations = NULL;
	}
	pkgconf_license_free(&relations);

	PKGCONF_FOREACH_LIST_ENTRY(pkg->required.head, node)
	{
		pkgconf_dependency_t *dep = node->data;
		pkgconf_pkg_t *match = dep->match;
		pkgconf_buffer_t relationship_buf = PKGCONF_BUFFER_INITIALIZER;

		pkgconf_buffer_append_fmt(&relationship_buf, "%s%cdependsOn%c%s", pkg->id, sep, sep, match->id);
		char *relationship_str = pkgconf_buffer_freeze(&relationship_buf);
		if (!relationship_str)
		{
			pkgconf_error(client, "spdxtool_software_package_to_object: out of memory");
			free(creation_info);
			free(spdx_id);
			free(agent);
			return spdxtool_serialize_null();
		}

		char *spdx_id_relation = spdxtool_util_get_spdx_id_string(client, "Relationship", relationship_str);
		free(relationship_str);
		if (!spdx_id_relation)
		{
			pkgconf_error(client, "spdxtool_software_package_to_object: out of memory");
			free(creation_info);
			free(spdx_id);
			free(agent);
			return spdxtool_serialize_null();
		}

		char *spdx_id_package = spdxtool_util_get_spdx_id_string(client, "Package", match->id);
		cpy_relations = calloc(1, sizeof(pkgconf_list_t));
		pkgconf_license_insert(client, cpy_relations, PKGCONF_LICENSE_UNKNOWN, spdx_id_package);
		spdxtool_core_relationship_t *relationship = spdxtool_core_relationship_new(client, creation_info, spdx_id_relation, spdx_id, cpy_relations, "dependsOn");
		free(spdx_id_relation);
		free(spdx_id_package);
		if (relationship)
			spdxtool_core_spdx_document_add_relationship(client, spdx, relationship);
	}

	free(creation_info);
	free(spdx_id);
	free(agent);
	return spdxtool_serialize_object(object);
}
