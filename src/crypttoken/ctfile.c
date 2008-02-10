/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: crypttoken.h 1113 2007-01-10 09:14:16Z martin $
    begin       : Wed Mar 16 2005
    copyright   : (C) 2005 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include "ctfile_p.h"
#include "i18n_l.h"
#include <gwenhywfar/ctf_context_be.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/padd.h>
#include <gwenhywfar/cryptkeyrsa.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>



GWEN_INHERIT(GWEN_CRYPT_TOKEN, GWEN_CRYPT_TOKEN_FILE)





int GWEN_Crypt_TokenFile__OpenFile(GWEN_CRYPT_TOKEN *ct, int wr, uint32_t gid){
  int fd;
  GWEN_CRYPT_TOKEN_FILE *lct;
  GWEN_FSLOCK_RESULT lres;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, GWEN_CRYPT_TOKEN_FILE, ct);
  assert(lct);

  lct->lock=GWEN_FSLock_new(GWEN_Crypt_Token_GetTokenName(ct),
			    GWEN_FSLock_TypeFile);
  lres=GWEN_FSLock_Lock(lct->lock, 10000, gid);
  if (lres!=GWEN_FSLock_ResultOk) {
    GWEN_FSLock_free(lct->lock);
    lct->lock=0;
    DBG_ERROR(GWEN_LOGDOMAIN, "Could not lock file");
    if (lres==GWEN_FSLock_ResultUserAbort)
      return GWEN_ERROR_USER_ABORTED;
    else
      return GWEN_ERROR_IO;
  }
  else {
    DBG_INFO(GWEN_LOGDOMAIN,
	     "Keyfile [%s] locked.",
	     GWEN_Crypt_Token_GetTokenName(ct));
  }

  if (wr) {
    /* write file */
    fd=open(GWEN_Crypt_Token_GetTokenName(ct),
            O_RDWR|O_CREAT
#ifdef OS_WIN32
            | O_BINARY
#endif
            ,
	    S_IRUSR|S_IWUSR | lct->keyfile_mode);
  }
  else {
    /* Remember the access permissions when opening the file */
    struct stat statbuffer;
    if (!stat(GWEN_Crypt_Token_GetTokenName(ct), &statbuffer)) {
      /* Save the access mode, but masked by the bit masks for
	 user/group/other permissions */
      lct->keyfile_mode = 
	statbuffer.st_mode & (S_IRWXU
#ifndef OS_WIN32
			      | S_IRWXG | S_IRWXO
#endif
			      );
    }
    else {
      DBG_ERROR(GWEN_LOGDOMAIN,
		"stat(%s): %s",
		GWEN_Crypt_Token_GetTokenName(ct),
		strerror(errno));

      GWEN_FSLock_Unlock(lct->lock);
      GWEN_FSLock_free(lct->lock);
      lct->lock=0;
      DBG_INFO(GWEN_LOGDOMAIN,
	       "Keyfile [%s] unlocked.",
	       GWEN_Crypt_Token_GetTokenName(ct));
      return GWEN_ERROR_IO;
    }

    /* and open the file */
    fd=open(GWEN_Crypt_Token_GetTokenName(ct),
            O_RDONLY
#ifdef OS_WIN32
            | O_BINARY
#endif
           );
  }

  if (fd==-1) {
    DBG_ERROR(GWEN_LOGDOMAIN,
	      "open(%s): %s",
	      GWEN_Crypt_Token_GetTokenName(ct),
	      strerror(errno));
    GWEN_FSLock_Unlock(lct->lock);
    GWEN_FSLock_free(lct->lock);
    lct->lock=0;
    DBG_INFO(GWEN_LOGDOMAIN,
	     "Keyfile [%s] unlocked.",
	     GWEN_Crypt_Token_GetTokenName(ct));
    return GWEN_ERROR_IO;
  }

  lct->fd=fd;

  return 0;
}



int GWEN_Crypt_TokenFile__CloseFile(GWEN_CRYPT_TOKEN *ct, uint32_t gid){
  GWEN_CRYPT_TOKEN_FILE *lct;
  GWEN_FSLOCK_RESULT lres;
  struct stat st;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, GWEN_CRYPT_TOKEN_FILE, ct);
  assert(lct);

  if (lct->fd==-1) {
    DBG_ERROR(GWEN_LOGDOMAIN, "Keyfile \"%s\"not open",
	      GWEN_Crypt_Token_GetTokenName(ct));
    return GWEN_ERROR_INTERNAL;
  }

  if (close(lct->fd)) {
    DBG_ERROR(GWEN_LOGDOMAIN, "close(%s): %s",
              GWEN_Crypt_Token_GetTokenName(ct), strerror(errno));
    lct->fd=-1;
    GWEN_FSLock_Unlock(lct->lock);
    GWEN_FSLock_free(lct->lock);
    lct->lock=0;
    DBG_INFO(GWEN_LOGDOMAIN,
	     "Keyfile [%s] unlocked.",
	     GWEN_Crypt_Token_GetTokenName(ct));
    return GWEN_ERROR_IO;
  }
  lct->fd=-1;

  lres=GWEN_FSLock_Unlock(lct->lock);
  if (lres!=GWEN_FSLock_ResultOk) {
    DBG_WARN(GWEN_LOGDOMAIN, "Error removing lock from \"%s\": %d",
             GWEN_Crypt_Token_GetTokenName(ct), lres);
  }
  GWEN_FSLock_free(lct->lock);
  lct->lock=0;
  DBG_INFO(GWEN_LOGDOMAIN,
	   "Keyfile [%s] unlocked.",
	   GWEN_Crypt_Token_GetTokenName(ct));

  /* get times */
  if (stat(GWEN_Crypt_Token_GetTokenName(ct), &st)) {
    DBG_ERROR(GWEN_LOGDOMAIN,
	      "stat(%s): %s",
	      GWEN_Crypt_Token_GetTokenName(ct),
	      strerror(errno));
    return GWEN_ERROR_IO;
  }

#ifndef OS_WIN32
  if (st.st_mode & 0007) {
    DBG_WARN(GWEN_LOGDOMAIN,
             "WARNING: Your keyfile \"%s\" is world accessible!\n"
             "Nobody but you should have access to the file. You \n"
	     "should probably change this with \"chmod 600 %s\"",
             GWEN_Crypt_Token_GetTokenName(ct),
             GWEN_Crypt_Token_GetTokenName(ct));
    GWEN_Gui_ProgressLog(gid, GWEN_LoggerLevel_Warning,
			 "WARNING: Your keyfile is world accessible!\n"
			 "Nobody but you should have access to the file.");
  }
#endif
  lct->mtime=st.st_mtime;
  lct->ctime=st.st_ctime;

  return 0;
}



int GWEN_Crypt_TokenFile__Read(GWEN_CRYPT_TOKEN *ct, uint32_t gid){
  GWEN_CRYPT_TOKEN_FILE *lct;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, GWEN_CRYPT_TOKEN_FILE, ct);
  assert(lct);

  assert(lct->readFn);
  if (lseek(lct->fd, 0, SEEK_SET)==-1) {
    DBG_ERROR(GWEN_LOGDOMAIN, "lseek(%s): %s",
	      GWEN_Crypt_Token_GetTokenName(ct),
	      strerror(errno));
    return GWEN_ERROR_IO;
  }
  return lct->readFn(ct, lct->fd, gid);
}



int GWEN_Crypt_TokenFile__Write(GWEN_CRYPT_TOKEN *ct, int cr, uint32_t gid){
  GWEN_CRYPT_TOKEN_FILE *lct;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, GWEN_CRYPT_TOKEN_FILE, ct);
  assert(lct);

  if (lct->writeFn==0) {
    DBG_WARN(GWEN_LOGDOMAIN,
             "No write function in crypt token type \"%s\"",
             GWEN_Crypt_Token_GetTypeName(ct));
    return GWEN_ERROR_NOT_SUPPORTED;
  }

  if (lseek(lct->fd, 0, SEEK_SET)==-1) {
    DBG_ERROR(GWEN_LOGDOMAIN, "lseek(%s): %s",
              GWEN_Crypt_Token_GetTokenName(ct),
              strerror(errno));
    return GWEN_ERROR_IO;
  }
  return lct->writeFn(ct, lct->fd, cr, gid);
}



int GWEN_Crypt_TokenFile__ReadFile(GWEN_CRYPT_TOKEN *ct, uint32_t gid){
  GWEN_CRYPT_TOKEN_FILE *lct;
  int rv;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, GWEN_CRYPT_TOKEN_FILE, ct);
  assert(lct);

  /* clear context list, it will be reloaded */
  GWEN_Crypt_Token_Context_List_Clear(lct->contextList);

  /* open file */
  rv=GWEN_Crypt_TokenFile__OpenFile(ct, 0, gid);
  if (rv) {
    DBG_INFO(GWEN_LOGDOMAIN,
             "Could not open keyfile for reading (%d)", rv);
    return rv;
  }

  /* read file */
  rv=GWEN_Crypt_TokenFile__Read(ct, gid);
  if (rv) {
    DBG_INFO(GWEN_LOGDOMAIN, "Error reading keyfile");
    GWEN_Crypt_TokenFile__CloseFile(ct, gid);
    return rv;
  }

  /* close file */
  rv=GWEN_Crypt_TokenFile__CloseFile(ct, gid);
  if (rv) {
    DBG_INFO(GWEN_LOGDOMAIN, "Could not close keyfile");
    return rv;
  }

  return 0;
}



int GWEN_Crypt_TokenFile__WriteFile(GWEN_CRYPT_TOKEN *ct, int cr, uint32_t gid){
  GWEN_CRYPT_TOKEN_FILE *lct;
  int rv;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, GWEN_CRYPT_TOKEN_FILE, ct);
  assert(lct);

  /* open file */
  rv=GWEN_Crypt_TokenFile__OpenFile(ct, 1, gid);
  if (rv) {
    DBG_INFO(GWEN_LOGDOMAIN,
             "Could not open keyfile for writing (%d)", rv);
    return rv;
  }

  /* write file */
  rv=GWEN_Crypt_TokenFile__Write(ct, cr, gid);
  if (rv) {
    DBG_INFO(GWEN_LOGDOMAIN, "Error writing keyfile");
    GWEN_Crypt_TokenFile__CloseFile(ct, gid);
    return rv;
  }

  /* close file */
  rv=GWEN_Crypt_TokenFile__CloseFile(ct, gid);
  if (rv) {
    DBG_INFO(GWEN_LOGDOMAIN, "Could not close keyfile");
    return rv;
  }

  return 0;
}



int GWEN_Crypt_TokenFile__ReloadIfNeeded(GWEN_CRYPT_TOKEN *ct, uint32_t gid){
  GWEN_CRYPT_TOKEN_FILE *lct;
  struct stat st;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, GWEN_CRYPT_TOKEN_FILE, ct);
  assert(lct);

  if (stat(GWEN_Crypt_Token_GetTokenName(ct), &st)) {
    DBG_ERROR(GWEN_LOGDOMAIN,
              "stat(%s): %s",
              GWEN_Crypt_Token_GetTokenName(ct),
              strerror(errno));
    return -1;
  }
  if (lct->mtime!=st.st_mtime ||
      lct->ctime!=st.st_ctime) {
    int rv;

    /* file has changed, reload it */
    DBG_NOTICE(GWEN_LOGDOMAIN,
               "Keyfile changed externally, reloading it");
    /* read file */
    rv=GWEN_Crypt_TokenFile__ReadFile(ct, gid);
    if (rv) {
      DBG_WARN(GWEN_LOGDOMAIN, "Error reloading keyfile");
      return rv;
    }
  }
  else {
    DBG_NOTICE(GWEN_LOGDOMAIN, "Keyfile unchanged, not reloading");
  }
  return 0;
}



void GWEN_Crypt_TokenFile_AddContext(GWEN_CRYPT_TOKEN *ct, GWEN_CRYPT_TOKEN_CONTEXT *ctx) {
  GWEN_CRYPT_TOKEN_FILE *lct;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, GWEN_CRYPT_TOKEN_FILE, ct);
  assert(lct);

  /* make sure the context is a file context */
  assert(GWEN_CTF_Context_IsOfThisType(ctx));
  GWEN_Crypt_Token_Context_List_Add(ctx, lct->contextList);
}



GWEN_CRYPT_TOKEN_CONTEXT *GWEN_Crypt_TokenFile_GetContext(GWEN_CRYPT_TOKEN *ct, int idx) {
  GWEN_CRYPT_TOKEN_FILE *lct;
  GWEN_CRYPT_TOKEN_CONTEXT *ctx;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, GWEN_CRYPT_TOKEN_FILE, ct);
  assert(lct);

  ctx=GWEN_Crypt_Token_Context_List_First(lct->contextList);
  while(ctx) {
    if (idx==0)
      return ctx;
    ctx=GWEN_Crypt_Token_Context_List_Next(ctx);
    idx--;
  }

  return NULL;
}



GWEN_CRYPT_TOKEN_FILE_READ_FN GWEN_Crypt_TokenFile_SetReadFn(GWEN_CRYPT_TOKEN *ct,
							     GWEN_CRYPT_TOKEN_FILE_READ_FN f) {
  GWEN_CRYPT_TOKEN_FILE *lct;
  GWEN_CRYPT_TOKEN_FILE_READ_FN of;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, GWEN_CRYPT_TOKEN_FILE, ct);
  assert(lct);

  of=lct->readFn;
  lct->readFn=f;

  return of;
}



GWEN_CRYPT_TOKEN_FILE_WRITE_FN GWEN_Crypt_TokenFile_SetWriteFn(GWEN_CRYPT_TOKEN *ct,
							       GWEN_CRYPT_TOKEN_FILE_WRITE_FN f) {
  GWEN_CRYPT_TOKEN_FILE *lct;
  GWEN_CRYPT_TOKEN_FILE_WRITE_FN of;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, GWEN_CRYPT_TOKEN_FILE, ct);
  assert(lct);

  of=lct->writeFn;
  lct->writeFn=f;

  return of;
}



int GWENHYWFAR_CB GWEN_Crypt_TokenFile_Create(GWEN_CRYPT_TOKEN *ct, uint32_t gid){
  GWEN_CRYPT_TOKEN_FILE *lct;
  struct stat st;
  int fd;
  int rv;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, GWEN_CRYPT_TOKEN_FILE, ct);
  assert(lct);

  if (!GWEN_Crypt_Token_GetTokenName(ct)) {
    DBG_ERROR(GWEN_LOGDOMAIN, "No medium name given");
    return GWEN_ERROR_INVALID;
  }

  if (stat(GWEN_Crypt_Token_GetTokenName(ct), &st)) {
    if (errno!=ENOENT) {
      DBG_ERROR(GWEN_LOGDOMAIN,
                "stat(%s): %s",
                GWEN_Crypt_Token_GetTokenName(ct),
                strerror(errno));
      return GWEN_ERROR_IO;
    }
  }
  else {
    DBG_ERROR(GWEN_LOGDOMAIN,
              "Keyfile \"%s\" already exists, will not create it",
              GWEN_Crypt_Token_GetTokenName(ct));
    return GWEN_ERROR_INVALID;
  }


  /* create file */
  fd=open(GWEN_Crypt_Token_GetTokenName(ct),
          O_RDWR | O_CREAT | O_EXCL
#ifdef OS_WIN32
          | O_BINARY
#endif
          ,
          S_IRUSR|S_IWUSR);


  if (fd==-1) {
    DBG_ERROR(GWEN_LOGDOMAIN,
              "open(%s): %s",
              GWEN_Crypt_Token_GetTokenName(ct),
              strerror(errno));
    return GWEN_ERROR_IO;
  }

  close(fd);

  rv=GWEN_Crypt_TokenFile__WriteFile(ct, 1, gid);
  if (rv) {
    DBG_INFO(GWEN_LOGDOMAIN, "here");
    return rv;
  }

  return 0;
}



int GWENHYWFAR_CB GWEN_Crypt_TokenFile_Open(GWEN_CRYPT_TOKEN *ct, int admin, uint32_t gid){
  GWEN_CRYPT_TOKEN_FILE *lct;
  int rv;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, GWEN_CRYPT_TOKEN_FILE, ct);
  assert(lct);

  rv=GWEN_Crypt_TokenFile__ReadFile(ct, gid);
  if (rv) {
    DBG_INFO(GWEN_LOGDOMAIN, "here");
    return rv;
  }

  return 0;
}



int GWENHYWFAR_CB GWEN_Crypt_TokenFile_Close(GWEN_CRYPT_TOKEN *ct, int abandon, uint32_t gid){
  GWEN_CRYPT_TOKEN_FILE *lct;
  int rv;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, GWEN_CRYPT_TOKEN_FILE, ct);
  assert(lct);

  if (!abandon)
    rv=GWEN_Crypt_TokenFile__WriteFile(ct, 0, gid);
  else
    rv=0;

  /* free/reset all data */
  GWEN_Crypt_Token_Context_List_Clear(lct->contextList);
  lct->mtime=0;
  lct->ctime=0;

  return rv;
}




int GWENHYWFAR_CB GWEN_Crypt_TokenFile__GetKeyIdList(GWEN_CRYPT_TOKEN *ct,
						     uint32_t *pIdList,
						     uint32_t *pCount,
						     uint32_t gid) {
  GWEN_CRYPT_TOKEN_FILE *lct;
  GWEN_CRYPT_TOKEN_CONTEXT *ctx;
  int i;
  int rv;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, GWEN_CRYPT_TOKEN_FILE, ct);
  assert(lct);

  /* reload if needed */
  rv=GWEN_Crypt_TokenFile__ReloadIfNeeded(ct, gid);
  if (rv) {
    DBG_INFO(GWEN_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  /* count keys */
  i=0;
  ctx=GWEN_Crypt_Token_Context_List_First(lct->contextList);
  while(ctx) {
    i+=GWEN_CRYPT_TOKEN_CONTEXT_KEYS;
    ctx=GWEN_Crypt_Token_Context_List_Next(ctx);
  }

  /* if no buffer given just return number of keys */
  if (pIdList==NULL) {
    *pCount=i;
    return 0;
  }

  if (*pCount<i) {
    DBG_INFO(GWEN_LOGDOMAIN, "Buffer too small");
    return GWEN_ERROR_BUFFER_OVERFLOW;
  }

  *pCount=i;
  i=0;
  ctx=GWEN_Crypt_Token_Context_List_First(lct->contextList);
  while(ctx) {
    int j;

    for (j=1; j<=GWEN_CRYPT_TOKEN_CONTEXT_KEYS; j++)
      *(pIdList++)=(i<<16)+j;

    ctx=GWEN_Crypt_Token_Context_List_Next(ctx);
    i++;
  }

  return 0;
}



const GWEN_CRYPT_TOKEN_KEYINFO* GWENHYWFAR_CB 
GWEN_Crypt_TokenFile__GetKeyInfo(GWEN_CRYPT_TOKEN *ct,
				 uint32_t id,
				 uint32_t flags,
				 uint32_t gid) {
  GWEN_CRYPT_TOKEN_FILE *lct;
  GWEN_CRYPT_TOKEN_CONTEXT *ctx;
  GWEN_CRYPT_TOKEN_KEYINFO *ki;
  int i;
  int rv;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, GWEN_CRYPT_TOKEN_FILE, ct);
  assert(lct);

  /* reload if needed */
  rv=GWEN_Crypt_TokenFile__ReloadIfNeeded(ct, gid);
  if (rv) {
    DBG_INFO(GWEN_LOGDOMAIN, "here (%d)", rv);
    return NULL;
  }

  i=id>>16;
  ctx=GWEN_Crypt_Token_Context_List_First(lct->contextList);
  while(ctx) {
    if (i==0)
      break;
    ctx=GWEN_Crypt_Token_Context_List_Next(ctx);
    i--;
  }

  if (ctx==NULL) {
    DBG_INFO(GWEN_LOGDOMAIN, "No key by id [%x] known (context out of range)", id);
    return NULL;
  }

  switch(id & 0xffff) {
  case 1: ki=GWEN_CTF_Context_GetLocalSignKeyInfo(ctx); break;
  case 2: ki=GWEN_CTF_Context_GetLocalCryptKeyInfo(ctx); break;
  case 3: ki=GWEN_CTF_Context_GetRemoteSignKeyInfo(ctx); break;
  case 4: ki=GWEN_CTF_Context_GetRemoteCryptKeyInfo(ctx); break;
  case 5: ki=GWEN_CTF_Context_GetLocalAuthKeyInfo(ctx); break;
  case 6: ki=GWEN_CTF_Context_GetRemoteAuthKeyInfo(ctx); break;
  default:
    DBG_INFO(GWEN_LOGDOMAIN, "No key by id [%x] known (key id out of range)", id);
    return NULL;
  }

  if (ki==NULL) {
    DBG_INFO(GWEN_LOGDOMAIN, "No key info stored for key %d", id);
    return NULL;
  }

  return ki;
}



int GWENHYWFAR_CB 
GWEN_Crypt_TokenFile__SetKeyInfo(GWEN_CRYPT_TOKEN *ct,
				 uint32_t id,
				 const GWEN_CRYPT_TOKEN_KEYINFO *ki,
				 uint32_t gid) {
  GWEN_CRYPT_TOKEN_FILE *lct;
  GWEN_CRYPT_TOKEN_CONTEXT *ctx;
  int i;
  int rv;
  GWEN_CRYPT_TOKEN_KEYINFO *nki;
  GWEN_CRYPT_KEY *key;
  uint32_t flags;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, GWEN_CRYPT_TOKEN_FILE, ct);
  assert(lct);

  flags=GWEN_Crypt_Token_KeyInfo_GetFlags(ki);

  /* reload if needed */
  rv=GWEN_Crypt_TokenFile__ReloadIfNeeded(ct, gid);
  if (rv) {
    DBG_INFO(GWEN_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  i=id>>16;
  ctx=GWEN_Crypt_Token_Context_List_First(lct->contextList);
  while(ctx) {
    if (i==0)
      break;
    ctx=GWEN_Crypt_Token_Context_List_Next(ctx);
    i--;
  }

  if (ctx==NULL) {
    DBG_INFO(GWEN_LOGDOMAIN, "No key by id [%x] known (context out of range)", id);
    return GWEN_ERROR_NOT_FOUND;
  }

  nki=GWEN_Crypt_Token_KeyInfo_dup(ki);
  assert(nki);
  switch(id & 0xffff) {
  case 1:
    GWEN_CTF_Context_SetLocalSignKeyInfo(ctx, nki);
    key=GWEN_CTF_Context_GetLocalSignKey(ctx);
    break;
  case 2:
    GWEN_CTF_Context_SetLocalCryptKeyInfo(ctx, nki);
    key=GWEN_CTF_Context_GetLocalCryptKey(ctx);
    break;
  case 3:
    GWEN_CTF_Context_SetRemoteSignKeyInfo(ctx, nki);
    key=GWEN_CTF_Context_GetRemoteSignKey(ctx);
    break;
  case 4:
    GWEN_CTF_Context_SetRemoteCryptKeyInfo(ctx, nki);
    key=GWEN_CTF_Context_GetRemoteCryptKey(ctx);
    break;
  case 5:
    GWEN_CTF_Context_SetLocalAuthKeyInfo(ctx, nki);
    key=GWEN_CTF_Context_GetLocalAuthKey(ctx);
    break;
  case 6:
    GWEN_CTF_Context_SetRemoteAuthKeyInfo(ctx, nki);
    key=GWEN_CTF_Context_GetRemoteAuthKey(ctx);
    break;
  default:
    DBG_INFO(GWEN_LOGDOMAIN, "No key by id [%x] known (key id out of range)", id);
    GWEN_Crypt_Token_KeyInfo_free(nki);
    return GWEN_ERROR_NOT_FOUND;
  }

  /* replace key if modulus and exponent are given */
  if ((flags & GWEN_CRYPT_TOKEN_KEYFLAGS_HASMODULUS) &&
      (flags & GWEN_CRYPT_TOKEN_KEYFLAGS_HASEXPONENT) &&
      id!=1 && /* don't change local keys */
      id!=2 &&
      id!=5) {
    GWEN_CRYPT_KEY *nkey;

    nkey=GWEN_Crypt_KeyRsa_fromModExp(GWEN_Crypt_Token_KeyInfo_GetKeySize(ki),
				      GWEN_Crypt_Token_KeyInfo_GetModulusData(ki),
				      GWEN_Crypt_Token_KeyInfo_GetModulusLen(ki),
				      GWEN_Crypt_Token_KeyInfo_GetExponentData(ki),
				      GWEN_Crypt_Token_KeyInfo_GetExponentLen(ki));
    assert(nkey);

    if (flags & GWEN_CRYPT_TOKEN_KEYFLAGS_HASKEYNUMBER)
      GWEN_Crypt_Key_SetKeyNumber(nkey, GWEN_Crypt_Token_KeyInfo_GetKeyNumber(ki));
    if (flags & GWEN_CRYPT_TOKEN_KEYFLAGS_HASKEYVERSION)
      GWEN_Crypt_Key_SetKeyVersion(nkey, GWEN_Crypt_Token_KeyInfo_GetKeyVersion(ki));

    /* replace public key */
    switch(id & 0xffff) {
    case 3: /* remote sign key */
      GWEN_CTF_Context_SetRemoteSignKey(ctx, nkey);
      break;
    case 4: /* remote crypt key */
      GWEN_CTF_Context_SetRemoteCryptKey(ctx, nkey);
      break;
    case 6: /* remote auth key */
      GWEN_CTF_Context_SetRemoteAuthKey(ctx, nkey);
      break;
    default:
      DBG_ERROR(GWEN_LOGDOMAIN,
		"Can't set modulus and exponent for private key");
      GWEN_Crypt_Key_free(nkey);
      return GWEN_ERROR_INVALID;
    }
    GWEN_Gui_ProgressLog(gid, GWEN_LoggerLevel_Notice,
			 I18N("Public key replaced"));
  }
  else {
    if (key) {
      if (flags & GWEN_CRYPT_TOKEN_KEYFLAGS_HASKEYNUMBER)
	GWEN_Crypt_Key_SetKeyNumber(key, GWEN_Crypt_Token_KeyInfo_GetKeyNumber(ki));
      if (flags & GWEN_CRYPT_TOKEN_KEYFLAGS_HASKEYVERSION)
	GWEN_Crypt_Key_SetKeyVersion(key, GWEN_Crypt_Token_KeyInfo_GetKeyVersion(ki));
    }
  }

  rv=GWEN_Crypt_TokenFile__WriteFile(ct, 0, gid);
  if (rv) {
    DBG_INFO(GWEN_LOGDOMAIN, "Unable to write file");
    GWEN_Gui_ProgressLog(gid, GWEN_LoggerLevel_Error,
			 I18N("Unable to write key file"));
    return rv;
  }

  GWEN_Gui_ProgressLog(gid, GWEN_LoggerLevel_Notice,
		       I18N("Key file saved"));

  return 0;
}



int GWENHYWFAR_CB
GWEN_Crypt_TokenFile__GetContextIdList(GWEN_CRYPT_TOKEN *ct,
				       uint32_t *pIdList,
				       uint32_t *pCount,
				       uint32_t gid) {
  GWEN_CRYPT_TOKEN_FILE *lct;
  GWEN_CRYPT_TOKEN_CONTEXT *ctx;
  int i;
  int rv;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, GWEN_CRYPT_TOKEN_FILE, ct);
  assert(lct);

  /* reload if needed */
  rv=GWEN_Crypt_TokenFile__ReloadIfNeeded(ct, gid);
  if (rv) {
    DBG_INFO(GWEN_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  /* count keys */
  i=0;
  ctx=GWEN_Crypt_Token_Context_List_First(lct->contextList);
  while(ctx) {
    i++;
    ctx=GWEN_Crypt_Token_Context_List_Next(ctx);
  }

  /* store number of entries */
  *pCount=i;

  /* if no buffer given just return number of keys */
  if (pIdList==NULL)
    return 0;

  if (*pCount<i) {
    DBG_INFO(GWEN_LOGDOMAIN, "Buffer too small");
    return GWEN_ERROR_BUFFER_OVERFLOW;
  }

  i=1;
  ctx=GWEN_Crypt_Token_Context_List_First(lct->contextList);
  while(ctx) {
    *(pIdList++)=i;
    ctx=GWEN_Crypt_Token_Context_List_Next(ctx);
    i++;
  }

  return 0;
}



const GWEN_CRYPT_TOKEN_CONTEXT* GWENHYWFAR_CB 
GWEN_Crypt_TokenFile__GetContext(GWEN_CRYPT_TOKEN *ct,
				 uint32_t id,
				 uint32_t gid) {
  GWEN_CRYPT_TOKEN_FILE *lct;
  GWEN_CRYPT_TOKEN_CONTEXT *ctx;
  int rv;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, GWEN_CRYPT_TOKEN_FILE, ct);
  assert(lct);

  /* reload if needed */
  rv=GWEN_Crypt_TokenFile__ReloadIfNeeded(ct, gid);
  if (rv) {
    DBG_INFO(GWEN_LOGDOMAIN, "here (%d)", rv);
    return NULL;
  }

  if (id==0) {
    DBG_INFO(GWEN_LOGDOMAIN, "Invalid context id 0");
    return NULL;
  }

  ctx=GWEN_Crypt_Token_Context_List_First(lct->contextList);
  while(ctx) {
    if (GWEN_Crypt_Token_Context_GetId(ctx)==id)
      break;
    ctx=GWEN_Crypt_Token_Context_List_Next(ctx);
  }

  if (ctx==NULL) {
    DBG_INFO(GWEN_LOGDOMAIN, "No context by id [%x] known", id);
    return NULL;
  }

  return ctx;
}



int GWENHYWFAR_CB 
GWEN_Crypt_TokenFile__SetContext(GWEN_CRYPT_TOKEN *ct,
				 uint32_t id,
				 const GWEN_CRYPT_TOKEN_CONTEXT *nctx,
				 uint32_t gid) {
  GWEN_CRYPT_TOKEN_FILE *lct;
  GWEN_CRYPT_TOKEN_CONTEXT *ctx;
  int rv;
  const char *s;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, GWEN_CRYPT_TOKEN_FILE, ct);
  assert(lct);

  if (id==0) {
    DBG_INFO(GWEN_LOGDOMAIN, "Invalid context id 0");
    return GWEN_ERROR_INVALID;
  }

  /* reload if needed */
  rv=GWEN_Crypt_TokenFile__ReloadIfNeeded(ct, gid);
  if (rv) {
    DBG_INFO(GWEN_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  ctx=GWEN_Crypt_Token_Context_List_First(lct->contextList);
  while(ctx) {
    if (GWEN_Crypt_Token_Context_GetId(ctx)==id)
      break;
    ctx=GWEN_Crypt_Token_Context_List_Next(ctx);
  }

  if (ctx==NULL) {
    DBG_INFO(GWEN_LOGDOMAIN, "No context by id [%x] known", id);
    return GWEN_ERROR_NOT_FOUND;
  }

  /* copy user data from context */
  s=GWEN_Crypt_Token_Context_GetServiceId(nctx);
  GWEN_Crypt_Token_Context_SetServiceId(ctx, s);
  s=GWEN_Crypt_Token_Context_GetUserId(nctx);
  GWEN_Crypt_Token_Context_SetUserId(ctx, s);
  s=GWEN_Crypt_Token_Context_GetUserName(nctx);
  GWEN_Crypt_Token_Context_SetUserName(ctx, s);
  s=GWEN_Crypt_Token_Context_GetPeerId(nctx);
  GWEN_Crypt_Token_Context_SetPeerId(ctx, s);
  s=GWEN_Crypt_Token_Context_GetAddress(nctx);
  GWEN_Crypt_Token_Context_SetAddress(ctx, s);
  GWEN_Crypt_Token_Context_SetPort(ctx, GWEN_Crypt_Token_Context_GetPort(nctx));
  s=GWEN_Crypt_Token_Context_GetSystemId(nctx);
  GWEN_Crypt_Token_Context_SetSystemId(ctx, s);

  return 0;
}



GWEN_CRYPT_KEY *GWEN_Crypt_TokenFile__GetKey(GWEN_CRYPT_TOKEN *ct, uint32_t id, uint32_t gid) {
  GWEN_CRYPT_TOKEN_FILE *lct;
  GWEN_CRYPT_TOKEN_CONTEXT *ctx;
  int i;
  int rv;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, GWEN_CRYPT_TOKEN_FILE, ct);
  assert(lct);

  /* reload if needed */
  rv=GWEN_Crypt_TokenFile__ReloadIfNeeded(ct, gid);
  if (rv) {
    DBG_INFO(GWEN_LOGDOMAIN, "here (%d)", rv);
    return NULL;
  }

  i=id>>16;
  ctx=GWEN_Crypt_Token_Context_List_First(lct->contextList);
  while(ctx) {
    if (i==0)
      break;
    ctx=GWEN_Crypt_Token_Context_List_Next(ctx);
    i--;
  }

  if (ctx==NULL) {
    DBG_INFO(GWEN_LOGDOMAIN, "No key by id [%x] known (context out of range)", id);
    return NULL;
  }

  switch(id & 0xffff) {
  case 1: return GWEN_CTF_Context_GetLocalSignKey(ctx);
  case 2: return GWEN_CTF_Context_GetLocalCryptKey(ctx);
  case 3: return GWEN_CTF_Context_GetRemoteSignKey(ctx);
  case 4: return GWEN_CTF_Context_GetRemoteCryptKey(ctx);
  case 5: return GWEN_CTF_Context_GetLocalAuthKey(ctx);
  case 6: return GWEN_CTF_Context_GetRemoteAuthKey(ctx);
  default:
    DBG_INFO(GWEN_LOGDOMAIN, "No key by id [%x] known (key id out of range)", id);
    return NULL;
  }
}



int GWENHYWFAR_CB
GWEN_Crypt_TokenFile__Sign(GWEN_CRYPT_TOKEN *ct,
			   uint32_t keyId,
			   GWEN_CRYPT_PADDALGO *a,
			   const uint8_t *pInData,
			   uint32_t inLen,
			   uint8_t *pSignatureData,
			   uint32_t *pSignatureLen,
			   uint32_t *pSeqCounter,
			   uint32_t gid) {
  GWEN_CRYPT_TOKEN_FILE *lct;
  GWEN_CRYPT_TOKEN_CONTEXT *ctx;
  GWEN_CRYPT_KEY *k;
  int keyNum;
  GWEN_BUFFER *srcBuf;
  int i;
  int rv;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, GWEN_CRYPT_TOKEN_FILE, ct);
  assert(lct);

  DBG_INFO(GWEN_LOGDOMAIN, "Signing with key %d", keyId);

  /* reload if needed */
  rv=GWEN_Crypt_TokenFile__ReloadIfNeeded(ct, gid);
  if (rv) {
    DBG_INFO(GWEN_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  /* get context */
  i=(keyId>>16);
  ctx=GWEN_Crypt_Token_Context_List_First(lct->contextList);
  if (ctx==NULL) {
    DBG_ERROR(GWEN_LOGDOMAIN, "Token has no context");
    return GWEN_ERROR_NOT_FOUND;
  }
  while(ctx) {
    if (i==0)
      break;
    DBG_ERROR(GWEN_LOGDOMAIN, "Checking token %d (i==%d)",
	      GWEN_Crypt_Token_Context_GetId(ctx), i);
    ctx=GWEN_Crypt_Token_Context_List_Next(ctx);
    i--;
  }

  if (ctx==NULL) {
    DBG_INFO(GWEN_LOGDOMAIN, "No context by id [%x] known", (keyId>>16) & 0xffff);
    return GWEN_ERROR_NOT_FOUND;
  }

  /* get key */
  keyNum=keyId & 0xffff;
  if (keyNum!=1 && keyNum!=5) {
    /* neither localSignKey nor localAuthKey */
    DBG_INFO(GWEN_LOGDOMAIN, "Bad key for signing (%x)", keyId);
    return GWEN_ERROR_INVALID;
  }

  k=GWEN_Crypt_TokenFile__GetKey(ct, keyId, gid);
  if (k==NULL) {
    DBG_INFO(GWEN_LOGDOMAIN, "Key not found");
    return GWEN_ERROR_NOT_FOUND;
  }

  /* copy to a buffer for padding */
  srcBuf=GWEN_Buffer_new(0, inLen, 0, 0);
  GWEN_Buffer_AppendBytes(srcBuf, (const char*)pInData, inLen);

  /* padd according to given algo */
  rv=GWEN_Padd_ApplyPaddAlgo(a, srcBuf);
  if (rv) {
    DBG_INFO(GWEN_LOGDOMAIN, "here (%d)", rv);
    GWEN_Buffer_free(srcBuf);
    return rv;
  }

  /* sign with key */
  rv=GWEN_Crypt_Key_Sign(k,
			 (const uint8_t*)GWEN_Buffer_GetStart(srcBuf),
			 GWEN_Buffer_GetUsedBytes(srcBuf),
			 pSignatureData,
			 pSignatureLen);
  GWEN_Buffer_free(srcBuf);
  if (rv) {
    DBG_INFO(GWEN_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  if (pSeqCounter) {
    GWEN_CRYPT_TOKEN_KEYINFO *ki;

    /* signature sequence counter is to be incremented */
    switch(keyId & 0xffff) {
    case 1:  ki=GWEN_CTF_Context_GetLocalSignKeyInfo(ctx); break;
    case 5:  ki=GWEN_CTF_Context_GetLocalAuthKeyInfo(ctx); break;
    default: ki=NULL;
    }
    if (ki &&
	(GWEN_Crypt_Token_KeyInfo_GetFlags(ki) & GWEN_CRYPT_TOKEN_KEYFLAGS_HASSIGNCOUNTER)) {
      unsigned int seq;

      seq=GWEN_Crypt_Token_KeyInfo_GetSignCounter(ki);
      *pSeqCounter=seq;
      GWEN_Crypt_Token_KeyInfo_SetSignCounter(ki, ++seq);

      rv=GWEN_Crypt_TokenFile__WriteFile(ct, 0, gid);
      if (rv) {
	DBG_INFO(GWEN_LOGDOMAIN, "Unable to write file");
	return rv;
      }
    }
    else {
      DBG_WARN(GWEN_LOGDOMAIN, "No sign counter for key %04x", keyId);
      *pSeqCounter=0;
    }
  }

  return 0;
}



int GWENHYWFAR_CB
GWEN_Crypt_TokenFile__Verify(GWEN_CRYPT_TOKEN *ct,
			     uint32_t keyId,
			     GWEN_CRYPT_PADDALGO *a,
			     const uint8_t *pInData,
			     uint32_t inLen,
			     const uint8_t *pSignatureData,
			     uint32_t signatureLen,
			     uint32_t seqCounter,
			     uint32_t gid) {
  GWEN_CRYPT_TOKEN_FILE *lct;
  GWEN_CRYPT_TOKEN_CONTEXT *ctx;
  GWEN_CRYPT_KEY *k;
  int keyNum;
  GWEN_BUFFER *srcBuf;
  int i;
  int rv;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, GWEN_CRYPT_TOKEN_FILE, ct);
  assert(lct);

  DBG_INFO(GWEN_LOGDOMAIN, "Verifying with key %d", keyId);

  /* reload if needed */
  rv=GWEN_Crypt_TokenFile__ReloadIfNeeded(ct, gid);
  if (rv) {
    DBG_INFO(GWEN_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  /* get context */
  i=(keyId>>16);
  ctx=GWEN_Crypt_Token_Context_List_First(lct->contextList);
  while(ctx) {
    if (i==0)
      break;
    ctx=GWEN_Crypt_Token_Context_List_Next(ctx);
    i--;
  }

  if (ctx==NULL) {
    DBG_INFO(GWEN_LOGDOMAIN, "No context by id [%x] known", (keyId>>16) & 0xffff);
    return GWEN_ERROR_NOT_FOUND;
  }

  /* get key */
  keyNum=keyId & 0xffff;
  if (keyNum!=1 && keyNum!=3 && keyNum!=6) {
    /* neither remoteSignKey nor remoteAuthKey */
    DBG_INFO(GWEN_LOGDOMAIN, "Bad key for verifying (%x)", keyId);
    return GWEN_ERROR_INVALID;
  }

  k=GWEN_Crypt_TokenFile__GetKey(ct, keyId, gid);
  if (k==NULL) {
    DBG_INFO(GWEN_LOGDOMAIN, "Key not found");
    return GWEN_ERROR_NO_KEY;
  }

  /* copy to a buffer for padding */
  srcBuf=GWEN_Buffer_new(0, inLen, 0, 0);
  GWEN_Buffer_AppendBytes(srcBuf, (const char*)pInData, inLen);

  /* padd according to given algo */
  rv=GWEN_Padd_ApplyPaddAlgo(a, srcBuf);
  if (rv) {
    DBG_INFO(GWEN_LOGDOMAIN, "here (%d)", rv);
    GWEN_Buffer_free(srcBuf);
    return rv;
  }

  /* sign with key */
  rv=GWEN_Crypt_Key_Verify(k,
			   (const uint8_t*)GWEN_Buffer_GetStart(srcBuf),
			   GWEN_Buffer_GetUsedBytes(srcBuf),
			   pSignatureData,
			   signatureLen);
  GWEN_Buffer_free(srcBuf);
  if (rv) {
    DBG_INFO(GWEN_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  if (seqCounter) {
    GWEN_CRYPT_TOKEN_KEYINFO *ki;

    /* signature sequence counter is to be checked */
    if (keyNum==3)
      ki=GWEN_CTF_Context_GetRemoteSignKeyInfo(ctx);
    else
      ki=GWEN_CTF_Context_GetRemoteAuthKeyInfo(ctx);
    if (ki &&
	(GWEN_Crypt_Token_KeyInfo_GetFlags(ki) & GWEN_CRYPT_TOKEN_KEYFLAGS_HASSIGNCOUNTER)) {
      unsigned int seq;

      seq=GWEN_Crypt_Token_KeyInfo_GetSignCounter(ki);

      if (seq>=seqCounter) {
	DBG_WARN(GWEN_LOGDOMAIN, "Bad remote sequence counter (possibly replay attack!)");
	return GWEN_ERROR_VERIFY;
      }
      GWEN_Crypt_Token_KeyInfo_SetSignCounter(ki, seqCounter);

      /* write file */
      rv=GWEN_Crypt_TokenFile__WriteFile(ct, 0, gid);
      if (rv) {
	DBG_INFO(GWEN_LOGDOMAIN, "Unable to write file");
	return rv;
      }
    }
    else {
      DBG_WARN(GWEN_LOGDOMAIN, "No sign counter for key %04x", keyId);
    }

  }

  return 0;
}



int GWENHYWFAR_CB
GWEN_Crypt_TokenFile__Encipher(GWEN_CRYPT_TOKEN *ct,
			       uint32_t keyId,
			       GWEN_CRYPT_PADDALGO *a,
			       const uint8_t *pInData,
			       uint32_t inLen,
			       uint8_t *pOutData,
			       uint32_t *pOutLen,
			       uint32_t gid) {
  GWEN_CRYPT_TOKEN_FILE *lct;
  GWEN_CRYPT_TOKEN_CONTEXT *ctx;
  GWEN_CRYPT_KEY *k;
  int keyNum;
  GWEN_BUFFER *srcBuf;
  int i;
  int rv;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, GWEN_CRYPT_TOKEN_FILE, ct);
  assert(lct);

  DBG_INFO(GWEN_LOGDOMAIN, "Enciphering with key %d", keyId);

  /* reload if needed */
  rv=GWEN_Crypt_TokenFile__ReloadIfNeeded(ct, gid);
  if (rv) {
    DBG_INFO(GWEN_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  /* get context */
  i=(keyId>>16);
  ctx=GWEN_Crypt_Token_Context_List_First(lct->contextList);
  while(ctx) {
    if (i==0)
      break;
    ctx=GWEN_Crypt_Token_Context_List_Next(ctx);
    i--;
  }

  if (ctx==NULL) {
    DBG_INFO(GWEN_LOGDOMAIN, "No context by id [%x] known", (keyId>>16) & 0xffff);
    return GWEN_ERROR_NOT_FOUND;
  }

  /* get key */
  keyNum=keyId & 0xffff;
  if (keyNum!=2 && keyNum!=4) {
    /* not remoteCryptKey */
    DBG_INFO(GWEN_LOGDOMAIN, "Bad key for encrypting (%x)", keyId);
    return GWEN_ERROR_INVALID;
  }

  k=GWEN_Crypt_TokenFile__GetKey(ct, keyId, gid);
  if (k==NULL) {
    DBG_INFO(GWEN_LOGDOMAIN, "Key %d not found", keyId);
    return GWEN_ERROR_NOT_FOUND;
  }

  /* copy to a buffer for padding */
  srcBuf=GWEN_Buffer_new(0, inLen, 0, 0);
  GWEN_Buffer_AppendBytes(srcBuf, (const char*)pInData, inLen);
  GWEN_Buffer_Rewind(srcBuf);

  /* padd according to given algo */
  rv=GWEN_Padd_ApplyPaddAlgo(a, srcBuf);
  if (rv) {
    DBG_INFO(GWEN_LOGDOMAIN, "here (%d)", rv);
    GWEN_Buffer_free(srcBuf);
    return rv;
  }

  /* encipher with key */
  rv=GWEN_Crypt_Key_Encipher(k,
			     (const uint8_t*)GWEN_Buffer_GetStart(srcBuf),
			     GWEN_Buffer_GetUsedBytes(srcBuf),
			     pOutData,
			     pOutLen);
  GWEN_Buffer_free(srcBuf);
  if (rv) {
    DBG_INFO(GWEN_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  return 0;
}



int GWENHYWFAR_CB
GWEN_Crypt_TokenFile__Decipher(GWEN_CRYPT_TOKEN *ct,
			       uint32_t keyId,
			       GWEN_CRYPT_PADDALGO *a,
			       const uint8_t *pInData,
			       uint32_t inLen,
			       uint8_t *pOutData,
			       uint32_t *pOutLen,
			       uint32_t gid) {
  GWEN_CRYPT_TOKEN_FILE *lct;
  GWEN_CRYPT_TOKEN_CONTEXT *ctx;
  GWEN_CRYPT_KEY *k;
  int keyNum;
  GWEN_BUFFER *srcBuf;
  int i;
  int rv;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, GWEN_CRYPT_TOKEN_FILE, ct);
  assert(lct);

  DBG_INFO(GWEN_LOGDOMAIN, "Deciphering with key %d", keyId);

  /* reload if needed */
  rv=GWEN_Crypt_TokenFile__ReloadIfNeeded(ct, gid);
  if (rv) {
    DBG_INFO(GWEN_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  /* get context */
  i=(keyId>>16);
  ctx=GWEN_Crypt_Token_Context_List_First(lct->contextList);
  while(ctx) {
    if (i==0)
      break;
    ctx=GWEN_Crypt_Token_Context_List_Next(ctx);
    i--;
  }

  if (ctx==NULL) {
    DBG_INFO(GWEN_LOGDOMAIN, "No context by id [%x] known", (keyId>>16) & 0xffff);
    return GWEN_ERROR_NOT_FOUND;
  }

  /* get key */
  keyNum=keyId & 0xffff;
  if (keyNum!=2 && keyNum!=4) {
    /* not localCryptKey */
    DBG_INFO(GWEN_LOGDOMAIN, "Bad key for decrypting (%x)", keyId);
    return GWEN_ERROR_INVALID;
  }

  k=GWEN_Crypt_TokenFile__GetKey(ct, keyId, gid);
  if (k==NULL) {
    DBG_INFO(GWEN_LOGDOMAIN, "Key not found");
    return GWEN_ERROR_NOT_FOUND;
  }

  /* copy to a buffer for padding */
  srcBuf=GWEN_Buffer_new(0, inLen, 0, 0);
  GWEN_Buffer_AppendBytes(srcBuf, (const char*)pInData, inLen);
  GWEN_Buffer_Rewind(srcBuf);

  /* padd according to given algo */
  rv=GWEN_Padd_UnapplyPaddAlgo(a, srcBuf);
  if (rv) {
    DBG_INFO(GWEN_LOGDOMAIN, "here (%d)", rv);
    GWEN_Buffer_free(srcBuf);
    return rv;
  }

  /* decipher with key */
  rv=GWEN_Crypt_Key_Decipher(k,
			     (const uint8_t*)GWEN_Buffer_GetStart(srcBuf),
			     GWEN_Buffer_GetUsedBytes(srcBuf),
			     pOutData,
			     pOutLen);
  GWEN_Buffer_free(srcBuf);
  if (rv) {
    DBG_INFO(GWEN_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  return 0;
}



int GWENHYWFAR_CB
GWEN_Crypt_TokenFile__GenerateKey(GWEN_CRYPT_TOKEN *ct,
				  uint32_t keyId,
				  const GWEN_CRYPT_CRYPTALGO *a,
				  uint32_t gid) {
  GWEN_CRYPT_TOKEN_FILE *lct;
  GWEN_CRYPT_KEY *pubKey;
  GWEN_CRYPT_KEY *secKey;
  int rv;
  uint32_t keyNum;
  GWEN_CRYPT_TOKEN_CONTEXT *ctx;
  int i;
  uint8_t kbuf[256];
  uint32_t klen;
  GWEN_CRYPT_TOKEN_KEYINFO *cki;
  GWEN_CRYPT_TOKEN_KEYINFO *ki;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, GWEN_CRYPT_TOKEN_FILE, ct);
  assert(lct);

  /* reload if needed */
  rv=GWEN_Crypt_TokenFile__ReloadIfNeeded(ct, gid);
  if (rv) {
    DBG_INFO(GWEN_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  keyNum=keyId & 0xffff;

  /* check key id */
  if (keyNum!=1 && keyNum!=2 && keyNum!=5) {
    DBG_INFO(GWEN_LOGDOMAIN, "Can only generate local keys.");
    GWEN_Gui_ProgressLog(gid, GWEN_LoggerLevel_Error,
			 I18N("Can only generate local keys."));
    return GWEN_ERROR_NOT_SUPPORTED;
  }

  /* check for algo */
  if (GWEN_Crypt_CryptAlgo_GetId(a)!=GWEN_Crypt_CryptAlgoId_Rsa) {
    DBG_INFO(GWEN_LOGDOMAIN, "Only RSA keys supported.");
    GWEN_Gui_ProgressLog(gid, GWEN_LoggerLevel_Error,
			 I18N("Only RSA keys supported."));
    return GWEN_ERROR_NOT_SUPPORTED;
  }

  /* get context */
  i=(keyId>>16);
  ctx=GWEN_Crypt_Token_Context_List_First(lct->contextList);
  while(ctx) {
    if (i==0)
      break;
    ctx=GWEN_Crypt_Token_Context_List_Next(ctx);
    i--;
  }

  /* generate key pair */
  rv=GWEN_Crypt_KeyRsa_GeneratePair(GWEN_Crypt_CryptAlgo_GetChunkSize(a),
				    (GWEN_Crypt_Token_GetModes(ct) &
				     GWEN_CRYPT_TOKEN_MODE_EXP_65537)?1:0,
				    &pubKey,
				    &secKey);
  if (rv) {
    DBG_INFO(GWEN_LOGDOMAIN, "here (%d)", rv);
    GWEN_Gui_ProgressLog(gid, GWEN_LoggerLevel_Error,
			 I18N("Could not generate key"));
    return rv;
  }

  GWEN_Gui_ProgressLog(gid, GWEN_LoggerLevel_Notice,
		       I18N("Key generated"));

  /* set key */
  if (keyNum==1)
    cki=GWEN_CTF_Context_GetLocalSignKeyInfo(ctx);
  else if (keyNum==3)
    cki=GWEN_CTF_Context_GetLocalCryptKeyInfo(ctx);
  else
    cki=GWEN_CTF_Context_GetLocalAuthKeyInfo(ctx);

  /* update key info for the key */
  ki=GWEN_Crypt_Token_KeyInfo_dup(cki);
  assert(ki);

  /* get modulus */
  klen=sizeof(kbuf);
  rv=GWEN_Crypt_KeyRsa_GetModulus(pubKey, kbuf, &klen);
  if (rv) {
    DBG_INFO(GWEN_LOGDOMAIN, "No modulus for key");
    GWEN_Crypt_Token_KeyInfo_free(ki);
    GWEN_Crypt_Key_free(pubKey);
    return rv;
  }
  GWEN_Crypt_Token_KeyInfo_SetModulus(ki, kbuf, klen);

  /* get exponent */
  klen=sizeof(kbuf);
  rv=GWEN_Crypt_KeyRsa_GetExponent(pubKey, kbuf, &klen);
  if (rv) {
    DBG_INFO(GWEN_LOGDOMAIN, "No exponent for key");
    GWEN_Crypt_Token_KeyInfo_free(ki);
    GWEN_Crypt_Key_free(pubKey);
    return rv;
  }
  GWEN_Crypt_Token_KeyInfo_SetExponent(ki, kbuf, klen);
  GWEN_Crypt_Token_KeyInfo_SetKeyNumber(ki, GWEN_Crypt_Key_GetKeyNumber(pubKey));
  GWEN_Crypt_Token_KeyInfo_SetKeyVersion(ki, GWEN_Crypt_Key_GetKeyVersion(pubKey));

  if (keyNum==1) {
    GWEN_CTF_Context_SetLocalSignKey(ctx, secKey);
    GWEN_CTF_Context_SetLocalSignKeyInfo(ctx, ki);
    GWEN_Crypt_Token_KeyInfo_AddFlags(ki,
				      GWEN_CRYPT_TOKEN_KEYFLAGS_HASMODULUS |
				      GWEN_CRYPT_TOKEN_KEYFLAGS_HASEXPONENT |
				      GWEN_CRYPT_TOKEN_KEYFLAGS_HASKEYNUMBER |
				      GWEN_CRYPT_TOKEN_KEYFLAGS_HASKEYVERSION |
				      GWEN_CRYPT_TOKEN_KEYFLAGS_HASSIGNCOUNTER |
				      GWEN_CRYPT_TOKEN_KEYFLAGS_HASACTIONFLAGS |
				      GWEN_CRYPT_TOKEN_KEYFLAGS_CANVERIFY |
				      GWEN_CRYPT_TOKEN_KEYFLAGS_CANSIGN);
  }
  else if (keyNum==2) {
    GWEN_CTF_Context_SetLocalCryptKey(ctx, secKey);
    GWEN_CTF_Context_SetLocalCryptKeyInfo(ctx, ki);
    GWEN_Crypt_Token_KeyInfo_AddFlags(ki,
				      GWEN_CRYPT_TOKEN_KEYFLAGS_HASMODULUS |
				      GWEN_CRYPT_TOKEN_KEYFLAGS_HASEXPONENT |
				      GWEN_CRYPT_TOKEN_KEYFLAGS_HASKEYNUMBER |
				      GWEN_CRYPT_TOKEN_KEYFLAGS_HASKEYVERSION |
				      GWEN_CRYPT_TOKEN_KEYFLAGS_HASSIGNCOUNTER |
				      GWEN_CRYPT_TOKEN_KEYFLAGS_HASACTIONFLAGS |
				      GWEN_CRYPT_TOKEN_KEYFLAGS_CANENCIPHER |
				      GWEN_CRYPT_TOKEN_KEYFLAGS_CANDECIPHER);
  }
  else {
    GWEN_CTF_Context_SetLocalAuthKey(ctx, secKey);
    GWEN_CTF_Context_SetLocalAuthKeyInfo(ctx, ki);
    GWEN_Crypt_Token_KeyInfo_AddFlags(ki,
				      GWEN_CRYPT_TOKEN_KEYFLAGS_HASMODULUS |
				      GWEN_CRYPT_TOKEN_KEYFLAGS_HASEXPONENT |
				      GWEN_CRYPT_TOKEN_KEYFLAGS_HASKEYNUMBER |
				      GWEN_CRYPT_TOKEN_KEYFLAGS_HASKEYVERSION |
				      GWEN_CRYPT_TOKEN_KEYFLAGS_HASSIGNCOUNTER |
				      GWEN_CRYPT_TOKEN_KEYFLAGS_HASACTIONFLAGS |
				      GWEN_CRYPT_TOKEN_KEYFLAGS_CANVERIFY |
				      GWEN_CRYPT_TOKEN_KEYFLAGS_CANSIGN);
  }

  /* the public key is not used */
  GWEN_Crypt_Key_free(pubKey);

  rv=GWEN_Crypt_TokenFile__WriteFile(ct, 0, gid);
  if (rv) {
    DBG_INFO(GWEN_LOGDOMAIN, "Unable to write file");
    GWEN_Gui_ProgressLog(gid, GWEN_LoggerLevel_Error,
			 I18N("Unable to write key file"));
    return rv;
  }

  GWEN_Gui_ProgressLog(gid, GWEN_LoggerLevel_Notice,
		       I18N("Key generated and set"));

  return 0;
}






GWENHYWFAR_CB
void GWEN_Crypt_TokenFile_freeData(void *bp, void *p) {
  GWEN_CRYPT_TOKEN_FILE *lct;

  lct=(GWEN_CRYPT_TOKEN_FILE*) p;
  GWEN_Crypt_Token_Context_List_free(lct->contextList);

  GWEN_FREE_OBJECT(lct);
}



GWEN_CRYPT_TOKEN *GWEN_Crypt_TokenFile_new(const char *typeName,
					   const char *tokenName) {
  GWEN_CRYPT_TOKEN *ct;
  GWEN_CRYPT_TOKEN_FILE *lct;

  ct=GWEN_Crypt_Token_new(GWEN_Crypt_Token_Device_File, typeName, tokenName);
  assert(ct);

  GWEN_NEW_OBJECT(GWEN_CRYPT_TOKEN_FILE, lct);
  lct->contextList=GWEN_Crypt_Token_Context_List_new();
  GWEN_INHERIT_SETDATA(GWEN_CRYPT_TOKEN, GWEN_CRYPT_TOKEN_FILE, ct, lct,
		       GWEN_Crypt_TokenFile_freeData);
  GWEN_Crypt_Token_SetOpenFn(ct, GWEN_Crypt_TokenFile_Open);
  GWEN_Crypt_Token_SetCreateFn(ct, GWEN_Crypt_TokenFile_Create);
  GWEN_Crypt_Token_SetCloseFn(ct, GWEN_Crypt_TokenFile_Close);
  GWEN_Crypt_Token_SetGetKeyIdListFn(ct, GWEN_Crypt_TokenFile__GetKeyIdList);
  GWEN_Crypt_Token_SetGetKeyInfoFn(ct, GWEN_Crypt_TokenFile__GetKeyInfo);
  GWEN_Crypt_Token_SetSetKeyInfoFn(ct, GWEN_Crypt_TokenFile__SetKeyInfo);
  GWEN_Crypt_Token_SetGetContextIdListFn(ct, GWEN_Crypt_TokenFile__GetContextIdList);
  GWEN_Crypt_Token_SetGetContextFn(ct, GWEN_Crypt_TokenFile__GetContext);
  GWEN_Crypt_Token_SetSetContextFn(ct, GWEN_Crypt_TokenFile__SetContext);
  GWEN_Crypt_Token_SetSignFn(ct, GWEN_Crypt_TokenFile__Sign);
  GWEN_Crypt_Token_SetVerifyFn(ct, GWEN_Crypt_TokenFile__Verify);
  GWEN_Crypt_Token_SetEncipherFn(ct, GWEN_Crypt_TokenFile__Encipher);
  GWEN_Crypt_Token_SetDecipherFn(ct, GWEN_Crypt_TokenFile__Decipher);
  GWEN_Crypt_Token_SetGenerateKeyFn(ct, GWEN_Crypt_TokenFile__GenerateKey);

  return ct;
}





