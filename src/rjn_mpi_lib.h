#pragma once
/*
 * rjn_mpi_lib.h
 * Converted from rjn_mpi_lib.f90
 * Copyright (c) 2026, arakawa@climtech.jp
 */

#include <mpi.h>
#include <stdint.h>
#include "rjn_constant.h"

/* Public variables */
extern int JML_ROOTS_TAG;   /* = 120 */
extern int JML_ANY_SOURCE;  /* = MPI_ANY_SOURCE */

/* Global communicator management */
void jml_set_global_comm(int global_comm);
int  jml_get_global_comm(void);

/* Init / Finalize */
void jml_init(void);
void jml_create_communicator(const int* my_comp_id, int n);
void jml_finalize(int is_call_finalize);
void jml_abort(void);

/* Query functions */
int  jml_GetCommGlobal(void);
int  jml_GetMyrankGlobal(void);
int  jml_GetCommSizeGlobal(void);
int  jml_GetCommLeader(void);
int  jml_GetComm(int component_id);
int  jml_GetCommNULL(void);
int  jml_isRoot(void);        /* returns 1(true)/0(false) */
int  jml_GetMyGroup(int component_id);
int  jml_GetMyrank(int component_id);
int  jml_GetCommSizeLocal(int component_id);
int  jml_GetModelRankOffset(int my_comp_id, int target_comp_id);
int  jml_GetMyrankModel(int my_comp_id, int target_comp_id);
int  jml_GetLeaderRank(int component_id);
int  jml_isLocalLeader(int component_id); /* returns 1/0 */

/* Barrier / Probe */
void jml_BarrierLeader(void);
int  jml_ProbeLeader(int source, int tag);  /* returns 1/0 */
int  jml_ProbeAll(int source);              /* returns 1/0 */

/* Global broadcast / send / recv (overloaded via separate names) */
void jml_BcastGlobal_str(char* str, int len, int source_rank);
void jml_BcastGlobal_strarr(char* str_arr, int num, int str_len, int source_rank);
void jml_BcastGlobal_int(int* data, int is, int ie, int source_rank);

void jml_SendGlobal_str(const char* str, int len, int dest);
void jml_SendGlobal_strarr(const char* str_arr, int num, int str_len, int dest);
void jml_SendGlobal_int(const int* data, int is, int ie, int dest);

void jml_RecvGlobal_str(char* str, int len, int source);
void jml_RecvGlobal_strarr(char* str_arr, int num, int str_len, int source);
void jml_RecvGlobal_int(int* data, int is, int ie, int source);

/* AllReduce */
void jml_AllreduceMax(int d, int* res);
void jml_AllreduceMin(int d, int* res);
void jml_AllReduceSum_int(int d, int* res);
void jml_AllReduceSum_intar(int nd, const int* d, int* res);
void jml_AllReduceMaxLocal(int comp, int d, int* res);
void jml_AllReduceMinLocal(int comp, int d, int* res);
void jml_AllReduceSumLocal(int comp, int d, int* res);

/* Reduce local */
void jml_ReduceSumLocal_int(int comp, int d, int* sum);
void jml_ReduceSumLocal_double(int comp, double d, double* sum);
void jml_ReduceMeanLocal(int comp, double d, double* mean);
void jml_ReduceMinLocal(int comp, int d, int* res);
void jml_ReduceMaxLocal(int comp, int d, int* res);

/* Local broadcast (overloaded by type) */
void jml_BcastLocal_str(int comp, char* str, int len, int source);
void jml_BcastLocal_int(int comp, int* data, int is, int ie, int source);
void jml_BcastLocal_int_scalar(int comp, int* data, int source);
void jml_BcastLocal_long(int comp, int64_t* data, int is, int ie, int source);
void jml_BcastLocal_float(int comp, float* data, int is, int ie, int source);
void jml_BcastLocal_double(int comp, double* data, int is, int ie, int source);

/* Gather / Scatter Local */
void jml_GatherLocal_int(int comp, const int* data, int is, int ie, int* recv_data);
void jml_GatherLocal_float(int comp, const float* data, int is, int ie, float* recv_data);
void jml_AllGatherLocal_int(int comp, const int* data, int is, int ie, int* recv_data);
void jml_ScatterLocal_int(int comp, const int* send_data, int num_of_data, int* recv_data);
void jml_GatherVLocal_int(int comp, const int* send_data, int num_of_my_data,
                          int* recv_data, const int* num_of_data, const int* offset);
void jml_GatherVLocal_double(int comp, const double* send_data, int num_of_my_data,
                             double* recv_data, const int* num_of_data, const int* offset);
void jml_ScatterVLocal_int(int comp, const int* send_data, const int* num_of_data,
                           const int* offset, int* recv_data, int num_of_my_data);
void jml_ScatterVLocal_double(int comp, const double* send_data, const int* num_of_data,
                              const int* offset, double* recv_data, int num_of_my_data);

/* SendLocal / RecvLocal (overloaded by type and dimension) */
void jml_SendLocal_int1d(int comp, const int* data, int is, int ie, int dest);
void jml_SendLocal_int2d(int comp, const int* data, int is, int ie, int js, int je, int dest);
void jml_SendLocal_int3d(int comp, const int* data, int is, int ie, int js, int je, int ks, int ke, int dest);
void jml_SendLocal_float2d(int comp, const float* data, int is, int ie, int js, int je, int dest);
void jml_SendLocal_float3d(int comp, const float* data, int is, int ie, int js, int je, int ks, int ke, int dest);
void jml_SendLocal_double1d(int comp, const double* data, int is, int ie, int dest);
void jml_SendLocal_double2d(int comp, const double* data, int is, int ie, int js, int je, int dest);
void jml_SendLocal_double3d(int comp, const double* data, int is, int ie, int js, int je, int ks, int ke, int dest);

void jml_RecvLocal_int1d(int comp, int* data, int is, int ie, int source);
void jml_RecvLocal_int2d(int comp, int* data, int is, int ie, int js, int je, int source);
void jml_RecvLocal_int3d(int comp, int* data, int is, int ie, int js, int je, int ks, int ke, int source);
void jml_RecvLocal_float2d(int comp, float* data, int is, int ie, int js, int je, int source);
void jml_RecvLocal_float3d(int comp, float* data, int is, int ie, int js, int je, int ks, int ke, int source);
void jml_RecvLocal_double1d(int comp, double* data, int is, int ie, int source);
void jml_RecvLocal_double2d(int comp, double* data, int is, int ie, int js, int je, int source);
void jml_RecvLocal_double3d(int comp, double* data, int is, int ie, int js, int je, int ks, int ke, int source);

/* SendLeader / RecvLeader */
void jml_SendLeader_str(const char* data, int len, int dest);
void jml_SendLeader_int1d(const int* data, int is, int ie, int dest);
void jml_SendLeader_int2d(const int* data, int is, int ie, int js, int je, int dest);
void jml_SendLeader_int3d(const int* data, int is, int ie, int js, int je, int ks, int ke, int dest);
void jml_SendLeader_float1d(const float* data, int is, int ie, int dest, int tag);
void jml_SendLeader_float2d(const float* data, int is, int ie, int js, int je, int dest);
void jml_SendLeader_float3d(const float* data, int is, int ie, int js, int je, int ks, int ke, int dest);
void jml_SendLeader_double1d(const double* data, int is, int ie, int dest, int tag);
void jml_SendLeader_double2d(const double* data, int is, int ie, int js, int je, int dest);
void jml_SendLeader_double3d(const double* data, int is, int ie, int js, int je, int ks, int ke, int dest);

void jml_RecvLeader_str(char* data, int len, int source);
void jml_RecvLeader_int1d(int* data, int is, int ie, int source);
void jml_RecvLeader_int2d(int* data, int is, int ie, int js, int je, int source);
void jml_RecvLeader_int3d(int* data, int is, int ie, int js, int je, int ks, int ke, int source);
void jml_RecvLeader_float1d(float* data, int is, int ie, int source, int tag);
void jml_RecvLeader_float2d(float* data, int is, int ie, int js, int je, int source);
void jml_RecvLeader_float3d(float* data, int is, int ie, int js, int je, int ks, int ke, int source);
void jml_RecvLeader_double1d(double* data, int is, int ie, int source, int tag);
void jml_RecvLeader_double2d(double* data, int is, int ie, int js, int je, int source);
void jml_RecvLeader_double3d(double* data, int is, int ie, int js, int je, int ks, int ke, int source);

/* SendModel / RecvModel */
void jml_SendModel_int1d(int my_comp, int target_comp, const int* data, int n, int dest, int tag);
void jml_SendModel_double1d(int my_comp, int target_comp, const double* data, int n, int dest, int tag);
void jml_SendModel_float2d(int my_comp, int target_comp, const float* data, int n1, int n2, int dest, int tag);
void jml_SendModel_double2d(int my_comp, int target_comp, const double* data, int n1, int n2, int dest, int tag);
void jml_SendModel_float3d(int my_comp, int target_comp, const float* data, int n1, int n2, int n3, int dest, int tag);
void jml_SendModel_double3d(int my_comp, int target_comp, const double* data, int n1, int n2, int n3, int dest, int tag);

void jml_RecvModel_int1d(int my_comp, int target_comp, int* data, int n, int source, int tag);
void jml_RecvModel_double1d(int my_comp, int target_comp, double* data, int n, int source, int tag);
void jml_RecvModel_float2d(int my_comp, int target_comp, float* data, int n1, int n2, int source, int tag);
void jml_RecvModel_double2d(int my_comp, int target_comp, double* data, int n1, int n2, int source, int tag);
void jml_RecvModel_float3d(int my_comp, int target_comp, float* data, int n1, int n2, int n3, int source, int tag);
void jml_RecvModel_double3d(int my_comp, int target_comp, double* data, int n1, int n2, int n3, int source, int tag);

/* Non-blocking (Isend/Irecv) */
void jml_set_num_of_isend(int n);
void jml_set_num_of_irecv(int n);

void jml_ISendLocal_double1d(int comp, const double* data, int is, int ie, int dest, int tag);
void jml_IRecvLocal_double1d(int comp, double* data, int is, int ie, int source, int tag);

void jml_ISendModel_double1d(int my_comp, int target_comp, const double* data, int n, int dest, int tag);
void jml_ISendModel_double2d(int my_comp, int target_comp, const double* data, int n1, int n2, int dest, int tag);
void jml_ISendModel_int1d(int my_comp, int target_comp, const int* data, int n, int dest, int tag);

void jml_IRecvModel_double1d(int my_comp, int target_comp, double* data, int n, int source, int tag);
void jml_IRecvModel_double2d(int my_comp, int target_comp, double* data, int n1, int n2, int source, int tag);
void jml_IRecvModel_int1d(int my_comp, int target_comp, int* data, int n, int source, int tag);

void jml_ISendModel2_double1d(int my_comp, int target_comp, const double* data, int n, int dest, int tag);
void jml_ISendModel2_double2d(int my_comp, int target_comp, const double* data, int n1, int n2, int dest, int tag);
void jml_ISendModel2_int1d(int my_comp, int target_comp, const int* data, int n, int dest, int tag);

void jml_IRecvModel2_double1d(int my_comp, int target_comp, double* data, int n, int source, int tag);
void jml_IRecvModel2_double2d(int my_comp, int target_comp, double* data, int n1, int n2, int source, int tag);
void jml_IRecvModel2_int1d(int my_comp, int target_comp, int* data, int n, int source, int tag);

void jml_ISendModel3_double1d(int my_comp, int target_comp, const double* data, int n, int dest, int tag);
void jml_ISendModel3_double2d(int my_comp, int target_comp, const double* data, int n1, int n2, int dest, int tag);

void jml_IRecvModel3_double1d(int my_comp, int target_comp, double* data, int n, int source, int tag);
void jml_IRecvModel3_double2d(int my_comp, int target_comp, double* data, int n1, int n2, int source, int tag);

void jml_send_waitall(void);
void jml_recv_waitall(void);

void jml_RecvAll(int source);
