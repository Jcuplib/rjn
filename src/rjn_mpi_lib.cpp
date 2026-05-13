/*
 * rjn_mpi_lib.cpp
 * Converted from rjn_mpi_lib.f90
 * Copyright (c) 2026, arakawa@climtech.jp
 */

#include "rjn_mpi_lib.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Public variables */
int JML_ROOTS_TAG  = 120;
int JML_ANY_SOURCE = MPI_ANY_SOURCE; /* initialised in jml_init */

/* Private MPI tag constant */
static const int MPI_MY_TAG = 0;
static int64_t   s_MPI_MAX_TAG = 32767;

/* Communicator type (mirrors Fortran comm_type) */
struct CommType {
    int group_id;
    MPI_Group group;
    int group_fint;
    int num_of_pe;
    int root_rank;
    int my_rank;
    int leader_rank;
    MPI_Comm mpi_comm;
    int pe_offset;
    CommType* inter_comm; /* array of size num_of_total_component */
};

/* Module-level static state */
static MPI_Comm   s_GLOBAL_COMM = MPI_COMM_WORLD;
static CommType   s_global;
static CommType   s_leader;
static int*       s_leader_pe        = NULL; /* 0-based index by comp_id-1 */
static int        s_num_of_total_component = 0;
static CommType*  s_local            = NULL; /* size: num_of_total_component */
static CommType*  s_current_comp     = NULL;
static int        s_isInitialized    = 0;
static int        s_num_of_models    = 0;

/* Buffer management */
static double*    s_local_buffer     = NULL;
static int        s_buffer_byte      = 8;
static int        s_buffer_count     = 10;
static int        s_buffer_data      = 1000000;
static int        s_buffer_size      = 0;
static int        s_is_use_buffer    = 0;

/* Non-blocking request arrays */
static int        s_isend_counter    = 0;
static MPI_Request* s_isend_request  = NULL;
static int          s_isend_capacity = 0;

static int        s_irecv_counter    = 0;
static MPI_Request* s_irecv_request  = NULL;
static int          s_irecv_capacity = 0;

/* ===== Internal helpers ===== */

static void init_comm(CommType* comm) {
    comm->group_id    = MPI_UNDEFINED;
    comm->group       = MPI_GROUP_NULL;
    comm->group_fint  = (int)MPI_Group_c2f(MPI_GROUP_NULL);
    comm->num_of_pe   = MPI_UNDEFINED;
    comm->root_rank   = MPI_UNDEFINED;
    comm->my_rank     = MPI_UNDEFINED;
    comm->leader_rank = MPI_UNDEFINED;
    comm->mpi_comm    = MPI_COMM_NULL;
    comm->pe_offset   = 0;
    comm->inter_comm  = NULL;
}

static int is_my_component(const CommType* comm) {
    return (comm->my_rank != MPI_UNDEFINED);
}

static void set_current_component(int component_id) {
    int i;
    for (i = 0; i < s_num_of_total_component; i++) {
        if (s_local[i].group_id == component_id) {
            s_current_comp = &s_local[i];
            return;
        }
    }
    fprintf(stderr, "set_current_component: no such component id %d\n", component_id);
    MPI_Abort(MPI_COMM_WORLD, 1);
}

static int cal_pe_offset(int my_leader_rank, int my_size, int target_leader_rank) {
    if (target_leader_rank >= my_leader_rank + my_size)
        return my_size;
    int v = target_leader_rank - my_leader_rank;
    return (v > 0) ? v : 0;
}

static int jml_isLeader(void) {
    return (s_leader.my_rank != MPI_UNDEFINED);
}

static void check_buffer_size(int data_size) {
    /* No-op in C++ version: buffer is pre-allocated */
    (void)data_size;
}

/* ===== Public API ===== */

void jml_set_global_comm(int global_comm) {
    s_GLOBAL_COMM = MPI_Comm_f2c((MPI_Fint)global_comm);
}

int jml_get_global_comm(void) {
    return (int)MPI_Comm_c2f(s_GLOBAL_COMM);
}

void jml_init(void) {
    int is_initialized;
    int flag;
    MPI_Initialized(&is_initialized);
    if (!is_initialized) MPI_Init(NULL, NULL);

    MPI_Comm_group(s_GLOBAL_COMM, &s_global.group);
    s_global.group_fint = (int)MPI_Group_c2f(s_global.group);
    MPI_Comm_size(s_GLOBAL_COMM, &s_global.num_of_pe);
    MPI_Comm_rank(s_GLOBAL_COMM, &s_global.my_rank);
    s_global.mpi_comm  = s_GLOBAL_COMM;
    s_global.root_rank = 0;

    s_isInitialized = 1;

    JML_ANY_SOURCE = MPI_ANY_SOURCE;

    if (!s_local_buffer) {
        int overhead;
        overhead      = MPI_BSEND_OVERHEAD;
        s_buffer_size = s_buffer_count * s_buffer_data * s_buffer_byte + overhead;
        s_local_buffer = (double*)malloc(s_buffer_size);
        if (s_is_use_buffer) {
            MPI_Buffer_attach(s_local_buffer, s_buffer_size);
        }
    }

    s_isend_counter  = 0;
    if (!s_isend_request) {
        s_isend_capacity = 10000;
        s_isend_request  = (MPI_Request*)malloc(s_isend_capacity * sizeof(MPI_Request));
    }
    s_irecv_counter  = 0;
    if (!s_irecv_request) {
        s_irecv_capacity = 10000;
        s_irecv_request  = (MPI_Request*)malloc(s_irecv_capacity * sizeof(MPI_Request));
    }

    {
        void* tag_ub_attr = NULL;
        MPI_Comm_get_attr(s_GLOBAL_COMM, MPI_TAG_UB, &tag_ub_attr, &flag);
        if (flag && tag_ub_attr) s_MPI_MAX_TAG = *(int*)tag_ub_attr;
        else      s_MPI_MAX_TAG = 32767;
    }
}

static void create_new_communicator(int color, int key, CommType* comm) {
    comm->group_id = color;
    MPI_Comm_split(s_global.mpi_comm, color, key, &comm->mpi_comm);
    if (comm->mpi_comm != MPI_COMM_NULL) {
        comm->root_rank = 0;
        MPI_Comm_group(comm->mpi_comm, &comm->group);
        comm->group_fint = (int)MPI_Group_c2f(comm->group);
        MPI_Comm_size(comm->mpi_comm, &comm->num_of_pe);
        MPI_Comm_rank(comm->mpi_comm, &comm->my_rank);
        if (comm->my_rank == 0) {
            comm->leader_rank = s_global.my_rank;
        }
    }
}

static void create_intercomponent_communicator(CommType* comm1, CommType* comm2) {
    MPI_Comm new_comm;
    int new_size, new_rank;
    int color = MPI_UNDEFINED;
    if (is_my_component(comm1) || is_my_component(comm2)) color = 1;
    MPI_Comm_split(s_global.mpi_comm, color, 0, &new_comm);
    if (is_my_component(comm1) || is_my_component(comm2)) {
        MPI_Comm_size(new_comm, &new_size);
        MPI_Comm_rank(new_comm, &new_rank);
        if (is_my_component(comm1)) {
            int tid = comm2->group_id - 1; /* 0-based */
            comm1->inter_comm[tid].group_id  = comm2->group_id;
            comm1->inter_comm[tid].mpi_comm  = new_comm;
            comm1->inter_comm[tid].my_rank   = new_rank;
            comm1->inter_comm[tid].pe_offset =
                cal_pe_offset(comm1->leader_rank, comm1->num_of_pe, comm2->leader_rank);
        }
        if (is_my_component(comm2)) {
            int tid = comm1->group_id - 1; /* 0-based */
            comm2->inter_comm[tid].group_id  = comm1->group_id;
            comm2->inter_comm[tid].mpi_comm  = new_comm;
            comm2->inter_comm[tid].my_rank   = new_rank;
            comm2->inter_comm[tid].pe_offset =
                cal_pe_offset(comm2->leader_rank, comm2->num_of_pe, comm1->leader_rank);
        }
    }
}

static void create_leader_communicator(void) {
    int color = MPI_UNDEFINED;
    int key   = 0;
    int i, res;

    if (s_num_of_total_component <= 0) return;
    s_leader_pe = (int*)calloc((size_t)s_num_of_total_component, sizeof(int));

    for (i = 0; i < s_num_of_total_component; i++) {
        if (s_local[i].leader_rank == s_global.my_rank) color = 1;
    }

    create_new_communicator(color, key, &s_leader);

    for (i = 0; i < s_num_of_total_component; i++) {
        if (s_local[i].leader_rank == s_global.my_rank)
            s_leader_pe[i] = s_leader.my_rank;
        if (s_leader.mpi_comm != MPI_COMM_NULL) {
            MPI_Allreduce(&s_leader_pe[i], &res, 1, MPI_INT, MPI_MAX, s_leader.mpi_comm);
            s_leader_pe[i] = res;
        }
    }
}

void jml_create_communicator(const int* my_comp_id, int n) {
    int send_buf, recv_buf;
    int i, j, color, key;

    /* find max comp id globally */
    send_buf = 0;
    for (i = 0; i < n; i++) if (my_comp_id[i] > send_buf) send_buf = my_comp_id[i];
    MPI_Allreduce(&send_buf, &recv_buf, 1, MPI_INT, MPI_MAX, s_global.mpi_comm);
    s_num_of_total_component = recv_buf;

    s_local = (CommType*)calloc(s_num_of_total_component, sizeof(CommType));
    for (i = 0; i < s_num_of_total_component; i++) {
        init_comm(&s_local[i]);
        s_local[i].inter_comm = (CommType*)calloc(s_num_of_total_component, sizeof(CommType));
        for (j = 0; j < s_num_of_total_component; j++) {
            init_comm(&s_local[i].inter_comm[j]);
        }
    }

    key = s_global.my_rank;
    for (i = 0; i < s_num_of_total_component; i++) {
        color = MPI_UNDEFINED;
        for (j = 0; j < n; j++) {
            if ((i + 1) == my_comp_id[j]) { color = i + 1; break; }
        }
        create_new_communicator(color, key, &s_local[i]);
    }

    /* broadcast leader_rank, num_of_pe, group, group_id for each comp */
    for (i = 0; i < s_num_of_total_component; i++) {
        MPI_Allreduce(&s_local[i].leader_rank, &recv_buf, 1, MPI_INT, MPI_MAX, s_global.mpi_comm);
        s_local[i].leader_rank = recv_buf;
    }
    for (i = 0; i < s_num_of_total_component; i++) {
        MPI_Bcast(&s_local[i].num_of_pe, 1, MPI_INT, s_local[i].leader_rank, s_global.mpi_comm);
    }
    for (i = 0; i < s_num_of_total_component; i++) {
        MPI_Bcast(&s_local[i].group_fint, 1, MPI_INT, s_local[i].leader_rank, s_global.mpi_comm);
    }
    for (i = 0; i < s_num_of_total_component; i++) {
        MPI_Bcast(&s_local[i].group_id, 1, MPI_INT, s_local[i].leader_rank, s_global.mpi_comm);
    }
    for (i = 0; i < s_num_of_total_component; i++) {
        MPI_Bcast(&s_local[i].leader_rank, 1, MPI_INT, s_local[i].leader_rank, s_global.mpi_comm);
    }

    for (i = 0; i < s_num_of_total_component; i++) {
        for (j = i; j < s_num_of_total_component; j++) {
            create_intercomponent_communicator(&s_local[i], &s_local[j]);
        }
    }

    init_comm(&s_leader);
    create_leader_communicator();
}

void jml_finalize(int is_call_finalize) {
    int is_finalized;
    void* buf_addr;
    int buf_size;
    if (s_is_use_buffer) MPI_Buffer_detach(&buf_addr, &buf_size);
    if (s_local_buffer) { free(s_local_buffer); s_local_buffer = NULL; }
    if (!is_call_finalize) return;
    if (s_isInitialized) {
        MPI_Finalized(&is_finalized);
        if (!is_finalized) MPI_Finalize();
    }
}

void jml_abort(void) {
    MPI_Abort(s_global.mpi_comm, 0);
}

int jml_GetCommGlobal(void)      { return (int)MPI_Comm_c2f(s_global.mpi_comm); }
int jml_GetMyrankGlobal(void)    { return s_global.my_rank; }
int jml_GetCommSizeGlobal(void)  { return s_global.num_of_pe; }
int jml_GetCommLeader(void)      { return (int)MPI_Comm_c2f(s_leader.mpi_comm); }
int jml_isRoot(void)             { return (s_global.root_rank == s_global.my_rank); }

int jml_GetComm(int component_id) {
    set_current_component(component_id);
    return (int)MPI_Comm_c2f(s_current_comp->mpi_comm);
}

int jml_GetCommNULL(void) { return (int)MPI_Comm_c2f(MPI_COMM_NULL); }

int jml_GetMyGroup(int component_id) {
    set_current_component(component_id);
    return s_current_comp->group_fint;
}

int jml_GetMyrank(int component_id) {
    set_current_component(component_id);
    return s_current_comp->my_rank;
}

int jml_GetCommSizeLocal(int component_id) {
    set_current_component(component_id);
    return s_current_comp->num_of_pe;
}

int jml_GetModelRankOffset(int my_comp_id, int target_comp_id) {
    return s_local[my_comp_id-1].inter_comm[target_comp_id-1].pe_offset;
}

int jml_GetMyrankModel(int my_comp_id, int target_comp_id) {
    return s_local[my_comp_id-1].inter_comm[target_comp_id-1].my_rank;
}

int jml_GetLeaderRank(int component_id) {
    set_current_component(component_id);
    return s_current_comp->leader_rank;
}

int jml_isLocalLeader(int component_id) {
    set_current_component(component_id);
    return (s_current_comp->my_rank == 0);
}

void jml_BarrierLeader(void) {
    MPI_Barrier(s_leader.mpi_comm);
}

int jml_ProbeLeader(int source, int tag) {
    MPI_Status status;
    int flag;
    int source_rank;
    if (!jml_isLeader()) return 0;
    source_rank = s_leader_pe[source-1];
    MPI_Iprobe(source_rank, tag, s_leader.mpi_comm, &flag, &status);
    return flag;
}

int jml_ProbeAll(int source) {
    /* Simplified: probe on global comm */
    MPI_Status status;
    int flag;
    MPI_Iprobe(source, MPI_ANY_TAG, s_global.mpi_comm, &flag, &status);
    return flag;
}

/* --- Global broadcast --- */
void jml_BcastGlobal_str(char* str, int len, int source_rank) {
    MPI_Bcast(str, len, MPI_CHAR, source_rank, s_global.mpi_comm);
}

void jml_BcastGlobal_strarr(char* str_arr, int num, int str_len, int source_rank) {
    MPI_Bcast(str_arr, num * str_len, MPI_CHAR, source_rank, s_global.mpi_comm);
}

void jml_BcastGlobal_int(int* data, int is, int ie, int source_rank) {
    MPI_Bcast(&data[is-1], ie - is + 1, MPI_INT, source_rank, s_global.mpi_comm);
}

/* --- Global Send/Recv --- */
void jml_SendGlobal_str(const char* str, int len, int dest) {
    MPI_Request req;
    MPI_Status  st;
    MPI_Isend((void*)str, len, MPI_CHAR, dest, MPI_MY_TAG, s_global.mpi_comm, &req);
    MPI_Wait(&req, &st);
}

void jml_SendGlobal_strarr(const char* str_arr, int num, int str_len, int dest) {
    MPI_Request req;
    MPI_Status  st;
    MPI_Isend((void*)str_arr, num * str_len, MPI_CHAR, dest, MPI_MY_TAG, s_global.mpi_comm, &req);
    MPI_Wait(&req, &st);
}

void jml_SendGlobal_int(const int* data, int is, int ie, int dest) {
    MPI_Request req;
    MPI_Status  st;
    int n = ie - is + 1;
    int* buf = (int*)malloc(n * sizeof(int));
    memcpy(buf, &data[is-1], n * sizeof(int));
    MPI_Isend(buf, n, MPI_INT, dest, MPI_MY_TAG, s_global.mpi_comm, &req);
    MPI_Wait(&req, &st);
    free(buf);
}

void jml_RecvGlobal_str(char* str, int len, int source) {
    MPI_Request req;
    MPI_Status  st;
    MPI_Irecv(str, len, MPI_CHAR, source, MPI_MY_TAG, s_global.mpi_comm, &req);
    MPI_Wait(&req, &st);
}

void jml_RecvGlobal_strarr(char* str_arr, int num, int str_len, int source) {
    MPI_Request req;
    MPI_Status  st;
    MPI_Irecv(str_arr, num * str_len, MPI_CHAR, source, MPI_MY_TAG, s_global.mpi_comm, &req);
    MPI_Wait(&req, &st);
}

void jml_RecvGlobal_int(int* data, int is, int ie, int source) {
    MPI_Request req;
    MPI_Status  st;
    int n = ie - is + 1;
    int* buf = (int*)malloc(n * sizeof(int));
    MPI_Irecv(buf, n, MPI_INT, source, MPI_MY_TAG, s_global.mpi_comm, &req);
    MPI_Wait(&req, &st);
    memcpy(&data[is-1], buf, n * sizeof(int));
    free(buf);
}

/* --- AllReduce --- */
void jml_AllreduceMax(int d, int* res) {
    MPI_Allreduce(&d, res, 1, MPI_INT, MPI_MAX, s_global.mpi_comm);
}

void jml_AllreduceMin(int d, int* res) {
    MPI_Allreduce(&d, res, 1, MPI_INT, MPI_MIN, s_global.mpi_comm);
}

void jml_AllReduceSum_int(int d, int* res) {
    MPI_Allreduce(&d, res, 1, MPI_INT, MPI_SUM, s_global.mpi_comm);
}

void jml_AllReduceSum_intar(int nd, const int* d, int* res) {
    MPI_Allreduce((void*)d, res, nd, MPI_INT, MPI_SUM, s_global.mpi_comm);
}

void jml_AllReduceMaxLocal(int comp, int d, int* res) {
    MPI_Allreduce(&d, res, 1, MPI_INT, MPI_MAX, s_local[comp-1].mpi_comm);
}

void jml_AllReduceMinLocal(int comp, int d, int* res) {
    MPI_Allreduce(&d, res, 1, MPI_INT, MPI_MIN, s_local[comp-1].mpi_comm);
}

void jml_AllReduceSumLocal(int comp, int d, int* res) {
    MPI_Allreduce(&d, res, 1, MPI_INT, MPI_SUM, s_local[comp-1].mpi_comm);
}

void jml_ReduceSumLocal_int(int comp, int d, int* sum) {
    MPI_Reduce(&d, sum, 1, MPI_INT, MPI_SUM, MPI_MY_TAG, s_local[comp-1].mpi_comm);
}

void jml_ReduceSumLocal_double(int comp, double d, double* sum) {
    MPI_Reduce(&d, sum, 1, MPI_DOUBLE, MPI_SUM, MPI_MY_TAG, s_local[comp-1].mpi_comm);
}

void jml_ReduceMeanLocal(int comp, double d, double* mean) {
    MPI_Reduce(&d, mean, 1, MPI_DOUBLE, MPI_SUM, MPI_MY_TAG, s_local[comp-1].mpi_comm);
    if (s_local[comp-1].my_rank == MPI_MY_TAG)
        *mean = *mean / s_local[comp-1].num_of_pe;
}

void jml_ReduceMinLocal(int comp, int d, int* res) {
    MPI_Reduce(&d, res, 1, MPI_INT, MPI_MIN, 0, s_local[comp-1].mpi_comm);
}

void jml_ReduceMaxLocal(int comp, int d, int* res) {
    MPI_Reduce(&d, res, 1, MPI_INT, MPI_MAX, 0, s_local[comp-1].mpi_comm);
}

/* --- Local Broadcast --- */
void jml_BcastLocal_str(int comp, char* str, int len, int source) {
    MPI_Bcast(str, len, MPI_CHAR, source, s_local[comp-1].mpi_comm);
}

void jml_BcastLocal_int(int comp, int* data, int is, int ie, int source) {
    MPI_Bcast(&data[is-1], ie - is + 1, MPI_INT, source, s_local[comp-1].mpi_comm);
}

void jml_BcastLocal_int_scalar(int comp, int* data, int source) {
    MPI_Bcast(data, 1, MPI_INT, source, s_local[comp-1].mpi_comm);
}

void jml_BcastLocal_long(int comp, int64_t* data, int is, int ie, int source) {
    /* Fortran uses 2*MPI_INTEGER; use MPI_INT64_T */
    MPI_Bcast(&data[is-1], ie - is + 1, MPI_INT64_T, source, s_local[comp-1].mpi_comm);
}

void jml_BcastLocal_float(int comp, float* data, int is, int ie, int source) {
    MPI_Bcast(&data[is-1], ie - is + 1, MPI_FLOAT, source, s_local[comp-1].mpi_comm);
}

void jml_BcastLocal_double(int comp, double* data, int is, int ie, int source) {
    MPI_Bcast(&data[is-1], ie - is + 1, MPI_DOUBLE, source, s_local[comp-1].mpi_comm);
}

/* --- Gather / Scatter Local --- */
void jml_GatherLocal_int(int comp, const int* data, int is, int ie, int* recv_data) {
    int n = ie - is + 1;
    MPI_Gather((void*)&data[is-1], n, MPI_INT, recv_data, n, MPI_INT, 0, s_local[comp-1].mpi_comm);
}

void jml_GatherLocal_float(int comp, const float* data, int is, int ie, float* recv_data) {
    int n = ie - is + 1;
    MPI_Gather((void*)&data[is-1], n, MPI_FLOAT, recv_data, n, MPI_FLOAT, 0, s_local[comp-1].mpi_comm);
}

void jml_AllGatherLocal_int(int comp, const int* data, int is, int ie, int* recv_data) {
    int n = ie - is + 1;
    MPI_Allgather((void*)&data[is-1], n, MPI_INT, recv_data, n, MPI_INT, s_local[comp-1].mpi_comm);
}

void jml_ScatterLocal_int(int comp, const int* send_data, int num_of_data, int* recv_data) {
    MPI_Scatter((void*)send_data, num_of_data, MPI_INT, recv_data, num_of_data, MPI_INT, 0, s_local[comp-1].mpi_comm);
}

void jml_GatherVLocal_int(int comp, const int* send_data, int num_of_my_data,
                          int* recv_data, const int* num_of_data, const int* offset) {
    MPI_Gatherv((void*)send_data, num_of_my_data, MPI_INT,
                recv_data, (int*)num_of_data, (int*)offset, MPI_INT,
                0, s_local[comp-1].mpi_comm);
}

void jml_GatherVLocal_double(int comp, const double* send_data, int num_of_my_data,
                             double* recv_data, const int* num_of_data, const int* offset) {
    MPI_Gatherv((void*)send_data, num_of_my_data, MPI_DOUBLE,
                recv_data, (int*)num_of_data, (int*)offset, MPI_DOUBLE,
                0, s_local[comp-1].mpi_comm);
}

void jml_ScatterVLocal_int(int comp, const int* send_data, const int* num_of_data,
                           const int* offset, int* recv_data, int num_of_my_data) {
    MPI_Scatterv((void*)send_data, (int*)num_of_data, (int*)offset, MPI_INT,
                 recv_data, num_of_my_data, MPI_INT, 0, s_local[comp-1].mpi_comm);
}

void jml_ScatterVLocal_double(int comp, const double* send_data, const int* num_of_data,
                              const int* offset, double* recv_data, int num_of_my_data) {
    MPI_Scatterv((void*)send_data, (int*)num_of_data, (int*)offset, MPI_DOUBLE,
                 recv_data, num_of_my_data, MPI_DOUBLE, 0, s_local[comp-1].mpi_comm);
}

/* --- SendLocal / RecvLocal --- */
void jml_SendLocal_int1d(int comp, const int* data, int is, int ie, int dest) {
    int n = ie - is + 1;
    int* buf = (int*)malloc(n * sizeof(int));
    MPI_Request req; MPI_Status st;
    memcpy(buf, &data[is-1], n * sizeof(int));
    MPI_Isend(buf, n, MPI_INT, dest, MPI_MY_TAG, s_local[comp-1].mpi_comm, &req);
    MPI_Wait(&req, &st);
    free(buf);
}

void jml_RecvLocal_int1d(int comp, int* data, int is, int ie, int source) {
    int n = ie - is + 1;
    int* buf = (int*)malloc(n * sizeof(int));
    MPI_Request req; MPI_Status st;
    MPI_Irecv(buf, n, MPI_INT, source, MPI_MY_TAG, s_local[comp-1].mpi_comm, &req);
    MPI_Wait(&req, &st);
    memcpy(&data[is-1], buf, n * sizeof(int));
    free(buf);
}

void jml_SendLocal_int2d(int comp, const int* data, int is, int ie, int js, int je, int dest) {
    int n = (ie-is+1)*(je-js+1);
    MPI_Request req; MPI_Status st;
    MPI_Isend((void*)data, n, MPI_INT, dest, MPI_MY_TAG, s_local[comp-1].mpi_comm, &req);
    MPI_Wait(&req, &st);
}

void jml_RecvLocal_int2d(int comp, int* data, int is, int ie, int js, int je, int source) {
    int n = (ie-is+1)*(je-js+1);
    MPI_Request req; MPI_Status st;
    MPI_Irecv(data, n, MPI_INT, source, MPI_MY_TAG, s_local[comp-1].mpi_comm, &req);
    MPI_Wait(&req, &st);
}

void jml_SendLocal_int3d(int comp, const int* data, int is, int ie, int js, int je, int ks, int ke, int dest) {
    int n = (ie-is+1)*(je-js+1)*(ke-ks+1);
    MPI_Request req; MPI_Status st;
    MPI_Isend((void*)data, n, MPI_INT, dest, MPI_MY_TAG, s_local[comp-1].mpi_comm, &req);
    MPI_Wait(&req, &st);
}

void jml_RecvLocal_int3d(int comp, int* data, int is, int ie, int js, int je, int ks, int ke, int source) {
    int n = (ie-is+1)*(je-js+1)*(ke-ks+1);
    MPI_Request req; MPI_Status st;
    MPI_Irecv(data, n, MPI_INT, source, MPI_MY_TAG, s_local[comp-1].mpi_comm, &req);
    MPI_Wait(&req, &st);
}

void jml_SendLocal_float2d(int comp, const float* data, int is, int ie, int js, int je, int dest) {
    int n = (ie-is+1)*(je-js+1);
    MPI_Request req; MPI_Status st;
    MPI_Isend((void*)data, n, MPI_FLOAT, dest, MPI_MY_TAG, s_local[comp-1].mpi_comm, &req);
    MPI_Wait(&req, &st);
}

void jml_RecvLocal_float2d(int comp, float* data, int is, int ie, int js, int je, int source) {
    int n = (ie-is+1)*(je-js+1);
    MPI_Request req; MPI_Status st;
    MPI_Irecv(data, n, MPI_FLOAT, source, MPI_MY_TAG, s_local[comp-1].mpi_comm, &req);
    MPI_Wait(&req, &st);
}

void jml_SendLocal_float3d(int comp, const float* data, int is, int ie, int js, int je, int ks, int ke, int dest) {
    int n = (ie-is+1)*(je-js+1)*(ke-ks+1);
    MPI_Request req; MPI_Status st;
    MPI_Isend((void*)data, n, MPI_FLOAT, dest, MPI_MY_TAG, s_local[comp-1].mpi_comm, &req);
    MPI_Wait(&req, &st);
}

void jml_RecvLocal_float3d(int comp, float* data, int is, int ie, int js, int je, int ks, int ke, int source) {
    int n = (ie-is+1)*(je-js+1)*(ke-ks+1);
    MPI_Request req; MPI_Status st;
    MPI_Irecv(data, n, MPI_FLOAT, source, MPI_MY_TAG, s_local[comp-1].mpi_comm, &req);
    MPI_Wait(&req, &st);
}

void jml_SendLocal_double1d(int comp, const double* data, int is, int ie, int dest) {
    int n = ie - is + 1;
    double* buf = (double*)malloc(n * sizeof(double));
    MPI_Request req; MPI_Status st;
    memcpy(buf, &data[is-1], n * sizeof(double));
    MPI_Isend(buf, n, MPI_DOUBLE, dest, MPI_MY_TAG, s_local[comp-1].mpi_comm, &req);
    MPI_Wait(&req, &st);
    free(buf);
}

void jml_RecvLocal_double1d(int comp, double* data, int is, int ie, int source) {
    int n = ie - is + 1;
    double* buf = (double*)malloc(n * sizeof(double));
    MPI_Request req; MPI_Status st;
    MPI_Irecv(buf, n, MPI_DOUBLE, source, MPI_MY_TAG, s_local[comp-1].mpi_comm, &req);
    MPI_Wait(&req, &st);
    memcpy(&data[is-1], buf, n * sizeof(double));
    free(buf);
}

void jml_SendLocal_double2d(int comp, const double* data, int is, int ie, int js, int je, int dest) {
    int n = (ie-is+1)*(je-js+1);
    MPI_Request req; MPI_Status st;
    MPI_Isend((void*)data, n, MPI_DOUBLE, dest, MPI_MY_TAG, s_local[comp-1].mpi_comm, &req);
    MPI_Wait(&req, &st);
}

void jml_RecvLocal_double2d(int comp, double* data, int is, int ie, int js, int je, int source) {
    int n = (ie-is+1)*(je-js+1);
    MPI_Request req; MPI_Status st;
    MPI_Irecv(data, n, MPI_DOUBLE, source, MPI_MY_TAG, s_local[comp-1].mpi_comm, &req);
    MPI_Wait(&req, &st);
}

void jml_SendLocal_double3d(int comp, const double* data, int is, int ie, int js, int je, int ks, int ke, int dest) {
    int n = (ie-is+1)*(je-js+1)*(ke-ks+1);
    MPI_Request req; MPI_Status st;
    MPI_Isend((void*)data, n, MPI_DOUBLE, dest, MPI_MY_TAG, s_local[comp-1].mpi_comm, &req);
    MPI_Wait(&req, &st);
}

void jml_RecvLocal_double3d(int comp, double* data, int is, int ie, int js, int je, int ks, int ke, int source) {
    int n = (ie-is+1)*(je-js+1)*(ke-ks+1);
    MPI_Request req; MPI_Status st;
    MPI_Irecv(data, n, MPI_DOUBLE, source, MPI_MY_TAG, s_local[comp-1].mpi_comm, &req);
    MPI_Wait(&req, &st);
}

/* --- SendLeader / RecvLeader --- */
void jml_SendLeader_str(const char* data, int len, int dest) {
    int dest_rank;
    MPI_Request req; MPI_Status st;
    if (!jml_isLeader()) return;
    dest_rank = s_leader_pe[dest-1];
    if (dest_rank == s_leader.my_rank) {
        MPI_Bsend((void*)data, len, MPI_CHAR, dest_rank, MPI_MY_TAG, s_leader.mpi_comm);
    } else {
        MPI_Isend((void*)data, len, MPI_CHAR, dest_rank, MPI_MY_TAG, s_leader.mpi_comm, &req);
        MPI_Wait(&req, &st);
    }
}

void jml_RecvLeader_str(char* data, int len, int source) {
    int source_rank;
    MPI_Request req; MPI_Status st;
    if (!jml_isLeader()) return;
    source_rank = s_leader_pe[source-1];
    MPI_Irecv(data, len, MPI_CHAR, source_rank, MPI_MY_TAG, s_leader.mpi_comm, &req);
    MPI_Wait(&req, &st);
}

void jml_SendLeader_int1d(const int* data, int is, int ie, int dest) {
    int dest_rank, data_size;
    int* buf;
    MPI_Request req; MPI_Status st;
    if (!jml_isLeader()) return;
    data_size = ie - is + 1;
    buf = (int*)malloc(data_size * sizeof(int));
    memcpy(buf, &data[is-1], data_size * sizeof(int));
    dest_rank = s_leader_pe[dest-1];
    if (dest_rank == s_leader.my_rank) {
        MPI_Bsend(buf, data_size, MPI_INT, dest_rank, MPI_MY_TAG, s_leader.mpi_comm);
    } else {
        MPI_Isend(buf, data_size, MPI_INT, dest_rank, MPI_MY_TAG, s_leader.mpi_comm, &req);
        MPI_Wait(&req, &st);
    }
    free(buf);
}

void jml_RecvLeader_int1d(int* data, int is, int ie, int source) {
    int source_rank;
    int* buf;
    int data_size = ie - is + 1;
    MPI_Request req; MPI_Status st;
    if (!jml_isLeader()) return;
    source_rank = s_leader_pe[source-1];
    buf = (int*)malloc(data_size * sizeof(int));
    MPI_Irecv(buf, data_size, MPI_INT, source_rank, MPI_MY_TAG, s_leader.mpi_comm, &req);
    MPI_Wait(&req, &st);
    memcpy(&data[is-1], buf, data_size * sizeof(int));
    free(buf);
}

void jml_SendLeader_float1d(const float* data, int is, int ie, int dest, int tag) {
    int dest_rank, data_size;
    float* buf;
    MPI_Request req; MPI_Status st;
    if (!jml_isLeader()) return;
    data_size = ie - is + 1;
    buf = (float*)malloc(data_size * sizeof(float));
    memcpy(buf, &data[is-1], data_size * sizeof(float));
    dest_rank = s_leader_pe[dest-1];
    if (dest_rank == s_leader.my_rank) {
        MPI_Bsend(buf, data_size, MPI_FLOAT, dest_rank, tag, s_leader.mpi_comm);
    } else {
        MPI_Isend(buf, data_size, MPI_FLOAT, dest_rank, tag, s_leader.mpi_comm, &req);
        MPI_Wait(&req, &st);
    }
    free(buf);
}

void jml_RecvLeader_float1d(float* data, int is, int ie, int source, int tag) {
    int source_rank;
    int data_size = ie - is + 1;
    MPI_Request req; MPI_Status st;
    if (!jml_isLeader()) return;
    source_rank = s_leader_pe[source-1];
    MPI_Irecv(&data[is-1], data_size, MPI_FLOAT, source_rank, tag, s_leader.mpi_comm, &req);
    MPI_Wait(&req, &st);
}

void jml_SendLeader_double1d(const double* data, int is, int ie, int dest, int tag) {
    int dest_rank, data_size;
    double* buf;
    MPI_Request req; MPI_Status st;
    if (!jml_isLeader()) return;
    data_size = ie - is + 1;
    buf = (double*)malloc(data_size * sizeof(double));
    memcpy(buf, &data[is-1], data_size * sizeof(double));
    dest_rank = s_leader_pe[dest-1];
    if (dest_rank == s_leader.my_rank) {
        MPI_Bsend(buf, data_size, MPI_DOUBLE, dest_rank, tag, s_leader.mpi_comm);
    } else {
        MPI_Isend(buf, data_size, MPI_DOUBLE, dest_rank, tag, s_leader.mpi_comm, &req);
        MPI_Wait(&req, &st);
    }
    free(buf);
}

void jml_RecvLeader_double1d(double* data, int is, int ie, int source, int tag) {
    int source_rank;
    int data_size = ie - is + 1;
    MPI_Request req; MPI_Status st;
    if (!jml_isLeader()) return;
    source_rank = s_leader_pe[source-1];
    MPI_Irecv(&data[is-1], data_size, MPI_DOUBLE, source_rank, tag, s_leader.mpi_comm, &req);
    MPI_Wait(&req, &st);
}

/* 2D / 3D leader variants (delegating to 1D with contiguous data) */
void jml_SendLeader_int2d(const int* data, int is, int ie, int js, int je, int dest) {
    jml_SendLeader_int1d(data, 1, (ie-is+1)*(je-js+1), dest);
}
void jml_RecvLeader_int2d(int* data, int is, int ie, int js, int je, int source) {
    jml_RecvLeader_int1d(data, 1, (ie-is+1)*(je-js+1), source);
}
void jml_SendLeader_int3d(const int* data, int is, int ie, int js, int je, int ks, int ke, int dest) {
    jml_SendLeader_int1d(data, 1, (ie-is+1)*(je-js+1)*(ke-ks+1), dest);
}
void jml_RecvLeader_int3d(int* data, int is, int ie, int js, int je, int ks, int ke, int source) {
    jml_RecvLeader_int1d(data, 1, (ie-is+1)*(je-js+1)*(ke-ks+1), source);
}
void jml_SendLeader_float2d(const float* data, int is, int ie, int js, int je, int dest) {
    jml_SendLeader_float1d(data, 1, (ie-is+1)*(je-js+1), dest, MPI_MY_TAG);
}
void jml_RecvLeader_float2d(float* data, int is, int ie, int js, int je, int source) {
    jml_RecvLeader_float1d(data, 1, (ie-is+1)*(je-js+1), source, MPI_MY_TAG);
}
void jml_SendLeader_float3d(const float* data, int is, int ie, int js, int je, int ks, int ke, int dest) {
    jml_SendLeader_float1d(data, 1, (ie-is+1)*(je-js+1)*(ke-ks+1), dest, MPI_MY_TAG);
}
void jml_RecvLeader_float3d(float* data, int is, int ie, int js, int je, int ks, int ke, int source) {
    jml_RecvLeader_float1d(data, 1, (ie-is+1)*(je-js+1)*(ke-ks+1), source, MPI_MY_TAG);
}
void jml_SendLeader_double2d(const double* data, int is, int ie, int js, int je, int dest) {
    jml_SendLeader_double1d(data, 1, (ie-is+1)*(je-js+1), dest, MPI_MY_TAG);
}
void jml_RecvLeader_double2d(double* data, int is, int ie, int js, int je, int source) {
    jml_RecvLeader_double1d(data, 1, (ie-is+1)*(je-js+1), source, MPI_MY_TAG);
}
void jml_SendLeader_double3d(const double* data, int is, int ie, int js, int je, int ks, int ke, int dest) {
    jml_SendLeader_double1d(data, 1, (ie-is+1)*(je-js+1)*(ke-ks+1), dest, MPI_MY_TAG);
}
void jml_RecvLeader_double3d(double* data, int is, int ie, int js, int je, int ks, int ke, int source) {
    jml_RecvLeader_double1d(data, 1, (ie-is+1)*(je-js+1)*(ke-ks+1), source, MPI_MY_TAG);
}

/* --- SendModel / RecvModel --- */
static MPI_Comm get_model_comm(int comp, int target_model) {
    return s_local[comp-1].inter_comm[target_model-1].mpi_comm;
}

static int get_dest_rank_model(int comp, int target_model, int dest_pe) {
    return dest_pe + s_local[comp-1].inter_comm[target_model-1].pe_offset;
}

static int get_source_rank_model(int comp, int source_model, int source_pe) {
    return source_pe + s_local[comp-1].inter_comm[source_model-1].pe_offset;
}

void jml_SendModel_int1d(int comp, int target_comp, const int* data, int n, int dest, int tag) {
    int dest_rank = get_dest_rank_model(comp, target_comp, dest);
    int my_rank   = s_local[comp-1].inter_comm[target_comp-1].my_rank;
    MPI_Comm comm = get_model_comm(comp, target_comp);
    int* buf = (int*)malloc(n * sizeof(int));
    MPI_Request req; MPI_Status st;
    memcpy(buf, data, n * sizeof(int));
    if (dest_rank == my_rank) {
        MPI_Bsend(buf, n, MPI_INT, dest_rank, tag, comm);
    } else {
        MPI_Isend(buf, n, MPI_INT, dest_rank, tag, comm, &req);
        MPI_Wait(&req, &st);
    }
    free(buf);
}

void jml_RecvModel_int1d(int comp, int target_comp, int* data, int n, int source, int tag) {
    int source_rank = get_source_rank_model(comp, target_comp, source);
    MPI_Comm comm   = get_model_comm(comp, target_comp);
    int* buf = (int*)malloc(n * sizeof(int));
    MPI_Request req; MPI_Status st;
    MPI_Irecv(buf, n, MPI_INT, source_rank, tag, comm, &req);
    MPI_Wait(&req, &st);
    memcpy(data, buf, n * sizeof(int));
    free(buf);
}

void jml_SendModel_double1d(int comp, int target_comp, const double* data, int n, int dest, int tag) {
    int dest_rank = get_dest_rank_model(comp, target_comp, dest);
    int my_rank   = s_local[comp-1].inter_comm[target_comp-1].my_rank;
    MPI_Comm comm = get_model_comm(comp, target_comp);
    double* buf = (double*)malloc(n * sizeof(double));
    MPI_Request req; MPI_Status st;
    memcpy(buf, data, n * sizeof(double));
    if (dest_rank == my_rank) {
        MPI_Bsend(buf, n, MPI_DOUBLE, dest_rank, tag, comm);
    } else {
        MPI_Isend(buf, n, MPI_DOUBLE, dest_rank, tag, comm, &req);
        MPI_Wait(&req, &st);
    }
    free(buf);
}

void jml_RecvModel_double1d(int comp, int target_comp, double* data, int n, int source, int tag) {
    int source_rank = get_source_rank_model(comp, target_comp, source);
    MPI_Comm comm   = get_model_comm(comp, target_comp);
    double* buf = (double*)malloc(n * sizeof(double));
    MPI_Request req; MPI_Status st;
    MPI_Irecv(buf, n, MPI_DOUBLE, source_rank, tag, comm, &req);
    MPI_Wait(&req, &st);
    memcpy(data, buf, n * sizeof(double));
    free(buf);
}

void jml_SendModel_float2d(int comp, int target_comp, const float* data, int n1, int n2, int dest, int tag) {
    int dest_rank = get_dest_rank_model(comp, target_comp, dest);
    int my_rank   = s_local[comp-1].inter_comm[target_comp-1].my_rank;
    MPI_Comm comm = get_model_comm(comp, target_comp);
    int n = n1*n2;
    MPI_Request req; MPI_Status st;
    if (dest_rank == my_rank) {
        MPI_Bsend((void*)data, n, MPI_FLOAT, dest_rank, tag, comm);
    } else {
        MPI_Isend((void*)data, n, MPI_FLOAT, dest_rank, tag, comm, &req);
        MPI_Wait(&req, &st);
    }
}
void jml_RecvModel_float2d(int comp, int target_comp, float* data, int n1, int n2, int source, int tag) {
    int source_rank = get_source_rank_model(comp, target_comp, source);
    MPI_Comm comm   = get_model_comm(comp, target_comp);
    int n = n1*n2;
    MPI_Request req; MPI_Status st;
    MPI_Irecv(data, n, MPI_FLOAT, source_rank, tag, comm, &req);
    MPI_Wait(&req, &st);
}
void jml_SendModel_double2d(int comp, int target_comp, const double* data, int n1, int n2, int dest, int tag) {
    jml_SendModel_double1d(comp, target_comp, data, n1*n2, dest, tag);
}
void jml_RecvModel_double2d(int comp, int target_comp, double* data, int n1, int n2, int source, int tag) {
    jml_RecvModel_double1d(comp, target_comp, data, n1*n2, source, tag);
}
void jml_SendModel_float3d(int comp, int target_comp, const float* data, int n1, int n2, int n3, int dest, int tag) {
    int dest_rank = get_dest_rank_model(comp, target_comp, dest);
    int my_rank   = s_local[comp-1].inter_comm[target_comp-1].my_rank;
    MPI_Comm comm = get_model_comm(comp, target_comp);
    int n = n1*n2*n3;
    MPI_Request req; MPI_Status st;
    if (dest_rank == my_rank) {
        MPI_Bsend((void*)data, n, MPI_FLOAT, dest_rank, tag, comm);
    } else {
        MPI_Isend((void*)data, n, MPI_FLOAT, dest_rank, tag, comm, &req);
        MPI_Wait(&req, &st);
    }
}
void jml_RecvModel_float3d(int comp, int target_comp, float* data, int n1, int n2, int n3, int source, int tag) {
    int source_rank = get_source_rank_model(comp, target_comp, source);
    MPI_Comm comm   = get_model_comm(comp, target_comp);
    int n = n1*n2*n3;
    MPI_Request req; MPI_Status st;
    MPI_Irecv(data, n, MPI_FLOAT, source_rank, tag, comm, &req);
    MPI_Wait(&req, &st);
}
void jml_SendModel_double3d(int comp, int target_comp, const double* data, int n1, int n2, int n3, int dest, int tag) {
    jml_SendModel_double1d(comp, target_comp, data, n1*n2*n3, dest, tag);
}
void jml_RecvModel_double3d(int comp, int target_comp, double* data, int n1, int n2, int n3, int source, int tag) {
    jml_RecvModel_double1d(comp, target_comp, data, n1*n2*n3, source, tag);
}

/* --- Non-blocking request management --- */
void jml_set_num_of_isend(int n) {
    s_isend_counter = 0;
    if (n <= 0) {
        if (!s_isend_request) {
            s_isend_capacity = 1;
            s_isend_request  = (MPI_Request*)malloc(sizeof(MPI_Request));
        }
        return;
    }
    if (s_isend_request && s_isend_capacity >= n) return;
    if (s_isend_request) free(s_isend_request);
    s_isend_capacity = n;
    s_isend_request  = (MPI_Request*)malloc(n * sizeof(MPI_Request));
}

void jml_set_num_of_irecv(int n) {
    s_irecv_counter = 0;
    if (n <= 0) {
        if (!s_irecv_request) {
            s_irecv_capacity = 1;
            s_irecv_request  = (MPI_Request*)malloc(sizeof(MPI_Request));
        }
        return;
    }
    if (s_irecv_request && s_irecv_capacity >= n) return;
    if (s_irecv_request) free(s_irecv_request);
    s_irecv_capacity = n;
    s_irecv_request  = (MPI_Request*)malloc(n * sizeof(MPI_Request));
}

/* ISend/IRecv Local */
void jml_ISendLocal_double1d(int comp, const double* data, int is, int ie, int dest, int tag) {
    if (s_isend_counter >= s_isend_capacity) {
        fprintf(stderr, "jml_ISendLocal_double1d: isend buffer overflow\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    MPI_Isend((void*)&data[is-1], ie-is+1, MPI_DOUBLE, dest, tag,
              s_local[comp-1].mpi_comm, &s_isend_request[s_isend_counter++]);
}

void jml_IRecvLocal_double1d(int comp, double* data, int is, int ie, int source, int tag) {
    if (s_irecv_counter >= s_irecv_capacity) {
        fprintf(stderr, "jml_IRecvLocal_double1d: irecv buffer overflow\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    MPI_Irecv(&data[is-1], ie-is+1, MPI_DOUBLE, source, tag,
              s_local[comp-1].mpi_comm, &s_irecv_request[s_irecv_counter++]);
}

/* ISendModel / IRecvModel */
static void isend_model_double(int comp, int target_comp, const double* data, int n, int dest, int tag) {
    int dest_rank = get_dest_rank_model(comp, target_comp, dest);
    int my_rank   = jml_GetMyrankModel(comp, target_comp);
    MPI_Comm comm = get_model_comm(comp, target_comp);
    if (dest_rank == my_rank) {
        MPI_Bsend((void*)data, n, MPI_DOUBLE, dest_rank, tag, comm);
    } else {
        if (s_isend_counter >= s_isend_capacity) {
            fprintf(stderr, "isend_model_double: overflow\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        MPI_Isend((void*)data, n, MPI_DOUBLE, dest_rank, tag, comm,
                  &s_isend_request[s_isend_counter++]);
    }
}

static void irecv_model_double(int comp, int target_comp, double* data, int n, int source, int tag) {
    int source_rank = get_source_rank_model(comp, target_comp, source);
    MPI_Comm comm   = get_model_comm(comp, target_comp);
    if (s_irecv_counter >= s_irecv_capacity) {
        fprintf(stderr, "irecv_model_double: overflow\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    MPI_Irecv(data, n, MPI_DOUBLE, source_rank, tag, comm,
              &s_irecv_request[s_irecv_counter++]);
}

void jml_ISendModel_double1d(int comp, int tc, const double* d, int n, int dest, int tag) { isend_model_double(comp,tc,d,n,dest,tag); }
void jml_ISendModel_double2d(int comp, int tc, const double* d, int n1, int n2, int dest, int tag) { isend_model_double(comp,tc,d,n1*n2,dest,tag); }
void jml_ISendModel_int1d(int comp, int tc, const int* d, int n, int dest, int tag) {
    int dest_rank = get_dest_rank_model(comp,tc,dest);
    int my_rank   = jml_GetMyrankModel(comp,tc);
    MPI_Comm comm = get_model_comm(comp,tc);
    if (dest_rank == my_rank) {
        MPI_Bsend((void*)d,n,MPI_INT,dest_rank,tag,comm);
    } else {
        MPI_Isend((void*)d,n,MPI_INT,dest_rank,tag,comm,&s_isend_request[s_isend_counter++]);
    }
}

void jml_IRecvModel_double1d(int comp, int tc, double* d, int n, int source, int tag) { irecv_model_double(comp,tc,d,n,source,tag); }
void jml_IRecvModel_double2d(int comp, int tc, double* d, int n1, int n2, int source, int tag) { irecv_model_double(comp,tc,d,n1*n2,source,tag); }
void jml_IRecvModel_int1d(int comp, int tc, int* d, int n, int source, int tag) {
    int source_rank = get_source_rank_model(comp,tc,source);
    MPI_Comm comm   = get_model_comm(comp,tc);
    MPI_Irecv(d,n,MPI_INT,source_rank,tag,comm,&s_irecv_request[s_irecv_counter++]);
}

/* Model2 variants: scalar pointer interface (same logic) */
void jml_ISendModel2_double1d(int comp, int tc, const double* d, int n, int dest, int tag) { isend_model_double(comp,tc,d,n,dest,tag); }
void jml_ISendModel2_double2d(int comp, int tc, const double* d, int n1, int n2, int dest, int tag) { isend_model_double(comp,tc,d,n1*n2,dest,tag); }
void jml_ISendModel2_int1d(int comp, int tc, const int* d, int n, int dest, int tag) { jml_ISendModel_int1d(comp,tc,d,n,dest,tag); }
void jml_IRecvModel2_double1d(int comp, int tc, double* d, int n, int source, int tag) { irecv_model_double(comp,tc,d,n,source,tag); }
void jml_IRecvModel2_double2d(int comp, int tc, double* d, int n1, int n2, int source, int tag) { irecv_model_double(comp,tc,d,n1*n2,source,tag); }
void jml_IRecvModel2_int1d(int comp, int tc, int* d, int n, int source, int tag) { jml_IRecvModel_int1d(comp,tc,d,n,source,tag); }

void jml_ISendModel3_double1d(int comp, int tc, const double* d, int n, int dest, int tag) { isend_model_double(comp,tc,d,n,dest,tag); }
void jml_ISendModel3_double2d(int comp, int tc, const double* d, int n1, int n2, int dest, int tag) { isend_model_double(comp,tc,d,n1*n2,dest,tag); }
void jml_IRecvModel3_double1d(int comp, int tc, double* d, int n, int source, int tag) { irecv_model_double(comp,tc,d,n,source,tag); }
void jml_IRecvModel3_double2d(int comp, int tc, double* d, int n1, int n2, int source, int tag) { irecv_model_double(comp,tc,d,n1*n2,source,tag); }

void jml_send_waitall(void) {
    MPI_Status* statuses = (MPI_Status*)malloc(s_isend_counter * sizeof(MPI_Status));
    if (s_isend_counter > 0)
        MPI_Waitall(s_isend_counter, s_isend_request, statuses);
    s_isend_counter = 0;
    free(statuses);
}

void jml_recv_waitall(void) {
    MPI_Status* statuses = (MPI_Status*)malloc(s_irecv_counter * sizeof(MPI_Status));
    if (s_irecv_counter > 0)
        MPI_Waitall(s_irecv_counter, s_irecv_request, statuses);
    s_irecv_counter = 0;
    free(statuses);
}

void jml_RecvAll(int source) {
    MPI_Status status;
    int flag;
    MPI_Iprobe(source, MPI_ANY_TAG, s_global.mpi_comm, &flag, &status);
    if (flag) {
        int count;
        MPI_Get_count(&status, MPI_BYTE, &count);
        char* buf = (char*)malloc(count);
        MPI_Recv(buf, count, MPI_BYTE, source, MPI_ANY_TAG, s_global.mpi_comm, &status);
        free(buf);
    }
}
