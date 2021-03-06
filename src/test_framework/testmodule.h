/**********************************************************
 * This file has been automatically created by "typemaker2"
 * from the file "testmodule.xml".
 * Please do not edit this file, all changes will be lost.
 * Better edit the mentioned source file instead.
 **********************************************************/

#ifndef GWEN_TEST_MODULE_TESTMODULE_H
#define GWEN_TEST_MODULE_TESTMODULE_H


#ifdef __cplusplus
extern "C" {
#endif

/** @page P_GWEN_TEST_MODULE Structure GWEN_TEST_MODULE
<p>This page describes the properties of GWEN_TEST_MODULE.</p>



<h1>GWEN_TEST_MODULE</h1>



@anchor GWEN_TEST_MODULE_id
<h2>id</h2>

<p>Set this property with @ref GWEN_Test_Module_SetId(), get it with @ref GWEN_Test_Module_GetId().</p>


@anchor GWEN_TEST_MODULE_name
<h2>name</h2>

<p>Set this property with @ref GWEN_Test_Module_SetName(), get it with @ref GWEN_Test_Module_GetName().</p>


@anchor GWEN_TEST_MODULE_description
<h2>description</h2>

<p>Set this property with @ref GWEN_Test_Module_SetDescription(), get it with @ref GWEN_Test_Module_GetDescription().</p>


@anchor GWEN_TEST_MODULE_result
<h2>result</h2>

<p>Set this property with @ref GWEN_Test_Module_SetResult(), get it with @ref GWEN_Test_Module_GetResult().</p>


@anchor GWEN_TEST_MODULE_paramsDb
<h2>paramsDb</h2>

<p>Set this property with @ref GWEN_Test_Module_SetParamsDb(), get it with @ref GWEN_Test_Module_GetParamsDb().</p>

*/

/* needed system headers */
#include <gwenhywfar/types.h>
#include <gwenhywfar/tree2.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/db.h>

/* pre-headers */
#include <gwenhywfar/types.h>

typedef struct GWEN_TEST_MODULE GWEN_TEST_MODULE;
GWEN_TREE2_FUNCTION_LIB_DEFS(GWEN_TEST_MODULE, GWEN_Test_Module, GWENHYWFAR_API)
GWEN_INHERIT_FUNCTION_LIB_DEFS(GWEN_TEST_MODULE, GWENHYWFAR_API)



/* post-headers */


/* definitions for virtual functions (post) */
typedef int GWENHYWFAR_CB(*GWEN_TEST_MODULE_TEST_FN)(GWEN_TEST_MODULE *p_struct);

/** Constructor. */
GWENHYWFAR_API GWEN_TEST_MODULE *GWEN_Test_Module_new(void);

/** Destructor. */
GWENHYWFAR_API void GWEN_Test_Module_free(GWEN_TEST_MODULE *p_struct);

GWENHYWFAR_API void GWEN_Test_Module_Attach(GWEN_TEST_MODULE *p_struct);

GWENHYWFAR_API GWEN_TEST_MODULE *GWEN_Test_Module_dup(const GWEN_TEST_MODULE *p_struct);

GWENHYWFAR_API GWEN_TEST_MODULE *GWEN_Test_Module_copy(GWEN_TEST_MODULE *p_struct, const GWEN_TEST_MODULE *p_src);

/** Getter.
 * Use this function to get the member "id" (see @ref GWEN_TEST_MODULE_id)
*/
GWENHYWFAR_API uint32_t GWEN_Test_Module_GetId(const GWEN_TEST_MODULE *p_struct);

/** Getter.
 * Use this function to get the member "name" (see @ref GWEN_TEST_MODULE_name)
*/
GWENHYWFAR_API const char *GWEN_Test_Module_GetName(const GWEN_TEST_MODULE *p_struct);

/** Getter.
 * Use this function to get the member "description" (see @ref GWEN_TEST_MODULE_description)
*/
GWENHYWFAR_API const char *GWEN_Test_Module_GetDescription(const GWEN_TEST_MODULE *p_struct);

/** Getter.
 * Use this function to get the member "result" (see @ref GWEN_TEST_MODULE_result)
*/
GWENHYWFAR_API int GWEN_Test_Module_GetResult(const GWEN_TEST_MODULE *p_struct);

/** Setter.
 * Use this function to set the member "id" (see @ref GWEN_TEST_MODULE_id)
*/
GWENHYWFAR_API void GWEN_Test_Module_SetId(GWEN_TEST_MODULE *p_struct, uint32_t p_src);

/** Setter.
 * Use this function to set the member "name" (see @ref GWEN_TEST_MODULE_name)
*/
GWENHYWFAR_API void GWEN_Test_Module_SetName(GWEN_TEST_MODULE *p_struct, const char *p_src);

/** Setter.
 * Use this function to set the member "description" (see @ref GWEN_TEST_MODULE_description)
*/
GWENHYWFAR_API void GWEN_Test_Module_SetDescription(GWEN_TEST_MODULE *p_struct, const char *p_src);

/** Setter.
 * Use this function to set the member "result" (see @ref GWEN_TEST_MODULE_result)
*/
GWENHYWFAR_API void GWEN_Test_Module_SetResult(GWEN_TEST_MODULE *p_struct, int p_src);

/* prototypes for virtual functions */
/**
 * Returns the list of ABS_ACCOUNT_INFO objects for all known accounts. The caller is responsible for freeing the list returned (if any) via @ref ABS_AccountInfo_List_free.
 */
GWENHYWFAR_API int GWEN_Test_Module_Test(GWEN_TEST_MODULE *p_struct);

/* setters for virtual functions */
GWENHYWFAR_API GWEN_TEST_MODULE_TEST_FN GWEN_Test_Module_SetTestFn(GWEN_TEST_MODULE *p_struct,
                                                                   GWEN_TEST_MODULE_TEST_FN fn);

GWENHYWFAR_API void GWEN_Test_Module_ReadDb(GWEN_TEST_MODULE *p_struct, GWEN_DB_NODE *p_db);

GWENHYWFAR_API int GWEN_Test_Module_WriteDb(const GWEN_TEST_MODULE *p_struct, GWEN_DB_NODE *p_db);

GWENHYWFAR_API GWEN_TEST_MODULE *GWEN_Test_Module_fromDb(GWEN_DB_NODE *p_db);

GWENHYWFAR_API int GWEN_Test_Module_toDb(const GWEN_TEST_MODULE *p_struct, GWEN_DB_NODE *p_db);

GWENHYWFAR_API GWEN_TEST_MODULE *GWEN_Test_Module_Tree2_GetById(const GWEN_TEST_MODULE *p_object, uint32_t p_cmp);

GWENHYWFAR_API GWEN_TEST_MODULE *GWEN_Test_Module_AddTest(GWEN_TEST_MODULE *st, const char *tName,
                                                          GWEN_TEST_MODULE_TEST_FN fn, const char *tDescr);
GWENHYWFAR_API GWEN_TEST_MODULE *GWEN_Test_Module_AddModule(GWEN_TEST_MODULE *st, const char *tName,
                                                            const char *tDescr);
GWENHYWFAR_API const char *GWEN_Test_Module_GetCharParam(const GWEN_TEST_MODULE *st, const char *paramName,
                                                         const char *defVal);
GWENHYWFAR_API void GWEN_Test_Module_SetCharParam(GWEN_TEST_MODULE *st, const char *paramName, const char *val);
/* end-headers */


#ifdef __cplusplus
}
#endif

#endif

