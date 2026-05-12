/*
 * rjn_comp.cpp
 * Converted from rjn_comp.f90
 * Copyright (c) 2026, arakawa@climtech.jp
 */

#include "rjn_comp.h"
#include "rjn_mpi_lib.h"
#include "rjn_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -----------------------------------------------------------------------
 * Private state (module-level variables)
 * ----------------------------------------------------------------------- */
static int  s_num_of_total_pe       = 0;
static int  s_my_rank_global        = 0;
static int  s_my_rank_local         = 0;
static int  s_my_model_id           = 0;
static int  s_num_of_model          = 0;
static int  s_num_of_task           = 0;
static int  s_num_of_my_component   = 0;
static int  s_num_of_total_component = 0;

static char s_my_component_name[MAX_MODEL][STR_SHORT];

static component_def_type* s_my_comp  = NULL;
static component_def_type* s_all_comp = NULL;

static int* s_config_model_id_to_comp_id = NULL;
static int* s_comp_id_to_config_model_id  = NULL;

/* -----------------------------------------------------------------------
 * Forward declarations of private helpers
 * ----------------------------------------------------------------------- */
static void set_global_component(char** total_comp_name_out, int* num_out);
static void search_next_process(int current_process,
                                char comp_name[][STR_SHORT], int* num_of_comp,
                                int* next_process);
static void add_comp_name(int current_process,
                          char comp_name[][STR_SHORT], int* num_of_comp);
static void init_my_component_info(const char* total_comp_name, int n_total);
static void set_component_info(void);
static void sort_component_name(char* comp_name, int n);  /* flat array n×STR_SHORT */

/* -----------------------------------------------------------------------
 * Public: set_my_component
 * ----------------------------------------------------------------------- */
void set_my_component(const char* component_name)
{
    if (s_num_of_my_component >= MAX_MODEL) {
        rjn_error("set_my_component", "too many components");
    }
    strncpy(s_my_component_name[s_num_of_my_component], component_name, STR_SHORT - 1);
    s_my_component_name[s_num_of_my_component][STR_SHORT - 1] = '\0';
    s_num_of_my_component++;
}

/* -----------------------------------------------------------------------
 * Private: add_comp_name
 * Adds names from my_component_name that are not already in comp_name[].
 * Only called by the current_process rank.
 * ----------------------------------------------------------------------- */
static void add_comp_name(int current_process,
                          char comp_name[][STR_SHORT], int* num_of_comp)
{
    int i, j;
    int same_flag;

    if (s_my_rank_global != current_process) return;

    for (i = 0; i < s_num_of_my_component; i++) {
        same_flag = 0;
        for (j = 0; j < *num_of_comp; j++) {
            if (strcmp(s_my_component_name[i], comp_name[j]) == 0) {
                same_flag = 1;
                break;
            }
        }
        if (!same_flag) {
            strncpy(comp_name[*num_of_comp], s_my_component_name[i], STR_SHORT - 1);
            comp_name[*num_of_comp][STR_SHORT - 1] = '\0';
            (*num_of_comp)++;
        }
    }
}

/* -----------------------------------------------------------------------
 * Private: search_next_process
 * Collaboratively find the next process (by global rank) whose component
 * set is not fully covered by the current set, and broadcast the updated
 * global component list.
 * ----------------------------------------------------------------------- */
static void search_next_process(int current_process,
                                char comp_name[][STR_SHORT], int* num_of_comp,
                                int* next_process)
{
    int int_buffer[1];
    int same_index = 999999999;
    int n_buf, i, j;
    int same_flag;
    char* name_buffer = NULL;

    if (s_my_rank_global == current_process) {
        add_comp_name(current_process, comp_name, num_of_comp);

        int_buffer[0] = *num_of_comp;
	
        jml_BcastGlobal_int(int_buffer, 1, 1, current_process);

        n_buf = int_buffer[0];
        name_buffer = (char*)malloc(n_buf * STR_SHORT);
        for (i = 0; i < n_buf; i++)
            memcpy(name_buffer + i * STR_SHORT, comp_name[i], STR_SHORT);

        jml_BcastGlobal_strarr(name_buffer, n_buf, STR_SHORT, current_process);

    } else {
        jml_BcastGlobal_int(int_buffer, 1, 1, current_process);
        n_buf = int_buffer[0];
	
        *num_of_comp = n_buf;

        name_buffer = (char*)malloc(n_buf * STR_SHORT);
        jml_BcastGlobal_strarr(name_buffer, n_buf, STR_SHORT, current_process);

        for (i = 0; i < n_buf; i++)
            memcpy(comp_name[i], name_buffer + i * STR_SHORT, STR_SHORT);

        if (s_my_rank_global > current_process) {
            for (i = 0; i < s_num_of_my_component; i++) {
                same_flag = 0;
                for (j = 0; j < n_buf; j++) {
                    if (strncmp(name_buffer + j * STR_SHORT,
                                s_my_component_name[i], STR_SHORT) == 0) {
                        same_flag = 1;
                        break;
                    }
                }
                if (!same_flag) {
                    same_index = s_my_rank_global;
                    break;
                }
            }
        }
    }

    free(name_buffer);

    int_buffer[0] = same_index;
    jml_AllreduceMin(int_buffer[0], next_process);
}

/* -----------------------------------------------------------------------
 * Private: set_global_component
 * Builds the list of all unique component names across all processes.
 * Returns a flat array (caller must free) and the count.
 * ----------------------------------------------------------------------- */
static void set_global_component(char** total_comp_name_out, int* num_out)
{
    char comp_name[MAX_MODEL][STR_SHORT];
    int  num_of_comp = 0;
    int  current_process = 0;
    int  next_process;
    int  i;
    char* result;

    memset(comp_name, 0, sizeof(comp_name));

    while (1) {
        search_next_process(current_process, comp_name, &num_of_comp, &next_process);
	  
        if (next_process > 999999) break;
        current_process = next_process;
    }

    
    result = (char*)malloc(num_of_comp * STR_SHORT);
    for (i = 0; i < num_of_comp; i++)
        memcpy(result + i * STR_SHORT, comp_name[i], STR_SHORT);

    *total_comp_name_out = result;
    *num_out = num_of_comp;
}

/* -----------------------------------------------------------------------
 * Private: sort_component_name
 * Selection-sort (by lexicographic order) for a flat char array.
 * ----------------------------------------------------------------------- */
static void sort_component_name(char* comp_name, int n)
{
    char* tmp = (char*)malloc(STR_SHORT);
    int i, j, min_idx;

    for (i = 0; i < n - 1; i++) {
        min_idx = i;
        for (j = i + 1; j < n; j++) {
            if (strncmp(comp_name + j * STR_SHORT,
                        comp_name + min_idx * STR_SHORT, STR_SHORT) < 0)
                min_idx = j;
        }
        if (min_idx != i) {
            memcpy(tmp, comp_name + i * STR_SHORT, STR_SHORT);
            memcpy(comp_name + i * STR_SHORT, comp_name + min_idx * STR_SHORT, STR_SHORT);
            memcpy(comp_name + min_idx * STR_SHORT, tmp, STR_SHORT);
        }
    }
    free(tmp);
}

/* -----------------------------------------------------------------------
 * Private: init_my_component_info
 * total_comp_name: flat array of n_total strings each STR_SHORT bytes.
 * ----------------------------------------------------------------------- */
static void init_my_component_info(const char* total_comp_name, int n_total)
{
    int* local_comp_flag  = (int*)malloc(n_total * sizeof(int));
    int* global_comp_flag = (int*)malloc(n_total * sizeof(int));
    int i, j, counter, local_counter;
    int error_flag;

    /* Verify that each of my components appears in total_comp_name */
    for (i = 0; i < s_num_of_my_component; i++) {
        error_flag = 1;
        for (j = 0; j < n_total; j++) {
            if (strncmp(total_comp_name + j * STR_SHORT,
                        s_my_component_name[i], STR_SHORT) == 0)
                error_flag = 0;
        }
        if (error_flag) {
            char msg[STR_MID];
            snprintf(msg, STR_MID, "no such component: %s listed in coupling.conf file",
                     s_my_component_name[i]);
            rjn_error("init_my_component_info", msg);
        }
    }

    /* Build local and global participation flags */
    for (i = 0; i < n_total; i++) local_comp_flag[i] = 0;
    for (i = 0; i < n_total; i++) global_comp_flag[i] = 0;

    for (i = 0; i < n_total; i++) {
        for (j = 0; j < s_num_of_my_component; j++) {
            if (strncmp(total_comp_name + i * STR_SHORT,
                        s_my_component_name[j], STR_SHORT) == 0) {
                local_comp_flag[i] = 1;
                break;
            }
        }
        jml_AllreduceMax(local_comp_flag[i], &global_comp_flag[i]);
    }

    /* Count active components */
    counter = 0;
    for (i = 0; i < n_total; i++)
        if (global_comp_flag[i] != 0) counter++;
    s_num_of_total_component = counter;

    s_config_model_id_to_comp_id = (int*)malloc(n_total * sizeof(int));
    s_comp_id_to_config_model_id = (int*)malloc(s_num_of_total_component * sizeof(int));

    for (i = 0; i < n_total; i++) s_config_model_id_to_comp_id[i] = 0;
    for (i = 0; i < s_num_of_total_component; i++) s_comp_id_to_config_model_id[i] = 0;

    counter = 0;
    for (i = 0; i < n_total; i++) {
        if (global_comp_flag[i] != 0) {
            s_config_model_id_to_comp_id[i] = counter + 1; /* 1-based comp_id */
            s_comp_id_to_config_model_id[counter] = i + 1; /* 1-based config index */
            counter++;
        }
    }

    /* Allocate and populate all_comp[] */
    s_all_comp = (component_def_type*)malloc(s_num_of_total_component
                                             * sizeof(component_def_type));
    counter = 0;
    for (i = 0; i < n_total; i++) {
        if (global_comp_flag[i] != 0) {
            strncpy(s_all_comp[counter].component_name,
                    total_comp_name + i * STR_SHORT, STR_SHORT - 1);
            s_all_comp[counter].component_name[STR_SHORT - 1] = '\0';
            s_all_comp[counter].component_id  = counter + 1; /* 1-based */
            s_all_comp[counter].leader_rank   = 0;
            s_all_comp[counter].num_of_pe     = 0;
            counter++;
        }
    }

    /* Allocate and populate my_comp[] */
    s_my_comp = (component_def_type*)malloc(s_num_of_my_component
                                            * sizeof(component_def_type));
    counter       = 0;
    local_counter = 0;
    for (i = 0; i < n_total; i++) {
        if (global_comp_flag[i] != 0) {
            if (local_comp_flag[i] != 0) {
                s_my_comp[local_counter] = s_all_comp[counter];
                local_counter++;
            }
            counter++;
        }
    }

    free(local_comp_flag);
    free(global_comp_flag);
}

/* -----------------------------------------------------------------------
 * Private: set_component_info
 * Fill in leader_rank and num_of_pe for all known components.
 * ----------------------------------------------------------------------- */
static void set_component_info(void)
{
    int i, comp_id;

    for (i = 0; i < s_num_of_total_component; i++) {
        comp_id = s_all_comp[i].component_id;
        s_all_comp[i].leader_rank = jml_GetLeaderRank(comp_id);
        s_all_comp[i].num_of_pe   = jml_GetCommSizeLocal(comp_id);
    }
}

/* -----------------------------------------------------------------------
 * Public: init_model_process
 * ----------------------------------------------------------------------- */
void init_model_process(void)
{
    char* total_comp_name = NULL;
    int   n_total         = 0;
    int*  my_comp_id      = NULL;
    int   p;

    jml_init();

    s_my_rank_global  = jml_GetMyrankGlobal();
    s_num_of_total_pe = jml_GetCommSizeGlobal();

    if (s_num_of_my_component == 0)
        rjn_error("init_model_process", "No component name assigned.");

    set_global_component(&total_comp_name, &n_total);

    sort_component_name(total_comp_name, n_total);

    init_my_component_info(total_comp_name, n_total);

    free(total_comp_name);

    my_comp_id = (int*)malloc(s_num_of_my_component * sizeof(int));
    for (p = 0; p < s_num_of_my_component; p++)
        my_comp_id[p] = s_my_comp[p].component_id;

    jml_create_communicator(my_comp_id, s_num_of_my_component);

    free(my_comp_id);

    set_component_info();
}

/* -----------------------------------------------------------------------
 * Public query functions
 * ----------------------------------------------------------------------- */
int get_num_of_total_component(void)
{
    return s_num_of_total_component;
}

void get_component_name(int component_id, char* name_out)
{
    strncpy(name_out, s_all_comp[component_id - 1].component_name, STR_SHORT - 1);
    name_out[STR_SHORT - 1] = '\0';
}

int get_num_of_my_component(void)
{
    return s_num_of_my_component;
}

int get_comp_id_from_name(const char* component_name)
{
    int i;
    for (i = 0; i < s_num_of_total_component; i++) {
        if (strcmp(s_all_comp[i].component_name, component_name) == 0)
            return s_all_comp[i].component_id;
    }
    {
        char msg[STR_MID];
        snprintf(msg, STR_MID, "no such component name : %s", component_name);
        rjn_error("get_comp_id_from_name", msg);
    }
    return 0;
}

int is_my_component_id(int component_id)
{
    int i;
    for (i = 0; i < s_num_of_my_component; i++) {
        if (component_id == s_my_comp[i].component_id) return 1;
    }
    return 0;
}

int is_my_component_name(const char* component_name)
{
    int comp_id = get_comp_id_from_name(component_name);
    return is_my_component_id(comp_id);
}

int is_model_running(const char* model_name)
{
    int i;
    for (i = 0; i < s_num_of_total_component; i++) {
        if (strcmp(s_all_comp[i].component_name, model_name) == 0) return 1;
    }
    return 0;
}

int get_component_relation(int my_component_id, int target_component_id)
{
    int my_min_pe, my_max_pe;
    int target_min_pe, target_max_pe;

    my_min_pe  = s_all_comp[my_component_id - 1].leader_rank;
    my_max_pe  = my_min_pe + s_all_comp[my_component_id - 1].num_of_pe - 1;
    target_min_pe = s_all_comp[target_component_id - 1].leader_rank;
    target_max_pe = target_min_pe + s_all_comp[target_component_id - 1].num_of_pe - 1;

    if (my_max_pe < target_min_pe || my_min_pe > target_max_pe)
        return COMP_PARALLEL;

    if (my_min_pe == target_min_pe && my_max_pe == target_max_pe)
        return COMP_SERIAL;

    if (my_min_pe <= target_min_pe && my_max_pe >= target_max_pe)
        return COMP_SUPERSET;

    if (my_min_pe >= target_min_pe && my_max_pe <= target_max_pe)
        return COMP_SUBSET;

    return COMP_OVERLAP;
}
