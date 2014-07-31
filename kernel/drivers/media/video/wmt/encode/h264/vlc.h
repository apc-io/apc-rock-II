
/*!
 *************************************************************************************
 * \file vlc.h
 *
 * \brief
 *    Prototypes for VLC coding funtions
 * \author
 *     Karsten Suehring
 *************************************************************************************
 */

#ifndef _VLC_H_
#define _VLC_H_


//! Syntax Element
typedef struct syntaxelement
{
  int                 type;           //!< type of syntax element for data part.
  int                 value1;         //!< numerical value of syntax element
  int                 value2;         //!< for blocked symbols, e.g. run/level
  int                 len;            //!< length of code
  int                 inf;            //!< info part of UVLC code
  unsigned int        bitpattern;     //!< UVLC bitpattern
  int                 context;        //!< CABAC context

  //!< for mapping of syntaxElement to UVLC
  void    (*mapping)(int value1, int value2, int* len_ptr, int* info_ptr);

} SyntaxElement;

static inline int iabs(int x)
{
  static const int INT_BITS = (sizeof(int) * 8) - 1;
  int y = x >> INT_BITS;
  return (x ^ y) - y;
}

#ifdef WMT_VLC_C /* allocate memory for variables only in wm8510-jdec.c */
    #define EXTERN
#else
    #define EXTERN   extern
#endif /* ifdef WM8440_JDEC_C */

/* EXTERN int      viaapi_xxxx; *//*Example*/

#undef EXTERN


extern int u_1  ( int value);
extern int se_v ( int value);
extern int ue_v ( int value);
extern int u_v  (int n, int value);
//extern void writeSE_UVLC                 (SyntaxElement *se, DataPartition *dp);
extern void  writeUVLC2buffer            (SyntaxElement *se);
extern int   symbol2uvlc                 (SyntaxElement *se);
extern void  ue_linfo       (int n, int dummy, int *len,int *info);
extern void  se_linfo       (int mvd, int dummy, int *len,int *info);



#endif

