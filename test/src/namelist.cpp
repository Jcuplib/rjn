/*
 * namelist.cpp
 * Parser for coupling.conf (Fortran namelist format).
 * Handles &coupler_config...&end and &nam_ici.../  blocks.
 */
#include "namelist.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_CONFIG 64
#define DEFAULT_FILL_VALUE (-9999.0)

/* ---- coupler global config ---- */

typedef struct {
    int    log_level;
    int    cpl_log_level;
    int    stop_step;
    int    debug_mode;
    double fill_value;
} coupler_conf_type;

static coupler_conf_type coupler_conf = {2, 0, 0, 0, DEFAULT_FILL_VALUE};
static type_ici_configure g_config[MAX_CONFIG];
static int num_of_config = 0;
static int current_config_idx = -1;

/* ======================================================================
 * Parser helpers
 * ====================================================================== */

/* Skip whitespace and commas (but not comment chars) */
static const char* skip_sep(const char* p) {
    while (*p && (isspace((unsigned char)*p) || *p == ',')) p++;
    return p;
}

/* Skip to end of line (past the newline) */
static const char* skip_eol(const char* p) {
    while (*p && *p != '\n') p++;
    if (*p) p++;
    return p;
}

/* Read identifier (alphanumeric + underscore), return new pos */
static const char* read_ident(const char* p, char* out, int max) {
    int n = 0;
    while (*p && (isalnum((unsigned char)*p) || *p == '_') && n < max - 1)
        out[n++] = *p++;
    out[n] = '\0';
    return p;
}

/* Read a value: quoted string or token until whitespace/comma/slash */
static const char* read_value(const char* p, char* out, int max) {
    int n = 0;
    if (*p == '"' || *p == '\'') {
        char q = *p++;
        while (*p && *p != q && n < max - 1) out[n++] = *p++;
        if (*p == q) p++;
    } else {
        while (*p && !isspace((unsigned char)*p) && *p != ',' && *p != '/'
               && n < max - 1)
            out[n++] = *p++;
    }
    out[n] = '\0';
    return p;
}

/* Find "&<name>" (case-insensitive) in src starting from pos p.
   Returns pointer to text immediately after the block-name, or NULL. */
static const char* find_block(const char* p, const char* name) {
    int nlen = (int)strlen(name);
    while (*p) {
        if (*p == '#' || *p == '!') { p = skip_eol(p); continue; }
        if (*p == '&') {
            const char* q = p + 1;
            if (strncasecmp(q, name, nlen) == 0) {
                char c = q[nlen];
                if (!c || isspace((unsigned char)c) || c == ',')
                    return q + nlen;
            }
        }
        p++;
    }
    return NULL;
}

/* Callback type for key-value handler */
typedef void (*kv_handler)(const char* key, const char* val, void* ctx);

/* Parse key=value pairs from p until '/' or '&end'.
   Calls handler for each pair.  Returns pointer past the terminator. */
static const char* parse_block(const char* p, kv_handler handler, void* ctx) {
    for (;;) {
        /* skip separators and comments */
        for (;;) {
            while (*p && (isspace((unsigned char)*p) || *p == ',')) p++;
            if (*p == '#' || *p == '!') { p = skip_eol(p); continue; }
            break;
        }
        if (!*p) return p;

        /* block terminators */
        if (*p == '/') return p + 1;
        if (*p == '&') {
            char word[NL_STR_SHORT];
            const char* q = read_ident(p + 1, word, sizeof(word));
            if (strcasecmp(word, "end") == 0) return q;
            return p;   /* start of another block - caller handles */
        }

        /* read key */
        char key[NL_STR_SHORT];
        const char* after_key = read_ident(p, key, sizeof(key));
        if (!key[0]) { p++; continue; }
        p = after_key;

        /* skip whitespace then '=' */
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p != '=') continue;
        p++;
        while (*p && isspace((unsigned char)*p)) p++;

        /* read value */
        char val[NL_STR_LONG];
        p = read_value(p, val, sizeof(val));

        if (handler) handler(key, val, ctx);
    }
}

/* ======================================================================
 * coupler_config block handler
 * ====================================================================== */

static void handle_coupler_config(const char* key, const char* val, void* /*ctx*/) {
    if (strcasecmp(key, "log_level") == 0) {
        if (strcmp(val, "SILENT")  == 0) coupler_conf.log_level = 0;
        else if (strcmp(val, "WHISPER") == 0) coupler_conf.log_level = 1;
        else if (strcmp(val, "LOUD")    == 0) coupler_conf.log_level = 2;
    } else if (strcasecmp(key, "cpl_log_level") == 0) {
        if (strcmp(val, "SILENT")  == 0) coupler_conf.cpl_log_level = 0;
        else if (strcmp(val, "WHISPER") == 0) coupler_conf.cpl_log_level = 1;
        else if (strcmp(val, "LOUD")    == 0) coupler_conf.cpl_log_level = 2;
    } else if (strcasecmp(key, "stop_step") == 0) {
        coupler_conf.stop_step = atoi(val);
    } else if (strcasecmp(key, "debug_mode") == 0) {
        coupler_conf.debug_mode =
            (strcasecmp(val, ".true.") == 0 || strcasecmp(val, "T") == 0) ? 1 : 0;
    } else if (strcasecmp(key, "fill_value") == 0) {
        coupler_conf.fill_value = atof(val);
    }
}

void read_coupler_config(const char* conf_file_name) {
    FILE* f = fopen(conf_file_name, "r");
    if (!f) {
        fprintf(stderr, "read_coupler_config: cannot open %s\n", conf_file_name);
        return;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    char* buf = (char*)malloc(sz + 1);
    fread(buf, 1, sz, f);
    fclose(f);
    buf[sz] = '\0';

    const char* q = find_block(buf, "coupler_config");
    if (q) parse_block(q, handle_coupler_config, NULL);

    free(buf);
}

/* ======================================================================
 * nam_ici block handler
 * ====================================================================== */

typedef struct {
    char   comp_put[NL_STR_SHORT];
    char   comp_get[NL_STR_SHORT];
    char   grid_put[NL_STR_SHORT];
    char   grid_get[NL_STR_SHORT];
    int    mapping_tag;
    char   var_put[NL_STR_SHORT];
    char   var_get[NL_STR_SHORT];
    char   var_put_vec[NL_STR_SHORT];
    char   var_get_vec[NL_STR_SHORT];
    int    grid_intpl_tag;
    int    time_intpl_tag;
    int    intvl;
    int    lag;
    char   flag[4];
    int    layer;
    double fill_value;
    int    is_ok;
    double factor;
} nam_ici_block;

static void handle_nam_ici(const char* key, const char* val, void* ctx) {
    nam_ici_block* b = (nam_ici_block*)ctx;
    if      (strcasecmp(key, "comp_put")      == 0) strncpy(b->comp_put,      val, NL_STR_SHORT-1);
    else if (strcasecmp(key, "comp_get")      == 0) strncpy(b->comp_get,      val, NL_STR_SHORT-1);
    else if (strcasecmp(key, "grid_put")      == 0) strncpy(b->grid_put,      val, NL_STR_SHORT-1);
    else if (strcasecmp(key, "grid_get")      == 0) strncpy(b->grid_get,      val, NL_STR_SHORT-1);
    else if (strcasecmp(key, "mapping_tag")   == 0) b->mapping_tag   = atoi(val);
    else if (strcasecmp(key, "var_put")       == 0) strncpy(b->var_put,       val, NL_STR_SHORT-1);
    else if (strcasecmp(key, "var_get")       == 0) strncpy(b->var_get,       val, NL_STR_SHORT-1);
    else if (strcasecmp(key, "var_put_vec")   == 0) strncpy(b->var_put_vec,   val, NL_STR_SHORT-1);
    else if (strcasecmp(key, "var_get_vec")   == 0) strncpy(b->var_get_vec,   val, NL_STR_SHORT-1);
    else if (strcasecmp(key, "grid_intpl_tag")== 0) b->grid_intpl_tag = atoi(val);
    else if (strcasecmp(key, "time_intpl_tag")== 0) b->time_intpl_tag = atoi(val);
    else if (strcasecmp(key, "intvl")         == 0) b->intvl          = atoi(val);
    else if (strcasecmp(key, "lag")           == 0) b->lag            = atoi(val);
    else if (strcasecmp(key, "flag")          == 0) strncpy(b->flag,          val, 3);
    else if (strcasecmp(key, "layer")         == 0) b->layer          = atoi(val);
    else if (strcasecmp(key, "fill_value")    == 0) b->fill_value     = atof(val);
    else if (strcasecmp(key, "is_ok")         == 0) b->is_ok          = atoi(val);
    else if (strcasecmp(key, "factor")        == 0) b->factor         = atof(val);
}

/* Compute mapping tag for a new config with put_comp/get_comp */
static int cal_mapping_tag(const char* put_comp, const char* get_comp, int up_to) {
    int tag = 1;
    for (int i = 0; i < up_to; i++) {
        if (strcmp(g_config[i].put_comp_name, put_comp) == 0 &&
            strcmp(g_config[i].get_comp_name, get_comp) == 0)
            tag++;
    }
    return tag;
}

static void process_nam_ici(const nam_ici_block* b) {
    if (b->comp_put[0] != '\0' && b->comp_get[0] != '\0') {
        /* Header block: start a new type_ici_configure entry */
        if (num_of_config >= MAX_CONFIG) {
            fprintf(stderr, "namelist: too many configs (max %d)\n", MAX_CONFIG);
            return;
        }
        type_ici_configure* cfg = &g_config[num_of_config];
        memset(cfg, 0, sizeof(*cfg));
        strncpy(cfg->put_comp_name, b->comp_put, NL_STR_SHORT-1);
        strncpy(cfg->put_grid_name, b->grid_put, NL_STR_SHORT-1);
        strncpy(cfg->get_comp_name, b->comp_get, NL_STR_SHORT-1);
        strncpy(cfg->get_grid_name, b->grid_get, NL_STR_SHORT-1);
        cfg->mapping_tag    = cal_mapping_tag(b->comp_put, b->comp_get, num_of_config);
        cfg->num_of_exchange = 0;
        cfg->start_ed        = NULL;
        cfg->ed              = NULL;
        current_config_idx   = num_of_config;
        num_of_config++;

    } else if (b->var_put[0] != '\0' || b->var_get[0] != '\0' ||
               b->var_put_vec[0] != '\0' || b->var_get_vec[0] != '\0') {
        /* Exchange data block: append to current config */
        if (current_config_idx < 0 || current_config_idx >= num_of_config) return;

        type_ici_configure* cfg = &g_config[current_config_idx];
        exchange_data_type* ed = (exchange_data_type*)calloc(1, sizeof(*ed));

        strncpy(ed->var_put,     b->var_put,     NL_STR_SHORT-1);
        strncpy(ed->var_get,     b->var_get,     NL_STR_SHORT-1);
        strncpy(ed->var_put_vec, b->var_put_vec, NL_STR_SHORT-1);
        strncpy(ed->var_get_vec, b->var_get_vec, NL_STR_SHORT-1);
        ed->is_vec        = (b->var_get[0] == '\0');
        ed->grid_intpl_tag = b->grid_intpl_tag;
        ed->time_intpl_tag = b->time_intpl_tag;
        ed->intvl          = b->intvl;
        ed->lag            = b->lag;
        strncpy(ed->flag,  b->flag, 3);
        ed->num_of_layer   = b->layer;
        ed->fill_value     = b->fill_value;
        ed->factor         = b->factor;
        ed->is_ok          = b->is_ok;
        ed->mapping_tag    = cfg->mapping_tag;
        ed->next_ptr       = NULL;

        cfg->num_of_exchange++;
        ed->exchange_tag = cfg->num_of_exchange;

        if (b->intvl == 0) ed->lag = 0;

        /* Append to linked list */
        if (cfg->start_ed == NULL) {
            cfg->start_ed = ed;
            cfg->ed       = ed;
        } else {
            cfg->ed->next_ptr = ed;
            cfg->ed           = ed;
        }
    }
}

void read_namelist(const char* file_name) {
    FILE* f = fopen(file_name, "r");
    if (!f) {
        fprintf(stderr, "read_namelist: cannot open %s\n", file_name);
        return;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    char* buf = (char*)malloc(sz + 1);
    fread(buf, 1, sz, f);
    fclose(f);
    buf[sz] = '\0';

    num_of_config      = 0;
    current_config_idx = -1;

    const char* p = buf;
    for (;;) {
        const char* q = find_block(p, "nam_ici");
        if (!q) break;

        nam_ici_block blk;
        memset(&blk, 0, sizeof(blk));
        blk.lag        = -1;
        blk.layer      = 1;
        blk.intvl      = -999;
        blk.fill_value = coupler_conf.fill_value;
        blk.is_ok      = 1;
        blk.factor     = 1.0;

        p = parse_block(q, handle_nam_ici, &blk);
        process_nam_ici(&blk);
    }

    free(buf);
}

/* ======================================================================
 * Accessor functions (1-based indices, matching Fortran convention)
 * ====================================================================== */

int get_num_of_configuration(void) { return num_of_config; }

int get_num_of_exchange(int conf_num) {
    return g_config[conf_num - 1].num_of_exchange;
}

exchange_data_type* get_ed_ptr(int conf_num, int exchange_num) {
    exchange_data_type* ed = g_config[conf_num - 1].start_ed;
    int counter = 0;
    while (ed) {
        counter++;
        if (counter == exchange_num) return ed;
        ed = ed->next_ptr;
    }
    return NULL;
}

const char* get_put_comp_name(int conf_num) { return g_config[conf_num-1].put_comp_name; }
const char* get_put_grid_name(int conf_num) { return g_config[conf_num-1].put_grid_name; }
const char* get_get_comp_name(int conf_num) { return g_config[conf_num-1].get_comp_name; }
const char* get_get_grid_name(int conf_num) { return g_config[conf_num-1].get_grid_name; }
