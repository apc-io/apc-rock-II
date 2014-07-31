
/*!
 ***************************************************************************
 * \file vlc.c
 *
 * \brief
 *    (CA)VLC coding functions
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *    - Inge Lille-Langoy               <inge.lille-langoy@telenor.com>
 *    - Detlev Marpe                    <marpe@hhi.de>
 *    - Stephan Wenger                  <stewe@cs.tu-berlin.de>
 ***************************************************************************
 */

#define WMT_VLC_C


#include "wmt-h264enc.h"
#include "vlc.h"

extern VEU veu;


/*!
 *************************************************************************************
 * \brief
 *    ue_v, writes an ue(v) syntax element, returns the length in bits
 *
 * \param tracestring
 *    the string for the trace file
 * \param value
 *    the value to be coded
 *  \param bitstream
 *    the target bitstream the value should be coded into
 *
 * \return
 *    Number of bits used by the coded syntax element
 *
 * \ note
 *    This function writes always the bit buffer for the progressive scan flag, and
 *    should not be used (or should be modified appropriately) for the interlace crap
 *    When used in the context of the Parameter Sets, this is obviously not a
 *    problem.
 *
 *************************************************************************************
 */
int ue_v (int value)
{
  SyntaxElement symbol, *sym=&symbol;
  sym->value1 = value;
  sym->value2 = 0;

  //assert (bitstream->streamBuffer != NULL);

  ue_linfo(sym->value1,sym->value2,&(sym->len),&(sym->inf));
  symbol2uvlc(sym);

  writeUVLC2buffer (sym);

  return (sym->len);
}


/*!
 *************************************************************************************
 * \brief
 *    se_v, writes an se(v) syntax element, returns the length in bits
 *
 * \param tracestring
 *    the string for the trace file
 * \param value
 *    the value to be coded
 *  \param bitstream
 *    the target bitstream the value should be coded into
 *
 * \return
 *    Number of bits used by the coded syntax element
 *
 * \ note
 *    This function writes always the bit buffer for the progressive scan flag, and
 *    should not be used (or should be modified appropriately) for the interlace crap
 *    When used in the context of the Parameter Sets, this is obviously not a
 *    problem.
 *
 *************************************************************************************
 */
int se_v ( int value)
{
  SyntaxElement symbol, *sym=&symbol;
  sym->value1 = value;
  sym->value2 = 0;

  se_linfo(sym->value1,sym->value2,&(sym->len),&(sym->inf));
  symbol2uvlc(sym);

  writeUVLC2buffer (sym);

  return (sym->len);
}


/*!
 *************************************************************************************
 * \brief
 *    u_1, writes a flag (u(1) syntax element, returns the length in bits,
 *    always 1
 *
 * \param tracestring
 *    the string for the trace file
 * \param value
 *    the value to be coded
 *  \param bitstream
 *    the target bitstream the value should be coded into
 *
 * \return
 *    Number of bits used by the coded syntax element (always 1)
 *
 * \ note
 *    This function writes always the bit buffer for the progressive scan flag, and
 *    should not be used (or should be modified appropriately) for the interlace crap
 *    When used in the context of the Parameter Sets, this is obviously not a
 *    problem.
 *
 *************************************************************************************
 */
int u_1 (int value)
{
  SyntaxElement symbol, *sym=&symbol;

  sym->bitpattern = value;
  sym->value1 = value;
  sym->len = 1;

  writeUVLC2buffer(sym);

  return sym->len;
}


/*!
 *************************************************************************************
 * \brief
 *    u_v, writes a n bit fixed length syntax element, returns the length in bits,
 *
 * \param n
 *    length in bits
 * \param tracestring
 *    the string for the trace file
 * \param value
 *    the value to be coded
 *  \param bitstream
 *    the target bitstream the value should be coded into
 *
 * \return
 *    Number of bits used by the coded syntax element
 *
 * \ note
 *    This function writes always the bit buffer for the progressive scan flag, and
 *    should not be used (or should be modified appropriately) for the interlace crap
 *    When used in the context of the Parameter Sets, this is obviously not a
 *    problem.
 *
 *************************************************************************************
 */

int u_v (int n,  int value)
{
  SyntaxElement symbol, *sym=&symbol;

  sym->bitpattern = value;
  sym->value1 = value;
  sym->len = n;  

  writeUVLC2buffer(sym);

  return (sym->len);
}


/*!
 ************************************************************************
 * \brief
 *    mapping for ue(v) syntax elements
 * \param ue
 *    value to be mapped
 * \param dummy
 *    dummy parameter
 * \param info
 *    returns mapped value
 * \param len
 *    returns mapped value length
 ************************************************************************
 */
void ue_linfo(int ue, int dummy, int *len,int *info)
{
  int i, nn =(ue + 1) >> 1;

  for (i=0; i < 33 && nn != 0; i++)
  {
    nn >>= 1;
  }
  *len  = (i << 1) + 1;
  *info = ue + 1 - (1 << i);
}


/*!
 ************************************************************************
 * \brief
 *    mapping for se(v) syntax elements
 * \param se
 *    value to be mapped
 * \param dummy
 *    dummy parameter
 * \param len
 *    returns mapped value length
 * \param info
 *    returns mapped value
 ************************************************************************
 */
void se_linfo(int se, int dummy, int *len,int *info)
{  
  int sign = (se <= 0) ? 1 : 0;
  int n = iabs(se) << 1;   //  n+1 is the number in the code table.  Based on this we find length and info
  int nn = (n >> 1);
  int i;
  for (i = 0; i < 33 && nn != 0; i++)
  {
    nn >>= 1;
  }
  *len  = (i << 1) + 1;
  *info = n - (1 << i) + sign;
}

/*!
 ************************************************************************
 * \brief
 *    Makes code word and passes it back
 *    A code word has the following format: 0 0 0 ... 1 Xn ...X2 X1 X0.
 *
 * \par Input:
 *    Info   : Xn..X2 X1 X0                                             \n
 *    Length : Total number of bits in the codeword
 ************************************************************************
 */
 // NOTE this function is called with sym->inf > (1<<(sym->len >> 1)).  The upper bits of inf are junk
int symbol2uvlc(SyntaxElement *sym)
{
  int suffix_len = sym->len >> 1;

  suffix_len = (1 << suffix_len);
  sym->bitpattern = suffix_len | (sym->inf & (suffix_len - 1));
  return 0;
}

/*!
 ************************************************************************
 * \brief
 *    writes UVLC code to the appropriate buffer
 ************************************************************************
 */
void  writeUVLC2buffer(SyntaxElement *se)
{

  unsigned int mask = 1 << (se->len - 1);
  unsigned char *byte_buf = &veu.byte_buf;
  int *bits_to_go = &veu.bits_to_go ;
  int i;


  // Add the new bits to the bitstream.
  // Write out a byte if it is full
  if ( se->len < 33 )
  {
    for (i = 0; i < se->len; i++)
    {
      *byte_buf <<= 1;

      if (se->bitpattern & mask)
        *byte_buf |= 1;

      mask >>= 1;

      if ((--(*bits_to_go)) == 0)
      {
        *bits_to_go = 8;      
        
        if (veu.hw_header_wen)
		{


		   veu.hw_byte_cnt++;	   
           if (veu.hw_byte_cnt == 1)
             veu.hw_byte1 = *byte_buf;
		   else if (veu.hw_byte_cnt == 2)
		     veu.hw_byte2 = *byte_buf;
		   else if (veu.hw_byte_cnt == 3)
		     veu.hw_byte3 = *byte_buf;
		   else if (veu.hw_byte_cnt == 4)
	    	 veu.hw_byte4 = *byte_buf;

           veu.hw_total_byte_cnt++;

	  		if (veu.hw_byte_cnt == 4)
	   		{
	    		 // Check if HW have space for write
        		 if (veu.hw_total_byte_cnt == 124)
	    		 {
	     			  veu.bytebf_ready = REG_GET32(REG_BASE_HEADER + 0x008);
	     			  veu.i = 0;
	    			  while (!veu.bytebf_ready)
	    			  {
	         			if (veu.i == 0)
	        			{
                   			TRACE("vlc.c: Wait HW BYTEBF_READY !!!\n");
		   					veu.i = 1;
	        			}	
            			veu.bytebf_ready = REG_GET32(REG_BASE_HEADER + 0x008);           			  }
	     		 }
	    		 veu.hw_byte_cnt = 0;
	     		 veu.hw_dwvalue = (veu.hw_byte1 << 24) | (veu.hw_byte2 << 16) | (veu.hw_byte3 << 8) | (veu.hw_byte4);
	     	 	 //if (veu.last_enc)
	    		 if (veu.last_enc && (i+1 == se->len))
	     		 {
	       			if (veu.slice_above){
	         			H264_REG_SET32(REG_BASE_HEADER + 0x000, 0xc0000020);  // REG_HEADER_LEN
	       			}
	       			else{
	         			H264_REG_SET32(REG_BASE_HEADER + 0x000, 0x80000020);  // REG_HEADER_LEN
	       			}
					veu.last_enc = 0;
	      			veu.slice_above = 0;
	       			veu.hw_header_wen = 0;
	     		 }
	     		 else{
           			H264_REG_SET32(REG_BASE_HEADER + 0x000, 0x00000020);  // REG_HEADER_LEN
	     		 }
				 H264_REG_SET32(REG_BASE_HEADER + 0x004, veu.hw_dwvalue); // REG_HEADER_CODE
	   		}  
		}

        *byte_buf = 0;    
      }
    }

    if (veu.last_enc)
    {
	  // Check if HW have space for write
      if (veu.hw_total_byte_cnt == 124)
      {
        veu.bytebf_ready = REG_GET32(REG_BASE_HEADER + 0x008);
        veu.i = 0;
        while (!veu.bytebf_ready)
        {
          if (veu.i == 0)
          {
            TRACE("vlc.c: Wait HW BYTEBF_READY !!!\n");
            veu.i = 1;
          }
          veu.bytebf_ready = REG_GET32(REG_BASE_HEADER + 0x008);
        }
      }
   
      veu.byte_lshift = *bits_to_go;
      if (veu.hw_byte_cnt == 0)
      {
        veu.last_len = 8 - *bits_to_go;
		veu.hw_byte1 = (*byte_buf << veu.byte_lshift);
		veu.hw_byte2 = 0;
		veu.hw_byte3 = 0;
		veu.hw_byte4 = 0;
      }	
      else if (veu.hw_byte_cnt == 1)
      {
        veu.last_len = (8 - *bits_to_go) + 8;
		veu.hw_byte2 = (*byte_buf << veu.byte_lshift);
		veu.hw_byte3 = 0;
		veu.hw_byte4 = 0;
      }
      else if (veu.hw_byte_cnt == 2)
      {
        veu.last_len = (8 - *bits_to_go) + 16;
		veu.hw_byte3 = (*byte_buf << veu.byte_lshift);
		veu.hw_byte4 = 0;
      }
      else if (veu.hw_byte_cnt == 3)
      {
        veu.last_len = (8 - *bits_to_go) + 24;
		veu.hw_byte4 = (*byte_buf << veu.byte_lshift);
      }
	  
      veu.dw_mask = ((1 << veu.last_len) - 1);

      veu.dw_rshift = 32 - veu.last_len;

      if (veu.slice_above)
        veu.last_len = (0xc0000000 | veu.last_len);
      else
        veu.last_len = (0x80000000 | veu.last_len);

      veu.hw_dwvalue = (veu.hw_byte1 << 24) | (veu.hw_byte2 << 16) | (veu.hw_byte3 << 8) | (veu.hw_byte4);     
	  H264_REG_SET32(REG_BASE_HEADER + 0x000, veu.last_len);  // REG_HEADER_LEN
      veu.last_enc = 0;
      veu.slice_above = 0;
      veu.hw_header_wen = 0;
      veu.hw_dwvalue = (veu.hw_dwvalue >> veu.dw_rshift) & veu.dw_mask;
      H264_REG_SET32(REG_BASE_HEADER + 0x004, veu.hw_dwvalue); // REG_HEADER_CODE
	  veu.bits_to_go = 8;
	  veu.hw_byte_cnt = 0;
    }  

  }
  else
  {
    // zeros
    for (i = 0; i < (se->len - 32); i++)
    {
      *byte_buf <<= 1;

      if ((--(*bits_to_go)) == 0)
      {
        *bits_to_go = 8;      
        //currStream->streamBuffer[currStream->byte_pos++] = *byte_buf;

        if (veu.hw_header_wen)
		{
	 	   veu.hw_byte_cnt++;	   
           if (veu.hw_byte_cnt == 1)
             veu.hw_byte1 = *byte_buf;
	  	   else if (veu.hw_byte_cnt == 2)
	   		 veu.hw_byte2 = *byte_buf;
	  	   else if (veu.hw_byte_cnt == 3)
	    	 veu.hw_byte3 = *byte_buf;
	  	   else if (veu.hw_byte_cnt == 4)
	     	 veu.hw_byte4 = *byte_buf;
	
           veu.hw_total_byte_cnt++;

	  	   if (veu.hw_byte_cnt == 4)
		   {
		       // Check if HW have space for write
               if (veu.hw_total_byte_cnt == 124)
	    	   {
	      			 veu.bytebf_ready = REG_GET32(REG_BASE_HEADER + 0x008);
	      			 veu.i = 0;
	      			 while (!veu.bytebf_ready)
	      			 {
	      				  if (veu.i == 0)
	       				  {
                  			 TRACE("Wait HW BYTEBF_READY !!!\n");
                  			 //fprintf(veu.cavlc_fp,"vlc.c: Wait HW BYTEBF_READY = 1\n");
		  					 veu.i = 1;
	         		      }
                 		  veu.bytebf_ready = REG_GET32(REG_BASE_HEADER + 0x008);
                 		  //AHB_NOP(100);
              		 }
	     		}
	     		veu.hw_byte_cnt = 0;
	     		veu.hw_dwvalue = (veu.hw_byte1 << 24) | (veu.hw_byte2 << 16) | (veu.hw_byte3 << 8) | (veu.hw_byte4);
            	H264_REG_SET32(REG_BASE_HEADER + 0x000, 0x00000020);  // REG_HEADER_LEN
	     		H264_REG_SET32(REG_BASE_HEADER + 0x004, veu.hw_dwvalue); // REG_HEADER_CODE
	   	  }  
		}

        *byte_buf = 0;      
      }
    }
    // actual info
    mask = (unsigned int) 1 << 31;
    for (i = 0; i < 32; i++)
    {
      *byte_buf <<= 1;

      if (se->bitpattern & mask)
        *byte_buf |= 1;

      mask >>= 1;

      if ((--(*bits_to_go)) == 0)
      {
        *bits_to_go = 8;      
        //currStream->streamBuffer[currStream->byte_pos++] = *byte_buf;

        if (veu.hw_header_wen)
		{
	   		veu.hw_byte_cnt++;	   
           	if (veu.hw_byte_cnt == 1)
             	veu.hw_byte1 = *byte_buf;
	   		else if (veu.hw_byte_cnt == 2)
	    		veu.hw_byte2 = *byte_buf;
	   		else if (veu.hw_byte_cnt == 3)
	     		veu.hw_byte3 = *byte_buf;
	   		else if (veu.hw_byte_cnt == 4)
	     		veu.hw_byte4 = *byte_buf;
	
           	veu.hw_total_byte_cnt++;

	   		if (veu.hw_byte_cnt == 4)
	   		{
	     		// Check if HW have space for write
             	if (veu.hw_total_byte_cnt == 124)
	     		{
	       			veu.bytebf_ready = REG_GET32(REG_BASE_HEADER + 0x008);
	       			veu.i = 0;
	       			while (!veu.bytebf_ready)
	       			{
	         			if (veu.i == 0)
	         			{
                   			TRACE("vlc.c: Wait HW BYTEBF_READY !!!\n");
		   					veu.i = 1;
	         			}
                 		veu.bytebf_ready = REG_GET32(REG_BASE_HEADER + 0x008);
               		}
	     		}
	     		veu.hw_byte_cnt = 0;
	     		veu.hw_dwvalue = (veu.hw_byte1 << 24) | (veu.hw_byte2 << 16) | (veu.hw_byte3 << 8) | (veu.hw_byte4);
	     		if (veu.last_enc && (i == 31))
	     		{
	       			if (veu.slice_above){
	        			H264_REG_SET32(REG_BASE_HEADER + 0x000, 0xc0000020);  // REG_HEADER_LEN
	       			}
					else{
	         			H264_REG_SET32(REG_BASE_HEADER + 0x000, 0x80000020);  // REG_HEADER_LEN
					}
	       			veu.last_enc = 0;
	       			veu.hw_header_wen = 0;
	     		}
	     		else{
               		H264_REG_SET32(REG_BASE_HEADER + 0x000, 0x00000020);  // REG_HEADER_LEN
	     		}
				H264_REG_SET32(REG_BASE_HEADER + 0x004, veu.hw_dwvalue); // REG_HEADER_CODE
	   		}  
		}

        *byte_buf = 0;      
      }
    }

    if (veu.last_enc)
    {
      // Check if HW have space for write
      if (veu.hw_total_byte_cnt == 124)
      {
        veu.bytebf_ready = REG_GET32(REG_BASE_HEADER + 0x008);
        veu.i = 0;
        while (!veu.bytebf_ready)
        {
          if (veu.i == 0)
          {
            TRACE("vlc.c: Wait HW BYTEBF_READY !!!\n");
            veu.i = 1;
          }
          veu.bytebf_ready = REG_GET32(REG_BASE_HEADER + 0x008);
        }
      }

      veu.byte_lshift = *bits_to_go;
      if (veu.hw_byte_cnt == 0)
      {
        veu.last_len = 8 - *bits_to_go;
		veu.hw_byte1 = (*byte_buf << veu.byte_lshift);
		veu.hw_byte2 = 0;
		veu.hw_byte3 = 0;
		veu.hw_byte4 = 0;
      }	
      else if (veu.hw_byte_cnt == 1)
      {
        veu.last_len = (8 - *bits_to_go) + 8;
		veu.hw_byte2 = (*byte_buf << veu.byte_lshift);
		veu.hw_byte3 = 0;
		veu.hw_byte4 = 0;
      }
      else if (veu.hw_byte_cnt == 2)
      {
        veu.last_len = (8 - *bits_to_go) + 16;
		veu.hw_byte3 = (*byte_buf << veu.byte_lshift);
		veu.hw_byte4 = 0;
      }
      else if (veu.hw_byte_cnt == 3)
      {
        veu.last_len = (8 - *bits_to_go) + 24;
		veu.hw_byte4 = (*byte_buf << veu.byte_lshift);
      }

      veu.dw_mask = ((1 << veu.last_len) - 1);
      veu.dw_rshift = 32 - veu.last_len;

      if (veu.slice_above)
        veu.last_len = (0xc0000000 | veu.last_len); 
      else
        veu.last_len = (0x80000000 | veu.last_len); 

      veu.hw_dwvalue = (veu.hw_byte1 << 24) | (veu.hw_byte2 << 16) | (veu.hw_byte3 << 8) | (veu.hw_byte4);
      H264_REG_SET32(REG_BASE_HEADER + 0x000, veu.last_len);  // REG_HEADER_LEN
      veu.last_enc = 0;
      veu.slice_above = 0;
      veu.hw_header_wen = 0;
      veu.hw_dwvalue = (veu.hw_dwvalue >> veu.dw_rshift) & veu.dw_mask;
      H264_REG_SET32(REG_BASE_HEADER + 0x004, veu.hw_dwvalue); // REG_HEADER_CODE
      veu.hw_byte_cnt = 0;
    }  

  }
}
