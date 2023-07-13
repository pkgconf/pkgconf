from ._libpkgconf import ffi, lib

class NodeIter:

    def __init__(self, pkgconf_list, ctype: str = "pkgconf_tuple_t*"):
        self._cur = pkgconf_list.head
        self._ctype = ctype

    def __iter__(self):
        return self

    def __next__(self):
        if self._cur == ffi.NULL:
            raise StopIteration
        data = self._cur.data
        self._cur = self._cur.next
        return ffi.cast(self._ctype, data)
