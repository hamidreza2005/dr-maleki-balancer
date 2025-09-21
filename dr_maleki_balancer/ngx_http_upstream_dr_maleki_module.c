#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

/* This structure holds the state for our load balancing algorithm. */
typedef struct {
    ngx_http_upstream_rr_peers_t  *peers; // Nginx's peer list
    ngx_uint_t                    limit;  // The current request limit for each server
    ngx_uint_t                    current; // The index of the server we are currently sending requests to
} ngx_http_upstream_dr_maleki_data_t;


// Function prototypes
static ngx_int_t ngx_http_upstream_init_dr_maleki(ngx_conf_t *cf, ngx_http_upstream_srv_conf_t *us);
static ngx_int_t ngx_http_upstream_init_dr_maleki_peer(ngx_http_request_t *r, ngx_http_upstream_srv_conf_t *us);
static ngx_int_t ngx_http_upstream_get_dr_maleki_peer(ngx_peer_connection_t *pc, void *data);
static void ngx_http_upstream_free_dr_maleki_peer(ngx_peer_connection_t *pc, void *data, ngx_uint_t state);
static char *ngx_http_upstream_dr_maleki(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);


/* This defines the "dr_maleki" directive that you use in nginx.conf */
static ngx_command_t  ngx_http_upstream_dr_maleki_commands[] = {
    { ngx_string("dr_maleki"),
      NGX_HTTP_UPS_CONF | NGX_CONF_NOARGS,
      ngx_http_upstream_dr_maleki,
      0,
      0,
      NULL },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_upstream_dr_maleki_module_ctx = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};


ngx_module_t  ngx_http_upstream_dr_maleki_module = {
    NGX_MODULE_V1,
    &ngx_http_upstream_dr_maleki_module_ctx,
    ngx_http_upstream_dr_maleki_commands,
    NGX_HTTP_MODULE,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NGX_MODULE_V1_PADDING
};


static char *
ngx_http_upstream_dr_maleki(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_upstream_srv_conf_t  *uscf;
    uscf = ngx_http_conf_get_module_srv_conf(cf, ngx_http_upstream_module);
    uscf->peer.init_upstream = ngx_http_upstream_init_dr_maleki;
    uscf->flags = NGX_HTTP_UPSTREAM_CREATE;
    return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_upstream_init_dr_maleki(ngx_conf_t *cf, ngx_http_upstream_srv_conf_t *us)
{
    ngx_http_upstream_dr_maleki_data_t  *dmpd;

    if (ngx_http_upstream_init_round_robin(cf, us) != NGX_OK) {
        return NGX_ERROR;
    }

    dmpd = ngx_pcalloc(cf->pool, sizeof(ngx_http_upstream_dr_maleki_data_t));
    if (dmpd == NULL) {
        return NGX_ERROR;
    }

    dmpd->peers = us->peer.data;
    dmpd->limit = 100;
    dmpd->current = 0;

    us->peer.data = dmpd;
    us->peer.init = ngx_http_upstream_init_dr_maleki_peer;

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_init_dr_maleki_peer(ngx_http_request_t *r,
    ngx_http_upstream_srv_conf_t *us)
{
    r->upstream->peer.data = us->peer.data;
    r->upstream->peer.get = ngx_http_upstream_get_dr_maleki_peer;
    r->upstream->peer.free = ngx_http_upstream_free_dr_maleki_peer;
    r->upstream->peer.tries = 1;
    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_get_dr_maleki_peer(ngx_peer_connection_t *pc, void *data)
{
    ngx_http_upstream_dr_maleki_data_t  *dmpd = data;
    ngx_http_upstream_rr_peer_t         *peer;
    ngx_uint_t                          i, all_full;

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, pc->log, 0,
                   "get dr_maleki peer, current limit: %ui", dmpd->limit);

    all_full = 1;
    for (i = 0; i < dmpd->peers->number; i++) {
        if (dmpd->peers->peer[i].conns < dmpd->limit) {
            all_full = 0;
            break;
        }
    }

    if (all_full) {
        ngx_uint_t new_limit = dmpd->limit + (dmpd->limit / 10);
        if (new_limit == dmpd->limit) {
            new_limit++;
        }
        
        ngx_log_error(NGX_LOG_INFO, pc->log, 0,
                      "dr_maleki: all peers at limit %ui. Raising to %ui.",
                      dmpd->limit, new_limit);
        
        dmpd->limit = new_limit;
        dmpd->current = 0;
    }
    
    // **IMPROVED LOGIC:** Keep advancing the current peer until we find one that is not full.
    while (dmpd->peers->peer[dmpd->current].conns >= dmpd->limit) {
        dmpd->current++;
        if (dmpd->current >= dmpd->peers->number) {
            dmpd->current = 0;
        }
    }

    peer = &dmpd->peers->peer[dmpd->current];
    
    pc->sockaddr = peer->sockaddr;
    pc->socklen = peer->socklen;
    pc->name = &peer->name;

    peer->conns++;

    return NGX_OK;
}


static void
ngx_http_upstream_free_dr_maleki_peer(ngx_peer_connection_t *pc, void *data,
    ngx_uint_t state)
{
    ngx_http_upstream_dr_maleki_data_t  *dmpd = data;

    /*
     * **THE FIX IS HERE:**
     * We must call the round-robin free function, but pass it the correct
     * data structure, which is the 'peers' member of our custom struct,
     * NOT our entire custom struct.
     */
    ngx_http_upstream_free_round_robin_peer(pc, dmpd->peers, state);
}