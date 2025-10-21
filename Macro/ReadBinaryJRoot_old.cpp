#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <cstring>
using namespace std;
//#include "TFile.h"
//#include "TTree.h"
#include "TString.h"
#include "TGraph.h"
#include "TCanvas.h"
#include "Getline.h"
#include "Rtypes.h"  


//#define MAXNFERS 128;

/*!
* @ingroup Macros
* @brief Readout constants
*/
#define MAX_WAVEFORM_LENGTH			2048	/*!< Maximum waveform length */
#define MAX_LIST_SIZE				2048	/*!< Maximum hits list size */
#define MAX_TEST_NWORDS				4		/*!< Maximum number of words in test event (5203) */
#define MAX_SERV_NWORDS				6		/*!< Max number of words in service event */

/*!
* @ingroup Macros
* @defgroup BRD_Constant Board constants
* @brief FERS board family constants
* @{
*/
#define	FERSLIB_MAX_NCNC		  4		/*!< Max number of concentrators */
#define	FERSLIB_MAX_NBRD		  128	/*!< Max number of connected boards */
#define	FERSLIB_MAX_NCH			  128	/*!< Max number of channels per board (for all boards) */
#define	FERSLIB_MAX_NCH_5202	  64	/*!< Max number of channels per board */
#define	FERSLIB_MAX_NCH_5203	  128	/*!< Max number of channels per board */
#define	FERSLIB_MAX_NCH_5204	  64	/*!< Max number of channels per board */
#define	PICOTDC_NCH  			  64	/*!< number of picoTDC channels */
#define	FERSLIB_MAX_NTDL		  8		/*!< Max number of TDlinks (chains) in a concentrator */
#define	FERSLIB_MAX_NNODES		  16	/*!< Max number of nodes in a TDL chain */
/*! @} */

/*!
* @ingroup Macros
* @defgroup DTQ Data qualifier
* @brief Data Qualifier
* @{
*/
#define	DTQ_SPECT	0x01  /*!< Spectroscopy Mode (Energy) */
#define	DTQ_TIMING  0x02  /*!< Timing Mode */
#define	DTQ_TSPECT	0x03  /*!< Spectroscopy + Timing Mode (Energy + Tstamp) */
#define	DTQ_COUNT	0x04  /*!< Counting Mode (MCS) */
#define	DTQ_WAVE	0x08  /*!< Waveform Mode */
#define	DTQ_SERVICE 0x2F  /*!< Service event */
#define	DTQ_TEST	0xFF  /*!< Test Mode */
#define	DTQ_START	0x0F  /*!< Start Event */
/*! @} */



#define GAIN_SEL_AUTO					0
#define GAIN_SEL_HIGH					1
#define GAIN_SEL_LOW					2
#define GAIN_SEL_BOTH					3


#define MAXNFERS  128
#define MAX_NCH   64

typedef struct {
	double tstamp_us;			//!< Time stamp of service event
	uint64_t update_time;		//!< Update time (epoch, ms)
	uint16_t pkt_size;			//!< Event size
	uint8_t version;			//!< Service event version
	uint8_t format;				//!< Event Format
	uint32_t ch_trg_cnt[FERSLIB_MAX_NCH_5202];  // Channel trigger counts 
	uint32_t q_or_cnt;			//!< Q-OR counts value
	uint32_t t_or_cnt;			//!< T-OR counts value
	float tempFPGA;				//!< FPGA core temperature 
	float tempBoard;			//!< Board temperature (near uC PIC)
	float tempTDC[2];			//!< Temperature of TDC0 and TDC1
	float tempHV;				//!< High-voltage module temperature
	float tempDetector;			//!< Detector temperature (referred to ?)
	float hv_Vmon;				//!< HV voltage monitor
	float hv_Imon;				//!< HV current monitor
	uint8_t hv_status_on;		//!< HV status ON/OFF
	uint8_t hv_status_ramp;		//!< HV ramp status
	uint8_t hv_status_ovv;		//!< HV over-voltage status
	uint8_t hv_status_ovc;		//!< HV over-current status
	uint16_t Status;			//!< Status Register
	uint16_t TDCROStatus;		//!< TDC Readout Status Register
	uint64_t ChAlmFullFlags[2];	//!< Channel Almost Full flag (from picoTDC)
	uint32_t ReadoutFlags;		//!< Readout Flags from picoTDC and FPGA
	uint32_t TotTrg_cnt;		//!< Total triggers counter
	uint32_t RejTrg_cnt;		//!< Rejected triggers counter
	uint32_t SupprTrg_cnt;		//!< Zero suppressed triggers counter
} ServEvent_t; // 5202 + 5203


void ReadTime(uint64_t ms_since_epoch) {
    time_t seconds = ms_since_epoch / 1000;

    struct tm *timeinfo = localtime(&seconds);  // per data/ora locale
    if (timeinfo == NULL) {
        perror("localtime");
        return;
    }

    int milliseconds = ms_since_epoch % 1000;

    printf("Start Time: %04d-%02d-%02d %02d:%02d:%02d.%03d\n",
           timeinfo->tm_year + 1900,
           timeinfo->tm_mon + 1,
           timeinfo->tm_mday,
           timeinfo->tm_hour,
           timeinfo->tm_min,
           timeinfo->tm_sec,
           milliseconds);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int ret= 0;
int n = 0;
char rootfile[256];
char rootfileservice[256];

//FILE HEADER PARAMETERS
char FERSSSS[8];
float tmpLSB = 0;
ULong64_t start_time = 0;
uint8_t NFersBoard = 0; // Number of FERS boards
uint8_t bindex[MAXNFERS] = {0};
uint8_t AcqMode[MAXNFERS] = {0};
ULong64_t ChMask[MAXNFERS] = {0x0};
uint8_t GainSelect[MAXNFERS] = {0};
uint8_t EnableToT[MAXNFERS] = {0};

//EVENT PARAMETERS
uint8_t EventBoard = 0;
double timestamp = 0;   
double rel_tstamp = 0;
double fine_tstamp = 0;
ULong64_t trigger_id = 0; 
uint8_t data_t[MAX_LIST_SIZE] = {0};
uint16_t EnergyHG[MAX_NCH] = {0};
uint16_t EnergyLG[MAX_NCH] = {0};
uint32_t ToA_LSB[MAX_LIST_SIZE] = {0};
uint16_t ToT_LSB[MAX_LIST_SIZE] = {0};
ULong64_t cntW[MAX_NCH] = { 0 }; //counts per channel
ULong64_t zerosupp_chmask; //channel mask with zero suppression & channel mask
uint16_t nhits = 0;
uint8_t  timing_channel[MAX_LIST_SIZE] = {0};

int activechannels = 0;
uint8_t activezschannels = 0;
uint16_t EnergyHG_TREE[MAX_NCH] = {0};
uint16_t EnergyLG_TREE[MAX_NCH] = {0};
uint32_t ToA_LSB_TREE[MAX_LIST_SIZE] = {0};
uint16_t ToT_LSB_TREE[MAX_LIST_SIZE] = {0};



void decode(const char *filename){

    


    ////////////////////////////////////OPEN FILE////////////////////////////////////

    // open file
    FILE *f = fopen(Form("%s", filename), "r");
    if (f == NULL) {
        printf("Cannot find file \'%s\'\n", filename);
        return;
    }

   /* //open the root file
    strcpy(rootfile, filename);
    for (int i=0 ; i<strlen(rootfile) ; i++)
        if (rootfile[i] == '.') {
            rootfile[i] = 0;
            break;
        }
    strcat(rootfile, ".root");*/
    const char *dot = strrchr(filename, '.'); 
    if (dot != nullptr) {
        size_t len = dot - filename;
        strncpy(rootfile, filename, len);
        rootfile[len] = '\0';
    } else {
        strcpy(rootfile, filename); 
    }
    strcat(rootfile, ".root");

    printf("Creating Root file %s\n",rootfile);
    TFile *outfile = new TFile(rootfile, "RECREATE");    
    // create a tree for the info
    TTree *info = new TTree("info","info");
    // create a tree for the data
    TTree *data = new TTree("data","data");

    // Set the branches for the information tree
    info->Branch("StartTime", &start_time, "StartTime/l");
    info->Branch("tmpLSB", &tmpLSB, "tmpLSB/F");
    info->Branch("NFersBoard", &NFersBoard, "NFersBoard/b");
    info->Branch("Board", bindex, "Board[NFersBoard]/b");
    info->Branch("AcqMode", AcqMode, "AcqMode[NFersBoard]/b");
    info->Branch("ChMask", ChMask, "ChMask[NFersBoard]/l");
    info->Branch("GainSelect", GainSelect, "GainSelect[NFersBoard]/b");
    info->Branch("EnableToT", EnableToT, "EnableToT[NFersBoard]/b");
    

    ////////////////////////////////////FILE HEADER////////////////////////////////////

    //First word
    /*ret = (int)fread(&FERSSSS, 4, 1, f); //IT WILL BE REMOVED
    if (ret < 1) {
        if (feof(f)) {
            printf("End of file reached.\n");
            return;
        } else {
            printf("Error reading file \'%s\'.\n", filename);
            fclose(f);
            return;
        }
    }
    cout<<FERSSSS<<endl;
    if (strncmp(FERSSSS, "FERS", 4) != 0) {
        printf("File Header doesn't correspond to FERSSSS file, aborting.\n");
        return;
    }*/

    //Time of day
    ret= (int)fread(&start_time, sizeof(start_time), 1, f);
    if (ret< 1) {
        if (feof(f)) {
            printf("End of file reached.\n");
            return;
        } else {
            printf("Error reading file \'%s\'.\n", filename);
            fclose(f);
            return;
        }
    }
    ReadTime(start_time);

    //Conversion from LSB to nanoseconds
    ret= (int)fread(&tmpLSB, sizeof(tmpLSB), 1, f);
    if (ret< 1) {
        if (feof(f)) {
            printf("End of file reached.\n");
            return;
        } else {
            printf("Error reading file \'%s\'.\n", filename);
            fclose(f);
            return;
        }
    }
    printf("Time conversion LSB to ns = %f\n", tmpLSB);

    //Number of Fers boards
    ret= (int)fread(&NFersBoard, sizeof(NFersBoard), 1, f);
    if (ret< 1) {
        if (feof(f)) {
            printf("End of file reached.\n");
            return;
        } else {
            printf("Error reading file \'%s\'.\n", filename);
            fclose(f);
            return;
        }
    }
    printf("Total number of Fers connected = %d\n", NFersBoard);

    //Loop on boards
    for(int ifers = 0; ifers<NFersBoard; ifers++){
        //board index
        ret= (int)fread(&bindex[ifers], sizeof(uint8_t), 1, f);
        if (ret< 1) {
            if (feof(f)) {
                printf("End of file reached.\n");
                return;
            } else {
                printf("Error reading file \'%s\'.\n", filename);
                fclose(f);
                return;
            }
        }
        //Acquisition mode
        ret= (int)fread(&AcqMode[ifers], sizeof(uint8_t), 1, f);
        if (ret< 1) {
            if (feof(f)) {
                printf("End of file reached.\n");
                return;
            } else {
                printf("Error reading file \'%s\'.\n", filename);
                fclose(f);
                return;
            }
        }
        //Channel mask
        ret= (int)fread(&ChMask[ifers], sizeof(uint64_t), 1, f);
        if (ret< 1) {
            if (feof(f)) {
                printf("End of file reached.\n");
                return;
            } else {
                printf("Error reading file \'%s\'.\n", filename);
                fclose(f);
                return;
            }
        }
        //Gain Select
        ret= (int)fread(&GainSelect[ifers], sizeof(uint8_t), 1, f);
        if (ret< 1) {
            if (feof(f)) {
                printf("End of file reached.\n");
                return;
            } else {
                printf("Error reading file \'%s\'.\n", filename);
                fclose(f);
                return;
            }
        }
        //Enable ToT
        ret= (int)fread(&EnableToT[ifers], sizeof(uint8_t), 1, f);
        if (ret< 1) {
            if (feof(f)) {
                printf("End of file reached.\n");
                return;
            } else {
                printf("Error reading file \'%s\'.\n", filename);
                fclose(f);
                return;
            }
        }
        //GainSelect[ifers] = 3; //TEMPORARY
        //EnableToT[ifers] = 1;  //TEMPORARY
        printf("    Board = %d, AcqMode = 0x%x, Channel mask = 0x%llx\n",bindex[ifers], AcqMode[ifers], ChMask[ifers]);
    }// end loop on boards
    //printf("Sto per fillare\n");
    //Fill info tree
    info->Fill();
    //printf("Ho fillato\n");
    
    if(NFersBoard == 1){ //stand alone. For more than one i still need to think of an efficient structure
        for(int i=0; i<64; i++){
            if((ChMask[0] >> i) & 1) {
                activechannels++;
            }
        }
        //Create branches for data tree based on Acquisition mode of board 0
        if((AcqMode[0] & 0x0F) == DTQ_SPECT) {
            data->Branch("Bindex", &EventBoard, "Bindex/b");
            data->Branch("TimeStamp", &timestamp, "TimeStamp/D");
            if(AcqMode[0] & 0x80) {
                data->Branch("RelativeTimeStamp", &rel_tstamp, "RelativeTimeStamp/D");
            }
            data->Branch("TriggerID", &trigger_id, "TriggerID/l");
            if((GainSelect[0] & GAIN_SEL_LOW) || (GainSelect[0] == GAIN_SEL_AUTO)){
                data->Branch("EnergyLG", EnergyLG_TREE, Form("EnergyLG[%d]/s", activechannels));
            }
            if((GainSelect[0] & GAIN_SEL_HIGH) || (GainSelect[0] == GAIN_SEL_AUTO)){
                data->Branch("EnergyHG", EnergyHG_TREE, Form("EnergyHG[%d]/s", activechannels));
            }
        }//end of DTQ_SPECT
        else if((AcqMode[0] & 0x0F) == DTQ_TSPECT) {
            data->Branch("Bindex", &EventBoard, "Bindex/b");
            data->Branch("TimeStamp", &timestamp, "TimeStamp/D");
            if(AcqMode[0] & 0x80) {
                data->Branch("RelativeTimeStamp", &rel_tstamp, "RelativeTimeStamp/D");
            }
            data->Branch("TriggerID", &trigger_id, "TriggerID/l");
            if((GainSelect[0] & GAIN_SEL_LOW) || (GainSelect[0] == GAIN_SEL_AUTO)){
                data->Branch("EnergyLG", EnergyLG_TREE, Form("EnergyLG[%d]/s", activechannels));
            }
            if((GainSelect[0] & GAIN_SEL_HIGH) || (GainSelect[0] == GAIN_SEL_AUTO)){
                data->Branch("EnergyHG", EnergyHG_TREE, Form("EnergyHG[%d]/s", activechannels));
            }
            data->Branch("ToA", ToA_LSB_TREE, Form("ToA[%d]/i", activechannels));
            if(EnableToT[0]){
                data->Branch("ToT", ToT_LSB_TREE, Form("ToT[%d]/s", activechannels));
            }
        }//end of DTQ_TSPECT
        else if((AcqMode[0] & 0x0F) == DTQ_COUNT) {
            data->Branch("Bindex", &EventBoard, "Bindex/b");
            data->Branch("TimeStamp", &timestamp, "TimeStamp/D");
            if(AcqMode[0] & 0x80) {
                data->Branch("RelativeTimeStamp", &rel_tstamp, "RelativeTimeStamp/D");
            }
            data->Branch("TriggerID", &trigger_id, "TriggerID/l");
            data->Branch("ZeroSuppChMask", &zerosupp_chmask, "ZeroSuppChMask/l");
            for(int i=0; i<64; i++){
                if((ChMask[0] >> i) & 1) {
                    activezschannels++;
                }
            }
            data->Branch("activezschannels", &activezschannels, "activezschannels/b");
            data->Branch("Counts", cntW, "Counts[activezschannels]/l");
        }//end of DTQ_COUNT
        else if((AcqMode[0] & 0x0F) == DTQ_TIMING) {
            data->Branch("Bindex", &EventBoard, "Bindex/b");
            data->Branch("FineTimeStamp", &fine_tstamp, "FineTimeStamp/D");
            data->Branch("NHits", &nhits, "NHits/s");
            data->Branch("Channel", timing_channel, "Channel[NHits]/b");
            data->Branch("ToA", ToA_LSB_TREE, "ToA[NHits]/i");
            if(EnableToT[0]){
                data->Branch("ToT", ToT_LSB_TREE, "ToT[NHits]/s");
            }
        }//end of DTQ_TIMING
        else{
            //acq mode not valid
            outfile->Close();
            fclose(f);
            return;
        }

    } else {
        printf("SORRYYYY but it is still work in progress for more than one board\n");
        outfile->Close();
        fclose(f);
        return;
    }

    ////////////////////////////////////EVENT////////////////////////////////////
    int endfile = 0;
    for(n = 0; !endfile ; n++){

        EventBoard = 0;
        timestamp = 0;   
        rel_tstamp = 0;
        fine_tstamp = 0;
        trigger_id = 0; 
        for(int i=0; i<MAX_NCH;i++){
            EnergyHG[i] = 0;
            EnergyLG[i] = 0;
            cntW[i] = 0;
            EnergyHG_TREE[i] = 0;
            EnergyLG_TREE[i] = 0;
        }
        for(int i=0; i<MAX_LIST_SIZE; i++){
            data_t[i] = 0;
            ToA_LSB[i] = 0;
            ToT_LSB[i] = 0;
            ToA_LSB_TREE[i] = 0;
            ToT_LSB_TREE[i] = 0;
            timing_channel[i] = 0;
        }
        
        zerosupp_chmask = 0; //channel mask with zero suppression & channel mask
        nhits = 0;


        ret= (int)fread(&EventBoard, sizeof(uint8_t), 1, f);
        if (ret< 1) {
            if (feof(f)) {
                printf("End of file reached.\n");
                endfile = 1;
                break;
            } else {
                printf("Error reading file \'%s\'.\n", filename);
                outfile->Close();
                fclose(f);
                return;
            }
        }
        //printf("n.%d, board %d\n", n, EventBoard);
        // ----------------------------------------------------------------------------------
        // SPECT/SPECT_TIMING MODE
        // ----------------------------------------------------------------------------------
        if(((AcqMode[EventBoard] & 0x0F) == DTQ_SPECT)||((AcqMode[EventBoard] & 0x0F) == DTQ_TSPECT)) {
            printf("Event n.%d, board = %d, SPECTROSCOPY\n", n, EventBoard);
            ret= (int)fread(&timestamp, sizeof(timestamp), 1, f); //read timestamp in us
            if (ret< 1) {
                if (feof(f)) {
                    printf("End of file reached.\n");
                    endfile = 1;
                    break;
                } else {
                    printf("Error reading file \'%s\'.\n", filename);
                    outfile->Close();
                    fclose(f);
                    return;
                }
            }
            //printf("    Time stamp = %f us\n",timestamp);
            if(AcqMode[EventBoard] & 0x80) {
                ret= (int)fread(&rel_tstamp, sizeof(rel_tstamp), 1, f); //read relative timestamp in us
                if (ret< 1) {
                    if (feof(f)) {
                        printf("End of file reached.\n");
                        endfile = 1;
                        break;
                    } else {
                        printf("Error reading file \'%s\'.\n", filename);
                        outfile->Close();
                        fclose(f);
                        return;
                    }
                }
            }
            ret= (int)fread(&trigger_id, sizeof(trigger_id), 1, f); //read trigger id
            if (ret< 1) {
                if (feof(f)) {
                    printf("End of file reached.\n");
                    endfile = 1;
                    break;
                } else {
                    printf("Error reading file \'%s\'.\n", filename);
                    outfile->Close();
                    fclose(f);
                    return;
                }
            }
            //printf("    Trigger ID = %ld\n",trigger_id);
            //loop on channels
            for(int i=0; i<MAX_NCH; i++) {
                if ((ChMask[EventBoard] >> i) & 1) {
                    //printf("    Channel %d\n", i);
                    ret= (int)fread(&data_t[i], sizeof(uint8_t), 1, f); //read data type 
                    if (ret< 1) {
                        if (feof(f)) {
                            printf("End of file reached.\n");
                            endfile = 1;
                            break;
                        } else {
                            printf("Error reading file \'%s\'.\n", filename);
                            outfile->Close();
                            fclose(f);
                            return;
                        }
                    }
                    //printf("        Data type = %d\n",data_t[i]);
                    if (data_t[i] & 0x01){
                        ret= (int)fread(&EnergyLG[i], sizeof(uint16_t), 1, f); //read energy LG
                        if (ret< 1) {
                            if (feof(f)) {
                                printf("End of file reached.\n");
                                endfile = 1;
                                break;
                            } else {
                                printf("Error reading file \'%s\'.\n", filename);
                                outfile->Close();
                                fclose(f);
                                return;
                            }
                        }
                        //printf("        Energy LG ch[%d] = %d\n", i, EnergyLG[i]);
                    } 
                    if (data_t[i] & 0x02) {
                        ret= (int)fread(&EnergyHG[i], sizeof(uint16_t), 1, f); //read energy HG
                        if (ret< 1) {
                            if (feof(f)) {
                                printf("End of file reached.\n");
                                endfile = 1;
                                break;
                            } else {
                                printf("Error reading file \'%s\'.\n", filename);
                                outfile->Close();
                                fclose(f);
                                return;
                            }
                        }
                        //printf("        Energy HG ch[%d] = %d\n", i, EnergyHG[i]);
                    }
                    if (data_t[i] & 0x10) {	
                        ret= (int)fread(&ToA_LSB[i], sizeof(uint32_t), 1, f); //read Time of Arrival
                        if (ret< 1) {
                            if (feof(f)) {
                                printf("End of file reached.\n");
                                endfile = 1;
                                break;
                            } else {
                                printf("Error reading file \'%s\'.\n", filename);
                                outfile->Close();
                                fclose(f);
                                return;
                            }
                        }
                        //printf("        ToA ch[%d] = %d\n", i, ToA_LSB[i]);
                    }
                    if (data_t[i] & 0x20) {
                        ret= (int)fread(&ToT_LSB[i], sizeof(uint16_t), 1, f); //read Time over Threshold
                        if (ret< 1) {
                            if (feof(f)) {
                                printf("End of file reached.\n");
                                endfile = 1;
                                break;
                            } else {
                                printf("Error reading file \'%s\'.\n", filename);
                                outfile->Close();
                                fclose(f);
                                return;
                            }
                        }
                        //printf("        ToT ch[%d] = %d\n", i, ToT_LSB[i]);
                    }
                } // end if channel in channelmask
            }// end loop on channel
            int j = 0;
            for(int i = 0; i<64;i++){
                if ((ChMask[EventBoard] >> i) & 1) {
                    EnergyHG_TREE[j] = EnergyHG[i];
                    EnergyLG_TREE[j] = EnergyLG[i];
                    ToA_LSB_TREE[j] = ToA_LSB[i];
                    ToT_LSB_TREE[j] = ToT_LSB[i];
                    j++;
                }
            }
            data->Fill();
        }// end DTQ_SPECT
        // ----------------------------------------------------------------------------------
        // COUNTING MODE
        // ----------------------------------------------------------------------------------
        if((AcqMode[EventBoard] & 0x0F) == DTQ_COUNT) {
            printf("Event n.%d, board = %d, COUNTING\n", n, EventBoard);
            ret= (int)fread(&timestamp, sizeof(timestamp), 1, f); //read timestamp in us
            if (ret< 1) {
                if (feof(f)) {
                    printf("End of file reached.\n");
                    endfile = 1;
                    break;
                } else {
                    printf("Error reading file \'%s\'.\n", filename);
                    outfile->Close();
                    fclose(f);
                    return;
                }
            }
            //printf("    Time stamp = %f us\n",timestamp);
            if(AcqMode[EventBoard] & 0x80) {
                ret= (int)fread(&rel_tstamp, sizeof(rel_tstamp), 1, f); //read relative timestamp in us
                if (ret< 1) {
                    if (feof(f)) {
                        printf("End of file reached.\n");
                        endfile = 1;
                        break;
                    } else {
                        printf("Error reading file \'%s\'.\n", filename);
                        outfile->Close();
                        fclose(f);
                        return;
                    }
                }
            }
            ret= (int)fread(&trigger_id, sizeof(trigger_id), 1, f); //read trigger id
            if (ret< 1) {
                if (feof(f)) {
                    printf("End of file reached.\n");
                endfile = 1;
                    break;
                } else {
                    printf("Error reading file \'%s\'.\n", filename);
                    outfile->Close();
                    fclose(f);
                    return;
                }
            }
            //printf("    Trigger ID = %ld\n",trigger_id);
            ret= (int)fread(&zerosupp_chmask, sizeof(zerosupp_chmask), 1, f); //read channel mask
            if (ret< 1) {
                if (feof(f)) {
                    printf("End of file reached.\n");
                    endfile = 1;
                    break;
                } else {
                    printf("Error reading file \'%s\'.\n", filename);
                    outfile->Close();
                    fclose(f);
                    return;
                }
            }
            //printf("    Zero Supp. ChMask = 0x%lx\n",zerosupp_chmask);
            //Loop on channels
            for(int i=0; i<MAX_NCH; i++) {
                if ((zerosupp_chmask >> i) & 1) {
                    ret= (int)fread(&cntW[i], sizeof(uint64_t), 1, f); //read channel count
                    if (ret< 1) {
                        if (feof(f)) {
                            printf("End of file reached.\n");
                            endfile = 1;
                            break;
                        } else {
                            printf("Error reading file \'%s\'.\n", filename);
                            outfile->Close();
                            fclose(f);
                            return;
                        }
                    }
                    //printf("    Channel %d\n",i);
                    //printf("        Counts = %ld\n",cntW[i]);
                } //end if channel in channelmask
            }// end loop on channels
            data->Fill();
        }// end DTQ_COUNT
        // ----------------------------------------------------------------------------------
        // TIMING MODE
        // ----------------------------------------------------------------------------------
        if((AcqMode[EventBoard] & 0x0F) == DTQ_TIMING) {
            printf("Event n.%d, board = %d, TIMING\n", n, EventBoard);
            ret= (int)fread(&fine_tstamp, sizeof(fine_tstamp), 1, f); //read timestamp in us
            if (ret< 1) {
                if (feof(f)) {
                    printf("End of file reached.\n");
                    endfile = 1;
                    break;
                } else {
                    printf("Error reading file \'%s\'.\n", filename);
                    outfile->Close();
                    fclose(f);
                    return;
                }
            }
            //printf("    Fine tstamp = %lf\n",fine_tstamp);
            ret= (int)fread(&nhits, sizeof(nhits), 1, f); //read number of hits
            if (ret< 1) {
                if (feof(f)) {
                    printf("End of file reached.\n");
                    endfile = 1;
                    break;
                } else {
                    printf("Error reading file \'%s\'.\n", filename);
                    outfile->Close();
                    fclose(f);
                    return;
                }
            }
            //printf("    Number of hits = %d\n",nhits);
            //Loop on hits
            for(int i = 0; i<nhits; i++) {
                ret= (int)fread(&timing_channel[i], sizeof(uint8_t), 1, f); //read channel
                if (ret< 1) {
                    if (feof(f)) {
                        printf("End of file reached.\n");
                        endfile = 1;
                        break;
                    } else {
                        printf("Error reading file \'%s\'.\n", filename);
                        outfile->Close();
                        fclose(f);
                        return;
                    }
                }
                //printf("    Hit n.%d, channel %d\n", i, timing_channel[i]);
                ret= (int)fread(&data_t[i], sizeof(uint8_t), 1, f); //read data type
                if (ret< 1) {
                    if (feof(f)) {
                        printf("End of file reached.\n");
                        endfile = 1;
                        break;
                    } else {
                        printf("Error reading file \'%s\'.\n", filename);
                        outfile->Close();
                        fclose(f);
                        return;
                    }
                }
                //printf("        Data type = %d\n",data_t[i]);
                if(data_t[i] & 0x10) {
                    ret= (int)fread(&ToA_LSB[i], sizeof(uint32_t), 1, f); //read data type
                    if (ret< 1) {
                        if (feof(f)) {
                            printf("End of file reached.\n");
                            endfile = 1;
                            break;
                        } else {
                            printf("Error reading file \'%s\'.\n", filename);
                            outfile->Close();
                            fclose(f);
                            return;
                        }
                    }
                    //printf("        ToA ch[%d] = %d\n", i, ToA_LSB[i]);
                }
                if(data_t[i] & 0x20) {
                    ret= (int)fread(&ToT_LSB[i], sizeof(uint16_t), 1, f); //read data type
                    if (ret< 1) {
                        if (feof(f)) {
                            printf("End of file reached.\n");
                            endfile = 1;
                            break;
                        } else {
                            printf("Error reading file \'%s\'.\n", filename);
                            outfile->Close();
                            fclose(f);
                            return;
                        }
                    }
                    //printf("        ToT ch[%d] = %d\n", i, ToT_LSB[i]);
                }
            }// end loop on hits
            int j = 0;
            for(int i = 0; i<64;i++){
                if ((ChMask[EventBoard] >> i) & 1) {
                    ToA_LSB_TREE[j] = ToA_LSB[i];
                    ToT_LSB_TREE[j] = ToT_LSB[i];
                    j++;
                }
            }
            data->Fill();
        }//end of DTQ_TIMING
    }// end loop on events
    printf("Total number of events: %d\n", n);
    printf("%d events processed, \"%s\" written.\n", n, rootfile);



    info->Write();
    data->Write();
    //service->Write();
    outfile->Close();
    fclose(f);

    return;
}//end of decode


void read(const char *filename){

    TString file= filename;
    TFile *input = new TFile(file,"read");
    TTree *info = (TTree*)input->Get("info");
    TTree *data = (TTree*)input->Get("data");
    //TTree *service = (TTree*)input->Get("service");

    //input->Get("service")->Print();

    

    // Set information tree branches
    info->SetBranchAddress("StartTime", &start_time);
    info->SetBranchAddress("tmpLSB", &tmpLSB);
    info->SetBranchAddress("NFersBoard", &NFersBoard);
    info->SetBranchAddress("Board", bindex);
    info->SetBranchAddress("AcqMode", AcqMode);
    info->SetBranchAddress("ChMask", ChMask);
    info->SetBranchAddress("GainSelect", GainSelect);
    info->SetBranchAddress("EnableToT", EnableToT);

    info->GetEntry(0);
    printf("Number of FERS %d\n", NFersBoard);

    if(NFersBoard==1){
        if((AcqMode[0] & 0x0F) == DTQ_SPECT) {
            data->SetBranchAddress("Bindex", &EventBoard);
            data->SetBranchAddress("TimeStamp", &timestamp);
            if(AcqMode[0] & 0x80) {
                data->SetBranchAddress("RelativeTimeStamp", &rel_tstamp);
            }
            data->SetBranchAddress("TriggerID", &trigger_id);
            if((GainSelect[0] & GAIN_SEL_LOW) || (GainSelect[0] == GAIN_SEL_AUTO)){
                data->SetBranchAddress("EnergyLG", EnergyLG_TREE);
            }
            if((GainSelect[0] & GAIN_SEL_HIGH) || (GainSelect[0] == GAIN_SEL_AUTO)){
                data->SetBranchAddress("EnergyHG", EnergyHG_TREE);
            }
        }//end of DTQ_SPECT
        else if((AcqMode[0] & 0x0F) == DTQ_TSPECT) {
            data->SetBranchAddress("Bindex", &EventBoard);
            data->SetBranchAddress("TimeStamp", &timestamp);
            if(AcqMode[0] & 0x80) {
                data->SetBranchAddress("RelativeTimeStamp", &rel_tstamp);
            }
            data->SetBranchAddress("TriggerID", &trigger_id);
            if((GainSelect[0] & GAIN_SEL_LOW) || (GainSelect[0] == GAIN_SEL_AUTO)){
                data->SetBranchAddress("EnergyLG", EnergyLG_TREE);
            }
            if((GainSelect[0] & GAIN_SEL_HIGH) || (GainSelect[0] == GAIN_SEL_AUTO)){
                data->SetBranchAddress("EnergyHG", EnergyHG_TREE);
            }
            data->SetBranchAddress("ToA", ToA_LSB_TREE);
            if(EnableToT[0]){
                data->SetBranchAddress("ToT", ToT_LSB_TREE);
            }
        }//end of DTQ_TSPECT
        else if((AcqMode[EventBoard] & 0x0F) == DTQ_COUNT) {
            data->SetBranchAddress("Bindex", &EventBoard);
            data->SetBranchAddress("TimeStamp", &timestamp);
            if(AcqMode[0] & 0x80) {
                data->SetBranchAddress("RelativeTimeStamp", &rel_tstamp);
            }
            data->SetBranchAddress("TriggerID", &trigger_id);
            data->SetBranchAddress("ZeroSuppChMask", &zerosupp_chmask);
            for(int i=0; i<64; i++){
                if((ChMask[0] >> i) & 1) {
                    activezschannels++;
                }
            }
            data->SetBranchAddress("activezschannels", &activezschannels);
            data->SetBranchAddress("Counts", cntW);
        }//end of DTQ_COUNT
        else if((AcqMode[EventBoard] & 0x0F) == DTQ_TIMING) {
            data->SetBranchAddress("Bindex", &EventBoard);
            data->SetBranchAddress("FineTimeStamp", &fine_tstamp);
            data->SetBranchAddress("NHits", &nhits);
            data->SetBranchAddress("Channel", timing_channel);
            data->SetBranchAddress("ToA", ToA_LSB_TREE);
            if(EnableToT[0]){
                data->SetBranchAddress("ToT", ToT_LSB_TREE);
            }
        }
    } else{
        printf("SORRYYYY but it is still work in progress for more than one board\n");
        return;
    }

    TH1F *histHG = new TH1F("histHG", "histHG", 8192, 0, 8192);
    TH1F *histToT = new TH1F("histToT", "histToT", 512, 0, 512);
    TH1F *histToA = new TH1F("histToA", "histToA", 2048, 0, 2048);
    TH2F *histHGToT = new TH2F("histHGToT", "histHGToT", 8192, 0, 8192, 512, 0, 512);
    TH2F *histToTToA = new TH2F("histToTToA", "histToTToA", 512, 0, 512, 2048, 0, 2048);

    n = data->GetEntries();
    cout<<n<<endl;
    for(int i = 0; i<n; i++) {
        data->GetEntry(i);
        histHG->Fill(EnergyHG_TREE[0]);
        histToT->Fill(ToT_LSB_TREE[0]);
        histToA->Fill(ToA_LSB_TREE[0]);
        if(ToA_LSB_TREE[0] != 0){
            histHGToT->Fill(EnergyHG_TREE[0],ToT_LSB_TREE[0]);
            histToTToA->Fill(ToT_LSB_TREE[0],ToA_LSB_TREE[0]);
        }
    }
    TCanvas *cHG = new TCanvas("HG");
    TCanvas *cToT = new TCanvas("ToT");
    TCanvas *cToA = new TCanvas("ToA");
    TCanvas *cHGToT = new TCanvas("HGToT");
    TCanvas *cToTToA = new TCanvas("ToTToA");

    cHG->cd();
    histHG->GetXaxis()->SetTitle("Energy [ADC channels]");
    histHG->GetYaxis()->SetTitle("Counts");
    histHG->Draw();

    cToT->cd();
    histToT->GetXaxis()->SetTitle("Time over Threshold [LSB = 0.5ns]");
    histToT->GetYaxis()->SetTitle("Counts");
    histToT->Draw();

    cToA->cd();
    histToA->GetXaxis()->SetTitle("Time of Arrival [LSB = 0.5ns]");
    histToA->GetYaxis()->SetTitle("Counts");
    histToA->Draw();

    cHGToT->cd();
    histHGToT->GetXaxis()->SetTitle("Energy [ADC channels]");
    histHGToT->GetYaxis()->SetTitle("Time over Threshold [LSB = 0.5ns]");
    histHGToT->Draw();

    cToTToA->cd();
    histToTToA->GetXaxis()->SetTitle("Time over Threshold [LSB = 0.5ns]");
    histToTToA->GetYaxis()->SetTitle("Time of Arrival [LSB = 0.5ns]");
    histToTToA->Draw();




}//end of read

