
libpkgconf `dependency` module
==============================

The `dependency` module provides support for building `dependency lists` (the basic component of the overall `dependency graph`) and
`dependency nodes` which store dependency information.

.. c:function:: pkgconf_dependency_t *pkgconf_dependency_add(pkgconf_list_t *list, const char *package, const char *version, pkgconf_pkg_comparator_t compare)

   Adds a parsed dependency to a dependency list as a dependency node.

   :param pkgconf_client_t* client: The client object that owns the package this dependency list belongs to.
   :param pkgconf_list_t* list: The dependency list to add a dependency node to.
   :param char* package: The package `atom` to set on the dependency node.
   :param char* version: The package `version` to set on the dependency node.
   :param pkgconf_pkg_comparator_t compare: The comparison operator to set on the dependency node.
   :return: A dependency node.
   :rtype: pkgconf_dependency_t *

.. c:function:: void pkgconf_dependency_append(pkgconf_list_t *list, pkgconf_dependency_t *tail)

   Adds a dependency node to a pre-existing dependency list.

   :param pkgconf_list_t* list: The dependency list to add a dependency node to.
   :param pkgconf_dependency_t* tail: The dependency node to add to the tail of the dependency list.
   :return: nothing

.. c:function:: void pkgconf_dependency_free(pkgconf_list_t *list)

   Release a dependency list and it's child dependency nodes.

   :param pkgconf_list_t* list: The dependency list to release.
   :return: nothing

.. c:function:: void pkgconf_dependency_parse_str(pkgconf_list_t *deplist_head, const char *depends)

   Parse a dependency declaration into a dependency list.
   Commas are counted as whitespace to allow for constructs such as ``@SUBSTVAR@, zlib`` being processed
   into ``, zlib``.

   :param pkgconf_client_t* client: The client object that owns the package this dependency list belongs to.
   :param pkgconf_list_t* deplist_head: The dependency list to populate with dependency nodes.
   :param char* depends: The dependency data to parse.
   :return: nothing

.. c:function:: void pkgconf_dependency_parse(const pkgconf_client_t *client, pkgconf_pkg_t *pkg, pkgconf_list_t *deplist, const char *depends)

   Preprocess dependency data and then process that dependency declaration into a dependency list.
   Commas are counted as whitespace to allow for constructs such as ``@SUBSTVAR@, zlib`` being processed
   into ``, zlib``.

   :param pkgconf_client_t* client: The client object that owns the package this dependency list belongs to.
   :param pkgconf_pkg_t* pkg: The package object that owns this dependency list.
   :param pkgconf_list_t* deplist: The dependency list to populate with dependency nodes.
   :param char* depends: The dependency data to parse.
   :return: nothing
