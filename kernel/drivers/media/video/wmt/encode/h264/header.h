
/*!
 *************************************************************************************
 * \file header.h
 *
 * \brief
 *    Prototypes for header.c
 *************************************************************************************
 */

#ifndef _HEADER_H_
#define _HEADER_H_

enum {
  LIST_0 = 0,
  LIST_1 = 1,
  BI_PRED = 2,
  BI_PRED_L0 = 3,
  BI_PRED_L1 = 4
};


extern int SliceHeader        (h264enc_ctx_t *ctx);
//extern int Partition_BC_Header(ve_slice_parameter_t *currSlice, int PartNo);
//extern int writeERPS          (ve_slice_parameter_t *sym, DataPartition *partition);
extern int dec_ref_pic_marking(h264enc_ctx_t *ctx);
extern int  GenerateSeq_parameter_set_rbsp (h264enc_ctx_t *ctx);

extern int  GeneratePic_parameter_set_rbsp (h264enc_ctx_t *ctx);



#endif

