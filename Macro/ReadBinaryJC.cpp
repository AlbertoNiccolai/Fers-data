#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <cstring>
using namespace std;

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


#define MAXNFERS  128
#define MAX_NCH   64


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


int main(int argc, const char *argv[]){

    if (argc < 2) {
        cerr << "usage file" << endl;
        return 1;
    }

    const char *filename = argv[1];

    int ret= 0;
    int n = 0;
    char rootfile[256];
    char rootfileservice[256];
    
    //FILE HEADER PARAMETERS
    char FERSSSS[8];
    float tmpLSB = 0;
    uint64_t start_time = 0;
    uint8_t NFersBoard = 0; // Number of FERS boards
    uint8_t bindex[MAXNFERS] = {0};
    uint8_t AcqMode[MAXNFERS] = {0};
    uint64_t ChMask[MAXNFERS] = {0x0};
    uint8_t GainSelect[MAXNFERS] = {0};
    uint8_t EnableToT[MAXNFERS] = {0};

    //EVENT PARAMETERS
    uint8_t EventBoard = 128;
    double timestamp = 0;   
    double rel_tstamp = 0;
    double fine_tstamp = 0;
    uint64_t trigger_id = 0; 
    uint8_t data_t[MAX_LIST_SIZE] = {0};
    uint16_t EnergyHG[MAX_NCH] = {0};
    uint16_t EnergyLG[MAX_NCH] = {0};
    uint32_t ToA_LSB[MAX_LIST_SIZE] = {0};
    uint16_t ToT_LSB[MAX_LIST_SIZE] = {0};
    uint64_t cntW[MAX_NCH] = { 0 }; //counts per channel
    uint64_t zerosupp_chmask; //channel mask with zero suppression & channel mask
    uint16_t nhits = 0;
    uint8_t  timing_channel[MAX_LIST_SIZE] = {0};
    



    ////////////////////////////////////OPEN FILE////////////////////////////////////

    // open file
    //FILE *f = fopen(Form("%s", filename), "r");
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        printf("Cannot find file \'%s\'\n", filename);
        return 1;
    }


    ////////////////////////////////////FILE HEADER////////////////////////////////////

    //First word
    /*ret = (int)fread(&FERSSSS, 4, 1, f);
    if (ret < 1) {
        if (feof(f)) {
            printf("End of file reached.\n");
            return 1;
        } else {
            printf("Error reading file \'%s\'.\n", filename);
            fclose(f);
            return 1;
        }
    }
    cout<<FERSSSS<<endl;
    if (strncmp(FERSSSS, "FERS", 4) != 0) {
        printf("File Header doesn't correspond to FERSSSS file, aborting.\n");
        return 1;
	}*/ 

    //Time of day
    ret= (int)fread(&start_time, sizeof(start_time), 1, f);
    if (ret< 1) {
        if (feof(f)) {
            printf("End of file reached.\n");
            return 1;
        } else {
            printf("Error reading file \'%s\'.\n", filename);
            fclose(f);
            return 1;
        }
    }
    ReadTime(start_time);

    //Conversion from LSB to nanoseconds
    ret= (int)fread(&tmpLSB, sizeof(tmpLSB), 1, f);
    if (ret< 1) {
        if (feof(f)) {
            printf("End of file reached.\n");
            return 1;
        } else {
            printf("Error reading file \'%s\'.\n", filename);
            fclose(f);
            return 1;
        }
    }
    printf("Time conversion LSB to ns = %f\n", tmpLSB);

    //Number of Fers boards
    ret= (int)fread(&NFersBoard, sizeof(NFersBoard), 1, f);
    if (ret< 1) {
        if (feof(f)) {
            printf("End of file reached.\n");
            return 1;
        } else {
            printf("Error reading file \'%s\'.\n", filename);
            fclose(f);
            return 1;
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
                return 1;
            } else {
                printf("Error reading file \'%s\'.\n", filename);
                fclose(f);
                return 1;
            }
        }
        //Acquisition mode
        ret= (int)fread(&AcqMode[ifers], sizeof(uint8_t), 1, f);
        if (ret< 1) {
            if (feof(f)) {
                printf("End of file reached.\n");
                return 1;
            } else {
                printf("Error reading file \'%s\'.\n", filename);
                fclose(f);
                return 1;
            }
        }
        //Channel mask
        ret= (int)fread(&ChMask[ifers], sizeof(uint64_t), 1, f);
        if (ret< 1) {
            if (feof(f)) {
                printf("End of file reached.\n");
                return 1;
            } else {
                printf("Error reading file \'%s\'.\n", filename);
                fclose(f);
                return 1;
            }
        }
        //Gain Select
        ret= (int)fread(&GainSelect[ifers], sizeof(uint8_t), 1, f); //IT WILL BE ADDED
        if (ret< 1) {
            if (feof(f)) {
                printf("End of file reached.\n");
                return 1;
            } else {
                printf("Error reading file \'%s\'.\n", filename);
                fclose(f);
                return 1;
            }
        }
        //Enable ToT
        ret= (int)fread(&EnableToT[ifers], sizeof(uint8_t), 1, f);
        if (ret< 1) {
            if (feof(f)) {
                printf("End of file reached.\n");
                return 1;
            } else {
                printf("Error reading file \'%s\'.\n", filename);
                fclose(f);
                return 1;
            }
        }
        printf("    Board = %d, AcqMode = 0x%x, Channel mask = 0x%lx\n",bindex[ifers], AcqMode[ifers], ChMask[ifers]);
    }// end loop on boards
    
    ////////////////////////////////////EVENT////////////////////////////////////
    int endfile = 0;
    for(n = 0; !endfile ; n++){
        ret= (int)fread(&EventBoard, sizeof(uint8_t), 1, f);
        if (ret< 1) {
            if (feof(f)) {
                printf("End of file reached.\n");
                endfile = 1;
                break;
            } else {
                printf("Error reading file \'%s\'.\n", filename);
                fclose(f);
                return 1;
            }
        }
        printf("n.%d, board %d\n", n, EventBoard);
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
                    fclose(f);
                    return 1;
                }
            }
            printf("    Time stamp = %f us\n",timestamp);
            if(AcqMode[EventBoard] & 0x80) {
                ret= (int)fread(&rel_tstamp, sizeof(rel_tstamp), 1, f); //read relative timestamp in us
                if (ret< 1) {
                    if (feof(f)) {
                        printf("End of file reached.\n");
                        endfile = 1;
                        break;
                    } else {
                        printf("Error reading file \'%s\'.\n", filename);
                        fclose(f);
                        return 1;
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
                    fclose(f);
                    return 1;
                }
            }
            printf("    Trigger ID = %ld\n",trigger_id);
            //loop on channels
            for(int i=0; i<MAX_NCH; i++) {
                if ((ChMask[EventBoard] >> i) & 1) {
                    printf("    Channel %d\n", i);
                    ret= (int)fread(&data_t[i], sizeof(uint8_t), 1, f); //read data type 
                    if (ret< 1) {
                        if (feof(f)) {
                            printf("End of file reached.\n");
                            endfile = 1;
                            break;
                        } else {
                            printf("Error reading file \'%s\'.\n", filename);
                            fclose(f);
                            return 1;
                        }
                    }
                    printf("        Data type = %d\n",data_t[i]);
                    if (data_t[i] & 0x01){
                        ret= (int)fread(&EnergyLG[i], sizeof(uint16_t), 1, f); //read energy LG
                        if (ret< 1) {
                            if (feof(f)) {
                                printf("End of file reached.\n");
                                endfile = 1;
                                break;
                            } else {
                                printf("Error reading file \'%s\'.\n", filename);
                                fclose(f);
                                return 1;
                            }
                        }
                        printf("        Energy LG ch[%d] = %d\n", i, EnergyLG[i]);
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
                                fclose(f);
                                return 1;
                            }
                        }
                        printf("        Energy HG ch[%d] = %d\n", i, EnergyHG[i]);
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
                                fclose(f);
                                return 1;
                            }
                        }
                        printf("        ToA ch[%d] = %d\n", i, ToA_LSB[i]);
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
                                fclose(f);
                                return 1;
                            }
                        }
                        printf("        ToT ch[%d] = %d\n", i, ToT_LSB[i]);
                    }
                } // end if channel in channelmask
            }// end loop on channel
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
                    fclose(f);
                    return 1;
                }
            }
            printf("    Time stamp = %f us\n",timestamp);
            if(AcqMode[EventBoard] & 0x80) {
                ret= (int)fread(&rel_tstamp, sizeof(rel_tstamp), 1, f); //read relative timestamp in us
                if (ret< 1) {
                    if (feof(f)) {
                        printf("End of file reached.\n");
                        endfile = 1;
                        break;
                    } else {
                        printf("Error reading file \'%s\'.\n", filename);
                        fclose(f);
                        return 1;
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
                    fclose(f);
                    return 1;
                }
            }
            printf("    Trigger ID = %ld\n",trigger_id);
            ret= (int)fread(&zerosupp_chmask, sizeof(zerosupp_chmask), 1, f); //read channel mask
            if (ret< 1) {
                if (feof(f)) {
                    printf("End of file reached.\n");
                    endfile = 1;
                    break;
                } else {
                    printf("Error reading file \'%s\'.\n", filename);
                    fclose(f);
                    return 1;
                }
            }
            printf("    Zero Supp. ChMask = 0x%lx\n",zerosupp_chmask);
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
                            fclose(f);
                            return 1;
                        }
                    }
                    printf("    Channel %d\n",i);
                    printf("        Counts = %ld\n",cntW[i]);
                } //end if channel in channelmask
            }// end loop on channels
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
                    fclose(f);
                    return 1;
                }
            }
            printf("    Fine tstamp = %lf\n",fine_tstamp);
            ret= (int)fread(&nhits, sizeof(nhits), 1, f); //read number of hits
            if (ret< 1) {
                if (feof(f)) {
                    printf("End of file reached.\n");
                    endfile = 1;
                    break;
                } else {
                    printf("Error reading file \'%s\'.\n", filename);
                    fclose(f);
                    return 1;
                }
            }
            printf("    Number of hits = %d\n",nhits);
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
                        fclose(f);
                        return 1;
                    }
                }
                printf("    Hit n.%d, channel %d\n", i, timing_channel[i]);
                ret= (int)fread(&data_t[i], sizeof(uint8_t), 1, f); //read data type
                if (ret< 1) {
                    if (feof(f)) {
                        printf("End of file reached.\n");
                        endfile = 1;
                        break;
                    } else {
                        printf("Error reading file \'%s\'.\n", filename);
                        fclose(f);
                        return 1;
                    }
                }
                printf("        Data type = %d\n",data_t[i]);
                if(data_t[i] & 0x10) {
		    ret= (int)fread(&ToA_LSB[i], sizeof(uint32_t), 1, f); //read ToA
                    if (ret< 1) {
                        if (feof(f)) {
                            printf("End of file reached.\n");
                            endfile = 1;
                            break;
                        } else {
                            printf("Error reading file \'%s\'.\n", filename);
                            fclose(f);
                            return 1;
                        }
                    }
		    printf("        ToA ch[%d] = %d\n", i, ToA_LSB[i]);
                }
                if(data_t[i] & 0x20) {
                    ret= (int)fread(&ToT_LSB[i], sizeof(uint16_t), 1, f); //read ToT
		    if (ret< 1) {
                        if (feof(f)) {
                            printf("End of file reached.\n");
                            endfile = 1;
                            break;
                        } else {
                            printf("Error reading file \'%s\'.\n", filename);
                            fclose(f);
                            return 1;
                        }
                    }
                    printf("        ToT ch[%d] = %d\n", i, ToT_LSB[i]);
                }
            }// end loop on hits
        }//end of DTQ_TIMING
    }// end loop on events

    printf("Total number of events: %d\n", n);
    //printf("%d events processed, \"%s\" written.\n", n, rootfile);
    //data->Write();
    //service->Write();
    //outfile->Close();
    fclose(f);

    return 0;
}
