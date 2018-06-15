/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*! \file rrc_UE.c
 * \brief rrc procedures for UE
 * \author Navid Nikaein and Raymond Knopp
 * \date 2011 - 2014
 * \version 1.0
 * \company Eurecom
 * \email: navid.nikaein@eurecom.fr and raymond.knopp@eurecom.fr
 */

#define RRC_UE
#define RRC_UE_C

#include "rrc_list.h"
//  header files for RRC message for NR might be change to add prefix in from of the file name.
#include "assertions.h"
#include "hashtable.h"
#include "asn1_conversions.h"
#include "defs.h"
#include "PHY/TOOLS/dB_routines.h"
#include "extern.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"
#include "LAYER2/RLC/rlc.h"
#include "COMMON/mac_rrc_primitives.h"
#include "UTIL/LOG/log.h"
#include "UTIL/LOG/vcd_signal_dumper.h"
#ifndef CELLULAR
#include "RRC/LITE/MESSAGES/asn1_msg.h"
#endif
#include "RRCConnectionRequest.h"
#include "RRCConnectionReconfiguration.h"
#include "UL-CCCH-Message.h"
#include "DL-CCCH-Message.h"
#include "UL-DCCH-Message.h"
#include "DL-DCCH-Message.h"
#include "BCCH-DL-SCH-Message.h"
#include "PCCH-Message.h"
#if defined(Rel10) || defined(Rel14)
#include "MCCH-Message.h"
#endif
#include "MeasConfig.h"
#include "MeasGapConfig.h"
#include "MeasObjectEUTRA.h"
#include "TDD-Config.h"
#include "UECapabilityEnquiry.h"
#include "UE-CapabilityRequest.h"
#include "RRC/NAS/nas_config.h"
#include "RRC/NAS/rb_config.h"
#if ENABLE_RAL
#include "rrc_UE_ral.h"
#endif

#if defined(ENABLE_SECURITY)
# include "UTIL/OSA/osa_defs.h"
#endif


#if defined(ENABLE_ITTI)
#include "intertask_interface.h"
#endif



// from NR SRB3
uint8_t nr_rrc_ue_decode_dcch(
    const uint8_t *buffer,
    const uint32_t size
){
    //  uper_decode by nr R15 rrc_connection_reconfiguration
    
    NR_DL_DCCH_Message_t *nr_dl_dcch_msg = (NR_DL_DCCH_Message_t *)0;

    uper_decode(NULL,
                &asn_DEF_NR_DL_DCCH_Message,    
                (void**)&nr_dl_dcch_msg,
                (uint8_t *)buffer,
                size, 0, 0); 

    if(nr_dl_dcch_msg != NULL){
        switch(nr_dl_dcch_msg->message.present){            
            case DL_DCCH_MessageType_PR_c1:

                switch(nr_dl_dcch_msg->message.choice.c1.present){
                    case DL_DCCH_MessageType__c1_PR_rrcReconfiguration:
                        nr_rrc_ue_process_rrcReconfiguration(&nr_dl_dcch_msg->message.choice.c1.choice.rrcReconfiguration);
                        break;

                    case DL_DCCH_MessageType__c1_PR_NOTHING:
                    case DL_DCCH_MessageType__c1_PR_spare15:
                    case DL_DCCH_MessageType__c1_PR_spare14:
                    case DL_DCCH_MessageType__c1_PR_spare13:
                    case DL_DCCH_MessageType__c1_PR_spare12:
                    case DL_DCCH_MessageType__c1_PR_spare11:
                    case DL_DCCH_MessageType__c1_PR_spare10:
                    case DL_DCCH_MessageType__c1_PR_spare9:
                    case DL_DCCH_MessageType__c1_PR_spare8:
                    case DL_DCCH_MessageType__c1_PR_spare7:
                    case DL_DCCH_MessageType__c1_PR_spare6:
                    case DL_DCCH_MessageType__c1_PR_spare5:
                    case DL_DCCH_MessageType__c1_PR_spare4:
                    case DL_DCCH_MessageType__c1_PR_spare3:
                    case DL_DCCH_MessageType__c1_PR_spare2:
                    case DL_DCCH_MessageType__c1_PR_spare1:
                    default:
                        //  not support or unuse
                        break;
                }   
                break;
            case DL_DCCH_MessageType_PR_NOTHING:
            case DL_DCCH_MessageType_PR_messageClassExtension:
            default:
                //  not support or unuse
                break;
        }
        
        //  release memory allocation
        free(nr_dl_dcch_msg);
    }else{
        //  log..
    }

    return 0;

}

// from LTE-RRC DL-DCCH RRCConnectionReconfiguration nr-secondary-cell-group-config (encoded)
uint8_t nr_rrc_ue_decode_secondary_cellgroup_config(
    const uint8_t *buffer,
    const uint32_t size
){
    NR_CellGroupConfig_t *cellGroupConfig = (NR_CellGroupConfig_t *)0;

    uper_decode(NULL,
                &asn_DEF_CellGroupConfig,   //might be added prefix later
                (void **)&cellGroupConfig,
                (uint8_t *)buffer,
                size, 0, 0); 

    if(NR_UE_rrc_inst->cell_group_config == (NR_CellGroupConfig_t *)0){
        NR_UE_rrc_inst->cell_group_config = cellGroupConfig;
        nr_rrc_ue_process_scg_config(cellGroupConfig);
    }else{
        nr_rrc_ue_process_scg_config(cellGroupConfig);
        asn_DEF_CellGroupConfig.free_struct(asn_DEF_CellGroupConfig, cellGroupConfig, 0);
    }

    nr_rrc_mac_config_req_ue(); 
}


// from LTE-RRC DL-DCCH RRCConnectionReconfiguration nr-secondary-cell-group-config (decoded)
// RRCReconfiguration
uint8_t nr_rrc_ue_process_rrcReconfiguration(NR_RRCReconfiguration_t *rrcReconfiguration){

    switch(rrcReconfiguration.criticalExtensions.present){
        case RRCReconfiguration__criticalExtensions_PR_rrcReconfiguration:
            if(rrcReconfiguration.criticalExtensions.rrcReconfiguration->radioBearerConfig != (NR_RadioBearerConfig_t *)0){
                if(NR_UE_rrc_inst->radio_bearer_config == (NR_RadioBearerConfig_t *)0){
                    NR_UE_rrc_inst->radio_bearer_config = rrcReconfiguration->radioBearerConfig;                
                }else{
                    nr_rrc_ue_process_radio_bearer_config(rrcReconfiguration->radioBearerConfig);
                }
            }

            if(rrcReconfiguration.criticalExtensions.rrcReconfiguration->secondaryCellGroup != (OCTET_STRING_t *)0){
                NR_CellGroupConfig_t *cellGroupConfig = (NR_CellGroupConfig_t *)0;
                uper_decode(NULL,
                            &asn_DEF_CellGroupConfig,   //might be added prefix later
                            (void **)&cellGroupConfig,
                            (uint8_t *)rrcReconfiguration->secondaryCellGroup->buffer,
                            rrcReconfiguration->secondaryCellGroup.size, 0, 0); 

                if(NR_UE_rrc_inst->cell_group_config == (NR_CellGroupConfig_t *)0){
                    //  first time receive the configuration, just use the memory allocated from uper_decoder. TODO this is not good implementation, need to maintain RRC_INST own structure every time.
                    NR_UE_rrc_inst->cell_group_config = cellGroupConfig;
                    nr_rrc_ue_process_scg_config(cellGroupConfig);
                }else{
                    //  after first time, update it and free the memory after.
                    nr_rrc_ue_process_scg_config(cellGroupConfig);
                    asn_DEF_CellGroupConfig.free_struct(asn_DEF_CellGroupConfig, cellGroupConfig, 0);
                }
                
            }

            if(rrcReconfiguration.criticalExtensions.rrcReconfiguration->measConfig != (MeasConfig *)0){
                if(NR_UE_rrc_inst->meas_config == (NR_MeasConfig_t *)0){
                    NR_UE_rrc_inst->meas_config = rrcReconfiguration->measConfig;
                }else{
                    //  if some element need to be updated
                    nr_rrc_ue_process_meas_config(rrcReconfiguration->measConfig);
                }
               
            }

            if(rrcReconfiguration.criticalExtensions.rrcReconfiguration->lateNonCriticalExtension != (OCTET_STRING_t *)0){
                //  unuse now
            }

            if(rrcReconfiguration.criticalExtensions.rrcReconfiguration->nonCriticalExtension != (RRCReconfiguration_IEs__nonCriticalExtension *)0){
                // unuse now
            }
            break;
        case RRCReconfiguration__criticalExtensions_PR_NOTHING:
        case RRCReconfiguration__criticalExtensions_PR_criticalExtensionsFuture:
        default:
            break;
    }
    nr_rrc_mac_config_req_ue(); 
}

uint8_t nr_rrc_ue_process_meas_config(NR_MeasConfig_t *meas_config){

}

uint8_t nr_rrc_ue_process_scg_config(NR_CellGroupConfig_t *cell_group_config){
    int i;
    void *listobj;
    if(NR_UE_rrc_inst->cell_group_config==NULL){
        //  initial list
      
        if(cell_group_config->spCellConfig != NULL){
            if(cell_group_config->spCellConfig->spCellConfigDedicated != NULL){
                if(cell_group_config->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList != NULL){
                    listobj = cell_group_config->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList;
                    for(i=0; i<((struct NR_ServingCellConfig__downlinkBWP_ToAddModList)listobj)->list.count; ++i){
                        RRC_LIST_MOD_ADD(NR_UE_rrc_inst.BWP_Downlink_list, ((struct NR_ServingCellConfig__downlinkBWP_ToAddModList)listobj)->list.array[i]);
                    }
                }
            }
        } 
       

    }else{
        //  maintain list
        if(cell_group_config->spCellConfig != NULL){
            if(cell_group_config->spCellConfig->spCellConfigDedicated != NULL){
                //  process element of list to be add by RRC message
                if(cell_group_config->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList != NULL){
                    listobj = cell_group_config->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList;
                    for(i=0; i<((struct NR_ServingCellConfig__downlinkBWP_ToAddModList)listobj)->list.count; ++i){
                        RRC_LIST_MOD_ADD(NR_UE_rrc_inst.BWP_Downlink_list, ((struct NR_ServingCellConfig__downlinkBWP_ToAddModList)listobj)->list.array[i]);
                    }
                }
                

                //  process element of list to be release by RRC message
                if(cell_group_config->spCellConfig->spCellconfigDedicated->downlinkBWP_ToReleaseList != NULL){
                    listobj = cell_group_config->spCellConfig->spCellConfigDedicated->downlinkBWP_ToReleaseList;
                    for(i=0; i<((struct NR_ServingCellconfig__downlinkBWP_ToReleaseList)listobj)->list.count; ++i){
                        RRC_LIST_MOD_REL(NR_UE_rrc_inst.BWP_Downlink_list, ((struct NR_ServingCellConfig__downlinkBWP_ToReleaseList)listlbj)->list.array[i]);
                    }
                }
            }
        }
    } 
}
uint8_t nr_rrc_ue_process_radio_bearer_config(NR_RadioBearerConfig_t *radio_bearer_config){

}


uint8_t openair_rrc_top_init_ue_nr(void){

    if(NB_NR_UE_INST > 0){
        NR_UE_rrc_inst = (NR_UE_RRC_INST_t *)malloc(NB_NR_UE_INST * sizeof(NR_UE_RRC_INST_t));
        memset(NR_UE_rrc_inst, 0, NB_NR_UE_INST * sizeof(NR_UE_RRC_INST_t));

        // fill UE-NR-Capability @ UE-CapabilityRAT-Container here.

        //  init RRC lists
        RRC_LIST_INIT(NR_UE_rrc_inst->RLC_Bearer_Config_list, NR_maxLC_ID);
        RRC_LIST_INIT(NR_UE_rrc_inst->SchedulingRequest_list, NR_maxNrofSR_ConfigPerCellGroup);
        RRC_LIST_INIT(NR_UE_rrc_inst->TAG_list, NR_maxNrofTAGs);
        RRC_LIST_INIT(NR_UE_rrc_inst->TDD_UL_DL_SlotConfig_list, NR_maxNrofSlots);
        RRC_LIST_INIT(NR_UE_rrc_inst->BWP_Downlink_list, NR_maxNrofBWPs);
        RRC_LIST_INIT(NR_UE_rrc_inst->ControlResourceSet_list[0], 3);   //  for init-dl-bwp
        RRC_LIST_INIT(NR_UE_rrc_inst->ControlResourceSet_list[1], 3);   //  for dl-bwp id=0
        RRC_LIST_INIT(NR_UE_rrc_inst->ControlResourceSet_list[2], 3);   //  for dl-bwp id=1
        RRC_LIST_INIT(NR_UE_rrc_inst->ControlResourceSet_list[3], 3);   //  for dl-bwp id=2
        RRC_LIST_INIT(NR_UE_rrc_inst->ControlResourceSet_list[4], 3);   //  for dl-bwp id=3
        RRC_LIST_INIT(NR_UE_rrc_inst->SearchSpace_list, 10);
        RRC_LIST_INIT(NR_UE_rrc_inst->SlotFormatCombinationsPerCell_list, NR_maxNrofAggregatedCellsPerCellGroup);
        RRC_LIST_INIT(NR_UE_rrc_inst->TCI_State_list, NR_maxNrofTCI_States);
        RRC_LIST_INIT(NR_UE_rrc_inst->RateMatchPattern_list, NR_maxNrofRateMatchPatterns);
        RRC_LIST_INIT(NR_UE_rrc_inst->ZP_CSI_RS_Resource_list, NR_maxNrofZP_CSI_RS_Resources);
        RRC_LIST_INIT(NR_UE_rrc_inst->Aperidic_ZP_CSI_RS_ResourceSet_list, NR_maxNrofZP_CSI_RS_Sets);
        RRC_LIST_INIT(NR_UE_rrc_inst->SP_ZP_CSI_RS_ResourceSet_list, NR_maxNrofZP_CSI_RS_Sets);
        RRC_LIST_INIT(NR_UE_rrc_inst->NZP_CSI_RS_Resource_list, NR_maxNrofNZP_CSI_RS_Resources);
        RRC_LIST_INIT(NR_UE_rrc_inst->NZP_CSI_RS_ResourceSet_list, NR_maxNrofNZP_CSI_RS_ResourceSets);
        RRC_LIST_INIT(NR_UE_rrc_inst->CSI_IM_Resource_list, NR_maxNrofCSI_IM_Resources);
        RRC_LIST_INIT(NR_UE_rrc_inst->CSI_IM_ResourceSet_list, NR_maxNrofCSI_IM_ResourceSets);
        RRC_LIST_INIT(NR_UE_rrc_inst->CSI_SSB_ResourceSet_list, NR_maxNrofCSI_SSB_ResourceSets);
        RRC_LIST_INIT(NR_UE_rrc_inst->CSI_ResourceConfig_list, NR_maxNrofCSI_ResourceConfigurations);
        RRC_LIST_INIT(NR_UE_rrc_inst->CSI_ReportConfig_list, NR_maxNrofCSI_ReportConfigurations);
    }else{
        NR_UE_rrc_inst = (NR_UE_RRC_INST_t *)0;
    }
}


uint8_t nr_ue_process_rlc_bearer_list(NR_CellGroupConfig_t *cell_group_config){

};

uint8_t nr_ue_process_secondary_cell_list(NR_CellGroupConfig_t *cell_group_config){

};

uint8_t nr_ue_process_mac_cell_group_config(NR_MAC_CellGroupConfig_t *mac_cell_group_config){

};

uint8_t nr_ue_process_physical_cell_group_config(NR_PhysicalCellGroupConfig_t *phy_cell_group_config){

};

uint8_t nr_ue_process_spcell_config(NR_SpCellConfig_t *spcell_config){

};