/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#ifndef _DEBUG_H
#define _DEBUG_H

#define DEBUG_REPORT	0
#define DEBUG_NOTIFY	1
#define DEBUG_WARN	2
#define DEBUG_SERIOUS	3

#define DEBUG_LEVEL(x)	(debug >= (x))

#endif
