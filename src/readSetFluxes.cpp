
#include "readSetFluxes.h"

// Read and set intra fluxes for all compartments
void readSetFluxes(JSONfiles& JSONfiles,Prj_StateVar& Prj_StateVar,std::vector<int> mobileCompt,
    std::string& filenamesExtention,int tmpst){

    int icmpMobile;
    int num_mobileCompt = mobileCompt.size(); // num of mobile compartments

    // loop over compartmennts: compare all mobile_compartments with the first compartment
    for (int icmp=0;icmp<num_mobileCompt;icmp++){ 

        icmpMobile = mobileCompt[icmp]; // mobile compartment i

        // fluxes file for timestep tmpst
        std::string filepath_i = JSONfiles.H2O[std::to_string(icmpMobile+1)]["water_fluxes_files"]["folder_path"];
        filepath_i.append("/");
        filepath_i.append(std::to_string(tmpst)); // timestep number
        filepath_i.append(filenamesExtention); // file extention

        // Get grid col locations
        std::vector<int> var_col = JSONfiles.H2O[std::to_string(icmpMobile+1)]["water_fluxes_files"]["var_col"];
        
        // loop over x-, y- and z-directions
        for (int xyz_i=0;xyz_i<3;xyz_i++){

            if (var_col[xyz_i] == 0) continue; // skip if var_col[xyz_i] == false 

                read_file_3Dcoldata(JSONfiles.H2O[std::to_string(icmpMobile+1)]["water_fluxes_files"],
                    (*Prj_StateVar.wflux)(icmpMobile)(xyz_i),
                    var_col[xyz_i], filepath_i);
        }
    }

}

// Read and set inter fluxes for all compartments
void readCompInteract(JSONfiles& JSONfiles,Prj_StateVar& Prj_StateVar, std::string& filenamesExtention,
    int tmpst){

    int numInter = JSONfiles.CMPI["interactions"].size();
    std::string exchange_type;
    std::string filepath_i;
    int index_chem;

    // loop over compartmennts: compare all mobile_compartments with the first compartment
    for (int it=0;it<numInter;it++){ 

        // fluxes file for timestep tmpst
        exchange_type = JSONfiles.CMPI[std::to_string(it+1)]["exchange_type"];

        if (exchange_type.compare("water_flux")==0){ // water fluxes
            filepath_i = JSONfiles.CMPI[std::to_string(it+1)]["mapping_file"]["folder_path"];
            filepath_i.append("/");
            filepath_i.append(std::to_string(tmpst)); // timestep number
            filepath_i.append(filenamesExtention); // file extention
        }else if (exchange_type.compare("chem_exchange")==0) // chem exchange
            filepath_i = JSONfiles.CMPI[std::to_string(it+1)]["mapping_file"]["file_path"];

        // Source and recipient compartments
        int source = JSONfiles.CMPI[std::to_string(it+1)]["exchange_compartments"]["compartment_source"];
        int recipient = JSONfiles.CMPI[std::to_string(it+1)]["exchange_compartments"]["compartment_recipient"];
        source --; // C++ starts with 0 and the JSON file users will use 1
        recipient --;

        // Get grid col locations
        //std::vector<int> grid_col_send = JSONfiles.CMPI[std::to_string(it+1)]["mapping_file"]["grid_col_send"];
        //std::vector<int> grid_col_receive = JSONfiles.CMPI[std::to_string(it+1)]["mapping_file"]["grid_col_receive"];

        // Extract water_flux or calculate chem_exchange
        read_file_CMPIcoldata(JSONfiles, Prj_StateVar, it, source, recipient, filepath_i, exchange_type,index_chem);

        // Perform the water or chem mass exchange
        int numrows = (*Prj_StateVar.wchem_exch)(it).size();
        int ix_s, iy_s, iz_s, ix_r, iy_r, iz_r;
        double wfllux_val, chemass_val;

        // if water_flux
        if (exchange_type.compare("water_flux")==0){
            for (int rowi=0;rowi<numrows;rowi++){
                source = (*Prj_StateVar.wchem_exch)(it)(0);
                ix_s = (*Prj_StateVar.wchem_exch)(it)(1);
                iy_s = (*Prj_StateVar.wchem_exch)(it)(2);
                iz_s = (*Prj_StateVar.wchem_exch)(it)(3);
                recipient = (*Prj_StateVar.wchem_exch)(it)(4);
                ix_r = (*Prj_StateVar.wchem_exch)(it)(5);
                iy_r = (*Prj_StateVar.wchem_exch)(it)(6);
                iz_r = (*Prj_StateVar.wchem_exch)(it)(7);
                wfllux_val = (*Prj_StateVar.wchem_exch)(it)(8);
                chemass_val = wfllux_val/(*Prj_StateVar.wmass)(source)(ix_s,iy_s,iz_s) * (*Prj_StateVar.chemass)(source)(index_chem)(ix_s,iy_s,iz_s);
                
                // source
                (*Prj_StateVar.wmass)(source)(ix_s,iy_s,iz_s) -= wfllux_val;
                (*Prj_StateVar.chemass)(source)(index_chem)(ix_s,iy_s,iz_s) -= chemass_val;

                // recipient
                (*Prj_StateVar.wmass)(recipient)(ix_r,iy_r,iz_r) += wfllux_val;
                (*Prj_StateVar.chemass)(recipient)(index_chem)(ix_r,iy_r,iz_r) += chemass_val;
                
            }
        // if chem_exchange
        }else if (exchange_type.compare("chem_exchange")==0){
            for (int rowi=0;rowi<numrows;rowi++){
                source = (*Prj_StateVar.wchem_exch)(it)(0);
                ix_s = (*Prj_StateVar.wchem_exch)(it)(1);
                iy_s = (*Prj_StateVar.wchem_exch)(it)(2);
                iz_s = (*Prj_StateVar.wchem_exch)(it)(3);
                recipient = (*Prj_StateVar.wchem_exch)(it)(4);
                ix_r = (*Prj_StateVar.wchem_exch)(it)(5);
                iy_r = (*Prj_StateVar.wchem_exch)(it)(6);
                iz_r = (*Prj_StateVar.wchem_exch)(it)(7);
                chemass_val = (*Prj_StateVar.wchem_exch)(it)(8);
                          
                // just chemass
                (*Prj_StateVar.chemass)(source)(index_chem)(ix_s,iy_s,iz_s) -= chemass_val;
                (*Prj_StateVar.chemass)(recipient)(index_chem)(ix_r,iy_r,iz_r) += chemass_val;

            }
        }


    }

}
