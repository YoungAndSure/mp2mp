/*	$OpenBSD$ */

/*
 * Copyright (c) 2013, 2016 Renato Westphal <renato@openbsd.org>
 * Copyright (c) 2009 Michele Marchetto <michele@openbsd.org>
 * Copyright (c) 2004, 2005 Esben Norby <norby@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _LDE_H_
#define _LDE_H_

#include "openbsd-queue.h"
#include "openbsd-tree.h"

enum fec_type {
	FEC_TYPE_IPV4,
	FEC_TYPE_IPV6,
	FEC_TYPE_PWID
};

enum stream_type{
	STREAM_TYPE_UP,
	STREAM_TYPE_DOWN
};

struct fec {
	RB_ENTRY(fec)		entry;
	enum fec_type		type;
	union {
		struct {
			uint8_t		prefixlen;//OV
			struct in_addr	prefix;//ROOT
			
		} ipv4;//mp2mp
		struct {
			struct in6_addr	prefix;
			uint8_t		prefixlen;
		} ipv6;
		struct {
			uint16_t	type;
			uint32_t	pwid;
			struct in_addr	lsr_id;
		} pwid;
	} u;
};
RB_HEAD(fec_tree, fec);
RB_PROTOTYPE(fec_tree, fec, entry, fec_compare)

/* request entries */
struct lde_req {
	struct fec		 fec;
	uint32_t		 msg_id;
};

/* mapping entries */
struct lde_map {
	struct fec		 fec;
	LIST_ENTRY(lde_map)	 entry;
	struct lde_nbr		*nexthop;
	struct map		 map;
};

/* withdraw entries */
struct lde_wdraw {
	struct fec		 fec;
	uint32_t		 label;
};

/* Addresses belonging to neighbor */
struct lde_addr {
	TAILQ_ENTRY(lde_addr)	 entry;
	int			 af;
	union ldpd_addr		 addr;
};

/* just the info LDE needs */
struct lde_nbr {
	RB_ENTRY(lde_nbr)	 entry;
	uint32_t		 peerid;
	struct in_addr		 id;
	int			 v4_enabled;	/* announce/process v4 msgs */
	int			 v6_enabled;	/* announce/process v6 msgs */
	struct fec_tree		 recv_req;
	struct fec_tree		 sent_req;
	struct fec_tree		 recv_map;
	struct fec_tree		 sent_map;
	struct fec_tree		 sent_wdraw;
	TAILQ_HEAD(, lde_addr)	 addr_list;
};
RB_HEAD(nbr_tree, lde_nbr);
RB_PROTOTYPE(nbr_tree, lde_nbr, entry, lde_nbr_compare)

struct fec_nh {
	LIST_ENTRY(fec_nh)	 entry;
	int			 af;
	union ldpd_addr		 nexthop;
	uint32_t		 remote_label;
	uint8_t			 priority;
};

struct fec_node {
	struct fec		 fec;

	LIST_HEAD(, fec_nh)	 nexthops;	/* fib nexthops */
	LIST_HEAD(, lde_map)	 downstream;	/* recv mappings */
	LIST_HEAD(, lde_map)	 upstream;	/* sent mappings */

	uint32_t		 local_label;
	void			*data;		/* fec specific data */
};
/////////////////////////////////////////////////////////////////////////
enum mldp_type {
	MLDP_TYPE_P2MP,
	MLDP_TYPE_MP2MP,
};

enum op_type {
	OP_TYPE_ADD,
	OP_TYPE_DEL,
};

struct mldp_lsp_info
{
    uint8_t        op;
    uint8_t        protocol_type;
    struct in_addr root_ip;
    uint8_t        lsp_id;
};


/////////////////////////////////////////////////////////////////////////
struct label_nbr{
	RB_ENTRY(label_nbr)	entry;
	uint32_t	label;//remote_label
	uint32_t	local_label;//
	uint32_t	peerid;
	struct fec	fec;//root+ov,唯一标识LSP
	enum stream_type type;
};
RB_HEAD(label_nbr_tree, label_nbr);

RB_PROTOTYPE(label_nbr_tree, label_nbr, entry,label_nbr_compare );

struct Information{
	int leaf;
	int root;
	struct in_addr root_addr;
	uint8_t	ov;
};

//////////////////////////////////////////////////////////////////////////
#define LDE_GC_INTERVAL 300

extern struct ldpd_conf	*ldeconf;
extern struct fec_tree	 ft;
extern struct nbr_tree	 lde_nbrs;
//extern struct Information info;
//////////////////////////////////////////////////////
extern struct label_nbr_tree label_nbrs;
extern int leaf;
//////////////////////////////////////////////////
extern struct thread	*gc_timer;

/* lde.c */
struct Information init_info(void);
void label_nbr_add(uint32_t,uint32_t ,uint32_t,struct fec *fec,enum stream_type);
struct lde_nbr *lde_nbr_find(uint32_t);
pid_t		 lde(const char *, const char *);
int		 lde_imsg_compose_parent(int, pid_t, void *, uint16_t);
int		 lde_imsg_compose_ldpe(int, uint32_t, pid_t, void *, uint16_t);
uint32_t	 lde_assign_label(void);
void		 lde_send_change_klabel(struct fec_node *, struct fec_nh *);
void		 lde_send_delete_klabel(struct fec_node *, struct fec_nh *);
void		 lde_fec2map(struct fec *, struct map *);
void		 lde_map2fec(struct map *, struct in_addr, struct fec *);
void		 lde_send_labelmapping(struct lde_nbr *, struct fec_node *,
		    int);
void		 lde_send_labelwithdraw(struct lde_nbr *, struct fec_node *,
		    uint32_t, struct status_tlv *);
void		 lde_send_labelwithdraw_all(struct fec_node *, uint32_t);
void		 lde_send_labelrelease(struct lde_nbr *, struct fec_node *,
		    uint32_t);
void		 lde_send_notification(uint32_t, uint32_t, uint32_t, uint16_t);
struct lde_nbr	*lde_nbr_find_by_lsrid(struct in_addr);
struct lde_nbr	*lde_nbr_find_by_addr(int, union ldpd_addr *);
struct lde_map	*lde_map_add(struct lde_nbr *, struct fec_node *, int);
void		 lde_map_del(struct lde_nbr *, struct lde_map *, int);
struct lde_req	*lde_req_add(struct lde_nbr *, struct fec *, int);
void		 lde_req_del(struct lde_nbr *, struct lde_req *, int);
struct lde_wdraw *lde_wdraw_add(struct lde_nbr *, struct fec_node *);
void		 lde_wdraw_del(struct lde_nbr *, struct lde_wdraw *);
void		 lde_change_egress_label(int, int);
struct lde_addr	*lde_address_find(struct lde_nbr *, int,
		    union ldpd_addr *);

/* lde_lib.c */
void		 fec_init(struct fec_tree *);
struct fec	*fec_find(struct fec_tree *, struct fec *);
int		 fec_insert(struct fec_tree *, struct fec *);
int		 fec_remove(struct fec_tree *, struct fec *);
void		 fec_clear(struct fec_tree *, void (*)(void *));
void		 rt_dump(pid_t);
void		 fec_snap(struct lde_nbr *);
void		 fec_tree_clear(void);
struct fec_nh	*fec_nh_find(struct fec_node *, int, union ldpd_addr *,
		    uint8_t);
uint32_t	 egress_label(enum fec_type);
void		 lde_kernel_insert(struct fec *, int, union ldpd_addr *,
		    uint8_t, int, void *);
void		 lde_kernel_remove(struct fec *, int, union ldpd_addr *,
		    uint8_t);
void		 lde_check_mapping(struct map *, struct lde_nbr *);
void		 lde_check_request(struct map *, struct lde_nbr *);
void		 lde_check_release(struct map *, struct lde_nbr *);
void		 lde_check_release_wcard(struct map *, struct lde_nbr *);
void		 lde_check_withdraw(struct map *, struct lde_nbr *);
void		 lde_check_withdraw_wcard(struct map *, struct lde_nbr *);
int		 lde_gc_timer(struct thread *);
void		 lde_gc_start_timer(void);
void		 lde_gc_stop_timer(void);

/* l2vpn.c */
struct l2vpn	*l2vpn_new(const char *);
struct l2vpn	*l2vpn_find(struct ldpd_conf *, const char *);
void		 l2vpn_del(struct l2vpn *);
void		 l2vpn_init(struct l2vpn *);
void		 l2vpn_exit(struct l2vpn *);
struct l2vpn_if	*l2vpn_if_new(struct l2vpn *, struct kif *);
struct l2vpn_if	*l2vpn_if_find(struct l2vpn *, unsigned int);
struct l2vpn_if	*l2vpn_if_find_name(struct l2vpn *, const char *);
struct l2vpn_pw	*l2vpn_pw_new(struct l2vpn *, struct kif *);
struct l2vpn_pw *l2vpn_pw_find(struct l2vpn *, unsigned int);
struct l2vpn_pw *l2vpn_pw_find_name(struct l2vpn *, const char *);
void		 l2vpn_pw_init(struct l2vpn_pw *);
void		 l2vpn_pw_exit(struct l2vpn_pw *);
void		 l2vpn_pw_reset(struct l2vpn_pw *);
int		 l2vpn_pw_ok(struct l2vpn_pw *, struct fec_nh *);
int		 l2vpn_pw_negotiate(struct lde_nbr *, struct fec_node *,
		    struct map *);
void		 l2vpn_send_pw_status(uint32_t, uint32_t, struct fec *);
void		 l2vpn_recv_pw_status(struct lde_nbr *, struct notify_msg *);
void		 l2vpn_sync_pws(int, union ldpd_addr *);
void		 l2vpn_pw_ctl(pid_t);
void		 l2vpn_binding_ctl(pid_t);

#endif	/* _LDE_H_ */
