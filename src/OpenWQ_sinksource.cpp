
// Copyright 2020, Diogo Costa, diogo.pinhodacosta@canada.ca
// This file is part of OpenWQ model.

// This program, openWQ, is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) aNCOLS later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "OpenWQ_sinksource.h"

/* #################################################
 // Check Sources and Sinks and Apply
 ################################################# */
void OpenWQ_sinksource::SetSinkSource(
    OpenWQ_json& OpenWQ_json,
    OpenWQ_vars& OpenWQ_vars,
    OpenWQ_hostModelconfig& OpenWQ_hostModelconfig,
    OpenWQ_wqconfig& OpenWQ_wqconfig,
    OpenWQ_units& OpenWQ_units,
    OpenWQ_output& OpenWQ_output){ 
    
    // Local variables
    bool foundflag = false;                 // iteractive boolean to identify if comp or chem was found
    std::vector<std::string> cmp_list;      // model compartment list
    std::vector<std::string> chem_list;     // model chemical list
    std::string err_text;                   // iteractive string for text to pass to error messages
    unsigned long cmpi_ssi;                 // model index for compartment Compartment_name_name
    unsigned long chem_ssi;                 // model index for compartment Compartment_name_name
    unsigned long sinksource_ssi;           // = 0 (source), = 1 (sink)

    unsigned int num_ssfiles;               // number of sink-source files 
                                            // (saved as sub-structure of SinkSource)
    unsigned int num_sschem;                // number of chemical loads per file
    unsigned int num_rowdata;               // number of rows of data in JSON (YYYY, MM, DD, HH,...)
    int int_n_elem;                         // iterative counting of number of elements in arma:mat
    
    std::string Chemical_name;              // from JSON file
    std::string Compartment_name;           // from JSON file
    std::string Type;                       // from JSON file
    std::string Units;                      // from JSON file

    std::string elemName;                   // temporary element name
    int YYYY_json;                          // Year in JSON-sink_source (interactive)
    int MM_json;                            // Month in JSON-sink_source (interactive)
    int DD_json;                            // Day in JSON-sink_source (interactive)
    int HH_json;                            // Hour in JSON-sink_source (interactive)
    int MIN_json;                           // Minutes in JSON-sink_source (interactive)
    int ix_json;                            // iteractive ix info for sink-source row data 
    int iy_json;                            // iteractive iy info for sink-source row data 
    int iz_json;                            // iteractive iz info for sink-source row data

    double ss_data_json;                    // data (sink or source) from row data
    std::string ss_units_json;              // units of row data
    std::vector<double> unit_multiplers;    // multiplers (numerator and denominator)
   
    arma::vec row_data_col;                 // new row data (initially as col data)
    arma::Mat<double> row_data_row;         // for conversion of row_data_col to row data

    std::string msg_string;                 // error/warning message string
    bool validEntryFlag;                    // valid entry flag to skip problematic row data


    // Get model comparment names list
    unsigned int num_cmp = OpenWQ_hostModelconfig.HydroComp.size();
    for (unsigned int ci=0;ci<num_cmp;ci++){
        cmp_list.push_back(
            std::get<1>(
                OpenWQ_hostModelconfig.HydroComp.at(ci))); // num of x elements 
    }

    // Get model chemical names list
    unsigned int BGC_general_num_chem = OpenWQ_wqconfig.BGC_general_num_chem;
    for (unsigned int chemi=0;chemi<BGC_general_num_chem;chemi++){
        chem_list.push_back(
            (OpenWQ_wqconfig.BGC_general_chem_species_list)[chemi]);
    }

    // Get number of sub-structures of SinkSource
    num_ssfiles = OpenWQ_json.SinkSource.size(); 

    /* ########################################
    // Loop over file (saved as sub-structure of SinkSource)
    ######################################## */
    for (unsigned int ssf=0;ssf<num_ssfiles;ssf++){
        
        // Get number of loads in each sub-structure 
        // (corresponding to different sink_source json files)
        num_sschem = OpenWQ_json.SinkSource[std::to_string(ssf+1)].size();
        
        /* ########################################
        // Loop over loads per sub-structure
        ######################################## */

        foundflag = false;      // reset to false at every 

        for (unsigned int ssi=0;ssi<num_sschem;ssi++){
            
            /* ########
            // Get chemical name, compartment name and SS_type (source or sink)
            ###########*/

            // needs this here because there can be entries that are not relevant e.g. COMMENTS
            try{
                Chemical_name =  OpenWQ_json.SinkSource // chemical name
                    [std::to_string(ssf+1)]
                    [std::to_string(ssi+1)]
                    ["CHEMICAL_NAME"];

            }catch(json::type_error){   
                continue;
            }

            Compartment_name = OpenWQ_json.SinkSource // compartment name
                [std::to_string(ssf+1)]
                [std::to_string(ssi+1)]
                ["COMPARTMENT_NAME"];

            Type = OpenWQ_json.SinkSource // type (sink or source)
                [std::to_string(ssf+1)]
                [std::to_string(ssi+1)]
                ["TYPE"];
            

            /* ########
            // Check if the requests are valid
            // chemical name, compartment name and SS_type (source or sink)
            ###########*/

            // Get chemical index
            err_text.assign("Chemical name");
            foundflag = getModIndex(
                OpenWQ_wqconfig,
                OpenWQ_output,
                chem_list,
                Chemical_name,
                err_text,
                chem_ssi);

            if (foundflag == false) continue; // skip if comp not found

            // Get compartment index
            err_text.assign("Compartment name");
            foundflag = getModIndex(
                OpenWQ_wqconfig,
                OpenWQ_output,
                cmp_list,
                Compartment_name,
                err_text,
                cmpi_ssi);

            if (foundflag == false) continue; // skip if comp not found

            // Set flag for sink or source
            if (Type.compare("SOURCE") == 0){
                sinksource_ssi = 0;
            }else if (Type.compare("SINK") == 0){
                sinksource_ssi = 1;
            }else{
                continue; // skip if Type is unknown
            }
            

            /* ########
            // Get units and actual input data
            ###########*/

            Units = OpenWQ_json.SinkSource // units
                [std::to_string(ssf+1)]
                [std::to_string(ssi+1)]
                ["UNITS"];

            // Get number of rows of data in JSON (YYYY, MM, DD, HH,...)
            num_rowdata = OpenWQ_json.SinkSource
                [std::to_string(ssf+1)]
                [std::to_string(ssi+1)]
                ["DATA"].size();

            /* ########################################
             // Loop over row data in sink-source file
            ######################################## */

            for (unsigned int di=0;di<num_rowdata;di++){
                
                // Reset the size to zero (the object will have no elements)
                row_data_col.reset(); 
                row_data_row.reset();

                // ###################
                // Year
                // ###################
                elemName = "Year";
                try{

                    YYYY_json = OpenWQ_json.SinkSource
                            [std::to_string(ssf+1)]
                            [std::to_string(ssi+1)]
                            ["DATA"]
                            [std::to_string(di+1)].at(0);

                }catch(...){

                    validEntryFlag = getSSVectEntry(
                        OpenWQ_wqconfig,
                        OpenWQ_output,
                        elemName,
                        (std::string) OpenWQ_json.SinkSource
                            [std::to_string(ssf+1)]
                            [std::to_string(ssi+1)]
                            ["DATA"]
                            [std::to_string(di+1)].at(0),
                        YYYY_json,
                        ssf,    // SS file
                        ssi,    // SS structure
                        di);    // SS row

                    if (!validEntryFlag){continue;}
                }
                
                // ###################
                // Month
                // ###################
                elemName = "Month";
                try{

                    MM_json = OpenWQ_json.SinkSource
                            [std::to_string(ssf+1)]
                            [std::to_string(ssi+1)]
                            ["DATA"]
                            [std::to_string(di+1)].at(1);

                }catch(...){

                    validEntryFlag = getSSVectEntry(
                        OpenWQ_wqconfig,
                        OpenWQ_output,
                        elemName,
                        (std::string) OpenWQ_json.SinkSource
                            [std::to_string(ssf+1)]
                            [std::to_string(ssi+1)]
                            ["DATA"]
                            [std::to_string(di+1)].at(1),
                        MM_json,
                        ssf,    // SS file
                        ssi,    // SS structure
                        di);    // SS row

                    if (!validEntryFlag){continue;}
                }
                
                // ###################
                // Day
                // ###################
                elemName = "Day";
                try{

                    DD_json =OpenWQ_json.SinkSource
                        [std::to_string(ssf+1)]
                        [std::to_string(ssi+1)]
                        ["DATA"]
                        [std::to_string(di+1)].at(2);

                }catch(...){

                    validEntryFlag = getSSVectEntry(
                        OpenWQ_wqconfig,
                        OpenWQ_output,
                        elemName,
                        (std::string) OpenWQ_json.SinkSource
                            [std::to_string(ssf+1)]
                            [std::to_string(ssi+1)]
                            ["DATA"]
                            [std::to_string(di+1)].at(2),
                        DD_json,
                        ssf,    // SS file
                        ssi,    // SS structure
                        di);    // SS row

                    if (!validEntryFlag){continue;}
                }

                // ###################
                // Hour
                // ###################
                elemName = "Hour";
                try{

                    HH_json =OpenWQ_json.SinkSource
                        [std::to_string(ssf+1)]
                        [std::to_string(ssi+1)]
                        ["DATA"]
                        [std::to_string(di+1)].at(3);

                }catch(...){

                    validEntryFlag = getSSVectEntry(
                        OpenWQ_wqconfig,
                        OpenWQ_output,
                        elemName,
                        (std::string) OpenWQ_json.SinkSource
                            [std::to_string(ssf+1)]
                            [std::to_string(ssi+1)]
                            ["DATA"]
                            [std::to_string(di+1)].at(3),
                        HH_json,
                        ssf,    // SS file
                        ssi,    // SS structure
                        di);    // SS row

                    if (!validEntryFlag){continue;}
                }        
                
                // ###################
                // Minute
                // ###################
                elemName = "Min";
                try{

                    MIN_json =OpenWQ_json.SinkSource
                        [std::to_string(ssf+1)]
                        [std::to_string(ssi+1)]
                        ["DATA"]
                        [std::to_string(di+1)].at(4);

                }catch(...){

                    validEntryFlag = getSSVectEntry(
                        OpenWQ_wqconfig,
                        OpenWQ_output,
                        elemName,
                        (std::string) OpenWQ_json.SinkSource
                            [std::to_string(ssf+1)]
                            [std::to_string(ssi+1)]
                            ["DATA"]
                            [std::to_string(di+1)].at(4),
                        MIN_json,
                        ssf,    // SS file
                        ssi,    // SS structure
                        di);    // SS row

                    if (!validEntryFlag){continue;}
                }

                // chemname_ssi -> already obtained above // chemical name

                // ###################
                // ix
                // ###################
                elemName = "ix";
                try{

                    ix_json =OpenWQ_json.SinkSource
                        [std::to_string(ssf+1)]
                        [std::to_string(ssi+1)]
                        ["DATA"]
                        [std::to_string(di+1)].at(5);

                    // Need to do "- 1" because C++ starts in zero
                    ix_json--;

                    // If entry in json is zero, we get here -1
                    // which is wrong, so need to send warning messsage
                    // and skip entry
                    if (ix_json == -1){
                        // Through a warning invalid entry           
                        msg_string = 
                            "<OpenWQ> WARNING: SS '" 
                            + elemName 
                            + "' cannot be zero. It needs to start in one (entry skipped): File=" 
                            + std::to_string(ssf+1)
                            + ", Sub_structure=" + std::to_string(ssi+1)
                            + ", Data_row=" + std::to_string(di + 1);   

                        OpenWQ_output.ConsoleLog(   // Print it (Console and/or Log file)
                            OpenWQ_wqconfig,        // for Log file name
                            msg_string,             // message
                            true,                   // print in console
                            true);                  // print in log file

                        continue; // skip entry
                    }

                }catch(...){

                    validEntryFlag = getSSVectEntry(
                        OpenWQ_wqconfig,
                        OpenWQ_output,
                        elemName,
                        (std::string) OpenWQ_json.SinkSource
                            [std::to_string(ssf+1)]
                            [std::to_string(ssi+1)]
                            ["DATA"]
                            [std::to_string(di+1)].at(5),
                        ix_json,
                        ssf,    // SS file
                        ssi,    // SS structure
                        di);    // SS row

                    if (!validEntryFlag){continue;}
                }

                // ###################
                // iy
                // ###################
                elemName = "iy";
                try{

                    iy_json =OpenWQ_json.SinkSource
                        [std::to_string(ssf+1)]
                        [std::to_string(ssi+1)]
                        ["DATA"]
                        [std::to_string(di+1)].at(6);

                    // Need to do "- 1" because C++ starts in zero
                    iy_json--;

                    // If entry in json is zero, we get here -1
                    // which is wrong, so need to send warning messsage
                    // and skip entry
                    if (iy_json == -1){
                        // Through a warning invalid entry           
                        msg_string = 
                            "<OpenWQ> WARNING: SS '" 
                            + elemName 
                            + "' cannot be zero. It needs to start in one (entry skipped): File=" 
                            + std::to_string(ssf+1)
                            + ", Sub_structure=" + std::to_string(ssi+1)
                            + ", Data_row=" + std::to_string(di + 1);   

                        OpenWQ_output.ConsoleLog(   // Print it (Console and/or Log file)
                            OpenWQ_wqconfig,        // for Log file name
                            msg_string,             // message
                            true,                   // print in console
                            true);                  // print in log file

                        continue; // skip entry
                    }

                }catch(...){

                    validEntryFlag = getSSVectEntry(
                        OpenWQ_wqconfig,
                        OpenWQ_output,
                        elemName,
                        (std::string) OpenWQ_json.SinkSource
                            [std::to_string(ssf+1)]
                            [std::to_string(ssi+1)]
                            ["DATA"]
                            [std::to_string(di+1)].at(6),
                        iy_json,
                        ssf,    // SS file
                        ssi,    // SS structure
                        di);    // SS row

                    if (!validEntryFlag){continue;}
                }

                // ###################
                // iz
                // ###################
                elemName = "iz";
                try{

                    iz_json =OpenWQ_json.SinkSource
                        [std::to_string(ssf+1)]
                        [std::to_string(ssi+1)]
                        ["DATA"]
                        [std::to_string(di+1)].at(7);

                    // Need to do "- 1" because C++ starts in zero
                    iz_json--;

                    // If entry in json is zero, we get here -1
                    // which is wrong, so need to send warning messsage
                    // and skip entry
                    if (iz_json == -1){
                        // Through a warning invalid entry           
                        msg_string = 
                            "<OpenWQ> WARNING: SS '" 
                            + elemName 
                            + "' cannot be zero. It needs to start in one (entry skipped): File=" 
                            + std::to_string(ssf+1)
                            + ", Sub_structure=" + std::to_string(ssi+1)
                            + ", Data_row=" + std::to_string(di + 1); 

                        OpenWQ_output.ConsoleLog(   // Print it (Console and/or Log file)
                            OpenWQ_wqconfig,        // for Log file name
                            msg_string,             // message
                            true,                   // print in console
                            true);                  // print in log file

                        continue; // skip entry
                    }

                }catch(...){

                    validEntryFlag = getSSVectEntry(
                        OpenWQ_wqconfig,
                        OpenWQ_output,
                        elemName,
                        (std::string) OpenWQ_json.SinkSource
                            [std::to_string(ssf+1)]
                            [std::to_string(ssi+1)]
                            ["DATA"]
                            [std::to_string(di+1)].at(7),
                        iz_json,
                        ssf,    // SS file
                        ssi,    // SS structure
                        di);    // SS row

                    if (!validEntryFlag){continue;}
                }

                // sink/source data
                ss_data_json = OpenWQ_json.SinkSource 
                    [std::to_string(ssf+1)]
                    [std::to_string(ssi+1)]
                    ["DATA"]
                    [std::to_string(di+1)].at(8);

                // sink/source units
                ss_units_json = OpenWQ_json.SinkSource 
                    [std::to_string(ssf+1)]
                    [std::to_string(ssi+1)]
                    ["UNITS"]; 
                
                // Convert SS units
                // Source/sink units (g -> default model mass units)
                // 1) Calculate unit multiplers
                std::vector<std::string> units;          // units (numerator and denominator)
                OpenWQ_units.Calc_Unit_Multipliers(
                    OpenWQ_wqconfig,
                    OpenWQ_output,
                    unit_multiplers,    // multiplers (numerator and denominator)
                    ss_units_json,      // input units
                    units,
                    true);              // direction of the conversion: 
                                        // to native (true) or 
                                        // from native to desired output units (false)

                // 2) Calculate value with new units
                OpenWQ_units.Convert_Units(
                    ss_data_json,       // ic_value passed by reference so that it can be changed
                    unit_multiplers);   // units

                // Get the vector with the data
                row_data_col = {
                    (double)chem_ssi,
                    (double)cmpi_ssi,
                    (double)sinksource_ssi,
                    (double)YYYY_json,
                    (double)MM_json,
                    (double)DD_json,
                    (double)HH_json,
                    (double)MIN_json,
                    (double)ix_json,
                    (double)iy_json,
                    (double)iz_json,
                    ss_data_json,
                    0,0,0,0,0       // field to specify the number of times it has been used aleady
                    };              // in the case of and "all" element (YYYY, MM, DD, HH, MIN)
                                    // it starts with 0 (zero), meaning that has not been used
                                    // if not an "all" element, then it's set to -1

                // Transpose vector for adding to SinkSource_FORC as a new row
                row_data_row = row_data_col.t();

                // Add new row_data_row to SinkSource_FORC   
                int_n_elem = (*OpenWQ_wqconfig.SinkSource_FORC).n_rows;  
                
                (*OpenWQ_wqconfig.SinkSource_FORC).insert_rows(
                    std::max(int_n_elem-1,0),
                    row_data_row); 

            }
        }
    }
}

/* #################################################
 // Check Sources and Sinks and Apply
 ################################################# */
void OpenWQ_sinksource::CheckApply(
    OpenWQ_vars& OpenWQ_vars,
    OpenWQ_hostModelconfig& OpenWQ_hostModelconfig,
    OpenWQ_wqconfig& OpenWQ_wqconfig,
    OpenWQ_units& OpenWQ_units,
    OpenWQ_output& OpenWQ_output,
    const unsigned int YYYY,         // current model step: Year
    const unsigned int MM,           // current model step: month
    const unsigned int DD,           // current model step: day
    const unsigned int HH,           // current model step: hour
    const unsigned int MIN){         // current model step: min
    
    // Local variables
    unsigned int num_rowdata;       // number of rows of data in JSON (YYYY, MM, DD, HH,...)
    int YYYY_json;                  // Year in JSON-sink_source (interactive)
    int MM_json;                    // Month in JSON-sink_source (interactive)
    int DD_json;                    // Day in JSON-sink_source (interactive)
    int HH_json;                    // Hour in JSON-sink_source (interactive)
    int MIN_json;                   // Hour in JSON-sink_source (interactive)
    int DD_max;                     // max number of days for a given month and year

    long sinksource_flag;           // source (=0) or sink (=1)
    time_t jsonTime;                // to get time as time_t for easier comparison with simTime
    time_t simTime;                 // to get time as time_t for easier comparison with jsonTime

    bool anyAll_flag;               // Flag to indicate when at least one "all" is present in row elements
    bool YYYYall_flag, MMall_flag, DDall_flag, \
         HHall_flag, MINall_flag;   // Flags "all" flags for specific date units
    bool addedIncrem_flag=true;    // flag to guarantee increment is added only in one time field (YYYY_json or MM_json or ...) 

    /* ########################################
    // Data update/clean-up at 1st timestep
    ######################################## */
    if (OpenWQ_wqconfig.tstep1_flag){

        // Remove requested loads that are prior to the simulation start datetime
        OpenWQ_sinksource::RemoveLoadBeforeSimStart(
            OpenWQ_wqconfig,
            OpenWQ_units,
            YYYY,         // current model step: Year
            MM,           // current model step: month
            DD,           // current model step: day
            HH,           // current model step: hour
            MIN           // current model step: min
        );

        // Update time increments for rows with "all" elements
        // Remove requested loads that are prior to the simulation start datetime
        OpenWQ_sinksource::UpdateAllElemTimeIncremts(
            OpenWQ_wqconfig,
            OpenWQ_units,
            YYYY,         // current model step: Year
            MM,           // current model step: month
            DD,           // current model step: day
            HH,           // current model step: hour
            MIN           // current model step: min
        );

    }

    // Convert sim time to time_t
    simTime = OpenWQ_units.convert_time(YYYY, MM, DD, HH, MIN);

    // Get number of rows in SinkSource_FORC
    num_rowdata = (*OpenWQ_wqconfig.SinkSource_FORC).n_rows; 

    /* ########################################
        // Loop over row data in sink-source file
    ######################################## */

    for (unsigned int ri=0;ri<num_rowdata;ri++){

        // Reset the addedIncrem_flag 
        addedIncrem_flag = true;

        // First check if row has already been used
        // only applicable to rows without "all" in any datetime row element
        // YYYY, MM, DD, HH, MIN
        if ((*OpenWQ_wqconfig.SinkSource_FORC)(ri,12) == -2) continue;

        // Reset anyAll_flag
        anyAll_flag = false, YYYYall_flag = false, MMall_flag = false, \
        DDall_flag = false, HHall_flag = false, MINall_flag = false; 

        // ########################################
        // Check if time in SinkSource_FORC row ri matches the current model time

        // Get requested JSON datetime
        YYYY_json = (*OpenWQ_wqconfig.SinkSource_FORC)(ri,3);
        MM_json = (*OpenWQ_wqconfig.SinkSource_FORC)(ri,4);  
        DD_json = (*OpenWQ_wqconfig.SinkSource_FORC)(ri,5);  
        HH_json = (*OpenWQ_wqconfig.SinkSource_FORC)(ri,6);  
        MIN_json = (*OpenWQ_wqconfig.SinkSource_FORC)(ri,7);

        // Add the appropriate year step to row elements
        // with "all" flad (= -1)
        if (YYYY_json == -1){YYYY_json += (*OpenWQ_wqconfig.SinkSource_FORC)(ri,12); anyAll_flag = true; YYYYall_flag = true;}
        if (MM_json == -1){MM_json += (*OpenWQ_wqconfig.SinkSource_FORC)(ri,13); anyAll_flag = true; MMall_flag = true;}
        if (DD_json == -1){DD_json += (*OpenWQ_wqconfig.SinkSource_FORC)(ri,14); anyAll_flag = true; DDall_flag = true;}
        if (HH_json == -1){HH_json += (*OpenWQ_wqconfig.SinkSource_FORC)(ri,15); anyAll_flag = true; HHall_flag = true;}
        if (MIN_json == -1){MIN_json += (*OpenWQ_wqconfig.SinkSource_FORC)(ri,16); anyAll_flag = true; MINall_flag = true;}

        // jsonTime in time_t
        jsonTime = OpenWQ_units.convert_time(YYYY_json, MM_json, DD_json, HH_json, MIN_json);

        // Skip if not time to load yet
        if (simTime < jsonTime) continue;

        // ########################################
        // If reached here, then it's time to apply load
        // ########################################

        // Update increment if "row" has all
        // Needed for setting the time for the next load
        // addedIncrem_flag makes sure increment is added only in YYYY_json or MM_json or ...
        // limit incremenets to max number of MIN, HH, DD, MM and YYYY
        // Also reset time elements to 1 when they reach their max value

        DD_max = OpenWQ_sinksource::getNumberOfDays(YYYY_json, MM_json);

        if (MINall_flag && addedIncrem_flag && MIN_json<60){
            (*OpenWQ_wqconfig.SinkSource_FORC)(ri,16)++;
            addedIncrem_flag = false;
        }else if (HHall_flag && addedIncrem_flag && HH_json<24){
            (*OpenWQ_wqconfig.SinkSource_FORC)(ri,15)++;
            if (MINall_flag && MIN_json==60) (*OpenWQ_wqconfig.SinkSource_FORC)(ri,16) = 1;
            addedIncrem_flag = false;
        }else if (DDall_flag && addedIncrem_flag && DD_json<DD_max){
            (*OpenWQ_wqconfig.SinkSource_FORC)(ri,14)++;
            if (MINall_flag && MIN_json==60) (*OpenWQ_wqconfig.SinkSource_FORC)(ri,16) = 1;
            if (HHall_flag && HH_json==24) (*OpenWQ_wqconfig.SinkSource_FORC)(ri,15) = 1;
            addedIncrem_flag = false;
        }else if (MMall_flag && addedIncrem_flag && MM_json<12){
            (*OpenWQ_wqconfig.SinkSource_FORC)(ri,13)++;
            if (MINall_flag && MIN_json==60) (*OpenWQ_wqconfig.SinkSource_FORC)(ri,16) = 1;
            if (HHall_flag && HH_json==24) (*OpenWQ_wqconfig.SinkSource_FORC)(ri,15) = 1;
            if (DDall_flag && DD_json==DD_max) (*OpenWQ_wqconfig.SinkSource_FORC)(ri,14) = 2;
            addedIncrem_flag = false;
        }else if (YYYYall_flag && addedIncrem_flag){
            (*OpenWQ_wqconfig.SinkSource_FORC)(ri,12)++;
            if (MINall_flag && MIN_json==60) (*OpenWQ_wqconfig.SinkSource_FORC)(ri,16) = 1;
            if (HHall_flag && HH_json==24) (*OpenWQ_wqconfig.SinkSource_FORC)(ri,15) = 1;
            if (DDall_flag && DD_json==DD_max) (*OpenWQ_wqconfig.SinkSource_FORC)(ri,14) = 2;
            if (MMall_flag && MM_json==12) (*OpenWQ_wqconfig.SinkSource_FORC)(ri,13) = 1;
            addedIncrem_flag = false;
        }

        // Set it as "used" (not for use anymore) if:
        // 1) exceeded possible increments in "all" elements
        // 2) when load is without "all" (only use one time)
        if (addedIncrem_flag) (*OpenWQ_wqconfig.SinkSource_FORC)(ri,12) = -2;
        if (!anyAll_flag) (*OpenWQ_wqconfig.SinkSource_FORC)(ri,12) = -2;

        // ########################################
        // Apply source or sink
        // ########################################

        sinksource_flag = (*OpenWQ_wqconfig.SinkSource_FORC)(ri,2);

        // if SOURCE
        if (sinksource_flag == 0){

            OpenWQ_sinksource::Apply_Source(
                OpenWQ_vars,
                OpenWQ_wqconfig,
                OpenWQ_hostModelconfig,
                OpenWQ_output,
                (*OpenWQ_wqconfig.SinkSource_FORC)(ri,1),       // compartment model index
                (*OpenWQ_wqconfig.SinkSource_FORC)(ri,0),       // chemical model index    
                (*OpenWQ_wqconfig.SinkSource_FORC)(ri,8),       // compartment model ix
                (*OpenWQ_wqconfig.SinkSource_FORC)(ri,9),       // compartment model iy
                (*OpenWQ_wqconfig.SinkSource_FORC)(ri,10),      // compartment model iz
                (*OpenWQ_wqconfig.SinkSource_FORC)(ri,11));     // source load

        }
        // if SINK
        else if (sinksource_flag == 1){

            OpenWQ_sinksource::Apply_Sink(
                OpenWQ_vars,
                OpenWQ_wqconfig,
                OpenWQ_hostModelconfig,
                OpenWQ_output,
                (*OpenWQ_wqconfig.SinkSource_FORC)(ri,1),       // compartment model index
                (*OpenWQ_wqconfig.SinkSource_FORC)(ri,0),       // chemical model index    
                (*OpenWQ_wqconfig.SinkSource_FORC)(ri,8),       // compartment model ix
                (*OpenWQ_wqconfig.SinkSource_FORC)(ri,9),       // compartment model iy
                (*OpenWQ_wqconfig.SinkSource_FORC)(ri,10),      // compartment model iz
                (*OpenWQ_wqconfig.SinkSource_FORC)(ri,11));     // source load
        }

    }

}


/* #################################################
 // Apply Source
 ################################################# */
void OpenWQ_sinksource::Apply_Source(
    OpenWQ_vars& OpenWQ_vars,
    OpenWQ_wqconfig& OpenWQ_wqconfig,
    OpenWQ_hostModelconfig& OpenWQ_hostModelconfig,
    OpenWQ_output& OpenWQ_output,
    const unsigned int cmpi,             // compartment model index
    const unsigned int chemi,            // chemical model index    
    int ix,          // compartment model ix
    int iy,          // compartment model iy
    int iz,          // compartment model iz
    const double ss_data_json){          // source load g

    // Local Variables
    std::string msg_string;             // error/warning message string
    unsigned int spX_min, spX_max, spY_min, spY_max, spZ_min, spZ_max;
    unsigned int nx = std::get<2>(OpenWQ_hostModelconfig.HydroComp[cmpi]);
    unsigned int ny = std::get<3>(OpenWQ_hostModelconfig.HydroComp[cmpi]);
    unsigned int nz = std::get<4>(OpenWQ_hostModelconfig.HydroComp[cmpi]);

    // #####################
    // Determine domain region (or simple grid cells) to add load
    // ix
    if(ix != -1){spX_min = ix; spX_max = ix;}
    else{spX_min = 0; spX_max = nx - 1;}
    // iy
    if(iy != -1){spY_min = iy; spY_max = iy;}
    else{spY_min = 0; spY_max = ny - 1;}
    // iz
    if(iz != -1){spZ_min = iz; spZ_max = iz;}
    else{spZ_min = 0; spZ_max = nz - 1;}

    try{
        // Add mass load (already converted to g units)
        (*OpenWQ_vars.d_chemass_ss)(cmpi)(chemi)(
            arma::span(spX_min, spX_max), 
            arma::span(spY_min, spY_max),
            arma::span(spZ_min, spZ_max)) += ss_data_json;

    }catch (...){
        
        // Through a warning if request out of boundaries
        //Create message
        msg_string = 
            "<OpenWQ> WARNING: Sink/Source load out of boundaries."
            "Requested load ignored: Compartment=" 
            + std::get<1>(OpenWQ_hostModelconfig.HydroComp.at(cmpi))
            + ", Chemical=" + OpenWQ_wqconfig.BGC_general_chem_species_list[chemi]
            + ", ix=" + std::to_string(ix)
            + ", iy=" + std::to_string(iy)
            + ", iz=" + std::to_string(iz)
            + "";

        // Print it (Console and/or Log file)
        OpenWQ_output.ConsoleLog(
            OpenWQ_wqconfig,    // for Log file name
            msg_string,         // message
            true,               // print in console
            true);              // print in log file
                        
    }

}

/* #################################################
 // Apply Sink
 ################################################# */
void OpenWQ_sinksource::Apply_Sink(
    OpenWQ_vars& OpenWQ_vars,
    OpenWQ_wqconfig& OpenWQ_wqconfig,
    OpenWQ_hostModelconfig& OpenWQ_hostModelconfig,
    OpenWQ_output& OpenWQ_output,
    const unsigned int cmpi,            // compartment model index
    const unsigned int chemi,           // chemical model index    
    int ix,                             // compartment model ix
    int iy,                             // compartment model iy
    int iz,                             // compartment model iz
    const double ss_data_json){         // source load g

    // Local Variables
    double mass_sink;
    std::string msg_string;             // error/warning message string
    unsigned int spX_min, spX_max, spY_min, spY_max, spZ_min, spZ_max;
    unsigned int nx = std::get<2>(OpenWQ_hostModelconfig.HydroComp[cmpi]);
    unsigned int ny = std::get<3>(OpenWQ_hostModelconfig.HydroComp[cmpi]);
    unsigned int nz = std::get<4>(OpenWQ_hostModelconfig.HydroComp[cmpi]);

    // #####################
    // Determine domain region (or simple grid cells) to add load
    // ix
    if(ix != -1){spX_min = ix; spX_max = ix;}
    else{spX_min = 0; spX_max = nx - 1;}
    // iy
    if(iy != -1){spY_min = iy; spY_max = iy;}
    else{spY_min = 0; spY_max = ny - 1;}
    // iz
    if(iz != -1){spZ_min = iz; spZ_max = iz;}
    else{spZ_min = 0; spZ_max = nz - 1;}

    try{

        // Remove mass load (already converted to g units)
        (*OpenWQ_vars.d_chemass_ss)(cmpi)(chemi)(
            arma::span(spX_min, spX_max), 
            arma::span(spY_min, spY_max),
            arma::span(spZ_min, spZ_max)) -= ss_data_json;

        // Replace all negative values by zero
        // Needed because ss_data_json can be larger than available mass
        (*OpenWQ_vars.d_chemass_ss)(cmpi)(chemi).transform( [](double val) { return (val < 0.0) ? 0.0 : val; });

    }catch (...){

        // Through a warning if request out of boundaries
        // Create Message
        msg_string = 
            "<OpenWQ> WARNING: Sink/Source load out of boundaries."
            "Requested load ignored: Compartment=" 
            + std::get<1>(OpenWQ_hostModelconfig.HydroComp.at(cmpi))
            + ", Chemical=" + OpenWQ_wqconfig.BGC_general_chem_species_list[chemi]
            + ", ix=" + std::to_string(ix)
            + ", iy=" + std::to_string(iy)
            + ", iz=" + std::to_string(iz);

        // Print it (Console and/or Log file)
        OpenWQ_output.ConsoleLog(
            OpenWQ_wqconfig,    // for Log file name
            msg_string,         // message
            true,               // print in console
            true);              // print in log file

    }

}

/* #################################################
 // Get model structure indexes for compartments and chemicals
 ################################################# */
bool OpenWQ_sinksource::getModIndex(
    OpenWQ_wqconfig& OpenWQ_wqconfig,
    OpenWQ_output& OpenWQ_output,
    std::vector<std::string> &vec_list,
    std::string &obj_name,
    std::string &obj_text,
    unsigned long &vec_obj_index){
    
    // Local Variables
    bool foundflag = false;
    std::vector<std::string>::iterator find_i;  // iteractor used to store the position or searched element
    std::string msg_string;             // error/warning message string

    // Try to find index
    find_i = 
        std::find(vec_list.begin(), 
        vec_list.end(), 
        obj_name);

    // If requested index exists, then okay 
    // (otherwise, throw warning and skip entry)

    if (find_i != vec_list.end()){
        vec_obj_index =   find_i - vec_list.begin();
        foundflag = true;
    }else{
        
        // Create Message (WARNING: entry skipped)
        msg_string = 
            "<OpenWQ> WARNNING (entry skipped): " 
            + obj_text 
            + " in source-sink file unkown: " 
            + obj_name;

        // Print it (Console and/or Log file)
        OpenWQ_output.ConsoleLog(
            OpenWQ_wqconfig,    // for Log file name
            msg_string,         // message
            true,               // print in console
            true);              // print in log file

    }
    return foundflag;

}


/* #################################################
 // Get SS vector element
 // CASE IF: elemEntry as string "all"
 ################################################# */
 bool OpenWQ_sinksource::getSSVectEntry(
    OpenWQ_wqconfig& OpenWQ_wqconfig,
    OpenWQ_output& OpenWQ_output,
    std::string elemName,
    std::__cxx11::basic_string<char> elemEntry,
    int& elemVal,
    unsigned int& file_i,
    unsigned int& struc_i,
    unsigned int& row_i){

    // Local Variable
    bool validEntryFlag = true;
    std::string msg_string;
    
    
    if(elemEntry.compare("ALL") == 0){
        
        elemVal = OpenWQ_wqconfig.allSS_flag;

    }else{ 
        
        // Through a warning invalid entry           
        msg_string = 
            "<OpenWQ> WARNING: SS '" 
                            + elemName 
                            + "' cannot be zero. It needs to start in one (entry skipped): File=" 
                            + std::to_string(file_i+1)
                            + ", Sub_structure=" + std::to_string(struc_i+1)
                            + ", Data_row=" + std::to_string(row_i + 1);   

        OpenWQ_output.ConsoleLog(   // Print it (Console and/or Log file)
            OpenWQ_wqconfig,        // for Log file name
            msg_string,             // message
            true,                   // print in console
            true);                  // print in log file

        validEntryFlag = false;
    }

    return validEntryFlag;

}

/* #################################################
// At timestep 1, 
// remove requested loads that are prior to the simulation start datetime
// only do this for rows that don't have any "all" elements
#################################################*/
void OpenWQ_sinksource::RemoveLoadBeforeSimStart(
    OpenWQ_wqconfig& OpenWQ_wqconfig,
    OpenWQ_units& OpenWQ_units,
    const unsigned int YYYY,         // current model step: Year
    const unsigned int MM,           // current model step: month
    const unsigned int DD,           // current model step: day
    const unsigned int HH,           // current model step: hour
    const unsigned int MIN){        // current model step: min

    // Local variables
    bool all_flag = false, allinYYYY_flag = false;
    int YYYY_json, MM_json, DD_json, HH_json, MIN_json;
    time_t jsonTime, simTime;
    unsigned int num_rowdata, n_elem, n_rows2remove;
    std::vector<int> rows2Remove;  // List of rows indexes to remove     

    // Convert sim time to time_t
    simTime = OpenWQ_units.convert_time(YYYY, MM, DD, HH, MIN);

    // Get number of rows in SinkSource_FORC
    num_rowdata = (*OpenWQ_wqconfig.SinkSource_FORC).n_rows; 

    /* ########################################
    // Loop over row data in sink-source file
    ######################################## */

    for (unsigned int ri=0;ri<num_rowdata;ri++){

        // Reset all entry exists flag
        all_flag = false;
        allinYYYY_flag = false; // this is helpful because we can exclude the rows that may have "all" in rows other than
                                // the YYYY. If that YYYY is lower than the simulation, then the row can also be removed

        // Get requested JSON datetime
        YYYY_json = (*OpenWQ_wqconfig.SinkSource_FORC)(ri,3);
        MM_json = (*OpenWQ_wqconfig.SinkSource_FORC)(ri,4);  
        DD_json = (*OpenWQ_wqconfig.SinkSource_FORC)(ri,5);  
        HH_json = (*OpenWQ_wqconfig.SinkSource_FORC)(ri,6);  
        MIN_json = (*OpenWQ_wqconfig.SinkSource_FORC)(ri,7);

        // Skip any entry = -1 ('all' flag, then replace by current sim time)
        if (YYYY_json == -1){YYYY_json = YYYY; all_flag=true; allinYYYY_flag=true;}
        if (MM_json == -1){MM_json = MM; all_flag=true;}
        if (DD_json == -1){DD_json = DD; all_flag=true;}
        if (HH_json == -1){HH_json = HH; all_flag=true;}
        if (MIN_json == -1){MIN_json = MIN; all_flag=true;}

        // jsonTime in time_t
        jsonTime = OpenWQ_units.convert_time(
            YYYY_json, 
            MM_json, 
            DD_json, 
            HH_json, 
            MIN_json);
        
        // Save index of row to remove
        // only for the !all_flag rows
        if (!all_flag && jsonTime < simTime){ // cases without any "all" (in any row element)
            rows2Remove.push_back(ri);
        }else if(!allinYYYY_flag && jsonTime < simTime 
                && YYYY_json < YYYY){ // case without "all" in YYYY element where YYYY_json < YYYY)
            rows2Remove.push_back(ri);
        }
    }

    // arma .shed_row only accepts uvec
    // so, need to convert std::vector into arma::uvec
    // initiate this vector
    n_rows2remove = rows2Remove.size();
    arma::uvec rows2Remove_uvec(n_rows2remove);  

    // Loop over number of to-remove indexes
    for (unsigned int ri=0;ri<n_rows2remove;ri++){
        rows2Remove_uvec(ri) = rows2Remove[ri];
    }

    // Remove the identified past row data 
    (*OpenWQ_wqconfig.SinkSource_FORC).shed_rows(rows2Remove_uvec); 

    // Flag to note that 1st time step has been completed
    OpenWQ_wqconfig.tstep1_flag = false;

}

/* #################################################
// At timestep 1, 
// adjust time increments for YYYY, MM, DD, HH, MIN
#################################################*/
void OpenWQ_sinksource::UpdateAllElemTimeIncremts(
    OpenWQ_wqconfig& OpenWQ_wqconfig,
    OpenWQ_units& OpenWQ_units,
    const unsigned int YYYY,         // current model step: Year
    const unsigned int MM,           // current model step: month
    const unsigned int DD,           // current model step: day
    const unsigned int HH,           // current model step: hour
    const unsigned int MIN){        // current model step: min

    // Local variables
    unsigned num_rowdata;                                       // number of SS row data
    bool all_YYYY_flag = false, all_MM_flag = false, all_DD_flag = false, \
         all_HH_flag = false, all_MIN_flag = false;
    bool initSet_increm_flag;
    int YYYY_json, MM_json, DD_json, HH_json, MIN_json;
    time_t jsonTime, simTime;
    unsigned int increm1, increm2, increm3, increm4, increm5;   // for interactive trial-error to get mininum increment
    std::vector<int> rows2Remove;                               // List of rows indexes to remove
    unsigned int numFreeElem;                                   // number of free elements
    int DD_max;                                                 // max number of days for a given month and year
    
    // Convert sim time to time_t
    simTime = OpenWQ_units.convert_time(YYYY, MM, DD, HH, MIN);

    // Get number of rows in SinkSource_FORC
    num_rowdata = (*OpenWQ_wqconfig.SinkSource_FORC).n_rows; 

    /* ########################################
        // Loop over row data in sink-source file
    ######################################## */

    for (unsigned int ri=0;ri<num_rowdata;ri++){

        // Reset all entry exists flag
        all_YYYY_flag = false; all_MM_flag = false; all_DD_flag = false;
        all_HH_flag = false; all_MIN_flag = false;
        initSet_increm_flag = false;
        increm1=0, increm2=0, increm3=0, increm4=0, increm5=0;

        // Get requested JSON datetime
        YYYY_json = (*OpenWQ_wqconfig.SinkSource_FORC)(ri,3);
        MM_json = (*OpenWQ_wqconfig.SinkSource_FORC)(ri,4);  
        DD_json = (*OpenWQ_wqconfig.SinkSource_FORC)(ri,5);  
        HH_json = (*OpenWQ_wqconfig.SinkSource_FORC)(ri,6);  
        MIN_json = (*OpenWQ_wqconfig.SinkSource_FORC)(ri,7);

        // Determine the "all" elements, which will be our degrees of freedom
        if (YYYY_json == -1){all_YYYY_flag=true;}
        if (MM_json == -1){all_MM_flag=true;}
        if (DD_json == -1){all_DD_flag=true;}
        if (HH_json == -1){all_HH_flag=true;}
        if (MIN_json == -1){all_MIN_flag=true;}

        // If there aren't any "all" elements, then set it to zero
        // and go to the next row
        if (!all_YYYY_flag && !all_MM_flag && 
            !all_DD_flag && !all_HH_flag && !all_MIN_flag){
                (*OpenWQ_wqconfig.SinkSource_FORC)(ri,12) = -1;
                continue;
        }

        // First interation to get closer to current timestep based on simtime
        // and degrees of freedom from "all" elements. Only changes the min step possible
        if (!all_YYYY_flag & YYYY_json > YYYY){
            // if YYYY_json is not "all" and is higher than the current sim YYYY,
            // then we just need to set the values of MM, DD, HH, MIN have to the min values
            // that means, Jan-1 00:00 of that yeat
            // for that, we need to add 2 because the "all" flag is -1, so we need to add 2 to get to 1
            if (all_MIN_flag){increm5 = 2;}
            if (all_HH_flag){increm4 = 2;}
            if (all_DD_flag){increm3 = 2;}
            if (all_MM_flag){increm2 = 2;}
        }else if (YYYY_json == YYYY){
            // if YYYY_json is in same year as YYYY, then we need to look for the closest month, day, hour and mib
            if (all_MIN_flag && !initSet_increm_flag){increm5 = MIN - MIN_json; initSet_increm_flag=true;}
            if (all_HH_flag && !initSet_increm_flag){increm4 = HH - HH_json; initSet_increm_flag=true;}
            if (all_DD_flag && !initSet_increm_flag){increm3 = DD - DD_json; initSet_increm_flag=true;}
            if (all_MM_flag && !initSet_increm_flag){increm2 = MM - MM_json; initSet_increm_flag=true;}
            if (all_YYYY_flag && !initSet_increm_flag){increm1 = YYYY - YYYY_json; initSet_increm_flag=true;}
        }

        // Determine new jsonTime if using the first guess for the increment
        jsonTime = OpenWQ_units.convert_time(
            YYYY_json + increm1, 
            MM_json + increm2, 
            DD_json + increm3, 
            HH_json + increm4, 
            MIN_json + increm5);

        // ###############################################
        // Find the minimum increment needed in the "all" elements, so that
        // we go ahead of the current simulation timestep
        
        if (jsonTime < simTime){

            // Try changing 
            // at MIN scale (if it is an "all" element)
            if (all_MIN_flag){
                while(jsonTime < simTime && (MIN_json + increm5) < 60){
                    increm5++;
                    jsonTime = OpenWQ_units.convert_time(
                        YYYY_json + increm1, MM_json + increm2, DD_json + increm3, 
                        HH_json + increm4, MIN_json + increm5);
                }
                // If MIN increment not sufficient, then set it to 1
                // so that it gets MIN_json=0, and then try the next 
                // datetime level
                if (jsonTime < simTime){increm5 = 1;}
            }else{
                increm5 = 1;}
            
            // Try changing 
            // at HH scale (if it is an "all" element)
            if (all_HH_flag){
                while(jsonTime < simTime && (HH_json + increm4) < 24){
                    increm4++;
                    jsonTime = OpenWQ_units.convert_time(
                        YYYY_json + increm1, MM_json + increm2, DD_json + increm3, 
                        HH_json + increm4, MIN_json + increm5);
                }
                // If MIN increment not sufficient, then set it to 1
                // so that it gets MIN_json=0, and then try the next 
                // datetime level
                if (jsonTime < simTime){increm4 = 1;}
            }else{
                increm4 = 1;}
            
            // Try changing 
            // at DD scale (if it is an "all" element)
            DD_max = OpenWQ_sinksource::getNumberOfDays(YYYY_json, MM_json);
            if (all_DD_flag){
                while(jsonTime < simTime && (DD_json + increm3) < DD_max){
                    increm3++;
                    jsonTime = OpenWQ_units.convert_time(
                        YYYY_json + increm1, MM_json + increm2, DD_json + increm3, 
                        HH_json + increm4, MIN_json + increm5);
                }
                // If MIN increment not sufficient, then set it to 1
                // so that it gets MIN_json=0, and then try the next 
                // datetime level
                if (jsonTime < simTime){increm3 = 1;}
            }else{
                increm3 = 1;}

            // Try changing 
            // at MM scale (if it is an "all" element)
            if (all_MM_flag){
                while(jsonTime < simTime && (MM_json + increm2) < 12){
                    increm2++;
                    jsonTime = OpenWQ_units.convert_time(
                        YYYY_json + increm1, MM_json + increm2, DD_json + increm3, 
                        HH_json + increm4, MIN_json + increm5);
                }
                // If MIN increment not sufficient, then set it to 1
                // so that it gets MIN_json=0, and then try the next 
                // datetime level
                if (jsonTime < simTime){increm2 = 1;}
            }else{
                increm2 = 1;}

            // Try changing 
            // at YYYY scale (if it is an "all" element)
            if (all_YYYY_flag){
                while(jsonTime < simTime){
                    increm1++;
                    jsonTime = OpenWQ_units.convert_time(
                        YYYY_json + increm1, MM_json + increm2, DD_json + increm3, 
                        HH_json + increm4, MIN_json + increm5);
                }
            }

        }

        // Update increments in SinkSource_FORC
        (*OpenWQ_wqconfig.SinkSource_FORC)(ri,12) = increm1;
        (*OpenWQ_wqconfig.SinkSource_FORC)(ri,13) = increm2;
        (*OpenWQ_wqconfig.SinkSource_FORC)(ri,14) = increm3;
        (*OpenWQ_wqconfig.SinkSource_FORC)(ri,15) = increm4;
        (*OpenWQ_wqconfig.SinkSource_FORC)(ri,16) = increm5;

    }

}

// Function to return total number of days in a given year and month
int  OpenWQ_sinksource::getNumberOfDays(
        const unsigned int YYYY_check,          // json: Year 
        const unsigned int MM_check)            // json: Month
{
	//leap year condition, if month is 2
	if(MM_check == 2)
	{
		if((YYYY_check%400==0) || (YYYY_check%4==0 && YYYY_check%100!=0))	
			return 29;
		else	
			return 28;
	}
	//months which has 31 days
	else if(MM_check == 1 || MM_check == 3 || MM_check == 5 || MM_check == 7 || MM_check == 8
	||MM_check == 10 || MM_check==12)	
		return 31;
	else 		
		return 30;

} 