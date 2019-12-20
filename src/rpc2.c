#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <hadoofus/lowlevel.h>
#include <hadoofus/objects.h>

#include "heapbuf.h"
#include "objects-internal.h"
#include "rpc2-internal.h"
#include "util.h"

#include "ClientNamenodeProtocol.pb-c.h"

/* Support logic for v2+ Namenode RPC (requests) */

// passing NULL for dest only returns the packed size
#define ENCODE_PREAMBLE(lowerCamel, CamelCase, UPPER_CASE)		\
static size_t								\
_rpc2_encode_ ## lowerCamel (struct hdfs_heap_buf *dest,		\
	struct hdfs_rpc_invocation *rpc)				\
{									\
	Hadoop__Hdfs__ ## CamelCase ## RequestProto req =		\
	    HADOOP__HDFS__ ## UPPER_CASE ## _REQUEST_PROTO__INIT;	\
	size_t sz;

	/*
	 * Additional validation of input object and population of req
	 * structure go here.
	 */

// Consider an approach that avoids local mallocs/frees if possible
#define ENCODE_POSTSCRIPT_EX(lower_case, destructor_ex)					\
	sz = hadoop__hdfs__ ## lower_case ## _request_proto__get_packed_size(&req);	\
	if (dest) {									\
		_hbuf_reserve(dest, sz);						\
		hadoop__hdfs__ ## lower_case ## _request_proto__pack(&req,		\
		    (void *)_hbuf_writeptr(dest));					\
		_hbuf_append(dest, sz);							\
	}										\
	destructor_ex;									\
	return sz;									\
}

#define ENCODE_POSTSCRIPT(lower_case) ENCODE_POSTSCRIPT_EX(lower_case, )

/* New in v2 methods */
ENCODE_PREAMBLE(getServerDefaults, GetServerDefaults, GET_SERVER_DEFAULTS)
{
	ASSERT(rpc->_nargs == 0);
}
ENCODE_POSTSCRIPT(get_server_defaults)


/* Compat for v1 methods */
ENCODE_PREAMBLE(getListing, GetListing, GET_LISTING)
{
	ASSERT(rpc->_nargs == 2);
	ASSERT(rpc->_args[0]->ob_type == H_STRING);
	ASSERT(rpc->_args[1]->ob_type == H_ARRAY_BYTE);

	req.src = rpc->_args[0]->ob_val._string._val;
	req.startafter.len = rpc->_args[1]->ob_val._array_byte._len;
	req.startafter.data = (void *)rpc->_args[1]->ob_val._array_byte._bytes;

	/* TODO Add hdfs2-only method for specifying needlocation */
	req.needlocation = false;
}
ENCODE_POSTSCRIPT(get_listing)

ENCODE_PREAMBLE(getBlockLocations, GetBlockLocations, GET_BLOCK_LOCATIONS)
{
	ASSERT(rpc->_nargs == 3);
	ASSERT(rpc->_args[0]->ob_type == H_STRING);
	ASSERT(rpc->_args[1]->ob_type == H_LONG);
	ASSERT(rpc->_args[2]->ob_type == H_LONG);

	req.src = rpc->_args[0]->ob_val._string._val;
	req.offset = rpc->_args[1]->ob_val._long._val;
	req.length = rpc->_args[2]->ob_val._long._val;
}
ENCODE_POSTSCRIPT(get_block_locations)

ENCODE_PREAMBLE(create, Create, CREATE)
	Hadoop__Hdfs__FsPermissionProto perms = HADOOP__HDFS__FS_PERMISSION_PROTO__INIT;
{
	ASSERT(rpc->_nargs == 7);
	ASSERT(rpc->_args[0]->ob_type == H_STRING);
	ASSERT(rpc->_args[1]->ob_type == H_FSPERMS);
	ASSERT(rpc->_args[2]->ob_type == H_STRING);
	ASSERT(rpc->_args[3]->ob_type == H_BOOLEAN);
	ASSERT(rpc->_args[4]->ob_type == H_BOOLEAN);
	ASSERT(rpc->_args[5]->ob_type == H_SHORT);
	ASSERT(rpc->_args[6]->ob_type == H_LONG);

	req.src = rpc->_args[0]->ob_val._string._val;
	req.masked = &perms;
	perms.perm = rpc->_args[1]->ob_val._fsperms._perms;
	req.clientname = rpc->_args[2]->ob_val._string._val;
	req.createflag = HADOOP__HDFS__CREATE_FLAG_PROTO__CREATE |
	    (rpc->_args[3]->ob_val._boolean._val?
	     HADOOP__HDFS__CREATE_FLAG_PROTO__OVERWRITE : 0);
	req.createparent = rpc->_args[4]->ob_val._boolean._val;
	req.replication = rpc->_args[5]->ob_val._short._val;
	req.blocksize = rpc->_args[6]->ob_val._long._val;
}
ENCODE_POSTSCRIPT(create)

ENCODE_PREAMBLE(delete, Delete, DELETE)
{
	ASSERT(rpc->_nargs == 2);
	ASSERT(rpc->_args[0]->ob_type == H_STRING);
	ASSERT(rpc->_args[1]->ob_type == H_BOOLEAN);

	req.src = rpc->_args[0]->ob_val._string._val;
	req.recursive = rpc->_args[1]->ob_val._boolean._val;
}
ENCODE_POSTSCRIPT(delete)

ENCODE_PREAMBLE(append, Append, APPEND)
{
	ASSERT(rpc->_nargs == 2);
	ASSERT(rpc->_args[0]->ob_type == H_STRING);
	ASSERT(rpc->_args[1]->ob_type == H_STRING);

	req.src = rpc->_args[0]->ob_val._string._val;
	req.clientname = rpc->_args[1]->ob_val._string._val;
}
ENCODE_POSTSCRIPT(append)

ENCODE_PREAMBLE(setReplication, SetReplication, SET_REPLICATION)
{
	ASSERT(rpc->_nargs == 2);
	ASSERT(rpc->_args[0]->ob_type == H_STRING);
	ASSERT(rpc->_args[1]->ob_type == H_SHORT);

	req.src = rpc->_args[0]->ob_val._string._val;
	req.replication = rpc->_args[1]->ob_val._short._val;
}
ENCODE_POSTSCRIPT(set_replication)

ENCODE_PREAMBLE(setPermission, SetPermission, SET_PERMISSION)
	Hadoop__Hdfs__FsPermissionProto perms = HADOOP__HDFS__FS_PERMISSION_PROTO__INIT;
{
	ASSERT(rpc->_nargs == 2);
	ASSERT(rpc->_args[0]->ob_type == H_STRING);
	ASSERT(rpc->_args[1]->ob_type == H_FSPERMS);

	req.src = rpc->_args[0]->ob_val._string._val;
	perms.perm = rpc->_args[1]->ob_val._fsperms._perms;
	req.permission = &perms;
}
ENCODE_POSTSCRIPT(set_permission)

ENCODE_PREAMBLE(setOwner, SetOwner, SET_OWNER)
{
	ASSERT(rpc->_nargs == 3);
	ASSERT(rpc->_args[0]->ob_type == H_STRING);
	ASSERT(rpc->_args[1]->ob_type == H_STRING);
	ASSERT(rpc->_args[2]->ob_type == H_STRING);

	req.src = rpc->_args[0]->ob_val._string._val;
	req.username = rpc->_args[1]->ob_val._string._val;
	req.groupname = rpc->_args[2]->ob_val._string._val;
}
ENCODE_POSTSCRIPT(set_owner)

ENCODE_PREAMBLE(complete, Complete, COMPLETE)
	Hadoop__Hdfs__ExtendedBlockProto last = HADOOP__HDFS__EXTENDED_BLOCK_PROTO__INIT;
{
	ASSERT(rpc->_nargs >= 2 && rpc->_nargs <= 4);
	ASSERT(rpc->_args[0]->ob_type == H_STRING);
	ASSERT(rpc->_args[1]->ob_type == H_STRING);
	ASSERT(rpc->_nargs <= 2 ||
	    rpc->_args[2]->ob_type == H_BLOCK ||
	    (rpc->_args[2]->ob_type == H_NULL
	     && rpc->_args[2]->ob_val._null._type == H_BLOCK));
	ASSERT(rpc->_nargs <= 3 || rpc->_args[3]->ob_type == H_LONG);

	req.src = rpc->_args[0]->ob_val._string._val;
	req.clientname = rpc->_args[1]->ob_val._string._val;

	if (rpc->_nargs >= 3 && rpc->_args[2]->ob_type != H_NULL) {
		struct hdfs_object *bobj = rpc->_args[2];

		last.blockid = bobj->ob_val._block._blkid;
		last.has_numbytes = true;
		last.numbytes  = bobj->ob_val._block._length;
		last.generationstamp = bobj->ob_val._block._generation;
		last.poolid = bobj->ob_val._block._pool_id;

		req.last = &last;
	} else {
		req.last = NULL;
	}

	if (rpc->_nargs >= 4) {
		struct hdfs_object *fobj = rpc->_args[3];

		if (fobj->ob_val._long._val != 0) {
			req.has_fileid = true;
			req.fileid = fobj->ob_val._long._val;
		}
	}
}
ENCODE_POSTSCRIPT(complete)

ENCODE_PREAMBLE(abandonBlock, AbandonBlock, ABANDON_BLOCK)
	Hadoop__Hdfs__ExtendedBlockProto eb = HADOOP__HDFS__EXTENDED_BLOCK_PROTO__INIT;
{
	ASSERT(rpc->_nargs == 3);
	ASSERT(rpc->_args[0]->ob_type == H_BLOCK);
	ASSERT(rpc->_args[0]->ob_val._block._pool_id);
	ASSERT(rpc->_args[1]->ob_type == H_STRING);
	ASSERT(rpc->_args[2]->ob_type == H_STRING);

	eb.poolid = rpc->_args[0]->ob_val._block._pool_id;
	eb.blockid = rpc->_args[0]->ob_val._block._blkid;
	eb.generationstamp = rpc->_args[0]->ob_val._block._generation;
	if (rpc->_args[0]->ob_val._block._length) {
		eb.has_numbytes = true;
		eb.numbytes = rpc->_args[0]->ob_val._block._length;
	}
	req.b = &eb;

	req.src = rpc->_args[1]->ob_val._string._val;
	req.holder = rpc->_args[2]->ob_val._string._val;
}
ENCODE_POSTSCRIPT(abandon_block)

ENCODE_PREAMBLE(addBlock, AddBlock, ADD_BLOCK)
	Hadoop__Hdfs__ExtendedBlockProto previous = HADOOP__HDFS__EXTENDED_BLOCK_PROTO__INIT;
	Hadoop__Hdfs__DatanodeInfoProto *dinfo_arr = NULL;
	Hadoop__Hdfs__DatanodeIDProto *did_arr = NULL;
{
	// XXX keeping the arg order as is to keep some compatibility with the v1 RPC
	ASSERT(rpc->_nargs >= 3 || rpc->_nargs <= 5);
	ASSERT(rpc->_args[0]->ob_type == H_STRING);
	ASSERT(rpc->_args[1]->ob_type == H_STRING);
	ASSERT(rpc->_args[2]->ob_type == H_ARRAY_DATANODE_INFO ||
	    (rpc->_args[2]->ob_type == H_NULL &&
	     rpc->_args[2]->ob_val._null._type == H_ARRAY_DATANODE_INFO));
	ASSERT(rpc->_nargs <= 3 ||
	    rpc->_args[3]->ob_type == H_BLOCK ||
	    (rpc->_args[3]->ob_type == H_NULL &&
	     rpc->_args[3]->ob_val._null._type == H_BLOCK));
	ASSERT(rpc->_nargs <= 4 || rpc->_args[4]->ob_type == H_LONG);

	req.src = rpc->_args[0]->ob_val._string._val;
	req.clientname = rpc->_args[1]->ob_val._string._val;

	if (rpc->_nargs >= 4 && rpc->_args[3]->ob_type != H_NULL) {
		struct hdfs_object *bobj = rpc->_args[3];

		previous.blockid = bobj->ob_val._block._blkid;
		previous.has_numbytes = true;
		previous.numbytes  = bobj->ob_val._block._length;
		previous.generationstamp = bobj->ob_val._block._generation;
		previous.poolid = bobj->ob_val._block._pool_id;

		req.previous = &previous;
	} else {
		req.previous = NULL;
	}

	req.n_excludenodes = 0;
	if (rpc->_args[2]->ob_type != H_NULL
	    || rpc->_args[2]->ob_val._array_datanode_info._len > 0) {
		req.n_excludenodes = rpc->_args[2]->ob_val._array_datanode_info._len;
		req.excludenodes = malloc(req.n_excludenodes * sizeof(*req.excludenodes));
		ASSERT(req.excludenodes);

		dinfo_arr = malloc(req.n_excludenodes * sizeof(*dinfo_arr));
		ASSERT(dinfo_arr);
		did_arr = malloc(req.n_excludenodes * sizeof(*did_arr));
		ASSERT(did_arr);

		for (unsigned i = 0; i < req.n_excludenodes; i++) {
			struct hdfs_object *h_dinfo_obj = rpc->_args[2]->ob_val._array_datanode_info._values[i];
			struct hdfs_datanode_info *h_dinfo = &h_dinfo_obj->ob_val._datanode_info;
			Hadoop__Hdfs__DatanodeInfoProto *dinfo = &dinfo_arr[i];
			Hadoop__Hdfs__DatanodeIDProto *did = &did_arr[i];

			hadoop__hdfs__datanode_info_proto__init(dinfo);
			hadoop__hdfs__datanode_idproto__init(did);

			did->ipaddr = h_dinfo->_ipaddr;
			did->hostname = h_dinfo->_hostname;
			did->datanodeuuid = h_dinfo->_uuid;
			did->xferport = strtol(h_dinfo->_port, NULL, 10); // XXX ep/error checking?
			did->infoport = h_dinfo->_infoport;
			did->ipcport = h_dinfo->_namenodeport;

			dinfo->id = did;
			if (h_dinfo->_location[0] != '\0')
				dinfo->location = h_dinfo->_location;
			// All of the other fields are listed as optional. It's unclear
			// what's actually necessary to include here

			req.excludenodes[i] = dinfo;
		}
	}

	if (rpc->_nargs >= 5) {
		struct hdfs_object *fobj = rpc->_args[4];

		if (fobj->ob_val._long._val != 0) {
			req.has_fileid = true;
			req.fileid = fobj->ob_val._long._val;
		}
	}

	req.n_favorednodes = 0;
}
ENCODE_POSTSCRIPT_EX(add_block,
	if (req.n_excludenodes > 0) {
		free(req.excludenodes);
		free(dinfo_arr);
		free(did_arr);
	}
)

ENCODE_PREAMBLE(rename, Rename, RENAME)
{
	ASSERT(rpc->_nargs == 2);
	ASSERT(rpc->_args[0]->ob_type == H_STRING);
	ASSERT(rpc->_args[1]->ob_type == H_STRING);

	req.src = rpc->_args[0]->ob_val._string._val;
	req.dst = rpc->_args[1]->ob_val._string._val;
}
ENCODE_POSTSCRIPT(rename)

ENCODE_PREAMBLE(mkdirs, Mkdirs, MKDIRS)
	Hadoop__Hdfs__FsPermissionProto perms = HADOOP__HDFS__FS_PERMISSION_PROTO__INIT;
{
	ASSERT(rpc->_nargs == 2);
	ASSERT(rpc->_args[0]->ob_type == H_STRING);
	ASSERT(rpc->_args[1]->ob_type == H_FSPERMS);

	req.src = rpc->_args[0]->ob_val._string._val;
	req.masked = &perms;
	perms.perm = rpc->_args[1]->ob_val._fsperms._perms;

	/* XXX Could have a seperate v2 RPC to specify */
	req.createparent = true;
}
ENCODE_POSTSCRIPT(mkdirs)

ENCODE_PREAMBLE(renewLease, RenewLease, RENEW_LEASE)
{
	ASSERT(rpc->_nargs == 1);
	ASSERT(rpc->_args[0]->ob_type == H_STRING);

	req.clientname = rpc->_args[0]->ob_val._string._val;
}
ENCODE_POSTSCRIPT(renew_lease)

ENCODE_PREAMBLE(recoverLease, RecoverLease, RECOVER_LEASE)
{
	ASSERT(rpc->_nargs == 2);
	ASSERT(rpc->_args[0]->ob_type == H_STRING);
	ASSERT(rpc->_args[1]->ob_type == H_STRING);

	req.src = rpc->_args[0]->ob_val._string._val;
	req.clientname = rpc->_args[1]->ob_val._string._val;
}
ENCODE_POSTSCRIPT(recover_lease)

ENCODE_PREAMBLE(getContentSummary, GetContentSummary, GET_CONTENT_SUMMARY)
{
	ASSERT(rpc->_nargs == 1);
	ASSERT(rpc->_args[0]->ob_type == H_STRING);

	req.path = rpc->_args[0]->ob_val._string._val;
}
ENCODE_POSTSCRIPT(get_content_summary)

ENCODE_PREAMBLE(setQuota, SetQuota, SET_QUOTA)
{
	ASSERT(rpc->_nargs == 3);
	ASSERT(rpc->_args[0]->ob_type == H_STRING);
	ASSERT(rpc->_args[1]->ob_type == H_LONG);
	ASSERT(rpc->_args[2]->ob_type == H_LONG);

	req.path = rpc->_args[0]->ob_val._string._val;
	req.namespacequota = rpc->_args[1]->ob_val._long._val;
	req.storagespacequota = rpc->_args[2]->ob_val._long._val;
}
ENCODE_POSTSCRIPT(set_quota)

ENCODE_PREAMBLE(fsync, Fsync, FSYNC)
{
	ASSERT(rpc->_nargs == 2);
	ASSERT(rpc->_args[0]->ob_type == H_STRING);
	ASSERT(rpc->_args[1]->ob_type == H_STRING);

	req.src = rpc->_args[0]->ob_val._string._val;
	req.client = rpc->_args[1]->ob_val._string._val;
}
ENCODE_POSTSCRIPT(fsync)

ENCODE_PREAMBLE(setTimes, SetTimes, SET_TIMES)
{
	ASSERT(rpc->_nargs == 3);
	ASSERT(rpc->_args[0]->ob_type == H_STRING);
	ASSERT(rpc->_args[1]->ob_type == H_LONG);
	ASSERT(rpc->_args[2]->ob_type == H_LONG);

	req.src = rpc->_args[0]->ob_val._string._val;
	req.mtime = rpc->_args[1]->ob_val._long._val;
	req.atime = rpc->_args[2]->ob_val._long._val;
}
ENCODE_POSTSCRIPT(set_times)

ENCODE_PREAMBLE(getFileInfo, GetFileInfo, GET_FILE_INFO)
{
	ASSERT(rpc->_nargs == 1);
	ASSERT(rpc->_args[0]->ob_type == H_STRING);

	req.src = rpc->_args[0]->ob_val._string._val;
}
ENCODE_POSTSCRIPT(get_file_info)

ENCODE_PREAMBLE(getFileLinkInfo, GetFileLinkInfo, GET_FILE_LINK_INFO)
{
	ASSERT(rpc->_nargs == 1);
	ASSERT(rpc->_args[0]->ob_type == H_STRING);

	req.src = rpc->_args[0]->ob_val._string._val;
}
ENCODE_POSTSCRIPT(get_file_link_info)

ENCODE_PREAMBLE(createSymlink, CreateSymlink, CREATE_SYMLINK)
	Hadoop__Hdfs__FsPermissionProto perms = HADOOP__HDFS__FS_PERMISSION_PROTO__INIT;
{
	ASSERT(rpc->_nargs == 4);
	ASSERT(rpc->_args[0]->ob_type == H_STRING);
	ASSERT(rpc->_args[1]->ob_type == H_STRING);
	ASSERT(rpc->_args[2]->ob_type == H_FSPERMS);
	ASSERT(rpc->_args[3]->ob_type == H_BOOLEAN);

	req.target = rpc->_args[0]->ob_val._string._val;
	req.link = rpc->_args[1]->ob_val._string._val;
	perms.perm = rpc->_args[2]->ob_val._fsperms._perms;
	req.dirperm = &perms;
	req.createparent = rpc->_args[3]->ob_val._boolean._val;
}
ENCODE_POSTSCRIPT(create_symlink)

ENCODE_PREAMBLE(getLinkTarget, GetLinkTarget, GET_LINK_TARGET)
{
	ASSERT(rpc->_nargs == 1);
	ASSERT(rpc->_args[0]->ob_type == H_STRING);

	req.path = rpc->_args[0]->ob_val._string._val;
}
ENCODE_POSTSCRIPT(get_link_target)

ENCODE_PREAMBLE(getDatanodeReport, GetDatanodeReport, GET_DATANODE_REPORT)
{
	ASSERT(rpc->_nargs == 1);
	ASSERT(rpc->_args[0]->ob_type == H_DNREPORTTYPE);

	req.type = _hdfs_datanode_report_type_to_proto(rpc->_args[0]->ob_val._dnreporttype._type);
}
ENCODE_POSTSCRIPT(get_datanode_report)

ENCODE_PREAMBLE(getAdditionalDatanode, GetAdditionalDatanode, GET_ADDITIONAL_DATANODE)
	Hadoop__Hdfs__ExtendedBlockProto blk = HADOOP__HDFS__EXTENDED_BLOCK_PROTO__INIT;
	Hadoop__Hdfs__DatanodeInfoProto *existings_dinfo_arr = NULL;
	Hadoop__Hdfs__DatanodeIDProto *existings_did_arr = NULL;
	Hadoop__Hdfs__DatanodeInfoProto *excludes_dinfo_arr = NULL;
	Hadoop__Hdfs__DatanodeIDProto *excludes_did_arr = NULL;
{
	struct hdfs_block *hb;

	ASSERT(rpc->_nargs >= 7 || rpc->_nargs <= 8);
	ASSERT(rpc->_args[0]->ob_type == H_STRING);
	ASSERT(rpc->_args[1]->ob_type == H_BLOCK);
	ASSERT(rpc->_args[2]->ob_type == H_ARRAY_DATANODE_INFO ||
	    (rpc->_args[2]->ob_type == H_NULL &&
	     rpc->_args[2]->ob_val._null._type == H_ARRAY_DATANODE_INFO));
	ASSERT(rpc->_args[3]->ob_type == H_ARRAY_DATANODE_INFO ||
	    (rpc->_args[3]->ob_type == H_NULL &&
	     rpc->_args[3]->ob_val._null._type == H_ARRAY_DATANODE_INFO));
	ASSERT(rpc->_args[4]->ob_type == H_INT); // XXX technically should be uint32
	ASSERT(rpc->_args[5]->ob_type == H_STRING);
	ASSERT(rpc->_args[6]->ob_type == H_ARRAY_STRING ||
	    (rpc->_args[6]->ob_type == H_NULL &&
	     rpc->_args[6]->ob_val._null._type == H_ARRAY_STRING));
	ASSERT(rpc->_nargs <= 7 || rpc->_args[7]->ob_type == H_LONG); // XXX technically should be uint64

	req.src = rpc->_args[0]->ob_val._string._val;

	hb = &rpc->_args[1]->ob_val._block;
	blk.poolid = hb->_pool_id;
	blk.blockid = hb->_blkid;
	blk.generationstamp = hb->_generation;
	blk.has_numbytes = true;
	blk.numbytes = hb->_length;
	req.blk = &blk;

	req.n_existings = 0;
	if (rpc->_args[2]->ob_type != H_NULL
	    || rpc->_args[2]->ob_val._array_datanode_info._len > 0) {
		req.n_existings = rpc->_args[2]->ob_val._array_datanode_info._len;
		req.existings = malloc(req.n_existings * sizeof(*req.existings));
		ASSERT(req.existings);

		existings_dinfo_arr = malloc(req.n_existings * sizeof(*existings_dinfo_arr));
		ASSERT(existings_dinfo_arr);
		existings_did_arr = malloc(req.n_existings * sizeof(*existings_did_arr));
		ASSERT(existings_did_arr);

		for (unsigned i = 0; i < req.n_existings; i++) {
			struct hdfs_object *h_dinfo_obj = rpc->_args[2]->ob_val._array_datanode_info._values[i];
			struct hdfs_datanode_info *h_dinfo = &h_dinfo_obj->ob_val._datanode_info;
			Hadoop__Hdfs__DatanodeInfoProto *dinfo = &existings_dinfo_arr[i];
			Hadoop__Hdfs__DatanodeIDProto *did = &existings_did_arr[i];

			hadoop__hdfs__datanode_info_proto__init(dinfo);
			hadoop__hdfs__datanode_idproto__init(did);

			did->ipaddr = h_dinfo->_ipaddr;
			did->hostname = h_dinfo->_hostname;
			did->datanodeuuid = h_dinfo->_uuid;
			did->xferport = strtol(h_dinfo->_port, NULL, 10); // XXX ep/error checking?
			did->infoport = h_dinfo->_infoport;
			did->ipcport = h_dinfo->_namenodeport;

			dinfo->id = did;
			if (h_dinfo->_location[0] != '\0')
				dinfo->location = h_dinfo->_location;
			// All of the other fields are listed as optional. It's unclear
			// what's actually necessary to include here

			req.existings[i] = dinfo;
		}
	}

	req.n_excludes = 0;
	if (rpc->_args[3]->ob_type != H_NULL
	    || rpc->_args[3]->ob_val._array_datanode_info._len > 0) {
		req.n_excludes = rpc->_args[3]->ob_val._array_datanode_info._len;
		req.excludes = malloc(req.n_excludes * sizeof(*req.excludes));
		ASSERT(req.excludes);

		excludes_dinfo_arr = malloc(req.n_excludes * sizeof(*excludes_dinfo_arr));
		ASSERT(excludes_dinfo_arr);
		excludes_did_arr = malloc(req.n_excludes * sizeof(*excludes_did_arr));
		ASSERT(excludes_did_arr);

		for (unsigned i = 0; i < req.n_excludes; i++) {
			struct hdfs_object *h_dinfo_obj = rpc->_args[3]->ob_val._array_datanode_info._values[i];
			struct hdfs_datanode_info *h_dinfo = &h_dinfo_obj->ob_val._datanode_info;
			Hadoop__Hdfs__DatanodeInfoProto *dinfo = &excludes_dinfo_arr[i];
			Hadoop__Hdfs__DatanodeIDProto *did = &excludes_did_arr[i];

			hadoop__hdfs__datanode_info_proto__init(dinfo);
			hadoop__hdfs__datanode_idproto__init(did);

			did->ipaddr = h_dinfo->_ipaddr;
			did->hostname = h_dinfo->_hostname;
			did->datanodeuuid = h_dinfo->_uuid;
			did->xferport = strtol(h_dinfo->_port, NULL, 10); // XXX ep/error checking?
			did->infoport = h_dinfo->_infoport;
			did->ipcport = h_dinfo->_namenodeport;

			dinfo->id = did;
			if (h_dinfo->_location[0] != '\0')
				dinfo->location = h_dinfo->_location;
			// All of the other fields are listed as optional. It's unclear
			// what's actually necessary to include here

			req.excludes[i] = dinfo;
		}
	}

	req.numadditionalnodes = rpc->_args[4]->ob_val._int._val;
	req.clientname = rpc->_args[5]->ob_val._string._val;

	req.n_existingstorageuuids = 0;
	if (rpc->_args[6]->ob_type != H_NULL
	    || rpc->_args[6]->ob_val._array_string._len > 0) {
		req.n_existingstorageuuids = rpc->_args[6]->ob_val._array_string._len;
		req.existingstorageuuids = malloc(req.n_existingstorageuuids * sizeof(req.existingstorageuuids));
		ASSERT(req.existingstorageuuids);

		for (unsigned i = 0; i < req.n_existingstorageuuids; i++) {
			req.existingstorageuuids[i] = rpc->_args[6]->ob_val._array_string._val[i];
		}
	}

	if (rpc->_nargs >= 8) {
		struct hdfs_long *flong = &rpc->_args[7]->ob_val._long;

		if (flong->_val != 0) {
			req.has_fileid = true;
			req.fileid = flong->_val;
		}
	}
}
ENCODE_POSTSCRIPT_EX(get_additional_datanode,
	if (req.n_existings > 0) {
		free(req.existings);
		free(existings_dinfo_arr);
		free(existings_did_arr);
	}
	if (req.n_excludes > 0) {
		free(req.excludes);
		free(excludes_dinfo_arr);
		free(excludes_did_arr);
	}
	if (req.n_existingstorageuuids > 0) {
		free(req.existingstorageuuids);
	}
)

ENCODE_PREAMBLE(updateBlockForPipeline, UpdateBlockForPipeline, UPDATE_BLOCK_FOR_PIPELINE)
	Hadoop__Hdfs__ExtendedBlockProto eb = HADOOP__HDFS__EXTENDED_BLOCK_PROTO__INIT;
{
	struct hdfs_block *hb;

	ASSERT(rpc->_nargs == 2);
	ASSERT(rpc->_args[0]->ob_type == H_BLOCK);
	ASSERT(rpc->_args[1]->ob_type == H_STRING);

	hb = &rpc->_args[0]->ob_val._block;
	eb.poolid = hb->_pool_id;
	eb.blockid = hb->_blkid;
	eb.generationstamp = hb->_generation;
	eb.has_numbytes = true;
	eb.numbytes = hb->_length;
	req.block = &eb;

	req.clientname = rpc->_args[1]->ob_val._string._val;
}
ENCODE_POSTSCRIPT(update_block_for_pipeline)

ENCODE_PREAMBLE(updatePipeline, UpdatePipeline, UPDATE_PIPELINE)
	Hadoop__Hdfs__ExtendedBlockProto oldblock = HADOOP__HDFS__EXTENDED_BLOCK_PROTO__INIT;
	Hadoop__Hdfs__ExtendedBlockProto newblock = HADOOP__HDFS__EXTENDED_BLOCK_PROTO__INIT;
	Hadoop__Hdfs__DatanodeIDProto *did_arr = NULL;
{
	struct hdfs_block *hb;

	ASSERT(rpc->_nargs == 5);
	ASSERT(rpc->_args[0]->ob_type == H_STRING);
	ASSERT(rpc->_args[1]->ob_type == H_BLOCK);
	ASSERT(rpc->_args[2]->ob_type == H_BLOCK);
	ASSERT(rpc->_args[3]->ob_type == H_ARRAY_DATANODE_INFO ||
	    (rpc->_args[3]->ob_type == H_NULL &&
	     rpc->_args[3]->ob_val._null._type == H_ARRAY_DATANODE_INFO));
	ASSERT(rpc->_args[4]->ob_type == H_ARRAY_STRING ||
	    (rpc->_args[4]->ob_type == H_NULL &&
	     rpc->_args[4]->ob_val._null._type == H_ARRAY_STRING));

	req.clientname = rpc->_args[0]->ob_val._string._val;

	hb = &rpc->_args[1]->ob_val._block;
	oldblock.poolid = hb->_pool_id;
	oldblock.blockid = hb->_blkid;
	oldblock.generationstamp = hb->_generation;
	oldblock.has_numbytes = true;
	oldblock.numbytes = hb->_length;
	req.oldblock = &oldblock;

	hb = &rpc->_args[2]->ob_val._block;
	newblock.poolid = hb->_pool_id;
	newblock.blockid = hb->_blkid;
	newblock.generationstamp = hb->_generation;
	newblock.has_numbytes = true;
	newblock.numbytes = hb->_length;
	req.newblock = &newblock;

	req.n_newnodes = 0;
	if (rpc->_args[3]->ob_type != H_NULL
	    || rpc->_args[3]->ob_val._array_datanode_info._len > 0) {
		req.n_newnodes = rpc->_args[3]->ob_val._array_datanode_info._len;
		req.newnodes = malloc(req.n_newnodes * sizeof(*req.newnodes));
		ASSERT(req.newnodes);

		did_arr = malloc(req.n_newnodes * sizeof(*did_arr));
		ASSERT(did_arr);

		for (unsigned i = 0; i < req.n_newnodes; i++) {
			struct hdfs_object *h_dinfo_obj = rpc->_args[3]->ob_val._array_datanode_info._values[i];
			struct hdfs_datanode_info *h_dinfo = &h_dinfo_obj->ob_val._datanode_info;
			Hadoop__Hdfs__DatanodeIDProto *did = &did_arr[i];

			hadoop__hdfs__datanode_idproto__init(did);

			did->ipaddr = h_dinfo->_ipaddr;
			did->hostname = h_dinfo->_hostname;
			did->datanodeuuid = h_dinfo->_uuid;
			did->xferport = strtol(h_dinfo->_port, NULL, 10); // XXX ep/error checking?
			did->infoport = h_dinfo->_infoport;
			did->ipcport = h_dinfo->_namenodeport;

			req.newnodes[i] = did;
		}
	}

	req.n_storageids = 0;
	if (rpc->_args[4]->ob_type != H_NULL
	    || rpc->_args[4]->ob_val._array_string._len > 0) {
		req.n_storageids = rpc->_args[4]->ob_val._array_string._len;
		req.storageids = malloc(req.n_storageids * sizeof(*req.storageids));
		ASSERT(req.storageids);

		for (unsigned i = 0; i < req.n_storageids; i++) {
			req.storageids[i] = rpc->_args[4]->ob_val._array_string._val[i];
		}
	}
}
ENCODE_POSTSCRIPT_EX(update_pipeline,
	if (req.n_newnodes > 0) {
		free(req.newnodes);
		free(did_arr);
	}
	if (req.n_storageids > 0) {
		free(req.storageids);
	}
)

typedef size_t (*_rpc2_encoder)(struct hdfs_heap_buf *, struct hdfs_rpc_invocation *);
static struct _rpc2_enc_lut {
	const char *	re_method;
	_rpc2_encoder	re_encoder;
} rpc2_encoders[] = {
#define _RENC(method)	{ #method , _rpc2_encode_ ## method , }
	_RENC(getServerDefaults),
	_RENC(getListing),
	_RENC(getBlockLocations),
	_RENC(create),
	_RENC(delete),
	_RENC(append),
	_RENC(setReplication),
	_RENC(setPermission),
	_RENC(setOwner),
	_RENC(complete),
	_RENC(abandonBlock),
	_RENC(addBlock),
	_RENC(rename),
	_RENC(mkdirs),
	_RENC(renewLease),
	_RENC(recoverLease),
	_RENC(getContentSummary),
	_RENC(setQuota),
	_RENC(fsync),
	_RENC(setTimes),
	_RENC(getFileInfo),
	_RENC(getFileLinkInfo),
	_RENC(createSymlink),
	_RENC(getLinkTarget),
	_RENC(getDatanodeReport),
	_RENC(getAdditionalDatanode),
	_RENC(updateBlockForPipeline),
	_RENC(updatePipeline),
	{ NULL, NULL, },
#undef _RENC
};

// XXX perhaps make this non-static and add _rpc2_request_get_size_by_index()
// and _rpc2_request_serialize_by_index() APIs to avoid multiple unnecessary index calculations
static unsigned
_rpc2_request_get_index(struct hdfs_rpc_invocation *rpc)
{
	/* XXXPERF Could be a hash/rt/sorted table */
	unsigned i;
	for (i = 0; rpc2_encoders[i].re_method; i++) {
		if (streq(rpc->_method, rpc2_encoders[i].re_method)) {
			return i;
		}
	}

	/* The method does not exist in HDFSv2, or it is not yet implemented. */
	ASSERT(false);
}

void
_rpc2_request_serialize(struct hdfs_heap_buf *dest,
	struct hdfs_rpc_invocation *rpc)
{
	unsigned i;

	i = _rpc2_request_get_index(rpc);
	rpc2_encoders[i].re_encoder(dest, rpc);
}

size_t
_rpc2_request_get_size(struct hdfs_rpc_invocation *rpc)
{
	unsigned i;

	i = _rpc2_request_get_index(rpc);
	return rpc2_encoders[i].re_encoder(NULL, rpc);
}

/* Support logic for Namenode RPC response parsing */

/*
 * These slurpers expect exactly one protobuf in the heapbuf passed in, and
 * advance the cursor (->used) to the end (->size) after a successful parse.
 */
#define DECODE_PB_EX(lowerCamel, CamelCase, lower_case, objbuilder_ex)			\
static struct hdfs_object *								\
_oslurp_ ## lowerCamel (struct hdfs_heap_buf *buf)					\
{											\
	Hadoop__Hdfs__ ## CamelCase ## ResponseProto *resp;				\
	struct hdfs_object *result;							\
											\
	result = NULL;									\
											\
	resp = hadoop__hdfs__ ## lower_case ## _response_proto__unpack(NULL,		\
	    buf->size - buf->used, (void *)&buf->buf[buf->used]);			\
	buf->used = buf->size;								\
	if (resp == NULL) {								\
		buf->used = _H_PARSE_ERROR;						\
		return NULL;								\
	}										\
											\
	objbuilder_ex;									\
											\
	hadoop__hdfs__ ## lower_case ## _response_proto__free_unpacked(resp, NULL);	\
	return result;									\
}

#define DECODE_PB(lowerCamel, CamelCase, lower_case, objbuilder, respfield)	\
	DECODE_PB_EX(lowerCamel, CamelCase, lower_case,				\
	    result = _hdfs_ ## objbuilder ## _new_proto(resp->respfield))

#define DECODE_PB_ARRAY(lowerCamel, CamelCase, lower_case, objbuilder, respfield)	\
	DECODE_PB_EX(lowerCamel, CamelCase, lower_case,					\
	    result = _hdfs_ ## objbuilder ## _new_proto(resp->respfield, resp->n_ ## respfield))

#define _DECODE_PB_NULLABLE(lowerCamel, CamelCase, lower_case, objbuilder, respfield, nullctor)	\
	DECODE_PB_EX(lowerCamel, CamelCase, lower_case,					\
		if (resp->respfield != NULL) {						\
			result = _hdfs_ ## objbuilder ## _new_proto(resp->respfield);	\
		} else {								\
			result = nullctor;					\
		}									\
	)

#define DECODE_PB_NULLABLE(lowerCamel, CamelCase, lower_case, objbuilder, respfield, htype)	\
	_DECODE_PB_NULLABLE(lowerCamel, CamelCase, lower_case, objbuilder, respfield,	\
	    hdfs_null_new(htype))

#define DECODE_PB_VOID(lowerCamel, CamelCase, lower_case)		\
	DECODE_PB_EX(lowerCamel, CamelCase, lower_case,			\
	    result = hdfs_void_new())


DECODE_PB(getServerDefaults, GetServerDefaults, get_server_defaults, fsserverdefaults, serverdefaults)
DECODE_PB_NULLABLE(getListing, GetListing, get_listing, directory_listing, dirlist, H_DIRECTORY_LISTING)
DECODE_PB_NULLABLE(getBlockLocations, GetBlockLocations, get_block_locations, located_blocks, locations, H_LOCATED_BLOCKS)
/*
 * HDFSv2.2+ returns a FileStatus, while 2.0.x and 1.x return void.
 *
 * This is represented in the shared protobuf as a NULL 'fs'.  Also, the same
 * high-level wrapper is used across all versions, so we represent void in
 * earlier versions as a return value of (FileStatus *)NULL.
 */
DECODE_PB_NULLABLE(create, Create, create, file_status, fs, H_FILE_STATUS)
DECODE_PB(delete, Delete, delete, boolean, result)
DECODE_PB_NULLABLE(append, Append, append, located_block, block,
    H_LOCATED_BLOCK);
DECODE_PB(setReplication, SetReplication, set_replication, boolean, result)
DECODE_PB_VOID(setPermission, SetPermission, set_permission)
DECODE_PB_VOID(setOwner, SetOwner, set_owner)
DECODE_PB(complete, Complete, complete, boolean, result)
DECODE_PB_VOID(abandonBlock, AbandonBlock, abandon_block)
DECODE_PB(addBlock, AddBlock, add_block, located_block, block)
DECODE_PB(rename, Rename, rename, boolean, result)
DECODE_PB(mkdirs, Mkdirs, mkdirs, boolean, result)
DECODE_PB_VOID(renewLease, RenewLease, renew_lease)
DECODE_PB(recoverLease, RecoverLease, recover_lease, boolean, result)
DECODE_PB(getContentSummary, GetContentSummary, get_content_summary, content_summary, summary)
DECODE_PB_VOID(setQuota, SetQuota, set_quota)
DECODE_PB_VOID(fsync, Fsync, fsync)
DECODE_PB_VOID(setTimes, SetTimes, set_times)
DECODE_PB_NULLABLE(getFileInfo, GetFileInfo, get_file_info, file_status, fs, H_FILE_STATUS)
DECODE_PB_NULLABLE(getFileLinkInfo, GetFileLinkInfo, get_file_link_info, file_status, fs, H_FILE_STATUS)
DECODE_PB_VOID(createSymlink, CreateSymlink, create_symlink)
DECODE_PB_EX(getLinkTarget, GetLinkTarget, get_link_target,
	result = hdfs_string_new(resp->targetpath))
DECODE_PB_ARRAY(getDatanodeReport, GetDatanodeReport, get_datanode_report, array_datanode_info, di)
DECODE_PB(getAdditionalDatanode, GetAdditionalDatanode, get_additional_datanode, located_block, block)
DECODE_PB(updateBlockForPipeline, UpdateBlockForPipeline, update_block_for_pipeline, located_block, block)
DECODE_PB_VOID(updatePipeline, UpdatePipeline, update_pipeline)

static struct _rpc2_dec_lut {
	const char *		rd_method;
	hdfs_object_slurper	rd_decoder;
} rpc2_decoders[] = {
#define _RDEC(method)	{ #method , _oslurp_ ## method , }
	_RDEC(getServerDefaults),
	_RDEC(getListing),
	_RDEC(getBlockLocations),
	_RDEC(create),
	_RDEC(delete),
	_RDEC(append),
	_RDEC(setReplication),
	_RDEC(setPermission),
	_RDEC(setOwner),
	_RDEC(complete),
	_RDEC(abandonBlock),
	_RDEC(addBlock),
	_RDEC(rename),
	_RDEC(mkdirs),
	_RDEC(renewLease),
	_RDEC(recoverLease),
	_RDEC(getContentSummary),
	_RDEC(setQuota),
	_RDEC(fsync),
	_RDEC(setTimes),
	_RDEC(getFileInfo),
	_RDEC(getFileLinkInfo),
	_RDEC(createSymlink),
	_RDEC(getLinkTarget),
	_RDEC(getDatanodeReport),
	_RDEC(getAdditionalDatanode),
	_RDEC(updateBlockForPipeline),
	_RDEC(updatePipeline),
	{ NULL, NULL, },
#undef _RDEC
};

hdfs_object_slurper
_rpc2_slurper_for_rpc(struct hdfs_object *rpc)
{
	/* XXXPERF s.a. */
	unsigned i;

	ASSERT(rpc->ob_type == H_RPC_INVOCATION);

	for (i = 0; rpc2_decoders[i].rd_method; i++)
		if (streq(rpc->ob_val._rpc_invocation._method,
		    rpc2_decoders[i].rd_method))
			return rpc2_decoders[i].rd_decoder;

	/*
	 * The method does not exist in HDFSv2 (this function can be called for
	 * HDFSv1 methods) or it is not yet implemented.
	 */
	return NULL;
}
