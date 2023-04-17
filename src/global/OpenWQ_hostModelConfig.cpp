// Copyright 2020, Diogo Costa
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

#include "OpenWQ_hostModelconfig.hpp"


// Constructor
OpenWQ_hostModelconfig::OpenWQ_hostModelconfig() 
{
    // Water volumes from hostmodel
    waterVol_hydromodel = std::unique_ptr<
        std::vector<                            // compartments
        arma::Cube<                             // ix, iy, iz
        double>>>(new std::vector<arma::cube>); 

    // Dependencies from hostmodel 
    // (to be available for BGC)
    dependVar = std::unique_ptr<
        std::vector<                            // dependency variable
        arma::Cube<                             // ix, iy, iz
        double>>>(new std::vector<arma::cube>);

    dependVar_scalar = std::unique_ptr<
        std::vector<
        double>>(new std::vector<double>);
}

// Destructor
OpenWQ_hostModelconfig::~OpenWQ_hostModelconfig() 
{
}

/******** 
 * Methods 
 * *********/
// Add a compartment to the vector of compartments
void OpenWQ_hostModelconfig::add_HydroComp(int index, std::string name, int num_cells_x, 
                    int num_cells_y, int num_cells_z) 
{
    this->HydroComp.push_back(OpenWQ_hostModelconfig::hydroTuple(
        index, name, num_cells_x, num_cells_y, num_cells_z));
}
// Add an external flux to the vector of external fluxes
void OpenWQ_hostModelconfig::add_HydroExtFlux(int index, std::string name, int num_cells_x, 
                    int num_cells_y, int num_cells_z) 
{
    this->HydroExtFlux.push_back(OpenWQ_hostModelconfig::hydroTuple(
        index, name, num_cells_x, num_cells_y, num_cells_z));
}
// Add a dependency to the vector of dependencies
void OpenWQ_hostModelconfig::add_HydroDepend(int index, std::string name, int num_cells_x, 
                    int num_cells_y, int num_cells_z) 
{
    this->HydroDepend.push_back(OpenWQ_hostModelconfig::hydroTuple(
        index, name, num_cells_x, num_cells_y, num_cells_z));
}



void OpenWQ_hostModelconfig::increment_interaction_step()
{
    this->interaction_step++;
}

bool OpenWQ_hostModelconfig::is_first_interaction_step()
{
    return (this->interaction_step == 1);
}

/**
 * HydroTuple methods
*/
unsigned int OpenWQ_hostModelconfig::get_num_HydroComp()
{
    return this->HydroComp.size();
}
unsigned int OpenWQ_hostModelconfig::get_num_HydroExtFlux()
{
    return this->HydroExtFlux.size();
}
unsigned int OpenWQ_hostModelconfig::get_num_HydroDepend()
{
    return this->HydroDepend.size();
}

// get names of compartments and external fluxes
// for a specific index
std::string OpenWQ_hostModelconfig::get_HydroComp_name_at(int index)
{
    return std::get<1>(this->HydroComp[index]);
}
std::string OpenWQ_hostModelconfig::get_HydroDepend_name_at(int index)
{
    return std::get<1>(this->HydroDepend[index]);
}

// Get vector of compartment names and external fluxes names
std::vector<std::string> OpenWQ_hostModelconfig::get_HydroComp_names()
{
    std::vector<std::string> names;
    for (auto &i : this->HydroComp)
    {
        names.push_back(std::get<1>(i));
    }
    return names;
}
std::vector<std::string> OpenWQ_hostModelconfig::get_HydroExtFlux_names()
{
    std::vector<std::string> names;
    for (auto &i : this->HydroExtFlux)
    {
        names.push_back(std::get<1>(i));
    }
    return names;
}

// Get number of cells in x, y and z directions - HydroComp
unsigned int OpenWQ_hostModelconfig::get_HydroComp_num_cells_x_at(int index)
{
    return std::get<2>(this->HydroComp[index]);
}
unsigned int OpenWQ_hostModelconfig::get_HydroComp_num_cells_y_at(int index)
{
    return std::get<3>(this->HydroComp[index]);
}
unsigned int OpenWQ_hostModelconfig::get_HydroComp_num_cells_z_at(int index)
{
    return std::get<4>(this->HydroComp[index]);
}
// Get number of cells in x, y and z directions - HydroExtFlux
unsigned int OpenWQ_hostModelconfig::get_HydroExtFlux_num_cells_x_at(int index)
{
    return std::get<2>(this->HydroExtFlux[index]);
}
unsigned int OpenWQ_hostModelconfig::get_HydroExtFlux_num_cells_y_at(int index)
{
    return std::get<3>(this->HydroExtFlux[index]);
}
unsigned int OpenWQ_hostModelconfig::get_HydroExtFlux_num_cells_z_at(int index)
{
    return std::get<4>(this->HydroExtFlux[index]);
}
// Get number of cells in x, y and z directions - HydroDepend
unsigned int OpenWQ_hostModelconfig::get_HydroDepend_num_cells_x_at(int index)
{
    return std::get<2>(this->HydroDepend[index]);
}
unsigned int OpenWQ_hostModelconfig::get_HydroDepend_num_cells_y_at(int index)
{
    return std::get<3>(this->HydroDepend[index]);
}
unsigned int OpenWQ_hostModelconfig::get_HydroDepend_num_cells_z_at(int index)
{
    return std::get<4>(this->HydroDepend[index]);
}



// time_step_methods
bool OpenWQ_hostModelconfig::is_time_step_0()
{
    return (this->time_step == 0);
}
void OpenWQ_hostModelconfig::set_time_step(double time_step)
{
    this->time_step = time_step;
}
double OpenWQ_hostModelconfig::get_time_step()
{
    return this->time_step;
}

// waterVol_minlim methods
double OpenWQ_hostModelconfig::get_watervol_minlim()
{
    return this->watervol_minlim;
}
