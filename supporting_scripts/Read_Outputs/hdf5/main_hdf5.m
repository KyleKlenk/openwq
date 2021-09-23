
% Copyright 2020, Diogo Costa, diogo.pinhodacosta@canada.ca
% This file is part of OpenWQ model.

% This program, openWQ, is free software: you can redistribute it and/or modify
% it under the terms of the GNU General Public License as published by
% the Free Software Foundation, either version 3 of the License, or
% (at your option) later version.
%
% This program is distributed in the hope that it will be useful,
% but WITHOUT ANY WARRANTY; without even the implied warranty of
% MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
% GNU General Public License for more details.

% You should have received a copy of the GNU General Public License
% along with this program.  If not, see <http://www.gnu.org/licenses/>.


% Read and plot HDF5 output data

function [tsnames,output_tscollect] = main_hdf5(...
    folderpath,...      % Provide fullpath to directory where the HDF5 files are located
    plot_elemt_flag,... % Flag to specify if to plot variables
    plotElm_info...    % If the flag above is 1, then provide details about the variables to plot
    )

    %{ 
        Examples of input arguments
    
        folderpath = '/media/dcosta/data/megasync/ec_main/models/crhm/code_crhm/build/Case_Studies/stc_openwq/Output_OpenWQ/HDF5/';

        plot_elemt_flag = true;
    
        plotElm_info = {...
            'SOIL_RECHR_LABILEP','all';...
            'SOIL_RECHR_REFRACTORYP',[1,1,1;...
                              6,1,1]...
            };
    
        %}

    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    % Read h5 data and save to timeseries collection
    output_tscollect = Read_h5_save_tscollection(folderpath);

    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    % Get names of timeseries in tscollection
    tsnames = Get_output_tsnames(output_tscollect);

    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    % Plot elements
    if plot_elemt_flag

        % plot requested data defined in plotElm_info
        plot_elements(...
            tsnames,...
            output_tscollect,...
            plotElm_info)

    end



    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    % Read timeseries names
    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    function stsnames = Get_output_tsnames(output_tscollect)

        % get number of timeseries
        num_ts = numel(output_tscollect);

        % initiate stsnames cell
        stsnames = cell(num_ts,1);

        for i = 1:num_ts

            % Get ts i
            ts = output_tscollect{i};

            % Save name of ts i
            stsnames{i} = ts.Name;

        end

    end

end



