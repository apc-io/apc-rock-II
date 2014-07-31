/*++
	Copyright (c) 2008  WonderMedia Technologies, Inc.   All Rights Reserved.

	This PROPRIETARY SOFTWARE is the property of WonderMedia Technologies, Inc.
	and may contain trade secrets and/or other confidential information of
	WonderMedia Technologies, Inc. This file shall not be disclosed to any third
	party, in whole or in part, without prior written consent of WonderMedia.

	THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS,
	WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED,
	AND WonderMedia TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
	NON-INFRINGEMENT.

	Module Name:

	Revision History:

	$JustDate: 2012/07/23 $
--*/

int wmt_setsyspara(char *varname, char *varval);
int wmt_getsyspara(char *varname, unsigned char *varval, int *varlen);
int wmt_getsocinfo(unsigned int *chipid, unsigned int *bondingid);