
/*!
 *************************************************************************************
 * \file header.c
 *
 * \brief
 *    H.264 Slice and Sequence headers
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *      - Stephan Wenger                  <stewe@cs.tu-berlin.de>
 *      - Karsten Suehring                <suehring@hhi.de>
 *************************************************************************************
 */

#include "wmt-h264enc.h"
#include "header.h"
#include "vlc.h"
//#include "math.h"

//! definition of H.264 syntax elements
typedef enum
{
  SE_HEADER,
  SE_PTYPE,
  SE_MBTYPE,
  SE_REFFRAME,
  SE_INTRAPREDMODE,
  SE_MVD,
  SE_CBP,
  SE_LUM_DC_INTRA,
  SE_CHR_DC_INTRA,
  SE_LUM_AC_INTRA,
  SE_CHR_AC_INTRA,
  SE_LUM_DC_INTER,
  SE_CHR_DC_INTER,
  SE_LUM_AC_INTER,
  SE_CHR_AC_INTER,
  SE_DELTA_QUANT,
  SE_BFRAME,
  SE_EOS,
  SE_MAX_ELEMENTS = 20 //!< number of maximum syntax elements
} SE_type;


//FREXT_HP = 100, FREXT_CAVLC444 = 44
#define IS_FREXT_PROFILE(profile_idc) ( profile_idc>=100 || profile_idc == 44 )

extern VEU veu;

const int * assignSE2partition[2] ;
const int assignSE2partition_NoDP[SE_MAX_ELEMENTS] =
  {  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
const int assignSE2partition_DP[SE_MAX_ELEMENTS] =
  // 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17
  {  0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 0, 0, 0 } ;


static int ref_pic_list_reordering(struct h264enc_prop *hprop);
static int get_picture_type       (struct h264enc_prop *hprop);

/********************************************************************************************
 * \brief
 *    Write a slice header
 *
 * \return
 *    number of bits used
 ********************************************************************************************
*/
int SliceHeader(h264enc_ctx_t *ctx)
{
	struct h264enc_prop *hprop = &ctx->prop;

  // ve_bitstream_t bitbuff,*bitstream=&bitbuff ;
  int len;
  int toppoc;
  unsigned int pic_order_cnt_lsb;

  len = 0;

	if (hprop->idr_flag)
		toppoc = 0;
	else
		toppoc = (hprop->frame_num) << 1;

  len  = ue_v(0);			//p_Vid->current_mb_nr

  len += ue_v( get_picture_type (hprop) );

  len += ue_v(0);			//active_pps->pic_parameter_set_id

  if( 0 == 1 )				//active_sps->separate_colour_plane_flag = 0
    len += u_v( 2,  0);		//p_Vid->colour_plane_id

  //p_Vid->log2_max_frame_num_minus4 + 4 (p_Vid->log2_max_frame_num_minus4 = 0)
  len += u_v (4, hprop->frame_num);

  if (hprop->idr_flag)
  {
    // idr_pic_id
    len += ue_v ( (hprop->frame_num & 0x01) ); // (p_Vid->number & 0x01) =>  p_Vid->number = framenumber
  }

  pic_order_cnt_lsb = (toppoc & ~((((unsigned int)(-1)) << (4))) );
  len += u_v(4,pic_order_cnt_lsb);

  if(hprop->slice_type == P_SLICE){
  	len +=  u_1(0);
  }

  len += ref_pic_list_reordering(hprop);

  //p_Vid->nal_reference_idc
  len += dec_ref_pic_marking(ctx);

  len += se_v((hprop->QP - 26));

  veu.last_enc = 1;
  veu.slice_above = 0;

  len += ue_v(1);	//disable_deblocking_filter_idc = 1

  return len;
}

/*!
 ********************************************************************************************
 * \brief
 *    writes the ref_pic_list_reordering syntax
 *    based on content of according fields in p_Vid structure
 *
 * \return
 *    number of bits used
 ********************************************************************************************
*/
static int ref_pic_list_reordering(struct h264enc_prop *hprop)
{
  int i, len=0;

  if ( hprop->slice_type != I_SLICE && hprop->slice_type != SI_SLICE )
  {
    len += u_1 (0);			//hprop->ref_pic_list_reordering_flag[LIST_0]
    if (0)					//hprop->ref_pic_list_reordering_flag[LIST_0]
    {
      i=-1;
      do
      {
        i++;
        len += ue_v (0);	//hprop->reordering_of_pic_nums_idc[LIST_0][i]
        if (0)				//hprop->reordering_of_pic_nums_idc[LIST_0][i]==0 || hprop->reordering_of_pic_nums_idc[LIST_0][i]==1
        {
          len += ue_v (0);	//hprop->abs_diff_pic_num_minus1[LIST_0][i]
        }
        else
        {
          if (0)			//hprop->reordering_of_pic_nums_idc[LIST_0][i]==2
          {
            len += ue_v (0);//hprop->long_term_pic_idx[LIST_0][i]
          }
        }

      } while (3 != 3);		//hprop->reordering_of_pic_nums_idc[LIST_0][i]
    }
  }
  return len;
}

/*!
 ************************************************************************
 * \brief
 *    write the memory management control operations
 *
 * \return
 *    number of bits used
 ************************************************************************
 */
int dec_ref_pic_marking(h264enc_ctx_t *ctx)
{
  int len=0;
  //int val;
  //int adaptive_ref_pic_buffering_flag;
  struct h264enc_prop *hprop = &ctx->prop;

  if (hprop->idr_flag)
  {
    len += u_1(0);			//no_output_of_prior_pics_flag
    len += u_1(0);			//long_term_reference_flag
  }
  else
  {

	len += u_1(1);			//adaptive_ref_pic_buffering_flag
	len += ue_v(1);			//val
	len += 1 + ue_v(0);		//tmp_drpm->difference_of_pic_nums_minus1
	len += ue_v(0);			//val

#if 0
	adaptive_ref_pic_buffering_flag = (p_drpm != NULL);

    len += u_1(adaptive_ref_pic_buffering_flag);

    if (adaptive_ref_pic_buffering_flag)
    {
      tmp_drpm = p_drpm;
      // write Memory Management Control Operation
      do
      {
        //if (tmp_drpm==NULL) error ("Error encoding MMCO commands", 500);

        val = tmp_drpm->memory_management_control_operation;
        len += ue_v(val);

        if ((val==1)||(val==3))
        {
          len += 1 + ue_v(tmp_drpm->difference_of_pic_nums_minus1);
        }
        if (val==2)
        {
          len+= ue_v(tmp_drpm->long_term_pic_num);
        }
        if ((val==3)||(val==6))
        {
          len+= ue_v(tmp_drpm->long_term_frame_idx);
        }
        if (val==4)
        {
          len += ue_v(tmp_drpm->max_long_term_frame_idx_plus1);
        }

        tmp_drpm=tmp_drpm->Next;

      } while (val != 0);

    }
#endif
  }
  return len;
}

/*!
 ************************************************************************
 * \brief
 *    Selects picture type and codes it to symbol
 *
 * \return
 *    symbol value for picture type
 ************************************************************************
 */
static int get_picture_type(struct h264enc_prop *hprop)
{
  // set this value to zero for transmission without signaling
  // that the whole picture has the same slice type
  int same_slicetype_for_whole_frame = 5;

#if (MVC_EXTENSION_ENABLE)
  if(1==2)		//currSlice->num_of_views
  {
    same_slicetype_for_whole_frame = 0;
  }
#endif

  switch (hprop->slice_type)
  {
  case I_SLICE:
    return 2 + same_slicetype_for_whole_frame;
    break;
  case P_SLICE:
    return 0 + same_slicetype_for_whole_frame;
    break;
  case B_SLICE:
    return 1 + same_slicetype_for_whole_frame;
    break;
  case SP_SLICE:
    return 3 + same_slicetype_for_whole_frame;
    break;
  case SI_SLICE:
    return 4 + same_slicetype_for_whole_frame;
    break;
  default:
    //error("Picture Type not supported!",1);
    break;
  }

  return 0;
}

/*!
 *************************************************************************************
 * \brief
 *    int GenerateSeq_parameter_set_rbsp (VideoParameters *p_Vid, seq_parameter_set_rbsp_t *sps, char *rbsp);
 *
 * \param p_Vid
 *    Image parameters structure
 * \param sps
 *    sequence parameter structure
 * \param rbsp
 *    buffer to be filled with the rbsp, size should be at least MAXIMUMPARSETRBSPSIZE
 * \return
 *    size of the RBSP in bytes
 *
 * \note
 *    Sequence Parameter VUI function is called, but the function implements
 *    an exit (-1)
 *************************************************************************************
 */

int GenerateSeq_parameter_set_rbsp (h264enc_ctx_t *ctx)
{
  int len = 0,i;
  int crop_right=0;
  int crop_bot=0;
  int crop_flag=0;
  struct h264enc_prop *hprop=&ctx->prop;
  int pic_width_in_mbs_minus1 ,pic_height_in_map_units_minus1;

  pic_width_in_mbs_minus1 = veu.mb_width -1;
  pic_height_in_map_units_minus1 = veu.mb_height -1;
  

  len+=u_v  (8, hprop->profile_idc);
  len+=u_1  (0);
  len+=u_1  (0);
  len+=u_1  (0);
  len+=u_1  (0);
  len+=u_v  (4,0);

  len+=u_v  (8,hprop->levelIDC);


  len+=ue_v (0);

  // Fidelity Range Extensions stuff
  if( IS_FREXT_PROFILE(hprop->profile_idc) )
  {
	  len+=ue_v (1);    	//chroma_format_idc
	  if(1 == 3)			//YUV420(1)  =/= YVU444(3) 
	      len+=u_1  (0);	//separate_colour_plane_flag
	  len+=ue_v (0);		//bit_depth_luma_minus8
	  len+=ue_v (0);		//bit_depth_chroma_minus8
	  len+=u_1  (0);		//lossless_qpprime_flag
	  //other chroma info to be added in the future
	  len+=u_1 (0);			//seq_scaling_matrix_present_flag
  }

  len+=ue_v (0);			//log2_max_frame_num_minus4
  len+=ue_v (0);			//pic_order_cnt_type

  if (0 == 0)				//sps->pic_order_cnt_type == 0
    len+=ue_v (0);			//log2_max_pic_order_cnt_lsb_minus4
  else if (0 == 1)			//sps->pic_order_cnt_type == 1
  {
    len+=u_1  (0);			//delta_pic_order_always_zero_flag
    len+=se_v (0);			//offset_for_non_ref_pic
    len+=se_v (0);			//offset_for_top_to_bottom_field
    len+=ue_v (0);			//num_ref_frames_in_pic_order_cnt_cycle
    for (i=0; i<0; i++)		//sps->num_ref_frames_in_pic_order_cnt_cycle
      len+=se_v (0);		//offset_for_ref_frame
  }

  len+=ue_v (1);			//num_ref_frames
  len+=u_1  (0);			//gaps_in_frame_num_value_allowed_flag
  len+=ue_v (pic_width_in_mbs_minus1);	//pic_width_in_mbs_minus1
  len+=ue_v (pic_height_in_map_units_minus1);	//pic_height_in_map_units_minus1
  len+=u_1  (1);			//frame_mbs_only_flag
  if (0)
  {
    len+=u_1  (0);			//mb_adaptive_frame_field_flag
  }
  len+=u_1  (1);			//direct_8x8_inference_flag

  if(veu.width != (veu.mb_width*16)){
  	crop_right = (veu.mb_width*16 - veu.width)/2;
	crop_flag = 1;
  }
  if(veu.height != (veu.mb_height*16)){
  	crop_bot = (veu.mb_height*16 - veu.height)/2;
	crop_flag = 1;
  }

  len+=u_1(crop_flag);			//frame_cropping_flag
  if (crop_flag)
  {
    len+=ue_v (0);			//frame_cropping_rect_left_offset
    len+=ue_v (crop_right);			//frame_cropping_rect_right_offset
    len+=ue_v (0);			//frame_cropping_rect_top_offset
    len+=ue_v (crop_bot);			//frame_cropping_rect_bottom_offset
  }

  if (veu.sps_en)
  {
    veu.last_enc = 1;
    veu.slice_above = 1;
  }

  len+=u_1  (0);			//vui_parameters_present_flag

  return len;
}

/*!
 ***********************************************************************************************
 * \brief
 *    int GeneratePic_parameter_set_rbsp (VideoParameters *p_Vid, pic_parameter_set_rbsp_t *pps, char *rbsp);
 *
 * \param p_Vid
 *    picture parameter structure
 * \param pps
 *    picture parameter structure
 * \param rbsp
 *    buffer to be filled with the rbsp, size should be at least MAXIMUMPARSETRBSPSIZE
 *
 * \return
 *    size of the RBSP in bytes, negative in case of an error
 *
 * \note
 *    Picture Parameter VUI function is called, but the function implements
 *    an exit (-1)
 ************************************************************************************************
 */

int GeneratePic_parameter_set_rbsp (h264enc_ctx_t *ctx)
{
  int len = 0;
  unsigned int i;
  unsigned int NumberBitsPerSliceGroupId;
  int slice_group_map_type=0;

	struct h264enc_prop *hprop = &ctx->prop;

  len+=ue_v (0);			//pic_parameter_set_id
  len+=ue_v (0);			//seq_parameter_set_id
  
  len+=u_1  (0);			//entropy_coding_mode_flag
  len+=u_1  (0);			//bottom_field_pic_order_in_frame_present_flag
  len+=ue_v (0);			//num_slice_groups_minus1

  // FMO stuff
  if(0 > 0 )
  {
    len+=ue_v (0);			//slice_group_map_type
    if (slice_group_map_type == 0)
      for (i=0; i<=0; i++)	//num_slice_groups_minus1
        len+=ue_v (0);		//run_length_minus1[i]
    else if (slice_group_map_type==2)
      for (i=0; i<0; i++)	//pps->num_slice_groups_minus1
      {

        len+=ue_v (0);		//top_left[i]
        len+=ue_v (0);		//bottom_right[i]
      }
    else if (slice_group_map_type == 3 ||
             slice_group_map_type == 4 ||
             slice_group_map_type == 5)
    {
      len+=u_1  (0);		//slice_group_change_direction_flag
      len+=ue_v (0);		//slice_group_change_rate_minus1
    }
    else if (slice_group_map_type == 6)
    {
      if (0>=4)				//num_slice_groups_minus1
        NumberBitsPerSliceGroupId=3;
      else if (0>=2)		//num_slice_groups_minus1
        NumberBitsPerSliceGroupId=2;
      else if (0>=1)		//num_slice_groups_minus1
        NumberBitsPerSliceGroupId=1;
      else
        NumberBitsPerSliceGroupId=0;

      len+=ue_v (0);		//pic_size_in_map_units_minus1
      for(i=0; i<=0; i++)	//pps->pic_size_in_map_units_minus1
        len+= u_v  (0,0);	//NumberBitsPerSliceGroupId,slice_group_id[i]
    }
  }
  // End of FMO stuff

  len+=ue_v (0);			//num_ref_idx_l0_active_minus1
  len+=ue_v (0);			//num_ref_idx_l1_active_minus1
  len+=u_1  (0);			//weighted_pred_flag
  len+=u_v  (2,0);			//weighted_bipred_idc
  len+=se_v (0);			//pic_init_qp_minus26
  len+=se_v (0);			//pic_init_qs_minus26


  if( (hprop->profile_idc>=100) || (hprop->profile_idc==44) )
    len+=se_v (0);			//chroma_qp_index_offset
  else
    len+=se_v (0);			//chroma_qp_index_offset

  len+=u_1  (1);			//deblocking_filter_control_present_flag
  len+=u_1  (0);			//constrained_intra_pred_flag

  if (veu.pps_en && !((hprop->profile_idc>=100) || (hprop->profile_idc==44)))
  {
    veu.last_enc = 1;
    veu.slice_above = 1;
  }
  len+=u_1  (0);			//redundant_pic_cnt_present_flag

  // Fidelity Range Extensions stuff
  if( (hprop->profile_idc>=100) || (hprop->profile_idc==44) )
  {
    len+=u_1  (0);			//transform_8x8_mode_flag
    len+=u_1  (0);			//pic_scaling_matrix_present_flag
    
    if (veu.pps_en)
    {
      veu.last_enc = 1;
      veu.slice_above = 1;
    }

    len+=se_v (0);			//cr_qp_index_offset
  }
  return len;
}

