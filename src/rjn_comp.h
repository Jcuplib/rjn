#pragma once
/*
 * rjn_comp.h
 * Converted from rjn_comp.f90
 * Copyright (c) 2011, arakawa@rist.jp
 */

#include "rjn_constant.h"

/* -----------------------------------------------------------------------
 * component_def_type
 * ----------------------------------------------------------------------- */
typedef struct {
    char component_name[STR_SHORT];
    int  component_id;
    int  leader_rank;
    int  num_of_pe;
} component_def_type;

/* -----------------------------------------------------------------------
 * Public interface
 * ----------------------------------------------------------------------- */

/* Register a component name that belongs to this process */
void set_my_component(const char* component_name);

/* Initialize MPI communicators and build global component list */
void init_model_process(void);

/* Query functions */
int  get_num_of_total_component(void);
void get_component_name(int component_id, char* name_out); /* name_out: char[STR_SHORT] */
int  get_num_of_my_component(void);
int  get_comp_id_from_name(const char* component_name);

/* is_my_component: overloaded in Fortran → two named C functions */
int  is_my_component_id(int component_id);     /* returns 1/0 */
int  is_my_component_name(const char* component_name); /* returns 1/0 */

int  is_model_running(const char* model_name); /* returns 1/0 */
int  get_component_relation(int my_component_id, int target_component_id);
