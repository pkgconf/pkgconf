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
 * .. c:function:: void spdxtool_software_sbom_serialize(pkgconf_client_t *client, pkgconf_buffer_t *buffer, spdxtool_software_sbom_t *sbom, bool last)
 *
 *    Serialize /Software/Sbom struct to JSON
 *
 *    :param pkgconf_client_t *client: The pkgconf client being accessed.
 *    :param pkgconf_buffer_t *buffer: Buffer where struct is serialized
 *    :param spdxtool_software_sbom_t *sbom: SimpleLicensingText struct to be serialized
 *    :param bool last: Is this last CreationInfo struct or does it need comma at the end. True comma False not
 *    :return: nothing
 */
void
spdxtool_software_sbom_serialize(pkgconf_client_t *client, pkgconf_buffer_t *buffer, spdxtool_software_sbom_t *sbom, bool last)
{
	pkgconf_node_t *node = NULL;
	char *value;
	char *spdx_id = spdxtool_util_tuple_lookup(client, &sbom->rootElement->vars, "spdxId");

	spdxtool_serialize_obj_start(buffer, 2);
	spdxtool_serialize_parm_and_string(buffer, "type", sbom->type, 3, true);
	spdxtool_serialize_parm_and_string(buffer, "creationInfo", sbom->creation_info, 3, true);
	spdxtool_serialize_parm_and_string(buffer, "spdxId", sbom->spdx_id, 3, true);
	spdxtool_serialize_parm_and_char(buffer, "software_sbomType", '[', 3, false);
	spdxtool_serialize_string(buffer, sbom->sbom_type, 4, false);
	spdxtool_serialize_array_end(buffer, 3, true);
	spdxtool_serialize_parm_and_char(buffer, "rootElement", '[', 3, false);
	spdxtool_serialize_string(buffer, spdx_id, 4, false);
	spdxtool_serialize_array_end(buffer, 3, true);
	spdxtool_serialize_parm_and_char(buffer, "element", '[', 3, false);
	PKGCONF_FOREACH_LIST_ENTRY(sbom->rootElement->required.head, node)
	{
		pkgconf_dependency_t *dep = node->data;
		pkgconf_pkg_t *match = dep->match;
		pkgconf_buffer_t relationship_buf = PKGCONF_BUFFER_INITIALIZER;
		char *spdx_id_relation = NULL;

		// New relation spdxId
		pkgconf_buffer_append_fmt(&relationship_buf, "%s/dependsOn/%s", sbom->rootElement->id, match->id);
		char *relationship_str = pkgconf_buffer_freeze(&relationship_buf);
		if(!relationship_str)
		{
			pkgconf_error(client, "spdxtool_software_sbom_serialize: out of memory");
			return;
		}

		spdx_id_relation = spdxtool_util_get_spdx_id_string(client, "Relationship", relationship_str);
		free(relationship_str);

		spdxtool_serialize_string(buffer, spdx_id_relation, 4, true);

		spdxtool_core_spdx_document_add_element(client, sbom->spdx_document, spdx_id_relation);

		free(spdx_id_relation);
	}

	value = spdxtool_util_tuple_lookup(client, &sbom->rootElement->vars, "hasDeclaredLicense");
	if (value != NULL)
	{
		spdxtool_serialize_string(buffer, value, 4, true);
		spdxtool_core_spdx_document_add_element(client, sbom->spdx_document, value);
		free(value);
	}

	value = spdxtool_util_tuple_lookup(client, &sbom->rootElement->vars, "hasConcludedLicense");
	if (value != NULL)
	{
		spdxtool_serialize_string(buffer, value, 4, false);
		spdxtool_core_spdx_document_add_element(client, sbom->spdx_document, value);
		free(value);
	}

	spdxtool_serialize_array_end(buffer, 3, false);
	spdxtool_serialize_obj_end(buffer, 2, last);

	spdxtool_software_package_serialize(client, buffer, sbom->rootElement, false);

	free(spdx_id);
}

/*
 * !doc
 *
 * .. c:function:: void spdxtool_software_sbom_serialize(pkgconf_client_t *client, pkgconf_buffer_t *buffer, spdxtool_software_sbom_t *sbom, bool last)
 *
 *    Serialize /Software/Package struct to JSON
 *    This is bit special case as it takes pkgconf pkg struct and serializes that one
 *    There is not new or free for this stype
 *
 *    :param pkgconf_client_t *client: The pkgconf client being accessed.
 *    :param pkgconf_buffer_t *buffer: Buffer where struct is serialized
 *    :param pkgconf_pkg_t *pkg: Pkgconf package
 *    :param bool last: Is this last CreationInfo struct or does it need comma at the end. True comma False not
 *    :return: nothing
 */
void
spdxtool_software_package_serialize(pkgconf_client_t *client, pkgconf_buffer_t *buffer, pkgconf_pkg_t *pkg, bool last)
{
	spdxtool_core_relationship_t *relationship_struct = NULL;
	char *spdx_id_license = NULL, *tuple_license = NULL;
	char *creation_info = spdxtool_util_tuple_lookup(client, &pkg->vars, "creationInfo");
	char *spdx_id = spdxtool_util_tuple_lookup(client, &pkg->vars, "spdxId");
	char *agent = spdxtool_util_tuple_lookup(client, &pkg->vars, "agent");
	pkgconf_node_t *node = NULL;

	(void) last;

	spdxtool_serialize_obj_start(buffer, 2);
	spdxtool_serialize_parm_and_string(buffer, "type", "software_Package", 3, true);
	spdxtool_serialize_parm_and_string(buffer, "creationInfo", creation_info, 3, true);
	spdxtool_serialize_parm_and_string(buffer, "spdxId", spdx_id, 3, true);
	spdxtool_serialize_parm_and_string(buffer, "name", pkg->realname, 3, true);
	spdxtool_serialize_parm_and_char(buffer, "originatedBy", '[', 3, false);
	spdxtool_serialize_string(buffer, agent, 4, false);
	spdxtool_serialize_array_end(buffer, 3, true);
	spdxtool_serialize_parm_and_string(buffer, "software_copyrightText", "NOASSERTION", 3, true);
	spdxtool_serialize_parm_and_string(buffer, "software_packageVersion", pkg->version, 3, false);
	spdxtool_serialize_obj_end(buffer, 2, true);

	/* These are must so add them right a way */
	spdx_id_license = spdxtool_util_get_spdx_id_string(client, "simplelicensing_LicenseExpression", pkg->license);

	tuple_license = spdxtool_util_tuple_lookup(client, &pkg->vars, "hasDeclaredLicense");
	if (tuple_license != NULL)
	{
		relationship_struct = spdxtool_core_relationship_new(client, creation_info, tuple_license, spdx_id, spdx_id_license, "hasDeclaredLicense");
		free(tuple_license);
		spdxtool_core_relationship_serialize(client, buffer, relationship_struct, true);
		spdxtool_core_relationship_free(relationship_struct);
	}

	tuple_license = spdxtool_util_tuple_lookup(client, &pkg->vars, "hasConcludedLicense");
	if (tuple_license != NULL)
	{
		relationship_struct = spdxtool_core_relationship_new(client, creation_info, tuple_license, spdx_id, spdx_id_license, "hasConcludedLicense");
		free(tuple_license);
		spdxtool_core_relationship_serialize(client, buffer, relationship_struct, true);
		spdxtool_core_relationship_free(relationship_struct);
	}

	free(spdx_id_license);

	PKGCONF_FOREACH_LIST_ENTRY(pkg->required.head, node)
	{
		pkgconf_dependency_t *dep = node->data;
		pkgconf_pkg_t *match = dep->match;
		pkgconf_buffer_t relationship_buf = PKGCONF_BUFFER_INITIALIZER;
		char *spdx_id_relation = NULL;
		char *spdx_id_package = NULL;

		// New relation spdxId
		pkgconf_buffer_append_fmt(&relationship_buf, "%s/dependsOn/%s", pkg->id, match->id);
		char *relationship_str = pkgconf_buffer_freeze(&relationship_buf);
		if (!relationship_str)
		{
			pkgconf_error(client, "spdxtool_software_package_serialize: out of memory");
			return;
		}

		spdx_id_relation = spdxtool_util_get_spdx_id_string(client, "Relationship", relationship_str);
		free(relationship_str);
		if (!spdx_id_relation)
		{
			pkgconf_error(client, "spdxtool_software_package_serialize: out of memory");
			return;
		}

		// new package spdxId which will be hopefully come later on
		spdx_id_package = spdxtool_util_get_spdx_id_string(client, "Package", match->id);

		relationship_struct = spdxtool_core_relationship_new(client, creation_info, spdx_id_relation, spdx_id, spdx_id_package, "dependsOn");
		spdxtool_core_relationship_serialize(client, buffer, relationship_struct, true);
		spdxtool_core_relationship_free(relationship_struct);
		free(spdx_id_relation);
		free(spdx_id_package);
	}

	free(creation_info);
	free(spdx_id);
	free(agent);
}
