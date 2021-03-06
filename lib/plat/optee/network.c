#include "core/private.h"


int
lws_plat_pipe_create(struct lws *wsi)
{
	return 1;
}

int
lws_plat_pipe_signal(struct lws *wsi)
{
	return 1;
}

void
lws_plat_pipe_close(struct lws *wsi)
{
}

LWS_VISIBLE int
lws_send_pipe_choked(struct lws *wsi)
{
	struct lws *wsi_eff;

#if defined(LWS_WITH_HTTP2)
	wsi_eff = lws_get_network_wsi(wsi);
#else
	wsi_eff = wsi;
#endif

	/* the fact we checked implies we avoided back-to-back writes */
	wsi_eff->could_have_pending = 0;

	/* treat the fact we got a truncated send pending as if we're choked */
	if (lws_has_buffered_out(wsi_eff)
#if defined(LWS_WITH_HTTP_STREAM_COMPRESSION)
	    || wsi->http.comp_ctx.buflist_comp ||
	       wsi->http.comp_ctx.may_have_more
#endif
	)
		return 1;

	/* okay to send another packet without blocking */

	return 0;
}

int
lws_poll_listen_fd(struct lws_pollfd *fd)
{
//	return poll(fd, 1, 0);

	return 0;
}


LWS_EXTERN int
_lws_plat_service_tsi(struct lws_context *context, int timeout_ms, int tsi)
{
	struct lws_context_per_thread *pt;
	int n = -1, m, c;
	//char buf;

	/* stay dead once we are dead */

	if (!context || !context->vhost_list)
		return 1;

	pt = &context->pt[tsi];

	if (timeout_ms < 0)
		goto faked_service;

	if (!pt->service_tid_detected) {
		struct lws _lws;

		memset(&_lws, 0, sizeof(_lws));
		_lws.context = context;

		pt->service_tid = context->vhost_list->protocols[0].callback(
			&_lws, LWS_CALLBACK_GET_THREAD_ID, NULL, NULL, 0);
		pt->service_tid_detected = 1;
	}

	/*
	 * is there anybody with pending stuff that needs service forcing?
	 */
	if (!lws_service_adjust_timeout(context, 1, tsi)) {
		lwsl_notice("%s: doing forced service\n", __func__);
		/* -1 timeout means just do forced service */
		_lws_plat_service_tsi(context, -1, pt->tid);
		/* still somebody left who wants forced service? */
		if (!lws_service_adjust_timeout(context, 1, pt->tid))
			/* yes... come back again quickly */
			timeout_ms = 0;
	}

	n = poll(pt->fds, pt->fds_count, timeout_ms);

	m = 0;

	if (pt->context->tls_ops &&
	    pt->context->tls_ops->fake_POLLIN_for_buffered)
		m = pt->context->tls_ops->fake_POLLIN_for_buffered(pt);

	if (/*!pt->ws.rx_draining_ext_list && */!m && !n) { /* nothing to do */
		lws_service_fd_tsi(context, NULL, tsi);
		return 0;
	}

faked_service:
	m = lws_service_flag_pending(context, tsi);
	if (m)
		c = -1; /* unknown limit */
	else
		if (n < 0) {
			if (LWS_ERRNO != LWS_EINTR)
				return -1;
			return 0;
		} else
			c = n;

	/* any socket with events to service? */
	for (n = 0; n < (int)pt->fds_count && c; n++) {
		if (!pt->fds[n].revents)
			continue;

		c--;
#if 0
		if (pt->fds[n].fd == pt->dummy_pipe_fds[0]) {
			if (read(pt->fds[n].fd, &buf, 1) != 1)
				lwsl_err("Cannot read from dummy pipe.");
			continue;
		}
#endif
		m = lws_service_fd_tsi(context, &pt->fds[n], tsi);
		if (m < 0)
			return -1;
		/* if something closed, retry this slot */
		if (m)
			n--;
	}

	return 0;
}

int
lws_plat_check_connection_error(struct lws *wsi)
{
	return 0;
}

int
lws_plat_service(struct lws_context *context, int timeout_ms)
{
	return _lws_plat_service_tsi(context, timeout_ms, 0);
}

int
lws_plat_set_socket_options(struct lws_vhost *vhost, int fd, int unix_skt)
{
	return 0;
}


int
lws_plat_write_cert(struct lws_vhost *vhost, int is_key, int fd, void *buf,
			int len)
{
	return 1;
}


/* cast a struct sockaddr_in6 * into addr for ipv6 */

int
lws_interface_to_sa(int ipv6, const char *ifname, struct sockaddr_in *addr,
		    size_t addrlen)
{
	return -1;
}

void
lws_plat_insert_socket_into_fds(struct lws_context *context, struct lws *wsi)
{
	struct lws_context_per_thread *pt = &context->pt[(int)wsi->tsi];

	pt->fds[pt->fds_count++].revents = 0;
}

void
lws_plat_delete_socket_from_fds(struct lws_context *context,
						struct lws *wsi, int m)
{
	struct lws_context_per_thread *pt = &context->pt[(int)wsi->tsi];

	pt->fds_count--;
}

void
lws_plat_service_periodic(struct lws_context *context)
{
}

int
lws_plat_change_pollfd(struct lws_context *context,
		      struct lws *wsi, struct lws_pollfd *pfd)
{
	return 0;
}

const char *
lws_plat_inet_ntop(int af, const void *src, char *dst, int cnt)
{
	//return inet_ntop(af, src, dst, cnt);
	return "lws_plat_inet_ntop";
}

int
lws_plat_inet_pton(int af, const char *src, void *dst)
{
	//return inet_pton(af, src, dst);
	return 1;
}


