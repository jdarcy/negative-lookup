This is a very simple translator to cache "negative lookups" for workloads in
which the same file is looked up many times in places where it doesn't exist.
In particular, web script files with many includes/requires and long paths can
generate *hundreds* of such lookups per front-end request.  If we don't cache
the negative results, this can mean hundreds of back-end network round trips
per front-end request.  So we cache.  Very simple tests for this kind of
workload on two machines connected via GigE show an approximately 3x
performance improvement.

This code is nowhere near ready for production use yet.  It was originally
developed as a pedagogical example, but one that *could lead* to something
truly useful as well.  Among other things, the following features need to be
added.

* Support for other namespace-modifying operations - link, symlink, mknod,
  rename, even funky xattr requests.

* Time-based cache expiration to cover the case where *another client* creates
  a file that's in our cache because it wasn't there when we first looked it
  up.  This might even include periodic pruning of entries that are already
  stale but will never be looked up (and therefore never reaped in-line) again.

* Locking on the cache for when we're called concurrently.

This is intended to be a learning tool.  I might not get back to this code
myself for a long time, but I always have time to help anyone who's learning to
write translators.  If you want to help move it along, *please* fork and send
me pull requests.
