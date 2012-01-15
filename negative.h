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

#ifndef __NEGATIVE_H__
#define __NEGATIVE_H__

#ifndef _CONFIG_H
#define _CONFIG_H
#include "config.h"
#endif
#include "mem-types.h"
#include "hashfn.h"

#define GHOST_BUCKETS 64
#define GHOST_HASH(x) (SuperFastHash(x,strlen(x)) % GHOST_BUCKETS)

typedef struct _ghost {
        struct _ghost *next;
        char          *path;
} ghost_t;

typedef struct {
        ghost_t *ghosts[GHOST_BUCKETS];
} negative_private_t;

enum gf_negative_mem_types_ {
        gf_negative_mt_priv = gf_common_mt_end + 1,
        gf_negative_mt_ghost,
        gf_negative_mt_end
};

#endif /* __NEGATIVE_H__ */
