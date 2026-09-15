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
#include "component.h"
#include "dependencies.h"
#include "util.h"


static bool
component_copyright_lines_to_object(pkgconfcli_serialize_object_list_t *object_list, const pkgconf_list_t *copyright_lines)
{
	pkgconf_buffer_t copyright_buf = PKGCONF_BUFFER_INITIALIZER;
	const pkgconf_node_t *node;

	if (copyright_lines->head == NULL)
		return true;

	PKGCONF_FOREACH_LIST_ENTRY(copyright_lines->head, node)
	{
		const pkgconf_bufferset_t *set = node->data;
		if (!pkgconf_buffer_join(&copyright_buf, '\n', pkgconf_buffer_str_or_empty(&set->buffer), NULL))
		{
			pkgconf_buffer_finalize(&copyright_buf);
			return false;
		}
	}

	bool ok = pkgconfcli_serialize_object_add_string(object_list, "copyright", pkgconf_buffer_str_or_empty(&copyright_buf)) != NULL;
	pkgconf_buffer_finalize(&copyright_buf);
	return ok;
}


/*
* !doc
*
* .. c:function:: pkgconfcli_serialize_value_t * sbomtool_cyclonedx_component_new(pkgconf_client_t *client, cyclonedx_component_type_t type, const char *name, const char *version, bool additional)
*
*    Create Component object
*
*    :param pkgconf_client_t *client: Pkgconf client struct
*    :param cyclonedx_component_type_t type: Type for this SBOM component
*    :param const char *name: Name for the component
*    :param const char *version: Version for the component
*    :param bool additional: Should licenses and externalReferences to be added
*    :return: pkgconfcli_serialize_value_t * representing the component object.
*/
pkgconfcli_serialize_value_t *
sbomtool_cyclonedx_component_new(pkgconf_client_t *client, cyclonedx_component_type_t type, const char *name, const char *version, bool additional)
{
	if (!client || !name || !version)
		return false;

	pkgconfcli_serialize_object_list_t *object_list = pkgconfcli_serialize_object_list_new();
	pkgconfcli_serialize_value_t *ret = NULL;
	const char *string_type = NULL;
	char *bom_ref = NULL;

	if (!object_list)
		goto err;

	switch (type)
	{
	case CYCLONEX_COMPONENT_APPLICATION:
		string_type = "application";
		break;
	case CYCLONEX_COMPONENT_FRAMEWORK:
		string_type = "framework";
		break;
	case CYCLONEX_COMPONENT_LIBRARY:
		string_type = "library";
		break;
	case CYCLONEX_COMPONENT_CONTAINER:
		string_type = "container";
		break;
	case CYCLONEX_COMPONENT_PLATFORM:
		string_type = "platform";
		break;
	case CYCLONEX_COMPONENT_OPERATING_SYSTEM:
		string_type = "operating-system";
		break;
	case CYCLONEX_COMPONENT_DEVICE:
		string_type = "device";
		break;
	case CYCLONEX_COMPONENT_DEVICE_DRIVER:
		string_type = "device-driver";
		break;
	case CYCLONEX_COMPONENT_FIRMWARE:
		string_type = "firmware";
		break;
	case CYCLONEX_COMPONENT_FILE:
		string_type = "file";
		break;
	case CYCLONEX_COMPONENT_MACHINE_LEARNING_MODEL:
		string_type = "machine-learning-model";
		break;
	case CYCLONEX_COMPONENT_DATA:
		string_type = "data";
		break;
	case CYCLONEX_COMPONENT_CRYPTOGRAHIC_ASSET:
		string_type = "cryptographic-asset";
		break;
	default:
		string_type = "application";
		break;

	}

	bom_ref = sbomtool_util_get_sbom_id_string(client, "component", name);
	if (!bom_ref)
		goto err;

	if (!pkgconfcli_serialize_object_add_string(object_list, "bom-ref", bom_ref))
		goto err;

	if (!(pkgconfcli_serialize_object_add_string(object_list, "type", string_type) &&
	        pkgconfcli_serialize_object_add_string(object_list, "name", name) &&
	        pkgconfcli_serialize_object_add_string(object_list, "version", version)))
		goto err;

	if (additional)
	{
		if (!(pkgconfcli_serialize_object_add_array(object_list, "licenses", pkgconfcli_serialize_array_new()) &&
		        pkgconfcli_serialize_object_add_array(object_list, "externalReferences", pkgconfcli_serialize_array_new())))
			goto err;
	}

	ret = pkgconfcli_serialize_value_object(object_list);
	object_list = NULL;

err:
	if (!ret)
		pkgconf_error(client, "sbomtool_cyclonedx_component_new: out of memory");

	free(bom_ref);
	pkgconfcli_serialize_object_list_free(object_list);
	return ret;
}

/*
* !doc
*
* .. c:function:: bool  sbomtool_cyclondex_component_add_license(pkgconf_client_t *client, pkgconfcli_serialize_array_t *licenses, const char *id)
*
*    Add license to component object
*
*    :param pkgconf_client_t *client: Pkgconf client struct
*    :param pkgconfcli_serialize_array_t *licenses: Type for this SBOM component
*    :param const char *id: Name for the component
*    :return: pkgconfcli_serialize_value_t * representing the component object.
*/
bool
sbomtool_cyclondex_component_add_license(pkgconf_client_t *client, pkgconfcli_serialize_array_t *licenses, const char *id)
{
	if (!client || !licenses || !id)
		return false;

	pkgconfcli_serialize_object_list_t *object_list = pkgconfcli_serialize_object_list_new();
	pkgconfcli_serialize_object_list_t *object_license_list = pkgconfcli_serialize_object_list_new();

	if (!object_list || !object_license_list)
		return false;

	if (!pkgconfcli_serialize_object_add_string(object_license_list, "id", id))
		goto err;

	pkgconfcli_serialize_object_add_object(object_list, "license", object_license_list);
	pkgconfcli_serialize_array_add_object(licenses, object_list);

	return true;
err:
	pkgconfcli_serialize_object_list_free(object_list);

	pkgconf_error(client, "sbomtool_cyclondex_component_add_license: out of memory");
	return false;

}

/*
* !doc
*
* .. c:function:: bool sbomtool_cyclondex_component_add_externalReference(pkgconf_client_t *client, pkgconfcli_serialize_array_t *ext_ref, cyclonedx_component_ext_t type, const char *url)
*
*    External references provide a way to document systems, sites, and information which is external to SBOM
*
*    :param pkgconf_client_t *client: Pkgconf client struct
*    :param pkgconfcli_serialize_array_t *ext_ref: External reference array
*    :param cyclonedx_component_ext_t type: Which kind of external reference this is?
*    :param onst char *url: URL to resource
*    :return: pkgconfcli_serialize_value_t * representing the component object.
*/
bool
sbomtool_cyclondex_component_add_externalReference(pkgconf_client_t *client, pkgconfcli_serialize_array_t *ext_ref, cyclonedx_component_ext_t type, const char *url)
{
	if (!client || !ext_ref || !url)
		return false;

	const char *type_str = "website";
	pkgconfcli_serialize_object_list_t *object_list = pkgconfcli_serialize_object_list_new();

	if (!object_list)
		return false;

	switch (type)
	{
	case CYCLONEX_COMPONENT_EXT_VCS:
		type_str = "vcs";
		break;
	case CYCLONEX_COMPONENT_EXT_ISSUE_TRACKER:
		type_str = "issue-tracker";
		break;
	case CYCLONEX_COMPONENT_EXT_WEBSITE:
		type_str = "website";
		break;
	case CYCLONEX_COMPONENT_EXT_ADVISORIES:
		type_str = "advisorites";
		break;
	case CYCLONEX_COMPONENT_EXT_BOM:
		type_str = "bom";
		break;
	case CYCLONEX_COMPONENT_EXT_MAILING_LIST:
		type_str = "mailing-list";
		break;
	case CYCLONEX_COMPONENT_EXT_SOCIAL:
		type_str = "social";
		break;
	case CYCLONEX_COMPONENT_EXT_CHAT:
		type_str = "chat";
		break;
	case CYCLONEX_COMPONENT_EXT_DOCUMENTATION:
		type_str = "documentation";
		break;
	case CYCLONEX_COMPONENT_EXT_SUPPORT:
		type_str = "support";
		break;
	case CYCLONEX_COMPONENT_EXT_SOURCE_DISTRIBUTION:
		type_str = "source-distribution";
		break;
	case CYCLONEX_COMPONENT_EXT_DISTRIBUTION:
		type_str = "distribution";
		break;
	case CYCLONEX_COMPONENT_EXT_DISTRIBUTION_INTAKE:
		type_str = "distribution-instake";
		break;
	case CYCLONEX_COMPONENT_EXT_DISTRIBUTION_LICENSE:
		type_str = "distribution-license";
		break;
	case CYCLONEX_COMPONENT_EXT_BUILD_META:
		type_str = "build-meta";
		break;
	case CYCLONEX_COMPONENT_EXT_BUILD_SYSTEM:
		type_str = "build-system";
		break;
	case CYCLONEX_COMPONENT_EXT_RELEASE_NOTES:
		type_str = "release-notes";
		break;
	case CYCLONEX_COMPONENT_EXT_SECURITY_CONTACT:
		type_str = "security-contact";
		break;
	case CYCLONEX_COMPONENT_EXT_MODEL_CARD:
		type_str = "model-card";
		break;
	case CYCLONEX_COMPONENT_EXT_LOG:
		type_str = "log";
		break;
	case CYCLONEX_COMPONENT_EXT_CONFIGURATION:
		type_str = "configuration";
		break;
	case CYCLONEX_COMPONENT_EXT_EVIDENCE:
		type_str = "evidence";
		break;
	case CYCLONEX_COMPONENT_EXT_FORMULATION:
		type_str = "formulation";
		break;
	case CYCLONEX_COMPONENT_EXT_ATTESTATION:
		type_str = "attestation";
		break;
	case CYCLONEX_COMPONENT_EXT_THREAD_MODEL:
		type_str = "thread-model";
		break;
	case CYCLONEX_COMPONENT_EXT_ADVESARY_MODEL:
		type_str = "advesary-model";
		break;
	case CYCLONEX_COMPONENT_EXT_RISK_ASSESSMENT:
		type_str = "risk-assement";
		break;
	case CYCLONEX_COMPONENT_EXT_VULNERABILITY_ASSERTION:
		type_str = "vulnerability-assertion";
		break;
	case CYCLONEX_COMPONENT_EXT_EXPLOITABILITY_STATEMENT:
		type_str = "exploitability-statement";
		break;
	case CYCLONEX_COMPONENT_EXT_PENTEST_REPORT:
		type_str = "pentest-report";
		break;
	case CYCLONEX_COMPONENT_EXT_STATIC_ANALYSIS_REPORT:
		type_str = "static-analysis-report";
		break;
	case CYCLONEX_COMPONENT_EXT_DYNAMIC_ANALYSIS_REPORT:
		type_str = "dynamic-analysis-report";
		break;
	case CYCLONEX_COMPONENT_EXT_RUNTIME_ANALYSIS_REPORT:
		type_str = "runtime-analysis-report";
		break;
	case CYCLONEX_COMPONENT_EXT_COMPONENT_ANALYSIS_REPORT:
		type_str = "component-analysis-report";
		break;
	case CYCLONEX_COMPONENT_EXT_MATURITY_REPORT:
		type_str = "maturity-report";
		break;
	case CYCLONEX_COMPONENT_EXT_CERTIFICATION_REPORT:
		type_str = "certification-report";
		break;
	case CYCLONEX_COMPONENT_EXT_CODIFIED_INDRASTRUCTURE:
		type_str = "codified-infrastructure";
		break;
	case CYCLONEX_COMPONENT_EXT_QUALITY_METRICS:
		type_str = "quality-metrics";
		break;
	case CYCLONEX_COMPONENT_EXT_POAM:
		type_str = "poam";
		break;
	case CYCLONEX_COMPONENT_EXT_DIGITAL_SIGNATURE:
		type_str = "digital-signature";
		break;
	case CYCLONEX_COMPONENT_EXT_RFC_9116:
		type_str = "rfc-9116";
		break;
	case CYCLONEX_COMPONENT_EXT_PATENT:
		type_str = "patent";
		break;
	case CYCLONEX_COMPONENT_EXT_PATENT_FAMILY:
		type_str = "patent-family";
		break;
	case CYCLONEX_COMPONENT_EXT_PATENT_ASSERTION:
		type_str = "patent-assertion";
		break;
	case CYCLONEX_COMPONENT_EXT_CITATION:
		type_str = "citation";
		break;
	case CYCLONEX_COMPONENT_EXT_OTHER:
		type_str = "other";
		break;
	default:
		type_str = "website";
		break;
	}

	if (!(pkgconfcli_serialize_object_add_string(object_list, "type", type_str) &&
	        pkgconfcli_serialize_object_add_string(object_list, "url", url)))
		goto err;

	pkgconfcli_serialize_array_add_object(ext_ref, object_list);

	return true;

err:
	pkgconfcli_serialize_object_list_free(object_list);

	pkgconf_error(client, "sbomtool_cyclondex_component_add_externalReference: out of memory");
	return false;

}

/*
 * !doc
 *
 * .. c:function:: bool sbomtool_cyclondex_component_add_author(pkgconf_client_t *client, pkgconfcli_serialize_array_t *authors, const char *name, const char *email, const char *phone)
 *
 *    Add author to component
 *
 *    :param pkgconf_client_t *client: Pkgconf client struct
 *    :param pkgconfcli_serialize_array_t *authors: Package struct
 *    :param const char *name: Name of author
 *    :param const char *email: Email of author
 *    :param const char *phone: Phone number of author
 *    :return: True if everything goes fine false if not
 */
bool
sbomtool_cyclondex_component_add_author(pkgconf_client_t *client, pkgconfcli_serialize_array_t *authors, const char *name, const char *email, const char *phone)
{
	(void) email;
	(void) phone;

	if (!client || !authors || !name)
		return false;

	pkgconfcli_serialize_object_list_t *object_list = pkgconfcli_serialize_object_list_new();

	if (!object_list)
		return false;

	if(!(pkgconfcli_serialize_object_add_string(object_list, "name", name) &&
	        pkgconfcli_serialize_object_add_string(object_list, "email", email) &&
	        pkgconfcli_serialize_object_add_string(object_list, "phone", phone)))
		goto err;

	pkgconfcli_serialize_array_add_object(authors, object_list);

	return true;

err:
	pkgconfcli_serialize_object_list_free(object_list);
	pkgconf_error(client, "sbomtool_cyclondex_component_add_maintainer: out of memory");
	return false;
}

/*
 * !doc
 *
 * .. c:function:: void sbomtool_cyclondex_component_add_package(pkgconf_client_t *client, pkgconf_pkg_t *pkg, void *ptr, unsigned int iter_flags)
 *
 *    This should be passes to package traverse function as callback. Adds components to CyclonDX SBOM
 *
 *    :param pkgconf_client_t *client: Pkgconf client struct
 *    :param  pkgconf_pkg_t *pkg: Package struct
 *    :param void *ptr: This should be CycloneDX struct
 *    :param onst char *iter_flags: Flags which are passed to function
 */
void
sbomtool_cyclondex_component_add_package(pkgconf_client_t *client, pkgconf_pkg_t *pkg, void *ptr, unsigned int iter_flags)
{
	(void) iter_flags;
	pkgconf_node_t *iter = NULL;

	if (!ptr)
		return;

	if (pkg->flags & PKGCONF_PKG_PROPF_VIRTUAL)
		return;

	pkgconfcli_serialize_value_t *cyclonedx_doc = (pkgconfcli_serialize_value_t *) ptr;
	pkgconfcli_serialize_value_t *components = pkgconfcli_serialize_value_object_find(cyclonedx_doc, "components");
	pkgconfcli_serialize_value_t *dependencies = pkgconfcli_serialize_value_object_find(cyclonedx_doc, "dependencies");
	pkgconfcli_serialize_value_t *component = NULL;
	pkgconfcli_serialize_value_t *licenses = NULL;
	pkgconfcli_serialize_value_t *ext_ref = NULL;
	pkgconfcli_serialize_value_t *component_bom_ref = NULL;
	pkgconfcli_serialize_value_t *dependency = NULL;
	pkgconfcli_serialize_value_t *dependency_depends_on = NULL;
	pkgconfcli_serialize_array_t *authors = NULL;
	char *dep_id = NULL;

	bool add_depens = false;

	if (!components || !dependencies)
	{
		goto err;
		return;
	}

	component = sbomtool_cyclonedx_component_new(client, CYCLONEX_COMPONENT_APPLICATION, pkg->id, pkg->version, true);

	if (!component)
		goto err;

	licenses = pkgconfcli_serialize_value_object_find(component, "licenses");
	ext_ref = pkgconfcli_serialize_value_object_find(component, "externalReferences");
	component_bom_ref = pkgconfcli_serialize_value_object_find(component, "bom-ref");

	if (!licenses || !ext_ref || !component_bom_ref)
		goto err;

	if(!pkgconfcli_serialize_array_add_object(pkgconfcli_serialize_value_get_array(components), pkgconfcli_serialize_value_get_object(component)))
		goto err;

	if (pkg->url)
		if (!sbomtool_cyclondex_component_add_externalReference(client, pkgconfcli_serialize_value_get_array(ext_ref), CYCLONEX_COMPONENT_EXT_WEBSITE, pkg->url))
			goto err;

	if (pkg->source)
		if (!sbomtool_cyclondex_component_add_externalReference(client, pkgconfcli_serialize_value_get_array(ext_ref), CYCLONEX_COMPONENT_EXT_SOURCE_DISTRIBUTION, pkg->source))
			goto err;

	if (!component_copyright_lines_to_object(pkgconfcli_serialize_value_get_object(component), &pkg->copyright))
		goto err;

	if (pkg->maintainer != NULL)
	{
		authors = pkgconfcli_serialize_array_new();

		if (!authors)
			goto err;

		if (!pkgconfcli_serialize_object_add_array(pkgconfcli_serialize_value_get_object(component), "authors", authors))
			goto err;

		if (!sbomtool_cyclondex_component_add_author(client, authors, pkg->maintainer, "", ""))
			goto err;
	}

	/* Free pkgconfcli_serialize_value_t wrapper */
	free(component);
	component = NULL;

	PKGCONF_FOREACH_LIST_ENTRY(pkg->license.head, iter)
	{
		const pkgconf_license_t *license = iter->data;
		if (license->type == PKGCONF_LICENSE_EXPRESSION)
		{
			if(!sbomtool_cyclondex_component_add_license(client, pkgconfcli_serialize_value_get_array(licenses), license->data))
				goto err;
		}
	}

	if(pkg->required.head || pkg->requires_private.head)
		add_depens = true;

	dependency = sbomtool_cyclonedx_dependencies_new(client, pkgconfcli_serialize_value_get_string(component_bom_ref), add_depens);

	if (!dependency)
		goto err;

	if (!pkgconfcli_serialize_array_add_object(pkgconfcli_serialize_value_get_array(dependencies), pkgconfcli_serialize_value_get_object(dependency)))
		goto err;

	/* If there is not dependencies then just bail out */
	if (!add_depens)
	{
		free(dependency);
		return;
	}

	dependency_depends_on = pkgconfcli_serialize_value_object_find(dependency, "dependsOn");

	if (!dependency_depends_on)
		goto err;

	/* Free pkgconfcli_serialize_value_t wrapper */
	free(dependency);
	dependency = NULL;

	PKGCONF_FOREACH_LIST_ENTRY(pkg->required.head, iter)
	{
		pkgconf_dependency_t *dep = iter->data;
		pkgconf_pkg_t *match = dep->match;
		dep_id = sbomtool_util_get_sbom_id_string(client, "component", match->id);

		if (!dep_id)
			goto err;

		if (!pkgconfcli_serialize_array_add_string(pkgconfcli_serialize_value_get_array(dependency_depends_on), dep_id))
			goto err;

		free(dep_id);
		dep_id = NULL;
	}

	PKGCONF_FOREACH_LIST_ENTRY(pkg->requires_private.head, iter)
	{
		pkgconf_dependency_t *dep = iter->data;
		pkgconf_pkg_t *match = dep->match;
		dep_id = sbomtool_util_get_sbom_id_string(client, "component", match->id);

		if (!dep_id)
			goto err;

		if (!pkgconfcli_serialize_array_add_string(pkgconfcli_serialize_value_get_array(dependency_depends_on), dep_id))
			goto err;

		free(dep_id);
		dep_id = NULL;
	}


	return;

err:
	pkgconfcli_serialize_value_free(component);
	pkgconfcli_serialize_value_free(dependency);
	free(dep_id);

}
