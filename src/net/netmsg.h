/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id$
    begin       : Mon Feb 09 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Lesser General Public            *
 *   License as published by the Free Software Foundation; either          *
 *   version 2.1 of the License, or (at your option) any later version.    *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston,                 *
 *   MA  02111-1307  USA                                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef GWEN_NETMSG_H
#define GWEN_NETMSG_H

#include <gwenhywfar/gwenhywfarapi.h>
#ifdef __cplusplus
extern "C" {
#endif
GWENHYWFAR_API
typedef struct GWEN_NETMSG GWEN_NETMSG;
#ifdef __cplusplus
}
#endif

#include <gwenhywfar/buffer.h>
#include <gwenhywfar/db.h>
#include <gwenhywfar/types.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/bufferedio.h>

#ifdef __cplusplus
extern "C" {
#endif


GWEN_LIST_FUNCTION_DEFS(GWEN_NETMSG, GWEN_NetMsg)
/* No trailing semicolon here because this is a macro call */


/** @name Constructors And Destructors
 *
 */
/*@{*/
GWENHYWFAR_API
GWEN_NETMSG *GWEN_NetMsg_new(GWEN_TYPE_UINT32 bufferSize);
GWENHYWFAR_API
void GWEN_NetMsg_free(GWEN_NETMSG *msg);
GWENHYWFAR_API
void GWEN_NetMsg_Attach(GWEN_NETMSG *msg);

/*@}*/


/** @name Getters And Setters
 *
 */
/*@{*/
/**
 * Returns a pointer to the buffer belonging to a message.
 * This function does NOT relinquish ownership.
 */
GWENHYWFAR_API
GWEN_BUFFER *GWEN_NetMsg_GetBuffer(const GWEN_NETMSG *msg);

GWENHYWFAR_API
GWEN_BUFFER *GWEN_NetMsg_TakeBuffer(GWEN_NETMSG *msg);

/**
 * Replaces the internal buffer with the given one.
 * Takes over ownership of the buffer.
 */
GWENHYWFAR_API
void GWEN_NetMsg_SetBuffer(GWEN_NETMSG *msg,
                           GWEN_BUFFER *buf);

/**
 * Returns the value of the size variable. The meaning of this variable
 * depends on the protocol this message belongs to.
 */
GWENHYWFAR_API
GWEN_TYPE_UINT32 GWEN_NetMsg_GetSize(const GWEN_NETMSG *msg);
GWENHYWFAR_API
void GWEN_NetMsg_SetSize(GWEN_NETMSG *msg,
                         GWEN_TYPE_UINT32 size);

/**
 * Decrements the size value by the given offset (will not go below zero).
 */
GWENHYWFAR_API
void GWEN_NetMsg_DecrementSize(GWEN_NETMSG *msg,
                               GWEN_TYPE_UINT32 offs);

/**
 * Increments the size value by the given offset (does not check for
 * wrap-around).
 */
GWENHYWFAR_API
void GWEN_NetMsg_IncrementSize(GWEN_NETMSG *msg,
                               GWEN_TYPE_UINT32 offs);

/**
 * Returns a pointer to the DB belonging to a message.
 * This function does NOT relinquish ownership.
 */
GWENHYWFAR_API
GWEN_DB_NODE *GWEN_NetMsg_GetDB(const GWEN_NETMSG *msg);


GWENHYWFAR_API
GWEN_BUFFEREDIO *GWEN_NetMsg_GetBufferedIO(const GWEN_NETMSG *msg);

GWENHYWFAR_API
GWEN_BUFFEREDIO *GWEN_NetMsg_TakeBufferedIO(GWEN_NETMSG *msg);

/**
 * Replaces the internal bufferedIO with the given one.
 * Takes over ownership of the bufferedIO.
 */
GWENHYWFAR_API
void GWEN_NetMsg_SetBufferedIO(GWEN_NETMSG *msg,
                               GWEN_BUFFEREDIO *bio);

GWENHYWFAR_API
int GWEN_NetMsg_GetProtocolMajorVersion(const GWEN_NETMSG *msg);

GWENHYWFAR_API
int GWEN_NetMsg_GetProtocolMinorVersion(const GWEN_NETMSG *msg);

GWENHYWFAR_API
void GWEN_NetMsg_SetProtocolVersion(GWEN_NETMSG *msg,
                                    int pmajor, int pminor);


/*@}*/

#ifdef __cplusplus
}
#endif


#endif
