/* 
** Purpose: Implement a demo application. This app exercises the OpenSat 
**          application framework and serves as a good starting point for
**          trying new ideas.
**
** Notes:
**   None
**
** License:
**   Written by David McComas, licensed under the copyleft GNU
**   General Public License (GPL).
**
** References:
**   1. OpenSatKit Object-based Application Developers Guide.
**   2. cFS Application Developer's Guide.
**
*/

/*
** Includes
*/

#include <string.h>
#include "osk_c_demo_app.h"


/*
** Local Function Prototypes
*/

static int32 InitApp(void);
static void ProcessCommands(void);

/*
** Global Data
*/

OSK_C_DEMO_Class  OskCDemo;
OSK_C_DEMO_HkPkt  OskCDemoHkPkt;
FaultRep_TlmMsg   OskCDemoFrPkt;

/*
** Convenience Macros
*/

#define  CMDMGR_OBJ   (&(OskCDemo.CmdMgr))
#define  TBLMGR_OBJ   (&(OskCDemo.TblMgr))
#define  DEMOBJ_OBJ   (&(OskCDemo.DemObj))
#define  FAULTREP_OBJ (&(OskCDemo.FaultRep))
#define  DEMOFR_OBJ   (&(OskCDemo.DemoFr))

/******************************************************************************
** Function: OSK_C_DEMO_Main
**
*/
void OSK_C_DEMO_AppMain(void)
{

   int32  Status    = CFE_SEVERITY_ERROR;
   uint32 RunStatus = CFE_ES_APP_ERROR;


   CFE_ES_PerfLogEntry(OSK_C_DEMO_MAIN_PERF_ID);
   Status = CFE_ES_RegisterApp();
   CFE_EVS_Register(NULL,0,0);

   /*
   ** Perform application specific initialization
   */
   if (Status == CFE_SUCCESS)
   {
       Status = InitApp();
   }

   /*
   ** At this point many flight apps use CFE_ES_WaitForStartupSync() to
   ** synchronize their startup timing with other apps. This is not
   ** needed for this demo.
   */

   if (Status == CFE_SUCCESS) RunStatus = CFE_ES_APP_RUN;

   /*
   ** Main process loop
   */
   while (CFE_ES_RunLoop(&RunStatus))
   {

      /*
      ** This is just a an example loop. There are many ways to control the
      ** main loop execution.
      */

      CFE_ES_PerfLogExit(OSK_C_DEMO_MAIN_PERF_ID);
      OS_TaskDelay(OSK_C_DEMO_RUNLOOP_DELAY);
      CFE_ES_PerfLogEntry(OSK_C_DEMO_MAIN_PERF_ID);

      DEMOBJ_Execute();
      DEMOFR_SimStep();
      
      FaultRep_GenTlmMsg(FAULTREP_OBJ, &OskCDemoFrPkt);
      CFE_SB_TimeStampMsg((CFE_SB_Msg_t *) &OskCDemoFrPkt);
      CFE_SB_SendMsg((CFE_SB_Msg_t *) &OskCDemoFrPkt);

      ProcessCommands();

   } /* End CFE_ES_RunLoop */


   /* Write to system log in case events not working */

   CFE_ES_WriteToSysLog("OSK_C_DEMO Terminating, RunLoop status = 0x%08X\n", RunStatus);

   CFE_EVS_SendEvent(OSK_C_DEMO_EXIT_ERR_EID, CFE_EVS_CRITICAL, "OSK_C_DEMO Terminating,  RunLoop status = 0x%08X", RunStatus);

   CFE_ES_PerfLogExit(OSK_C_DEMO_MAIN_PERF_ID);
   CFE_ES_ExitApp(RunStatus);  /* Let cFE kill the task (and any child tasks) */

} /* End of OSK_C_DEMO_Main() */


/******************************************************************************
** Function: OSK_C_DEMO_NoOpCmd
**
*/

boolean OSK_C_DEMO_NoOpCmd(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr)
{

   CFE_EVS_SendEvent (OSK_C_DEMO_CMD_NOOP_INFO_EID,
                      CFE_EVS_INFORMATION,
                      "No operation command received for OSK_C_DEMO version %d.%d.%d",
                      OSK_C_DEMO_MAJOR_VER,OSK_C_DEMO_MINOR_VER,OSK_C_DEMO_LOCAL_REV);

   return TRUE;


} /* End OSK_C_DEMO_NoOpCmd() */


/******************************************************************************
** Function: OSK_C_DEMO_ResetAppCmd
**
*/

boolean OSK_C_DEMO_ResetAppCmd(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr)
{

   CMDMGR_ResetStatus(CMDMGR_OBJ);
   TBLMGR_ResetStatus(TBLMGR_OBJ);
   /* No FaultRep_ResetStatus */
   
   DEMOBJ_ResetStatus();
   DEMOFR_ResetStatus();
  
   return TRUE;

} /* End OSK_C_DEMO_ResetAppCmd() */


/******************************************************************************
** Function: OSK_C_DEMO_SendHousekeepingPkt
**
*/
void OSK_C_DEMO_SendHousekeepingPkt(void)
{

   const TBLMGR_Tbl* Tbl = TBLMGR_GetLastTblStatus(TBLMGR_OBJ);
      
   /*
   ** CMDMGR Data
   */

   OskCDemoHkPkt.ValidCmdCnt   = OskCDemo.CmdMgr.ValidCmdCnt;
   OskCDemoHkPkt.InvalidCmdCnt = OskCDemo.CmdMgr.InvalidCmdCnt;

   
   /*
   ** EXTBL Data
   */
 
   OskCDemoHkPkt.LastTblId           = Tbl->Id;
   OskCDemoHkPkt.LastTblAction       = Tbl->LastAction;
   OskCDemoHkPkt.LastTblActionStatus = Tbl->LastActionStatus;

   /*
   ** DEMOFR & FaultRep Data
   */
   
   OskCDemoHkPkt.FaultRepTlmMode  = OskCDemo.FaultRep.TlmMode;
   OskCDemoHkPkt.SimEnabled       = OskCDemo.DemoFr.SimEnabled;
   OskCDemoHkPkt.SimMode          = OskCDemo.DemoFr.SimMode;
   
   OskCDemoHkPkt.FaultRepEnabled  = OskCDemo.FaultRep.FaultDet.Enabled[0];
   OskCDemoHkPkt.FaultRepLatched  = OskCDemo.FaultRep.FaultDet.Latched[0];
  

   /*
   ** DEMOBJ Data
   ** - At a minimum all OBJECT variables effected by a reset must be included
   */

   OskCDemoHkPkt.TblDataEnabled = DEMOBJ_GetTblData(&OskCDemoHkPkt.TblData);
   OskCDemoHkPkt.TblId    = OskCDemo.DemObj.TblId;
   OskCDemoHkPkt.TblIndex = OskCDemo.DemObj.TblIndex;

   CFE_SB_TimeStampMsg((CFE_SB_Msg_t *) &OskCDemoHkPkt);
   CFE_SB_SendMsg((CFE_SB_Msg_t *) &OskCDemoHkPkt);

} /* End OSK_C_DEMO_SendHousekeepingPkt() */


/******************************************************************************
** Function: InitApp
**
*/
static int32 InitApp(void)
{

   int32 Status = CFE_SUCCESS;

   CFE_EVS_SendEvent(OSK_C_DEMO_INIT_DEBUG_EID, OSK_C_DEMO_INIT_EVS_TYPE, "OSK_C_DEMO_InitApp() Entry\n");
    
   /*
   ** Initialize 'entity' objects
   */
 
    DEMOBJ_Constructor(DEMOBJ_OBJ);
    DEMOFR_Constructor(DEMOFR_OBJ, FAULTREP_OBJ);

    /*
    ** Initialize application managers
    */

    CFE_SB_CreatePipe(&OskCDemo.CmdPipe, OSK_C_DEMO_CMD_PIPE_DEPTH, OSK_C_DEMO_CMD_PIPE_NAME);
    CFE_SB_Subscribe(OSK_C_DEMO_CMD_MID, OskCDemo.CmdPipe);
    CFE_SB_Subscribe(OSK_C_DEMO_SEND_HK_MID, OskCDemo.CmdPipe);

    CFE_EVS_SendEvent(OSK_C_DEMO_INIT_DEBUG_EID, OSK_C_DEMO_INIT_EVS_TYPE, "OSK_C_DEMO_InitApp() Before CMDMGR calls\n");
    CMDMGR_Constructor(CMDMGR_OBJ);
    CMDMGR_RegisterFunc(CMDMGR_OBJ, CMDMGR_NOOP_CMD_FC,  NULL, OSK_C_DEMO_NoOpCmd,     0);
    CMDMGR_RegisterFunc(CMDMGR_OBJ, CMDMGR_RESET_CMD_FC, NULL, OSK_C_DEMO_ResetAppCmd, 0);
    
    CMDMGR_RegisterFunc(CMDMGR_OBJ, OSK_C_DEMO_DEMOBJ_TBL_LOAD_CMD_FC,     TBLMGR_OBJ,   TBLMGR_LoadTblCmd,          TBLMGR_LOAD_TBL_CMD_DATA_LEN);
    CMDMGR_RegisterFunc(CMDMGR_OBJ, OSK_C_DEMO_DEMOBJ_TBL_DUMP_CMD_FC,     TBLMGR_OBJ,   TBLMGR_DumpTblCmd,          TBLMGR_DUMP_TBL_CMD_DATA_LEN);
    CMDMGR_RegisterFunc(CMDMGR_OBJ, OSK_C_DEMO_ENA_TBL_DATA_CMD_FC,        DEMOBJ_OBJ,   DEMOBJ_EnableTblDataCmd,    DEMOBJ_ENABLE_TBL_DATA_CMD_DATA_LEN);
    CMDMGR_RegisterFunc(CMDMGR_OBJ, OSK_C_DEMO_SET_ACTIVE_TBL_CMD_FC,      DEMOBJ_OBJ,   DEMOBJ_SetActiveTblCmd,     DEMOBJ_SET_ACTIVE_TBL_CMD_DATA_LEN);
    CMDMGR_RegisterFunc(CMDMGR_OBJ, OSK_C_DEMO_FAULTREP_CFG_CMD_FC,        FAULTREP_OBJ, FaultRep_ConfigFaultDetCmd, FAULTREP_CFG_FAULT_DET_CMD_DATA_LEN);
    CMDMGR_RegisterFunc(CMDMGR_OBJ, OSK_C_DEMO_FAULTREP_CLR_CMD_FC,        FAULTREP_OBJ, FaultRep_ClearFaultDetCmd,  FAULTREP_CLR_FAULT_DET_CMD_DATA_LEN);
    CMDMGR_RegisterFunc(CMDMGR_OBJ, OSK_C_DEMO_DEMOFR_SET_TLM_MODE_CMD_FC, DEMOFR_OBJ,   DEMOFR_SetTlmModeCmd,       DEMOFR_SET_TLM_MODE_CMD_DATA_LEN);
    CMDMGR_RegisterFunc(CMDMGR_OBJ, OSK_C_DEMO_DEMOFR_SIM_FAULT_CMD_FC,    DEMOFR_OBJ,   DEMOFR_SimFaultCmd,         DEMOFR_SIM_FAULT_CMD_DATA_LEN);

    CFE_EVS_SendEvent(OSK_C_DEMO_INIT_DEBUG_EID, OSK_C_DEMO_INIT_EVS_TYPE, "OSK_C_DEMO_InitApp() Before TBLMGR calls\n");
    
    /*
    ** Table IDs are assigned during registration so the order is important. COSMOS cmd/tlm definitions assume
    ** 0=XML, 1=SCANF, 2=JSON.
    */
    TBLMGR_Constructor(TBLMGR_OBJ);
    TBLMGR_RegisterTblWithDef(TBLMGR_OBJ, XMLTBL_LoadCmd,   XMLTBL_DumpCmd,   OSK_C_DEMO_XML_TBL_DEF_LOAD_FILE);
    TBLMGR_RegisterTblWithDef(TBLMGR_OBJ, SCANFTBL_LoadCmd, SCANFTBL_DumpCmd, OSK_C_DEMO_SCANF_TBL_DEF_LOAD_FILE);
    TBLMGR_RegisterTblWithDef(TBLMGR_OBJ, JSONTBL_LoadCmd,  JSONTBL_DumpCmd,  OSK_C_DEMO_JSON_TBL_DEF_LOAD_FILE);

    FaultRep_Constructor(FAULTREP_OBJ, OSK_C_DEMO_FAULT_ID_MAX);
    
    CFE_SB_InitMsg(&OskCDemoFrPkt, OSK_C_DEMO_TLM_FR_MID, FAULTREP_FAULT_REP_PKT_LEN, TRUE);
                                          
    CFE_SB_InitMsg(&OskCDemoHkPkt, OSK_C_DEMO_TLM_HK_MID, OSK_C_DEMO_TLM_HK_LEN, TRUE);

    /*
    ** Application startup event message
    */
    Status = CFE_EVS_SendEvent(OSK_C_DEMO_INIT_INFO_EID,
                               CFE_EVS_INFORMATION,
                               "OSK_C_DEMO Initialized. Version %d.%d.%d",
                               OSK_C_DEMO_MAJOR_VER,
                               OSK_C_DEMO_MINOR_VER,
                               OSK_C_DEMO_LOCAL_REV);

    return(Status);

} /* End of InitApp() */


/******************************************************************************
** Function: ProcessCommands
**
*/
static void ProcessCommands(void)
{

   int32           Status;
   CFE_SB_Msg_t*   CmdMsgPtr;
   CFE_SB_MsgId_t  MsgId;

   Status = CFE_SB_RcvMsg(&CmdMsgPtr, OskCDemo.CmdPipe, CFE_SB_POLL);

   if (Status == CFE_SUCCESS)
   {

      MsgId = CFE_SB_GetMsgId(CmdMsgPtr);

      switch (MsgId)
      {
         case OSK_C_DEMO_CMD_MID:
            CMDMGR_DispatchFunc(CMDMGR_OBJ, CmdMsgPtr);
            break;

         case OSK_C_DEMO_SEND_HK_MID:
            CFETBL_Manage();
            OSK_C_DEMO_SendHousekeepingPkt();
            break;

         default:
            CFE_EVS_SendEvent(OSK_C_DEMO_CMD_INVALID_MID_ERR_EID, CFE_EVS_ERROR,
                              "Received invalid command packet,MID = 0x%4X",MsgId);

            break;

      } /* End Msgid switch */

   } /* End if SB received a packet */

} /* End ProcessCommands() */
