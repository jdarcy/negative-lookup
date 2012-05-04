/*
  Copyright (c) 2006-2011 Gluster, Inc. <http://www.gluster.com>
  This file is part of GlusterFS.

  GlusterFS is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published
  by the Free Software Foundation; either version 3 of the License,
  or (at your option) any later version.

  GlusterFS is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <http://www.gnu.org/licenses/>.
*/

#include <ctype.h>
#include <sys/uio.h>

#ifndef _CONFIG_H
#define _CONFIG_H
#include "config.h"
#endif

#include "glusterfs.h"
#include "xlator.h"
#include "logging.h"

#include "negative.h"

#define COMPARE_GFIDS(x,y)      memcmp(x,y,sizeof(uuid_t))

void
exorcise (xlator_t *this, uuid_t gfid, char *name)
{
        negative_private_t *priv   = this->private;
        ghost_t            *gp     = NULL;
        ghost_t           **gpp    = NULL;
        uint32_t            bucket = 0;

        bucket = GHOST_HASH(gfid,name);
        for (gpp = &priv->ghosts[bucket]; *gpp; gpp = &(*gpp)->next) {
                gp = *gpp;
                if (COMPARE_GFIDS(gp->gfid,gfid)) {
                        continue;
                }
                if (strcmp(gp->name,name)) {
                        continue;
                }
                *gpp = gp->next;
                GF_FREE(gp->name);
                GF_FREE(gp);
                gf_log (this->name, GF_LOG_DEBUG,
                        "removed %s/%s", uuid_utoa(gfid), name);
                break;
        }
}

int32_t
negative_lookup_cbk (call_frame_t *frame, void *cookie, xlator_t *this,
                     int32_t op_ret, int32_t op_errno, inode_t *inode,
                     struct iatt *buf, dict_t *xdata, struct iatt *postparent)
{
        negative_private_t *priv   = this->private;
        ghost_t            *gp     = cookie;
        uint32_t            bucket = 0;

        if (op_ret < 0) {
                bucket = GHOST_HASH(gp->gfid,gp->name);
                /* TBD: locking */
                gp->next = priv->ghosts[bucket];
                priv->ghosts[bucket] = gp;
                gf_log (this->name, GF_LOG_DEBUG,
                        "added %s/%s", uuid_utoa(gp->gfid), gp->name);
        }
        else {
                gf_log (this->name, GF_LOG_DEBUG,
                        "found %s/%s", uuid_utoa(gp->gfid), gp->name);
                exorcise(this,gp->gfid,gp->name);
                GF_FREE(gp->name);
                GF_FREE(gp);
        }

        STACK_UNWIND_STRICT (lookup, frame, op_ret, op_errno, inode, buf,
                             xdata, postparent);
        return 0;
}

int32_t
negative_lookup (call_frame_t *frame, xlator_t *this, loc_t *loc,
                 dict_t *xdata)
{
        negative_private_t *priv   = this->private;
        ghost_t            *gp     = NULL;
        uint32_t            bucket = 0;

        bucket = GHOST_HASH(loc->pargfid,loc->name);
        for (gp = priv->ghosts[bucket]; gp; gp = gp->next) {
                if (COMPARE_GFIDS(gp->gfid,loc->pargfid)) {
                        continue;
                }
                if (strcmp(gp->name,loc->name)) {
                        continue;
                }
                gf_log(this->name,GF_LOG_DEBUG,"%s/%s => HIT (%p)",
                       uuid_utoa(loc->gfid), loc->name, loc->inode);
                STACK_UNWIND_STRICT (lookup, frame, -1, ENOENT,
                                     NULL, NULL, NULL, NULL);
                return 0;
        }

        gf_log(this->name,GF_LOG_DEBUG,"%s/%s => MISS",
               uuid_utoa(loc->gfid), loc->name);
        gp = GF_CALLOC(1,sizeof(ghost_t),gf_negative_mt_ghost);
        if (gp) {
                uuid_copy(gp->gfid,loc->pargfid);
                gp->name = gf_strdup(loc->name);
        }
        STACK_WIND_COOKIE (frame, negative_lookup_cbk, gp,
                           FIRST_CHILD(this), FIRST_CHILD(this)->fops->lookup,
                           loc, xdata);
        return 0;
}

int32_t
negative_create_cbk (call_frame_t *frame, void *cookie, xlator_t *this,
                     int32_t op_ret, int32_t op_errno, fd_t *fd, inode_t *inode,
                     struct iatt *buf, struct iatt *preparent,
                     struct iatt *postparent, dict_t *xdata)
{
        ghost_t *gp = cookie;

        exorcise(this,gp->gfid,gp->name);
        GF_FREE(gp->name);
        GF_FREE(gp);

        STACK_UNWIND_STRICT (create, frame, op_ret, op_errno, fd, inode, buf,
                             preparent, postparent, xdata);
        return 0;
}

int32_t
negative_create (call_frame_t *frame, xlator_t *this, loc_t *loc, int32_t flags,
                 mode_t mode, mode_t umask, fd_t *fd, dict_t *xdata)
{
        ghost_t *gp = NULL;

        gp = GF_CALLOC(1,sizeof(ghost_t),gf_negative_mt_ghost);
        if (gp) {
                uuid_copy(gp->gfid,loc->pargfid);
                gp->name = gf_strdup(loc->name);
        }

        STACK_WIND_COOKIE (frame, negative_create_cbk, gp,
                           FIRST_CHILD(this), FIRST_CHILD(this)->fops->create,
                           loc, flags, mode, umask, fd, xdata);
        return 0;
}

int32_t
negative_mkdir_cbk (call_frame_t *frame, void *cookie, xlator_t *this,
                    int32_t op_ret, int32_t op_errno, inode_t *inode,
                    struct iatt *buf, struct iatt *preparent,
                    struct iatt *postparent, dict_t *xdata)
{
        ghost_t *gp = cookie;

        exorcise(this,gp->gfid,gp->name);
        GF_FREE(gp->name);
        GF_FREE(gp);

        STACK_UNWIND_STRICT (mkdir, frame, op_ret, op_errno, inode,
                             buf, preparent, postparent, xdata);
        return 0;
}

int
negative_mkdir (call_frame_t *frame, xlator_t *this, loc_t *loc, mode_t mode,
                mode_t umask, dict_t *xdata)
{
        ghost_t *gp = NULL;

        gp = GF_CALLOC(1,sizeof(ghost_t),gf_negative_mt_ghost);
        if (gp) {
                uuid_copy(gp->gfid,loc->pargfid);
                gp->name = gf_strdup(loc->name);
        }

        STACK_WIND_COOKIE (frame, negative_mkdir_cbk, gp,
                           FIRST_CHILD(this), FIRST_CHILD(this)->fops->mkdir,
                           loc, mode, umask, xdata);
        return 0;
}

int32_t
init (xlator_t *this)
{
	negative_private_t *priv = NULL;

	if (!this->children || this->children->next) {
		gf_log ("negative", GF_LOG_ERROR, 
			"FATAL: negative should have exactly one child");
		return -1;
	}

	if (!this->parents) {
		gf_log (this->name, GF_LOG_WARNING,
			"dangling volume. check volfile ");
	}

	priv = GF_CALLOC (1, sizeof (negative_private_t), gf_negative_mt_priv);
        if (!priv)
                return -1;

	this->private = priv;
	gf_log ("negative", GF_LOG_DEBUG, "negative xlator loaded");
	return 0;
}

void
fini (xlator_t *this)
{
	negative_private_t *priv = this->private;

        if (!priv)
                return;
        this->private = NULL;
	GF_FREE (priv);

	return;
}

struct xlator_fops fops = {
        .lookup = negative_lookup,
        .create = negative_create,
        .mkdir  = negative_mkdir,
};

struct xlator_cbks cbks = {
};

struct volume_options options[] = {
	{ .key  = {NULL} },
};
