

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


#include "DEMOS_OpenWQ_watertransp.h"

// Mass transport
void DEMOS_OpenWQ_watertransp::Adv(
        DEMOS_OpenWQ_json& JSONfiles,
        DEMOS_OpenWQ_vars& DEMOS_OpenWQ_vars,
        int source,
        int ix_s, 
        int iy_s,
        int iz_s,
        int recipient,
        int ix_r,
        int iy_r,
        int iz_r,
        double wflux_s2r,
        double wmass_recipient){

    double chemass_flux;
    int ichem_mob;

    // CHANGE THE LINE BELOW: see my notes -> there should be no icmp because all compartments should have the same number of mobile species
    std::vector<int> chemspec_mobile = JSONfiles.BGCcycling["CHEMICAL_SPECIES"]["mobile_species"];
    int numspec = chemspec_mobile.size();

    // Loop for mobile chemical species
    for (int c=0;c<numspec;c++){

        // mobile chemical species index
        ichem_mob = chemspec_mobile[c] - 1; // because C array indexing starts in 0 

        // Chemical mass flux between source and recipient 
        chemass_flux = wflux_s2r / wmass_recipient *
            (*DEMOS_OpenWQ_vars.chemass)(source)(ichem_mob)(ix_s,iy_s,iz_s);
        
        // Remove Chemical mass flux from SOURCE 
        (*DEMOS_OpenWQ_vars.chemass)(source)(ichem_mob)(ix_s,iy_s,iz_s) -= chemass_flux;

        // Add Chemical mass flux to RECIPIENT 
        (*DEMOS_OpenWQ_vars.chemass)(recipient)(ichem_mob)(ix_r,iy_r,iz_r) += chemass_flux;
    }
                
}

/*
// Mass exchange between compartments
void DEMOS_OpenWQ_watertransp::ChemCompExchange(
    DEMOS_OpenWQ_json& DEMOS_OpenWQ_json, 
    DEMOS_OpenWQ_vars& DEMOS_OpenWQ_vars, 
    int source, std::string kinetics, 
    std::vector<std::string> parameter_names, 
    std::vector<double> parameter_values,
    std::array<double,7> & linedata, 
    int & index_chem){

    typedef exprtk::symbol_table<double> symbol_table_t;
    typedef exprtk::expression<double> expression_t;
    typedef exprtk::parser<double> parser_t;

    double wmass_exchange;
    std::string kinetics_modif = kinetics;
    std::string chemname;
    int index_i;
    int index_transf;
    double chemass_transf;

    
    // Get chemical species in the source compartment
    std::vector<std::string> chem_species = DEMOS_OpenWQ_json.WQ["compartments"][std::to_string(source+1)]["chem_species"];

   // Find chem of relevance
    int ii = 0;
    for(int chemi=0;chemi<chem_species.size();chemi++){
        chemname = chem_species[chemi];

        // Consumedchemass_consumed, chemass_produced;ty()) 
        index_i = kinetics.find(chemname);
        if (index_i!=-1 && !kinetics.empty()){

            index_chem = chemi; // index
            kinetics_modif.replace(index_i,index_i + chemname.size(),"chemass_transf");

            break;
            }
        }

    // Parmeters
    for (int i=0;i<parameter_names.size();i++){
        index_i = kinetics_modif.find(parameter_names[i]);
        kinetics_modif.replace(index_i,index_i + parameter_names[i].size(),std::to_string(parameter_values[i]));
    }

    // Add variables to symbol_table
    symbol_table_t symbol_table;

    symbol_table.add_variable("chemass_transf",chemass_transf);
    // symbol_table.add_constants();

    expression_t expression;
    expression.register_symbol_table(symbol_table);

    parser_t parser;
    parser.compile(kinetics_modif,expression);

    chemass_transf = (*DEMOS_OpenWQ_vars.chemass)(source)(index_chem)(linedata[0],linedata[1],linedata[2]);

    // mass transfered
    linedata[6] = expression.value(); 

}


// main loop
void DEMOS_OpenWQ_watertransp::AdvDisp(
    DEMOS_OpenWQ_json& DEMOS_OpenWQ_json,
    DEMOS_OpenWQ_vars& DEMOS_OpenWQ_vars){

    int numspec;
    int icmpMob, tmpst_i;
    bool mobile;
    std::vector<std::vector<std::string>> fluxes_filenames, compFluxInt_filenames;
    std::vector<std::vector<double>> fluxes_filenames_num,compFluxInt_filenames_num,all_filenames_num;
    std::string fluxes_fileExtention,compFluxInt_fileExtention;
    std::vector<int> mobileCompt;
    std::string res_folder = DEMOS_OpenWQ_json.Master["export_results_folder"];
    std::vector<int>::iterator is_mobile; // to check if mobile in compartment loop
    
    int num_HydroComp = DEMOS_OpenWQ_json.H2O["compartments"].size();
    double disp_x = DEMOS_OpenWQ_json.WQ["dispersion"]["x-dir"];
    double disp_y = DEMOS_OpenWQ_json.WQ["dispersion"]["y-dir"];
    double disp_z = DEMOS_OpenWQ_json.WQ["dispersion"]["z-dir"];

        
    is_mobile = find (mobileCompt.begin(), mobileCompt.end(), icmp);
    if (is_mobile != mobileCompt.end()){// if mobile
        // Run ADE_solver
        icmp = mobileCompt[icmp]; // get mobile compartments
        std::vector<std::string> chemspec_i = DEMOS_OpenWQ_json.WQ["compartments"][std::to_string(icmp+1)]["chem_species"];
        numspec = chemspec_i.size();
        for (int ichem=0;ichem<numspec;ichem++) 
            ADE_solver_1(DEMOS_OpenWQ_json,DEMOS_OpenWQ_vars,icmp,ichem);                                       
    }
}

          
 */